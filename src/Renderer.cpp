#include "Renderer.hpp"
#include "SpaceShip.hpp"
#include <iostream>

Renderer::Renderer() {
    // ИСПРАВЛЕНИЕ: Точный путь к твоей картинке Земли внутри проекта
    if (!earth_texture_.loadFromFile("assets/Earth_2.png")) {
        std::cerr << "Error: Could not find assets/Earth_2.png. Check your working directory!\n";
    }
    earth_texture_.setSmooth(true); 
}

void Renderer::Draw( sf::RenderWindow& window, Sequence<std::shared_ptr<ICelestialBody>>* universe ) const {
    if (!universe) return; 

    for ( int i = 0; i < universe->GetLength(); ++i ) {
        std::shared_ptr<ICelestialBody> body = universe->Get( i );

        sf::CircleShape shape( static_cast<float>( body->GetRadius() ) );
        shape.setOrigin( {static_cast<float>( body->GetRadius() ), static_cast<float>( body->GetRadius() )} );
        shape.setPosition( {static_cast<float>( body->GetPosition().x ), static_cast<float>( body->GetPosition().y )} );

        if ( body->GetName() == "Earth" ) {
            shape.setTexture( &earth_texture_ );
        } else if ( body->GetName() == "Moon" ) {
            shape.setFillColor( sf::Color( 160, 160, 160 ) ); 
        } else {
            shape.setFillColor( sf::Color::Red );             
        }

        window.draw( shape );

        // Интерфейс HUD топлива
        auto ship = std::dynamic_pointer_cast<SpaceShip>( body );
        if ( ship ) {
            sf::RectangleShape fuel_bg( {200.f, 20.f} );
            fuel_bg.setPosition( {20.f, 20.f} );
            fuel_bg.setFillColor( sf::Color( 60, 60, 60 ) );
            window.draw( fuel_bg );

            float fuel_ratio = static_cast<float>( ship->GetFuelMass() / ship->GetMaxFuelMass() );
            
            sf::RectangleShape fuel_bar( {200.f * fuel_ratio, 20.f} );
            fuel_bar.setPosition( {20.f, 20.f} );
            if ( fuel_ratio > 0.25f ) {
                fuel_bar.setFillColor( sf::Color::Green );
            } else {
                fuel_bar.setFillColor( sf::Color::Red );
            }
            window.draw( fuel_bar );
        }
    }
}
