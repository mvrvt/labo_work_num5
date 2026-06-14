#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <memory> 
#include <iostream>
#include <cmath>
#include <optional> 

#include "Vector.hpp"
#include "Planet.hpp"
#include "Moon.hpp"
#include "SpaceShip.hpp"
#include "Simulation.hpp"
#include "Renderer.hpp"
#include "StateMachine.hpp" 

// Данные телеметрии для автомата
struct Telemetry {
    Vector earth_pos;
    Vector moon_pos;
    Vector ship_pos;
    Vector ship_vel;
    Vector moon_vel; 

    double dist_to_earth;
    double dist_to_moon;

    Telemetry() : dist_to_earth(0.0), dist_to_moon(0.0) { }

    Telemetry( Vector e, Vector m, Vector s, Vector sv, Vector mv ) 
        : earth_pos( e ), moon_pos( m ), ship_pos( s ), ship_vel( sv ), moon_vel( mv ) { 
        dist_to_earth = (s - e).Length();
        dist_to_moon = (s - m).Length();
    }
};

// 1. Ждем правильного фазового угла. Берем чуть раньше (1.50 - 1.60 рад), 
// чтобы Луна своей огромной массой сама затянула нас.
struct ConditionStartBurn {
    bool operator()( const Telemetry& t ) const {
        Vector to_ship = t.ship_pos - t.earth_pos;
        Vector to_moon = t.moon_pos - t.earth_pos;

        double dot = to_ship.x * to_moon.x + to_ship.y * to_moon.y;
        double det = to_ship.x * to_moon.y - to_ship.y * to_moon.x;
        double angle = std::atan2( det, dot );

        return angle > 1.50 && angle < 1.60;
    }
};

// 2. Разгоняемся ровно до 480. Этого хватит, чтобы долететь до орбиты Луны.
struct ConditionStopBurn {
    bool operator()( const Telemetry& t ) const {
        return t.ship_vel.Length() >= 480.0; 
    }
};

// 3. Зона захвата (Sphere of Influence). Включаем умный контроллер сильно заранее!
struct ConditionStartCapture {
    bool operator()( const Telemetry& t ) const {
        return t.dist_to_moon <= 180.0;
    }
};

// 4. Считаем, что мы на орбите, если дистанция около 45, а радиальная скорость (падение) близка к нулю.
struct ConditionOrbitEntered {
    bool operator()( const Telemetry& t ) const {
        if (t.dist_to_moon > 50.0 || t.dist_to_moon < 40.0) return false;
        
        Vector to_moon = t.moon_pos - t.ship_pos;
        Vector dir = to_moon.Normalized();
        Vector rel_vel = t.ship_vel - t.moon_vel;
        
        // Радиальная скорость (насколько быстро мы отдаляемся/приближаемся к центру Луны)
        double radial_v = std::abs(rel_vel.x * dir.x + rel_vel.y * dir.y);
        return radial_v < 5.0; // Орбита стабилизировалась
    }
};

