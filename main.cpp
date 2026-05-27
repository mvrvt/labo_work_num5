#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <memory> 

#include "Vector.hpp"
#include "Planet.hpp"
#include "SpaceShip.hpp"
#include "Simulation.hpp"
#include "Renderer.hpp"

int main() {
    sf::RenderWindow window( sf::VideoMode({800, 800}), "Space Simulation: Lab 5" );
    window.setFramerateLimit( 120 );

    Simulation simulation;
    
    simulation.AddBody( std::make_shared<Planet>(
        "Earth", 10000.0, 40.0, Vector( 400.0, 400.0 )
    ));

    // Создаем корабль и ЗАПОМИНАЕМ указатель на него в отдельную переменную
    auto player_ship = std::make_shared<SpaceShip>(
        "Apollo", 10.0, 50.0, 5.0, Vector( 600.0, 400.0 ), Vector( 0.0, -220.0 )
    );
    
    // Добавляем этот же корабль в симуляцию
    simulation.AddBody( player_ship );

    Renderer renderer;
    sf::Clock physics_clock;

    // Мощность нашего двигателя
    const double kEnginePower = 5000.0;

    while ( window.isOpen() ) {
        while ( const std::optional<sf::Event> event = window.pollEvent() ) {
            if ( event->is<sf::Event::Closed>() ) {
                window.close();
            }
        }

        // --- УПРАВЛЕНИЕ С КЛАВИАТУРЫ ---
        Vector current_thrust( 0.0, 0.0 );

        // Проверяем, если топливо есть - даем рулить
        if ( player_ship->GetFuelMass() > 0.0 ) {
            // В SFML 3 используется sf::Keyboard::isKeyPressed()
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

        // Передаем команду в корабль
        player_ship->SetThrust( current_thrust );
        // -------------------------------

        double dt = physics_clock.restart().asSeconds();
        simulation.Update( dt );

        window.clear( sf::Color::Black );
        renderer.Draw( window, simulation.GetUniverse() );
        window.display();
    }

    return 0;
}