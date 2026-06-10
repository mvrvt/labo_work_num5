#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <memory> 
#include <iostream>

#include "Vector.hpp"
#include "Planet.hpp"
#include "Moon.hpp"
#include "SpaceShip.hpp"
#include "Simulation.hpp"
#include "Renderer.hpp"
#include "StateMachine.hpp" 

// Данные телеметрии для автомата
struct Telemetry {
    double dist_to_earth;
    double moon_dist_to_earth;
    double dist_to_moon;

    // ИСПРАВЛЕНИЕ: Конструктор по умолчанию обязателен для DynamicArray
    Telemetry() : dist_to_earth(0.0), moon_dist_to_earth(0.0), dist_to_moon(0.0) {}

    Telemetry(double d1, double d2, double d3) 
        : dist_to_earth(d1), moon_dist_to_earth(d2), dist_to_moon(d3) {}
};

int main() {
    sf::RenderWindow window( sf::VideoMode({800, 800}), "Space Simulation: Lab 5" );
    window.setFramerateLimit( 120 );

    Simulation simulation;
    
    auto earth = std::make_shared<Planet>( "Earth", 10000.0, 40.0, Vector( 400.0, 400.0 ) );
    
    // ИСПРАВЛЕНИЕ: Увеличили массу Луны до 2500.0, чтобы её гравитация работала честно!
    auto moon = std::make_shared<Moon>( "Moon", 2500.0, 15.0, Vector( 400.0, 120.0 ), Vector( 189.0, 0.0 ) );
    auto player_ship = std::make_shared<SpaceShip>( "Apollo", 10.0, 800.0, 5.0, Vector( 580.0, 400.0 ), Vector( 0.0, -235.0 ) );

    simulation.AddBody( earth );
    simulation.AddBody( moon );
    simulation.AddBody( player_ship );

    Renderer renderer;
    sf::Clock physics_clock;
    
    const double kEnginePower = 6000.0;

    fsm::StateMachine<Telemetry> autopilot;
    
    autopilot.AddState("Off", true);
    autopilot.AddState("ProgradeBurn");
    autopilot.AddState("Coast");
    autopilot.AddState("Capture", true);
    autopilot.SetInitialState("Off");

    autopilot.AddTransition("ProgradeBurn", "Coast", [](const Telemetry& t) {
        return t.dist_to_earth >= t.moon_dist_to_earth * 0.95;
    });
    autopilot.AddTransition("Coast", "Capture", [](const Telemetry& t) {
        return t.dist_to_moon < 80.0;
    });
    autopilot.AddTransition("Coast", "ProgradeBurn", [](const Telemetry& t) {
        return t.dist_to_earth < t.moon_dist_to_earth * 0.8; 
    });

    while ( window.isOpen() ) {
        while ( const std::optional<sf::Event> event = window.pollEvent() ) {
            if ( event->is<sf::Event::Closed>() ) {
                window.close();
            }

            if ( const auto* key_pressed = event->getIf<sf::Event::KeyPressed>() ) {
                if ( key_pressed->code == sf::Keyboard::Key::A ) {
                    if ( autopilot.GetCurrentState() == "Off" ) {
                        autopilot.SetCurrentState("ProgradeBurn");
                        std::cout << ">> Autopilot: ENGAGED. State: Prograde Burn (Expanding Orbit)\n";
                    } else {
                        autopilot.SetCurrentState("Off");
                        std::cout << ">> Autopilot: DISABLED. Switching to manual control.\n";
                    }
                }
            }
        }

        Vector current_thrust( 0.0, 0.0 );

        if ( player_ship->GetFuelMass() > 0.0 ) {
            
            Telemetry t(
                (player_ship->GetPosition() - earth->GetPosition()).Length(),
                (moon->GetPosition() - earth->GetPosition()).Length(),
                (player_ship->GetPosition() - moon->GetPosition()).Length()
            );

            bool state_changed = autopilot.Step(t);
            if (state_changed) {
                std::cout << ">> Autopilot transition to state: [" << autopilot.GetCurrentState() << "]\n";
            }

            std::string current_state = autopilot.GetCurrentState();
            
            if ( current_state == "ProgradeBurn" ) {
                Vector prograde = player_ship->GetVelocity().Normalized();
                current_thrust = prograde * kEnginePower;
            } 
            else if ( current_state == "Capture" ) {
                Vector to_target = moon->GetPosition() - player_ship->GetPosition();
                Vector relative_velocity = moon->GetVelocity() - player_ship->GetVelocity();
                
                double kp = 40.0; 
                double kv = 60.0; 
                
                current_thrust = ( to_target * kp ) + ( relative_velocity * kv );
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
        window.display();
    }

    return 0;
}
