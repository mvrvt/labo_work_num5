#include "Renderer.hpp"
#include "SpaceShip.hpp"

void Renderer::Draw( sf::RenderWindow& window, const std::vector<std::shared_ptr<ICelestialBody> >& universe ) const {
    for ( const auto& body : universe ) {
        // Создание кружка нужного радиуса 
        sf::CircleShape shape( static_cast<float>( body->GetRadius() ) );
        shape.setOrigin( {static_cast<float>( body->GetRadius() ), static_cast<float>( body->GetRadius() )} );
        shape.setPosition( {static_cast<float>( body->GetPosition().x ), static_cast<float>( body->GetPosition().y )} );

        // --- Раскрашиваем объекты полиморфно ---
        if ( body->GetName() == "Moon" ) {
            shape.setFillColor( sf::Color( 160, 160, 160 ) ); // Серый цвет для спутника
        } else if ( body->IsStatic() ) {
            shape.setFillColor( sf::Color::Blue );            // Синий для Земли
        } else {
            shape.setFillColor( sf::Color::Red );             // Красный для корабля
        }

        window.draw( shape );

        // --- ДИНАМИЧЕСКОЕ ПРИВЕДЕНИЕ ТИПОВ ДЛЯ HUD ---
        // Пытаемся безопасно "привести" базовый интерфейс к конкретному Кораблю
        auto ship = std::dynamic_pointer_cast<SpaceShip>( body );
        if ( ship ) {
            // Рисуем серую подложку бака в левом верхнем углу (20, 20)
            sf::RectangleShape fuel_bg( {200.f, 20.f} );
            fuel_bg.setPosition( {20.f, 20.f} );
            fuel_bg.setFillColor( sf::Color( 60, 60, 60 ) );
            window.draw( fuel_bg );

            // Вычисляем процент оставшегося топлива
            float fuel_ratio = static_cast<float>( ship->GetFuelMass() / ship->GetMaxFuelMass() );
            
            // Рисуем зелёную полоску поверх (если мало топлива - делаем её красной!)
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
