#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
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

enum class AppState {
    MainMenu,
    Simulation1, 
    Simulation2  
};

void InitSimulation1( Simulation* sim, ICelestialBody*& out_earth, ICelestialBody*& out_target, ICelestialBody*& out_ship ) {
    double center_x = 0.0;
    double center_y = 0.0;

    auto earth = new Planet( "Earth", 5.972e24_kg, 40000000.0, Vector( center_x, center_y ), Vector(0,0), true );
    auto moon = new Moon( "Moon", 7.342e22_kg, 15000000.0, Vector( center_x, center_y - 384400000.0 ), Vector( 1022.0, 0.0 ) );
    auto selene = new Moon( "Selene", 1.0e22_kg, 10000000.0, Vector( center_x, center_y + 800000000.0 ), Vector( -705.0, 0.0 ) );
    auto player_ship = new SpaceShip( "Soyuz", 7000.0_kg, 35000.0_kg, 5000000.0, Vector( center_x + 85000000.0, center_y ), Vector( 0.0, 2164.0 ) );
    
    sim->AddBody( earth );
    sim->AddBody( moon );
    sim->AddBody( selene );
    sim->AddBody( player_ship );

    out_earth = earth;
    out_target = moon;
    out_ship = player_ship;
}

void InitSimulation2( Simulation* sim ) {
    double G = 6.67430e-11;
    double planet_mass = 5.972e24; 
    double dist_from_center = 2.5e8; 

    double v_planet = std::sqrt( (G * planet_mass) / (4.0 * dist_from_center) );

    auto planet_a = new Planet( "Alpha", Mass(planet_mass), 30000000.0, Vector( -dist_from_center, 0.0 ), Vector( 0.0, -v_planet ), false );
    auto planet_b = new Planet( "Beta", Mass(planet_mass), 30000000.0, Vector( dist_from_center, 0.0 ), Vector( 0.0, v_planet ), false );

    double moon_mass = 7.342e22;
    double moon_dist = 4.0e7; 
    double v_moon_rel = std::sqrt( (G * planet_mass) / moon_dist );

    auto moon_a = new Moon( "AlphaMoon", Mass(moon_mass), 8000000.0, Vector( -dist_from_center - moon_dist, 0.0 ), Vector( 0.0, -v_planet - v_moon_rel ) );
    auto moon_b = new Moon( "BetaMoon", Mass(moon_mass), 8000000.0, Vector( dist_from_center + moon_dist, 0.0 ), Vector( 0.0, v_planet + v_moon_rel ) );

    double outer_dist = 2.0e9;
    double v_outer = std::sqrt( (G * (planet_mass * 2.0)) / outer_dist );
    auto outer_moon = new Moon( "Omega (Outer)", Mass(moon_mass), 12000000.0, Vector( 0.0, -outer_dist ), Vector( v_outer, 0.0 ) );

    sim->AddBody( planet_a );
    sim->AddBody( planet_b );
    sim->AddBody( moon_a );
    sim->AddBody( moon_b );
    sim->AddBody( outer_moon );
}

