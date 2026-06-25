#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <memory> 
#include <iostream>
#include <cmath>
#include <optional> 
#include <string>

#include "imgui.h"
#include "imgui-SFML.h"

#include "Vector.hpp"
#include "Planet.hpp"
#include "Moon.hpp"
#include "SpaceShip.hpp"
#include "Simulation.hpp"
#include "Renderer.hpp"
#include "StateMachine.hpp" 
#include "PhysicsUnits.hpp"
#include "AutopilotLogic.hpp"

using namespace physics; 

int main() {
    unsigned int window_width  = 1600;
    unsigned int window_height = 1000;

    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8; 

    sf::RenderWindow window( sf::VideoMode( { window_width, window_height} ), "Space Simulation: Lab 5", sf::State::Windowed, settings );
    window.setFramerateLimit( 120 );

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Не удалось инициализировать ImGui-SFML!" << std::endl;
        return -1;
    }

    sf::View camera = window.getDefaultView();
    camera.setCenter( sf::Vector2f( 0.0f, 0.0f ) );
    camera.setSize( sf::Vector2f( 2.4e9f, 1.5e9f ) );
    window.setView( camera ); 

    Simulation simulation;

    double center_x = 0.0;
    double center_y = 0.0;

    auto earth = new Planet( "Earth", 5.972e24_kg, 40000000.0, Vector( center_x, center_y ) );
    auto moon = new Moon( "Moon", 7.342e22_kg, 15000000.0, Vector( center_x, center_y - 384400000.0 ), Vector( 1022.0, 0.0 ) );
    auto selene = new Moon( "Selene", 1.0e22_kg, 10000000.0, Vector( center_x, center_y + 800000000.0 ), Vector( -705.0, 0.0 ) );

    auto player_ship = new SpaceShip( "Soyuz", 7000.0_kg, 35000.0_kg, 5000000.0, Vector( center_x + 85000000.0, center_y ), Vector( 0.0, 2164.0 ) );
    
    simulation.AddBody( earth );
    simulation.AddBody( moon );
    simulation.AddBody( selene );
    simulation.AddBody( player_ship );

    ICelestialBody* current_target = moon; 
    int target_index = 0; 

    Renderer renderer;
    sf::Clock physics_clock;
    sf::Clock imgui_clock; 
    
    const double kEnginePower = 200000.0; 

    fsm::StateMachine<Telemetry> autopilot; 
    
    autopilot.AddState( "Off", true );
    autopilot.AddState( "WaitAlignment" );  
    autopilot.AddState( "ProgradeBurn" );   
    autopilot.AddState( "Coast" );          
    autopilot.AddState( "Capture" );        
    autopilot.AddState( "OrbitMoon", true); 
    autopilot.SetInitialState( "Off" );

    double mu = simulation.GetkGravity() * earth->GetMass().in_kg();
    autopilot.AddTransition( "WaitAlignment", "ProgradeBurn", ConditionStartBurn( mu ) );
    autopilot.AddTransition( "ProgradeBurn", "Coast", ConditionStopBurn( mu ) );
    autopilot.AddTransition( "Coast", "Capture", ConditionStartCapture() );
    autopilot.AddTransition( "Capture", "OrbitMoon", ConditionOrbitEntered() );

    bool is_game_over = false;
    std::string game_over_reason = "";

    float time_warp = 1.0f;

    while ( window.isOpen() ) {
        while ( const std::optional<sf::Event> event = window.pollEvent() ) {
            // Передаем события в ImGui
            ImGui::SFML::ProcessEvent( window, event.value() );
            if ( event->is<sf::Event::Closed>() ) {
                window.close();
            }
        }

        ImGui::SFML::Update( window, imgui_clock.restart() );

        double real_dt = physics_clock.restart().asSeconds();
        if ( real_dt > 0.1 ) real_dt = 0.1; 

        if ( !is_game_over ) {
            double sim_dt = real_dt * time_warp; 
            double time_simulated = 0.0;

            while ( time_simulated < sim_dt ) {
                double target_orbit_radius = ( current_target->GetPosition() - earth->GetPosition() ).Length();
                double target_soi = target_orbit_radius * std::pow( current_target->GetMass().in_kg() / earth->GetMass().in_kg(), 0.4 ) * 1.5;
                
                Telemetry t( earth->GetPosition(), current_target->GetPosition(), player_ship->GetPosition(), player_ship->GetVelocity(), current_target->GetVelocity(), current_target->GetMass().in_kg(), current_target->GetRadius(), target_soi );

                bool state_changed = autopilot.Step( t );
                if (state_changed) {
                    std::cout << ">> AI State updated inside Warp: [" << autopilot.GetCurrentState() << "]\n";
                }

                std::string current_state = autopilot.GetCurrentState();
                Vector current_thrust( 0.0, 0.0 );

                if ( current_state == "Off" ) {
                    double manual_power = 50000.0; 
                    if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Up ) ) current_thrust.y = -manual_power; 
                    if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Down ) ) current_thrust.y = manual_power;  
                    if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Left ) ) current_thrust.x = -manual_power; 
                    if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Right ) ) current_thrust.x = manual_power;  
                }
                else if ( current_state == "WaitAlignment" || current_state == "Coast" ) {
                    current_thrust = Vector( 0.0, 0.0 );
                }
                else if ( current_state == "OrbitMoon" ) {
                    Vector to_target = current_target->GetPosition() - player_ship->GetPosition(); 
                    double dist = to_target.Length();
                    double park_radius = current_target->GetRadius() * 2.0; 
                    double dist_error = dist - park_radius; 
                    
                    // МЁРТВАЯ ЗОНА: Не тратим топливо, если орбита стабильна (+- 500 км)
                    if ( std::abs(dist_error) < 500000.0 ) {
                        current_thrust = Vector(0,0);
                    } else {
                        Vector dir_to_target = to_target.Normalized(); 
                        Vector relative_vel = player_ship->GetVelocity() - current_target->GetVelocity();
                        
                        double desired_approach_v = dist_error * 0.05;
                        if ( desired_approach_v > 100.0 ) desired_approach_v = 100.0; 
                        if ( desired_approach_v < -100.0 ) desired_approach_v = -100.0; 

                        double v_circ = std::sqrt( simulation.GetkGravity() * current_target->GetMass().in_kg() / dist ); 
                        Vector tangent( -dir_to_target.y, dir_to_target.x ); 
                        if ( tangent.x * relative_vel.x + tangent.y * relative_vel.y < 0 ) tangent = Vector( dir_to_target.y, -dir_to_target.x );

                        Vector ideal_rel_vel = ( dir_to_target * desired_approach_v ) + ( tangent * v_circ );
                        Vector target_vel_global = current_target->GetVelocity() + ideal_rel_vel;
                        Vector vel_error = target_vel_global - player_ship->GetVelocity();
                        
                        if (vel_error.Length() > 5.0) {
                            current_thrust = vel_error * 10000.0;
                            if ( current_thrust.Length() > kEnginePower ) current_thrust = current_thrust.Normalized() * kEnginePower;
                        }
                    }
                }
                else if ( current_state == "ProgradeBurn" ) {
                    Vector prograde = player_ship->GetVelocity().Normalized();
                    current_thrust = prograde * kEnginePower;
                } 
                else if ( current_state == "Capture" ) {
                    Vector to_target = current_target->GetPosition() - player_ship->GetPosition(); 
                    double dist = to_target.Length();
                    Vector relative_vel = player_ship->GetVelocity() - current_target->GetVelocity();

                    double target_radius = current_target->GetRadius() * 2.0; 

                    // ЭКОНОМИЯ ТОПЛИВА: Пока мы далеко от перицентра, просто падаем под гравитацией
                    if ( dist > target_radius * 1.5 ) {
                        current_thrust = Vector(0,0);
                    } else {
                        // БЬЁМ ПО ТОРМОЗАМ: Выход на орбиту (Retrograde burn)
                        Vector dir_to_target = to_target.Normalized(); 
                        double dist_error = dist - target_radius; 
                        double desired_approach_v = dist_error * 0.1; 
                        
                        if ( desired_approach_v > 100.0 ) desired_approach_v = 100.0; 
                        if ( desired_approach_v < -100.0 ) desired_approach_v = -100.0; 

                        double safe_dist = std::max(dist, current_target->GetRadius() + 50000.0); 
                        double v_circ = std::sqrt( simulation.GetkGravity() * current_target->GetMass().in_kg() / safe_dist );

                        Vector tangent( -dir_to_target.y, dir_to_target.x );
                        if ( tangent.x * relative_vel.x + tangent.y * relative_vel.y < 0 ) tangent = Vector( dir_to_target.y, -dir_to_target.x );

                        Vector ideal_rel_vel = ( dir_to_target * desired_approach_v ) + ( tangent * v_circ );
                        Vector target_vel_global = current_target->GetVelocity() + ideal_rel_vel;
                        Vector vel_error = target_vel_global - player_ship->GetVelocity();
                        
                        if (vel_error.Length() > 2.0) {
                            current_thrust = vel_error * 50000.0; 
                            if ( current_thrust.Length() > kEnginePower ) current_thrust = current_thrust.Normalized() * kEnginePower;
                        }
                    }
                }

                if ( player_ship->GetFuelMass().in_kg() <= 0.0 ) {
                    current_thrust = Vector( 0.0, 0.0 );
                    if (current_state != "Off") autopilot.SetCurrentState("Off");
                }

                player_ship->SetThrust( current_thrust );

                double max_step = (current_thrust.LengthSquared() > 0.1) ? 0.5 : 300.0;
                double remaining_dt = sim_dt - time_simulated;
                double step_dt = (remaining_dt < max_step) ? remaining_dt : max_step;

                double dist_to_earth = ( player_ship->GetPosition() - earth->GetPosition() ).Length();
                double dist_to_moon = ( player_ship->GetPosition() - moon->GetPosition() ).Length();
                double dist_to_selene = ( player_ship->GetPosition() - selene->GetPosition() ).Length();

                if ( dist_to_earth <= earth->GetRadius() * 0.75 ) { is_game_over = true; game_over_reason = "CRITICAL FAILURE: Crashed into Earth!"; break; }
                if ( dist_to_moon <= moon->GetRadius() * 0.75 ) { is_game_over = true; game_over_reason = "CRITICAL FAILURE: Crashed into the Moon!"; break; }
                if ( dist_to_selene <= selene->GetRadius() * 0.75 ) { is_game_over = true; game_over_reason = "CRITICAL FAILURE: Crashed into Selene!"; break; }
                if ( dist_to_earth > 2.0e9 ) { is_game_over = true; game_over_reason = "MISSION FAILED: Lost in deep space!"; break; }

                simulation.Update( step_dt );
                time_simulated += step_dt;
            }
        } else {
            player_ship->SetThrust(Vector(0.0, 0.0));
            autopilot.SetCurrentState("Off");
        }

        // ===== ОТРИСОВКА ИНТЕРФЕЙСА IMGUI =====

        ImGui::SetNextWindowPos(ImVec2(20.f, 20.f), ImGuiCond_Once);
        ImGuiWindowFlags fuel_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        ImGui::Begin("Fuel System", nullptr, fuel_flags); 
            ImGui::Text("Fuel Level:"); 
            float fuel_ratio = static_cast<float>(player_ship->GetFuelMass().in_kg() / player_ship->GetMaxFuelMass().in_kg());
            
            if (fuel_ratio < 0.25f) ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.1f, 0.1f, 1.0f)); 
            else ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.1f, 0.8f, 0.1f, 1.0f)); 
            
            ImGui::ProgressBar(fuel_ratio, ImVec2(250.f, 25.f));
            ImGui::PopStyleColor(); 
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(20.f, 100.f), ImGuiCond_Once);
        ImGuiWindowFlags time_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        ImGui::Begin("Time Control", nullptr, time_flags);
            ImGui::Text("Simulation Speed:");
            ImGui::SliderFloat("##TimeWarp", &time_warp, 1.0f, 1000000.0f, "%.0f x", ImGuiSliderFlags_Logarithmic);
        ImGui::End();

        std::string current_orbit;
        double dist_to_earth = (player_ship->GetPosition() - earth->GetPosition()).Length();
        double dist_to_target = (player_ship->GetPosition() - current_target->GetPosition()).Length();
        double ui_target_orbit_radius = (current_target->GetPosition() - earth->GetPosition()).Length();
        double ui_target_soi = ui_target_orbit_radius * std::pow(current_target->GetMass().in_kg() / earth->GetMass().in_kg(), 0.4) * 1.5;
        double display_speed = 0.0;

        if (dist_to_target < ui_target_soi) { 
            current_orbit = "Target Orbit";
            display_speed = (player_ship->GetVelocity() - current_target->GetVelocity()).Length();
        } else if (dist_to_earth < 250000000.0) { 
            current_orbit = "Earth Parking Orbit";
            display_speed = player_ship->GetVelocity().Length();
        } else {
            current_orbit = "Transfer Trajectory"; 
            display_speed = player_ship->GetVelocity().Length();
        }

        ImGui::SetNextWindowPos(ImVec2(20.f, 180.f), ImGuiCond_Once);
        ImGuiWindowFlags telemetry_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        ImGui::Begin("Ship Telemetry", nullptr, telemetry_flags);
            double current_mass = player_ship->GetMass().in_kg();
            ImGui::Text( "Rel. Speed: %.2f m/s", display_speed );
            ImGui::Text( "Mass:       %.2f kg", current_mass );
        ImGui::End();

        std::string state_str = autopilot.GetCurrentState();
        std::string phase_text;
        if ( state_str == "Off" )                phase_text = "0 - Manual Control";
        else if ( state_str == "WaitAlignment" ) phase_text = "1 - Await Alignment";
        else if ( state_str == "ProgradeBurn" )  phase_text = "2 - Prograde Burn";
        else if ( state_str == "Coast" )         phase_text = "3 - Coasting";
        else if ( state_str == "Capture" )       phase_text = "4 - Capture Burn";
        else if ( state_str == "OrbitMoon" )     phase_text = "5 - Orbit Stabilized";

        ImGui::SetNextWindowPos( ImVec2( ( window_width / 2.0f ) - 200.f, window_height - 75.f ), ImGuiCond_Once );
        ImGui::SetNextWindowSize( ImVec2( 400.f, 0.f ), ImGuiCond_Once ); 
        ImGuiWindowFlags status_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
        ImGui::Begin( "Flight Status", nullptr, status_flags );
            ImGui::Columns( 2, "status_columns", true ); 
            ImGui::Text( "Current Location:" );
            ImGui::TextColored( ImVec4( 0.4f, 0.8f, 1.0f, 1.0f ), "%s", current_orbit.c_str() );
            ImGui::NextColumn(); 
            ImGui::Text( "Autopilot Phase:" );
            if (state_str == "Off") ImGui::TextColored( ImVec4( 0.6f, 0.6f, 0.6f, 1.0f ), "%s", phase_text.c_str() ); 
            else ImGui::TextColored( ImVec4( 0.1f, 0.8f, 0.1f, 1.0f ), "%s", phase_text.c_str() ); 
            ImGui::Columns( 1 ); 
        ImGui::End();

        ImGui::SetNextWindowPos( ImVec2( window_width - 270.f, window_height - 180.f ), ImGuiCond_Once );
        ImGuiWindowFlags auto_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        ImGui::Begin( "Autopilot Control", nullptr, auto_flags );
            std::string autopilot_ui_state = autopilot.GetCurrentState();
            ImGui::Text( "AI State: %s", autopilot_ui_state.c_str() );
            ImGui::Spacing();
            
            ImGui::TextColored( ImVec4( 0.8f, 0.8f, 0.8f, 1.0f ), "Select Mission Target:");
            if ( autopilot_ui_state != "Off" && autopilot_ui_state != "OrbitMoon" ) ImGui::BeginDisabled();
            if (ImGui::RadioButton( "Moon", &target_index, 0 ) ) current_target = moon;
            ImGui::SameLine();
            if ( ImGui::RadioButton( "Selene", &target_index, 1 ) ) current_target = selene;
            if ( autopilot_ui_state != "Off" && autopilot_ui_state != "OrbitMoon" ) ImGui::EndDisabled();
            
            ImGui::Spacing();
            
            if ( autopilot_ui_state == "Off" ) ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.4f, 0.4f, 0.4f, 1.0f ) ); 
            else ImGui::PushStyleColor( ImGuiCol_Button, ImVec4( 0.1f, 0.6f, 0.1f, 1.0f ) ); 

            if (ImGui::Button( "ENGAGE AUTOPILOT", ImVec2( 250.f, 40.f) ) ) {
                if ( autopilot_ui_state == "Off" || autopilot_ui_state == "OrbitMoon" ) autopilot.SetCurrentState( "WaitAlignment" );
                else autopilot.SetCurrentState( "Off" );
            }
            ImGui::PopStyleColor(); 
        ImGui::End();

        if ( is_game_over ) {
            ImGui::SetNextWindowPos(ImVec2( window_width / 2.0f, window_height / 2.0f ), ImGuiCond_Always, ImVec2( 0.5f, 0.5f ) );
            ImGuiWindowFlags over_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
            
            ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.1f, 0.0f, 0.0f, 0.95f ) );
            ImGui::PushStyleColor( ImGuiCol_Border, ImVec4( 1.0f, 0.0f, 0.0f, 1.0f ) );

            ImGui::Begin( "SYSTEM ALERT", nullptr, over_flags );
                ImGui::TextColored( ImVec4( 1.0f, 0.2f, 0.2f, 1.0f ), "%s", game_over_reason.c_str() );
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                if ( ImGui::Button( "EXIT SIMULATION", ImVec2( 300.f, 40.f ) ) ) window.close(); 
            ImGui::End();
            ImGui::PopStyleColor( 2 );
        }

        window.clear( sf::Color::Black );
        renderer.Draw( window, simulation.GetUniverse() );
        
        ImGui::SFML::Render( window );
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
