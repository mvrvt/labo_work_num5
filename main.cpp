#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include "Vector.hpp"
#include "Planet.hpp"

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 800}), "Space Simulation: Lab 5");
    window.setFramerateLimit(60);

    // Создаем нашу планету (Землю)
    // Имя: Earth, Масса: огромная (пока просто число), Радиус: 40 пикселей, Позиция: x=400, y=400 (центр экрана)
    Planet earth("Earth", 5.97e24, 40.0, Vector(400.0, 400.0));

    // Создаем графическое представление (Круг) для SFML
    sf::CircleShape earth_shape(earth.GetRadius());
    
    // SFML рисует круг от левого верхнего угла. 
    // Чтобы центр круга совпадал с координатами планеты, нужно сдвинуть "центр опоры" (Origin)
    earth_shape.setOrigin({static_cast<float>(earth.GetRadius()), static_cast<float>(earth.GetRadius())});
    
    // Задаем цвет (например, синий)
    earth_shape.setFillColor(sf::Color::Blue);

    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        // --- ЛОГИКА СИМУЛЯЦИИ (пока пустая, планета стоит на месте) ---
        
        // Синхронизируем позицию графики с позицией физического объекта
        Vector pos = earth.GetPosition();
        earth_shape.setPosition({static_cast<float>(pos.x), static_cast<float>(pos.y)});

        // --- ОТРИСОВКА ---
        window.clear(sf::Color::Black);
        
        window.draw(earth_shape); // Рисуем планету
        
        window.display();
    }

    return 0;
}
