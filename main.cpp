#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "Planet.hpp"
#include "SpaceShip.hpp"
#include "Simulation.hpp"
#include "Renderer.hpp"

int main() {
    sf::RenderWindow window( sf::VideoMode({800, 800}), "Space Simulation: Lab 5" );
    window.setFramerateLimit( 120 );

    // 1. Создаем физический движок
    Simulation simulation;
    
    // Добавляем объекты
    simulation.AddBody( std::make_shared<Planet>(
        "Earth", 10000.0, 40.0, Vector( 400.0, 400.0 )
    ));

    simulation.AddBody( std::make_shared<SpaceShip>(
        "Apollo", 10.0, 50.0, 5.0, Vector( 200.0, 400.0 ), Vector( 0.0, -100.0 )
    ));

    // 2. Создаем отрисовщик
    Renderer renderer;

    sf::Clock physics_clock;

    // Главный цикл
    while ( window.isOpen() ) {
        while ( const std::optional<sf::Event> event = window.pollEvent() ) {
            if ( event->is<sf::Event::Closed>() ) {
                window.close();
            }
        }

        // Логика (Физика)
        double dt = physics_clock.restart().asSeconds();
        simulation.Update( dt );

        // Отрисовка
        window.clear( sf::Color::Black );
        
        // Передаем окно и данные вселенной в рендерер
        renderer.Draw( window, simulation.GetUniverse() );
        
        window.display();
    }

    return 0;
}
