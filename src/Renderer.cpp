#include "Renderer.hpp"

void Renderer::Draw( sf::RenderWindow& window, const std::vector<std::shared_ptr<ICelestialBody> >& universe ) const {
    for ( const auto& body : universe ) {
        // Создание кружка нужного радиуса 
        sf::CircleShape shape( static_cast<float>( body->GetRadius() ) );
        shape.setOrigin( {static_cast<float>( body->GetRadius() ), static_cast<float>( body->GetRadius() )} );
        shape.setPosition( {static_cast<float>( body->GetPosition().x ), static_cast<float>( body->GetPosition().y )} );

        // Раскраишиваем
        if ( body->IsStatic() ) {
            shape.setFillColor( sf::Color::Blue );
        } else {
            shape.setFillColor( sf::Color::Red );
        }

        window.draw( shape );
    }
}
