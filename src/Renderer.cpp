#include "Renderer.hpp"
#include <iostream>

Renderer::Renderer() {
    if ( !earth_texture_.loadFromFile( "assets/Earth.png" ) ) std::cerr << "Error loading Earth\n";
    earth_texture_.setSmooth( true ); 
    if ( !moon_texture_.loadFromFile( "assets/Moon.png" ) ) std::cerr << "Error loading Moon\n";
    moon_texture_.setSmooth( true );
    if ( !selene_texture_.loadFromFile( "assets/Selene(Pluto).png" ) ) std::cerr << "Error loading Selene\n";
    selene_texture_.setSmooth( true );
}

void Renderer::Draw( sf::RenderWindow& window, Sequence<ICelestialBody*>* universe ) const {
    if (!universe) return; 

    // 1. Границы орбит и SOI (рисуются только в Симуляции 1)
    for ( int i = 0; i < universe->GetLength(); ++i ) {
        ICelestialBody* body = universe->Get( i );
        
        if ( body->GetName() == "Earth" ) {
            sf::CircleShape moon_orbit( 350.f );
            moon_orbit.setPointCount( 150 ); 
            moon_orbit.setOrigin( {350.f, 350.f} );
            moon_orbit.setPosition( {static_cast<float>(body->GetPosition().x), static_cast<float>(body->GetPosition().y)} );
            moon_orbit.setFillColor( sf::Color::Transparent );
            moon_orbit.setOutlineThickness( 1.f );
            moon_orbit.setOutlineColor( sf::Color(100, 100, 100, 100) ); 
            window.draw( moon_orbit );
            
            sf::CircleShape ship_orbit( 70.f );
            ship_orbit.setPointCount( 100 ); 
            ship_orbit.setOrigin( {70.f, 70.f} );
            ship_orbit.setPosition( {static_cast<float>(body->GetPosition().x), static_cast<float>(body->GetPosition().y)} );
            ship_orbit.setFillColor( sf::Color::Transparent );
            ship_orbit.setOutlineThickness( 1.f );
            ship_orbit.setOutlineColor( sf::Color(100, 255, 100, 100) ); 
            window.draw( ship_orbit );

            sf::CircleShape selene_orbit( 800.f );
            selene_orbit.setPointCount( 200 ); 
            selene_orbit.setOrigin( {800.f, 800.f} );
            selene_orbit.setPosition( {static_cast<float>(body->GetPosition().x), static_cast<float>(body->GetPosition().y)} );
            selene_orbit.setFillColor( sf::Color::Transparent );
            selene_orbit.setOutlineThickness( 1.f );
            selene_orbit.setOutlineColor( sf::Color( 100, 100, 100, 100 ) ); 
            window.draw( selene_orbit );
        }
        else if ( body->GetName() == "Moon" ) {
            sf::CircleShape capture_zone( 100.f );
            capture_zone.setOrigin( {100.f, 100.f} );
            capture_zone.setPosition( {static_cast<float>(body->GetPosition().x), static_cast<float>(body->GetPosition().y)} );
            capture_zone.setFillColor( sf::Color(50, 50, 255, 20) ); 
            capture_zone.setOutlineThickness( 1.f );
            capture_zone.setOutlineColor( sf::Color(100, 100, 255, 120) );
            window.draw( capture_zone );
        } 
        else if ( body->GetName() == "Selene" ) {
            double soi = 800.0 * std::pow(300000.0 / 100000000.0, 0.4); 
            sf::CircleShape soi_shape( static_cast<float>(soi) );
            soi_shape.setOrigin( {static_cast<float>(soi), static_cast<float>(soi)} );
            soi_shape.setPosition( {static_cast<float>(body->GetPosition().x), static_cast<float>(body->GetPosition().y)} );
            soi_shape.setFillColor( sf::Color( 100, 50, 150, 40 ) ); 
            soi_shape.setOutlineThickness( 1.f );
            soi_shape.setOutlineColor( sf::Color( 150, 100, 200, 120 ) );
            window.draw( soi_shape );
        }
    }

    // 2. Рисуем тела. Адаптировано для Симуляции 1 и 2!
    for ( int i = 0; i < universe->GetLength(); ++i ) {
        ICelestialBody* body = universe->Get( i );

        sf::CircleShape shape( static_cast<float>( body->GetRadius() ) );
        shape.setOrigin( {static_cast<float>( body->GetRadius() ), static_cast<float>( body->GetRadius() )} );
        shape.setPosition( {static_cast<float>( body->GetPosition().x ), static_cast<float>( body->GetPosition().y )} );

        std::string name = body->GetName();
        // Красивое распределение текстур для планет N-body системы
        if ( name == "Earth" || name == "Alpha" ) {
            shape.setTexture( &earth_texture_ );
        } else if ( name == "Moon" || name == "AlphaMoon" || name == "BetaMoon" || name == "Omega (Outer)" ) {
            shape.setTexture( &moon_texture_ );
        } else if ( name == "Selene" || name == "Beta" ) {
            shape.setFillColor( sf::Color( 180, 150, 220 ) );
            shape.setTexture( &selene_texture_ );
        } else { 
            shape.setFillColor( sf::Color::Red ); // Корабль
        }

        window.draw( shape );
    }
}
