#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include "ICelestialBody.hpp"

class Renderer {
public:
    // Метод константный, так как отрисовка не должна менять состояние рендерера или окна
    // Окно SFML передается по ссылке (sf::RenderWindow&), так как копировать окна нельзя
    void Draw( sf::RenderWindow& window, const std::vector<std::shared_ptr<ICelestialBody>>& universe ) const;
};
