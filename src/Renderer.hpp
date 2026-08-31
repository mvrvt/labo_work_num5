#pragma once

#include <SFML/Graphics.hpp>
#include "ICelestialBody.hpp"
#include "lab2_files/Sequence.hpp"

class Renderer {
public:
    Renderer();
    void Draw( sf::RenderWindow& window, Sequence<ICelestialBody*>* universe ) const;

private:
    sf::Texture earth_texture_; 
    sf::Texture moon_texture_;  
    sf::Texture selene_texture_;
};
