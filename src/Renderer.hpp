#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include "ICelestialBody.hpp"
#include "lab2_files/Sequence.hpp"

class Renderer {
public:
    // Метод константный, так как отрисовка не должна менять состояние рендерера или окна
    // Окно SFML передается по ссылке (sf::RenderWindow&), так как копировать окна нельзя

    Renderer();
    // Передаем указатель на изменяемый Sequence, чтобы избежать конфликтов с константностью методов ЛР-2
    void Draw( sf::RenderWindow& window, Sequence<std::shared_ptr<ICelestialBody>>* universe ) const;

private:
    sf::Texture earth_texture_; // Поле для хранения текстуры Земли 
    sf::Texture moon_texture_;  // Поле для изображения Луны
};
