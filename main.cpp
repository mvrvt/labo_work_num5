#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <memory> 
#include <iostream>

#include "Vector.hpp"
#include "Planet.hpp"
#include "Moon.hpp"
#include "SpaceShip.hpp"
#include "Simulation.hpp"
#include "Renderer.hpp"

int main() {
    sf::RenderWindow window( sf::VideoMode({800, 800}), "Space Simulation: Lab 5" );
    window.setFramerateLimit( 120 );

    Simulation simulation;
    
    // Статичная Земля в центре
    simulation.AddBody( std::make_shared<Planet>(
        "Earth", 10000.0, 100.0, Vector( 400.0, 400.0 )
    ));

    // Рассчитываем точную круговую скорость для Луны на дистанции 280 пикселей.
    // Формула: v = sqrt(G * M / r) -> sqrt(1000.0 * 10000.0 / 280.0) ≈ 189.0
    auto moon = std::make_shared<Moon>(
        "Moon", 150.0, 15.0, Vector( 400.0, 120.0 ), Vector( 189.0, 0.0 )
    );
    simulation.AddBody( moon );

    // Создаем корабль на низкой орбите Земли (дистанция 180 пикселей, v ≈ 235.0)
    auto player_ship = std::make_shared<SpaceShip>(
        "Apollo", 10.0, 400.0, 5.0, Vector( 580.0, 400.0 ), Vector( 0.0, -235.0 )
        // SpaceShip( const std::string& name, double dry_mass, double fuel_mass, double radius, const Vector& position, const Vector& velocity );
    );
    simulation.AddBody( player_ship );

    Renderer renderer;
    sf::Clock physics_clock;

    const double kEnginePower = 6000.0;

    while ( window.isOpen() ) {
        while ( const std::optional<sf::Event> event = window.pollEvent() ) {
            if ( event->is<sf::Event::Closed>() ) {
                window.close();
            }
        }

        Vector current_thrust( 0.0, 0.0 );

        if ( player_ship->GetFuelMass() > 0.0 ) {
            // --- РЕЖИМ АВТОПИЛОТА (ЗАЖАТА КЛАВИША 'A') ---
            if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::A ) ) {
                // Вектор от корабля к Луне
                Vector to_target = moon->GetPosition() - player_ship->GetPosition();
                
                // Разница скоростей между Луной и кораблем (чтобы затормозить при подлёте)
                Vector relative_velocity = moon->GetVelocity() - player_ship->GetVelocity();
                
                // Коэффициенты физического регулятора
                double kp = 35.0;  // Сила, притягивающая к Луне
                double kv = 50.0;  // Демпфирование (гашение скорости, чтобы не пролететь мимо)
                
                current_thrust = ( to_target * kp ) + ( relative_velocity * kv );
                
                // Ограничиваем рассчитанную автопилотом тягу максимальной мощностью двигателя
                double thrust_magnitude = current_thrust.Length();
                if ( thrust_magnitude > kEnginePower ) {
                    current_thrust = ( current_thrust / thrust_magnitude ) * kEnginePower;
                }
            } 
            // --- РУЧНОЕ УПРАВЛЕНИЕ (СТРЕЛКИ) ---
            else {
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Up ) ) {
                    current_thrust.y -= kEnginePower;
                }
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Down ) ) {
                    current_thrust.y += kEnginePower;
                }
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Left ) ) {
                    current_thrust.x -= kEnginePower;
                } 
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Right ) ) {
                    current_thrust.x += kEnginePower;
                }
            }
        }

        player_ship->SetThrust( current_thrust );

        double dt = physics_clock.restart().asSeconds();
        
        // Предотвращаем резкие скачки физики при лагах окна (например, при перетаскивании)
        if ( dt > 0.1 ) dt = 0.1; 

        simulation.Update( dt );

        window.clear( sf::Color::Black );
        renderer.Draw( window, simulation.GetUniverse() );
        window.display();
    }

    return 0;
}