int main() {
    sf::RenderWindow window( sf::VideoMode({1200, 900}), "Space Simulation: Lab 5" );
    window.setFramerateLimit( 120 );

    Simulation simulation;
    
    auto earth = std::make_shared<Planet>( "Earth", 10000.0, 40.0, Vector( 600.0, 450.0 ) );
    auto moon = std::make_shared<Moon>( "Moon", 2500.0, 15.0, Vector( 600.0, 100.0 ), Vector( 169.0, 0.0 ) );
    auto player_ship = std::make_shared<SpaceShip>( "Apollo", 0.01, 0.8, 5.0, Vector( 670.0, 450.0 ), Vector( 0.0, 378.0 ) );

    simulation.AddBody( earth );
    simulation.AddBody( moon );
    simulation.AddBody( player_ship );

    Renderer renderer;
    sf::Clock physics_clock;
    
    // Делаем тягу мощнее, чтобы контроллер успевал выруливать в критических ситуациях
    const double kEnginePower = 400.0; 

    fsm::StateMachine<Telemetry> autopilot;
    
    autopilot.AddState( "Off", true );
    autopilot.AddState( "WaitAlignment" ); 
    autopilot.AddState( "ProgradeBurn" );  
    autopilot.AddState( "Coast" );         
    autopilot.AddState( "Capture" );       
    autopilot.AddState( "OrbitMoon", true); 
    autopilot.SetInitialState( "Off" );

    autopilot.AddTransition( "WaitAlignment", "ProgradeBurn", ConditionStartBurn() );
    autopilot.AddTransition( "ProgradeBurn", "Coast", ConditionStopBurn() );
    autopilot.AddTransition( "Coast", "Capture", ConditionStartCapture() );
    autopilot.AddTransition( "Capture", "OrbitMoon", ConditionOrbitEntered() );

    // --- Настройка UI Кнопки ---
    sf::RectangleShape autopilot_btn(sf::Vector2f(150.f, 50.f));
    autopilot_btn.setPosition(sf::Vector2f(1020.f, 820.f)); 
    autopilot_btn.setOutlineThickness(3.f);
    autopilot_btn.setOutlineColor(sf::Color::White);

    while ( window.isOpen() ) {
        while ( const std::optional<sf::Event> event = window.pollEvent() ) {
            if ( event->is<sf::Event::Closed>() ) {
                window.close();
            }

            if ( const auto* mouse_btn = event->getIf<sf::Event::MouseButtonReleased>() ) {
                if ( mouse_btn->button == sf::Mouse::Button::Left ) {
                    sf::Vector2f mouse_pos_f(static_cast<float>(mouse_btn->position.x), static_cast<float>(mouse_btn->position.y));
                    
                    if ( autopilot_btn.getGlobalBounds().contains(mouse_pos_f) ) {
                        std::string current = autopilot.GetCurrentState();
                        if ( current == "Off" || current == "OrbitMoon" ) {
                            autopilot.SetCurrentState("WaitAlignment");
                            std::cout << ">> Autopilot: ENGAGED. Waiting for phase alignment...\n";
                        } else {
                            autopilot.SetCurrentState("Off");
                            std::cout << ">> Autopilot: DISABLED. Manual control.\n";
                        }
                    }
                }
            }
        }

        Vector current_thrust( 0.0, 0.0 );

        if ( player_ship->GetFuelMass() > 0.0 ) {
            
            Telemetry t(
                earth->GetPosition(), 
                moon->GetPosition(), 
                player_ship->GetPosition(),
                player_ship->GetVelocity(), 
                moon->GetVelocity()
            );

            bool state_changed = autopilot.Step(t);
            if (state_changed) {
                std::cout << ">> Autopilot transition to state: [" << autopilot.GetCurrentState() << "]\n";
            }

            std::string current_state = autopilot.GetCurrentState();
            
            // Цветовая индикация кнопки
            if ( current_state == "Off" ) {
                autopilot_btn.setFillColor(sf::Color(128, 128, 128)); // Серая
            } else {
                autopilot_btn.setFillColor(sf::Color(0, 200, 0));     // Зеленая
            }

            // --- Логика маневров ---
            if ( current_state == "WaitAlignment" || current_state == "Coast" || current_state == "OrbitMoon" ) {
                current_thrust = Vector( 0.0, 0.0 ); 
            }
            else if ( current_state == "ProgradeBurn" ) {
                // Разгон по вектору скорости (Prograde)
                Vector prograde = player_ship->GetVelocity().Normalized();
                current_thrust = prograde * kEnginePower;
            } 
            else if ( current_state == "Capture" ) {
                // --- УМНЫЙ АЛГОРИТМ ЗАХВАТА ---
                Vector to_moon = moon->GetPosition() - player_ship->GetPosition(); 
                double dist = to_moon.Length();
                Vector dir_to_moon = to_moon.Normalized(); // Вектор ОТ корабля К Луне
                Vector relative_vel = player_ship->GetVelocity() - moon->GetVelocity();

                // Целевая орбита
                double target_radius = 45.0; 
                double dist_error = dist - target_radius; // Насколько мы далеко от нужной орбиты

                // Желаемая скорость сближения (если далеко - сближаемся, если близко - выравниваемся)
                double desired_approach_v = dist_error * 1.2;
                if (desired_approach_v > 80.0) desired_approach_v = 80.0; // Ограничиваем скорость падения
                if (desired_approach_v < -15.0) desired_approach_v = -15.0; // Позволяем чуть-чуть отлететь, если промахнулись

                // Идеальная круговая скорость на целевой орбите
                double v_circ = std::sqrt( 1000.0 * 2500.0 / target_radius );

                // Выбираем вектор касательной (по направлению движения)
                Vector tangent(-dir_to_moon.y, dir_to_moon.x);
                if (tangent.x * relative_vel.x + tangent.y * relative_vel.y < 0) {
                    tangent = Vector(dir_to_moon.y, -dir_to_moon.x);
                }

                // Строим идеальный вектор скорости (сближение + закручивание по орбите)
                Vector ideal_rel_vel = (dir_to_moon * desired_approach_v) + (tangent * v_circ);
                Vector target_vel_global = moon->GetVelocity() + ideal_rel_vel;

                // Вычисляем ошибку и даем тягу
                Vector vel_error = target_vel_global - player_ship->GetVelocity();
                current_thrust = vel_error * 15.0; // Сильный PD-коэффициент для четкого управления
                
                if ( current_thrust.Length() > kEnginePower ) {
                    current_thrust = current_thrust.Normalized() * kEnginePower;
                }
            } 
            else if ( current_state == "Off" ) {
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Up ) )    current_thrust.y -= kEnginePower;
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Down ) )  current_thrust.y += kEnginePower;
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Left ) )  current_thrust.x -= kEnginePower;
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Right ) ) current_thrust.x += kEnginePower;
            }
        }

        player_ship->SetThrust( current_thrust );

        double dt = physics_clock.restart().asSeconds();
        if ( dt > 0.1 ) dt = 0.1; 
        simulation.Update( dt );

        window.clear( sf::Color::Black );
        renderer.Draw( window, simulation.GetUniverse() );
        
        window.draw(autopilot_btn);
        window.display();
    }

    return 0;
}