int main() {
    unsigned int window_width  = 1600;
    unsigned int window_height = 1000;

    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8; 

    sf::RenderWindow window( sf::VideoMode( { window_width, window_height} ), "Space Simulation: Lab 5", sf::State::Windowed, settings );
    window.setFramerateLimit( 120 );

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Не удалось инициализировать ImGui-SFML!\n";
        return -1;
    }

    sf::View camera = window.getDefaultView();
    camera.setCenter( sf::Vector2f( 0.0f, 0.0f ) );
    
    Renderer renderer;
    sf::Clock physics_clock;
    sf::Clock imgui_clock; 

    AppState current_state = AppState::MainMenu;
    Simulation* simulation = nullptr; 

    ICelestialBody* earth_ptr = nullptr;
    ICelestialBody* current_target = nullptr; 
    ICelestialBody* player_ship_ptr = nullptr;
    
    int target_index = 0; 
    const double kEnginePower = 200000.0; 
    float time_warp = 1.0f;
    bool is_game_over = false;
    std::string game_over_reason = "";

    // ИСПРАВЛЕНИЕ: Автопилот теперь тоже сырой указатель
    fsm::StateMachine<Telemetry>* autopilot = nullptr; 

    while ( window.isOpen() ) {
        while ( const std::optional<sf::Event> event = window.pollEvent() ) {
            ImGui::SFML::ProcessEvent( window, event.value() );
            if ( event->is<sf::Event::Closed>() ) {
                window.close();
            }
        }

        ImGui::SFML::Update( window, imgui_clock.restart() );
        double real_dt = physics_clock.restart().asSeconds();
        if ( real_dt > 0.1 ) real_dt = 0.1; 

        // =========================================================
        // ЛОГИКА ГЛАВНОГО МЕНЮ
        // =========================================================
        if ( current_state == AppState::MainMenu ) {
            ImGui::SetNextWindowPos(ImVec2(window_width / 2.0f, window_height / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400, 300));
            ImGui::Begin("Simulation Mode Selection", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
            
            ImGui::TextWrapped("Welcome to the Orbital Dynamics Sandbox. Select a simulation mode below:");
            ImGui::Spacing(); ImGui::Spacing();

            if ( ImGui::Button("1. Earth-Moon System (Autopilot Mission)", ImVec2(-1, 60)) ) {
                if (simulation) delete simulation; 
                simulation = new Simulation();
                
                InitSimulation1( simulation, earth_ptr, current_target, player_ship_ptr );
                
                // Пересоздаем автопилот для очистки состояний
                if (autopilot) delete autopilot;
                autopilot = new fsm::StateMachine<Telemetry>();
                
                autopilot->AddState( "Off", true );
                autopilot->AddState( "WaitAlignment" );  
                autopilot->AddState( "ProgradeBurn" );   
                autopilot->AddState( "Coast" );          
                autopilot->AddState( "Capture" );        
                autopilot->AddState( "OrbitMoon", true); 
                autopilot->SetInitialState( "Off" );

                double mu = simulation->GetkGravity() * earth_ptr->GetMass().in_kg();
                autopilot->AddTransition( "WaitAlignment", "ProgradeBurn", ConditionStartBurn( mu ) );
                autopilot->AddTransition( "ProgradeBurn", "Coast", ConditionStopBurn( mu ) );
                autopilot->AddTransition( "Coast", "Capture", ConditionStartCapture() );
                autopilot->AddTransition( "Capture", "OrbitMoon", ConditionOrbitEntered() );

                camera.setSize( sf::Vector2f( 2.4e9f, 1.5e9f ) );
                window.setView( camera ); 

                is_game_over = false;
                time_warp = 1.0f;
                current_state = AppState::Simulation1;
            }

            ImGui::Spacing();

            if ( ImGui::Button("2. Binary Planetary System (N-Body Orbit)", ImVec2(-1, 60)) ) {
                if (simulation) delete simulation;
                simulation = new Simulation();
                
                InitSimulation2( simulation );
                
                camera.setSize( sf::Vector2f( 6.0e9f, 4.0e9f ) );
                window.setView( camera ); 

                is_game_over = false;
                time_warp = 1.0f;
                current_state = AppState::Simulation2;
            }

            ImGui::End();
        } 
        // =========================================================
        // ЛОГИКА СИМУЛЯЦИИ 1
        // =========================================================
        else if ( current_state == AppState::Simulation1 && simulation ) {
            SpaceShip* player_ship = dynamic_cast<SpaceShip*>(player_ship_ptr);

            ImGui::SetNextWindowPos(ImVec2(window_width - 120.f, 20.f), ImGuiCond_Once);
            ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
            if (ImGui::Button("Back to Menu", ImVec2(100, 40))) {
                current_state = AppState::MainMenu;
                continue;
            }
            ImGui::End();

            bool manual_override = false;
            if ( autopilot && autopilot->GetCurrentState() == "Off" && !is_game_over ) {
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Up ) ||
                     sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Down ) ||
                     sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Left ) ||
                     sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Right ) ) {
                    if (time_warp > 500.0f) time_warp = 500.0f; 
                    manual_override = true;
                }
            }

            if ( !is_game_over && player_ship && autopilot ) {
                double sim_dt = real_dt * time_warp; 
                double time_simulated = 0.0;

                while ( time_simulated < sim_dt ) {
                    double target_orbit_radius = ( current_target->GetPosition() - earth_ptr->GetPosition() ).Length();
                    double target_soi = target_orbit_radius * std::pow( current_target->GetMass().in_kg() / earth_ptr->GetMass().in_kg(), 0.4 ) * 1.5;
                    
                    Telemetry t( earth_ptr->GetPosition(), current_target->GetPosition(), player_ship->GetPosition(), player_ship->GetVelocity(), current_target->GetVelocity(), current_target->GetMass().in_kg(), current_target->GetRadius(), target_soi );

                    autopilot->Step( t );
                    std::string current_state_fsm = autopilot->GetCurrentState();
                    Vector current_thrust( 0.0, 0.0 );

                    if ( current_state_fsm == "Off" ) {
                        double manual_power = 50000.0; 
                        if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Up ) ) current_thrust.y = -manual_power; 
                        if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Down ) ) current_thrust.y = manual_power;  
                        if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Left ) ) current_thrust.x = -manual_power; 
                        if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Right ) ) current_thrust.x = manual_power;  
                    }
                    else if ( current_state_fsm == "OrbitMoon" ) {
                        Vector to_target = current_target->GetPosition() - player_ship->GetPosition(); 
                        double dist = to_target.Length();
                        double park_radius = current_target->GetRadius() * 2.0; 
                        double dist_error = dist - park_radius; 
                        
                        if ( std::abs(dist_error) > 500000.0 ) {
                            Vector dir_to_target = to_target.Normalized(); 
                            Vector relative_vel = player_ship->GetVelocity() - current_target->GetVelocity();
                            double desired_approach_v = dist_error * 0.05;
                            if ( desired_approach_v > 100.0 ) desired_approach_v = 100.0; 
                            if ( desired_approach_v < -100.0 ) desired_approach_v = -100.0; 

                            double v_circ = std::sqrt( simulation->GetkGravity() * current_target->GetMass().in_kg() / dist ); 
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
                    else if ( current_state_fsm == "ProgradeBurn" ) {
                        current_thrust = player_ship->GetVelocity().Normalized() * kEnginePower;
                    } 
                    else if ( current_state_fsm == "Capture" ) {
                        Vector to_target = current_target->GetPosition() - player_ship->GetPosition(); 
                        double dist = to_target.Length();
                        Vector relative_vel = player_ship->GetVelocity() - current_target->GetVelocity();
                        double target_radius = current_target->GetRadius() * 2.0; 

                        if ( dist <= target_radius * 1.5 ) {
                            double dist_error = dist - target_radius; 
                            double desired_approach_v = std::clamp(dist_error * 0.1, -100.0, 100.0); 

                            double safe_dist = std::max(dist, current_target->GetRadius() + 50000.0); 
                            double v_circ = std::sqrt( simulation->GetkGravity() * current_target->GetMass().in_kg() / safe_dist );

                            Vector dir_to_target = to_target.Normalized(); 
                            Vector tangent( -dir_to_target.y, dir_to_target.x );
                            if ( tangent.x * relative_vel.x + tangent.y * relative_vel.y < 0 ) tangent = Vector( dir_to_target.y, -dir_to_target.x );

                            Vector ideal_rel_vel = ( dir_to_target * desired_approach_v ) + ( tangent * v_circ );
                            Vector vel_error = (current_target->GetVelocity() + ideal_rel_vel) - player_ship->GetVelocity();
                            
                            if (vel_error.Length() > 2.0) {
                                current_thrust = vel_error * 50000.0; 
                                if ( current_thrust.Length() > kEnginePower ) current_thrust = current_thrust.Normalized() * kEnginePower;
                            }
                        }
                    }

                    if ( player_ship->GetFuelMass().in_kg() <= 0.0 ) {
                        current_thrust = Vector( 0.0, 0.0 );
                        if (current_state_fsm != "Off") autopilot->SetCurrentState("Off");
                    }

                    player_ship->SetThrust( current_thrust );

                    double max_step = (current_thrust.LengthSquared() > 0.1) ? 0.5 : 300.0;
                    double remaining_dt = sim_dt - time_simulated;
                    double step_dt = (remaining_dt < max_step) ? remaining_dt : max_step;

                    if ( (player_ship->GetPosition() - earth_ptr->GetPosition()).Length() > 2.0e9 ) {
                        is_game_over = true; game_over_reason = "MISSION FAILED: Lost in deep space!"; break; 
                    }

                    simulation->Update( step_dt );
                    time_simulated += step_dt;
                }
            }

            ImGui::SetNextWindowPos(ImVec2(20.f, 20.f), ImGuiCond_Once);
            ImGui::Begin("Fuel System", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove); 
                float fuel_ratio = static_cast<float>(player_ship->GetFuelMass().in_kg() / player_ship->GetMaxFuelMass().in_kg());
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, fuel_ratio < 0.25f ? ImVec4(0.8f, 0.1f, 0.1f, 1.0f) : ImVec4(0.1f, 0.8f, 0.1f, 1.0f)); 
                ImGui::ProgressBar(fuel_ratio, ImVec2(250.f, 25.f));
                ImGui::PopStyleColor(); 
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(20.f, 100.f), ImGuiCond_Once);
            ImGui::Begin("Time Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
                if (manual_override) {
                    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "[ PHYSICAL WARP: MAX 500x ]");
                    ImGui::BeginDisabled();
                }
                ImGui::SliderFloat("##TimeWarp", &time_warp, 1.0f, 1000000.0f, "%.0f x", ImGuiSliderFlags_Logarithmic);
                if (manual_override) ImGui::EndDisabled();
            ImGui::End();

            if (autopilot) {
                ImGui::SetNextWindowPos( ImVec2( window_width - 270.f, window_height - 180.f ), ImGuiCond_Once );
                ImGui::Begin( "Autopilot Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove );
                    std::string ai_state = autopilot->GetCurrentState();
                    ImGui::Text( "AI State: %s", ai_state.c_str() );
                    ImGui::Spacing();
                    
                    if ( ai_state != "Off" && ai_state != "OrbitMoon" ) ImGui::BeginDisabled();
                    if (ImGui::RadioButton( "Moon", &target_index, 0 ) ) current_target = simulation->GetUniverse()->Get(1);
                    ImGui::SameLine();
                    if ( ImGui::RadioButton( "Selene", &target_index, 1 ) ) current_target = simulation->GetUniverse()->Get(2);
                    if ( ai_state != "Off" && ai_state != "OrbitMoon" ) ImGui::EndDisabled();
                    
                    ImGui::Spacing();
                    ImGui::PushStyleColor( ImGuiCol_Button, ai_state == "Off" ? ImVec4(0.4f, 0.4f, 0.4f, 1.0f) : ImVec4(0.1f, 0.6f, 0.1f, 1.0f) ); 
                    if (ImGui::Button( "ENGAGE AUTOPILOT", ImVec2( 250.f, 40.f) ) ) {
                        autopilot->SetCurrentState( (ai_state == "Off" || ai_state == "OrbitMoon") ? "WaitAlignment" : "Off" );
                    }
                    ImGui::PopStyleColor(); 
                ImGui::End();
            }

            if ( is_game_over ) {
                ImGui::SetNextWindowPos(ImVec2(window_width / 2.0f, window_height / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4( 0.1f, 0.0f, 0.0f, 0.95f ) );
                ImGui::Begin( "SYSTEM ALERT", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
                    ImGui::TextColored( ImVec4(1.f, 0.2f, 0.2f, 1.f), "%s", game_over_reason.c_str() );
                    if ( ImGui::Button( "EXIT SIMULATION", ImVec2( 300.f, 40.f ) ) ) current_state = AppState::MainMenu; 
                ImGui::End();
                ImGui::PopStyleColor();
            }

            window.clear( sf::Color::Black );
            renderer.Draw( window, simulation->GetUniverse() );
        }
        // =========================================================
        // ЛОГИКА СИМУЛЯЦИИ 2 (Бинарная система)
        // =========================================================
        else if ( current_state == AppState::Simulation2 && simulation ) {
            ImGui::SetNextWindowPos(ImVec2(window_width - 120.f, 20.f), ImGuiCond_Once);
            ImGui::Begin("Menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
            if (ImGui::Button("Back to Menu", ImVec2(100, 40))) {
                current_state = AppState::MainMenu;
                continue;
            }
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(20.f, 20.f), ImGuiCond_Once);
            ImGui::Begin("Time Control", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
                ImGui::Text("Simulation Speed:");
                ImGui::SliderFloat("##TimeWarp", &time_warp, 1.0f, 1000000.0f, "%.0f x", ImGuiSliderFlags_Logarithmic);
            ImGui::End();

            double sim_dt = real_dt * time_warp; 
            double time_simulated = 0.0;
            while ( time_simulated < sim_dt ) {
                double step_dt = (sim_dt - time_simulated > 300.0) ? 300.0 : (sim_dt - time_simulated);
                simulation->Update( step_dt );
                time_simulated += step_dt;
            }

            window.clear( sf::Color::Black );
            renderer.Draw( window, simulation->GetUniverse() );
        }

        ImGui::SFML::Render( window );
        window.display();
    }

    if (simulation) delete simulation; 
    if (autopilot) delete autopilot;
    
    ImGui::SFML::Shutdown();
    return 0;
}
