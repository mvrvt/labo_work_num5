#include "Renderer.hpp"
#include "SpaceShip.hpp"
#include <iostream>

Renderer::Renderer() {
    // Код для того, чтобы Земля отображалась как изображение Земли, а не как синий круг
    if ( !earth_texture_.loadFromFile( "assets/Earth.png" ) ) {
        std::cerr << "Error: Could not find assets/Earth.png Check your working directory!" << std::endl;
    }
    earth_texture_.setSmooth( true ); 

    // Отрисовка текстуры Луны
    if ( !moon_texture_.loadFromFile( "assets/Moon.png" ) ) {
        std::cerr << "Error: Could not find assets/Moon.png Check your working directory!" << std::endl;
    }
    moon_texture_.setSmooth( true );

}

void Renderer::Draw( sf::RenderWindow& window, Sequence<std::shared_ptr<ICelestialBody>>* universe ) const {
    if (!universe) return; 

    // 1. Сначала рисуем визуальные границы орбит и гравитации
    for ( int i = 0; i < universe->GetLength(); ++i ) {
        std::shared_ptr<ICelestialBody> body = universe->Get( i );
        
        if ( body->GetName() == "Earth" ) {
            // Траектория орбиты Луны
            sf::CircleShape moon_orbit( 350.f );
            moon_orbit.setOrigin( {350.f, 350.f} );
            moon_orbit.setPosition( {static_cast<float>(body->GetPosition().x), static_cast<float>(body->GetPosition().y)} );
            moon_orbit.setFillColor( sf::Color::Transparent );
            moon_orbit.setOutlineThickness( 1.f );
            moon_orbit.setOutlineColor( sf::Color(100, 100, 100, 100) ); // Полупрозрачный серый
            window.draw( moon_orbit );
            
            // Траектория парковочной орбиты Корабля
            sf::CircleShape ship_orbit( 70.f );
            ship_orbit.setOrigin( {70.f, 70.f} );
            ship_orbit.setPosition( {static_cast<float>(body->GetPosition().x), static_cast<float>(body->GetPosition().y)} );
            ship_orbit.setFillColor( sf::Color::Transparent );
            ship_orbit.setOutlineThickness( 1.f );
            ship_orbit.setOutlineColor( sf::Color(100, 255, 100, 100) ); // Полупрозрачный зеленый
            window.draw( ship_orbit );
        }
        else if ( body->GetName() == "Moon" ) {
            // Сфера гравитационного влияния Луны (Capture zone = 100)
            sf::CircleShape capture_zone( 100.f );
            capture_zone.setOrigin( {100.f, 100.f} );
            capture_zone.setPosition( {static_cast<float>(body->GetPosition().x), static_cast<float>(body->GetPosition().y)} );
            capture_zone.setFillColor( sf::Color(50, 50, 255, 20) ); 
            capture_zone.setOutlineThickness( 1.f );
            capture_zone.setOutlineColor( sf::Color(100, 100, 255, 120) );
            window.draw( capture_zone );
        }
    }

    // 2. Затем рисуем сами небесные тела и корабль
    for ( int i = 0; i < universe->GetLength(); ++i ) {
        std::shared_ptr<ICelestialBody> body = universe->Get( i );

        sf::CircleShape shape( static_cast<float>( body->GetRadius() ) );
        shape.setOrigin( {static_cast<float>( body->GetRadius() ), static_cast<float>( body->GetRadius() )} );
        shape.setPosition( {static_cast<float>( body->GetPosition().x ), static_cast<float>( body->GetPosition().y )} );

        if ( body->GetName() == "Earth" ) {
            shape.setTexture( &earth_texture_ );
        } else if ( body->GetName() == "Moon" ) {
            shape.setTexture( &moon_texture_ );
            // shape.setFillColor( sf::Color( 160, 160, 160 ) ); 
        } else { // Корабль
            shape.setFillColor( sf::Color::Red );
        }

        window.draw( shape );

        
        // // Интерфейс HUD топлива
        // auto ship = std::dynamic_pointer_cast<SpaceShip>( body );
        // if ( ship ) {
        //     sf::RectangleShape fuel_bg( {200.f, 20.f} );
        //     fuel_bg.setPosition( {20.f, 20.f} );
        //     fuel_bg.setFillColor( sf::Color( 60, 60, 60 ) );
        //     window.draw( fuel_bg );

        //     float fuel_ratio = static_cast<float>( ship->GetFuelMass() / ship->GetMaxFuelMass() );
            
        //     sf::RectangleShape fuel_bar( {200.f * fuel_ratio, 20.f} );
        //     fuel_bar.setPosition( {20.f, 20.f} );
        //     if ( fuel_ratio > 0.25f ) {
        //         fuel_bar.setFillColor( sf::Color::Green );
        //     } else {
        //         fuel_bar.setFillColor( sf::Color::Red );
        //     }
        //     window.draw( fuel_bar );
        // }
    }
}
