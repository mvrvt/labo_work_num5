#include "SpaceShip.hpp"

SpaceShip::SpaceShip( const std::string& name, physics::Mass dry_mass, physics::Mass fuel_mass, double radius, const Vector& position, const Vector& velocity )
    : name_( name ), dry_mass_( dry_mass ), fuel_mass_( fuel_mass ), max_fuel_mass_( fuel_mass ), radius_( radius ), position_( position ), velocity_( velocity ), thrust_( 0.0, 0.0 ) { }

std::string SpaceShip::GetName() const { return name_; }

physics::Mass SpaceShip::GetMass() const {
    return dry_mass_ + fuel_mass_;
}

double SpaceShip::GetRadius()   const { return radius_; }
Vector SpaceShip::GetPosition() const { return position_; }
Vector SpaceShip::GetVelocity() const { return velocity_; }

bool SpaceShip::IsStatic() const {
    return false; 
}

void SpaceShip::UpdateState( const Vector& new_position, const Vector& new_velocity ) {
    position_ = new_position;
    velocity_ = new_velocity;
}

physics::Mass SpaceShip::GetFuelMass() const {
    return fuel_mass_;
}

void SpaceShip::ConsumeFuel( physics::Mass amount ) {
    fuel_mass_ = std::max( physics::Mass( 0.0 ), fuel_mass_ - amount );
}

void SpaceShip::SetThrust( const Vector& thrust ) {
    thrust_ = thrust;
}

Vector SpaceShip::GetThrust() const {
    return thrust_;
}

void SpaceShip::Tick( double dt ) {
    // Если двигатель работает (вектор тяги не нулевой) 
    if ( thrust_.LengthSquared() > 0.0 ) {
        
        // Реалистичная физика: Скорость истечения газов (Exhaust velocity)
        // Для вакуумного маршевого двигателя это примерно 3100 м/с
        const double kExhaustVelocity = 3100.0;

        // Расход массы = Тяга / Скорость истечения
        double mass_flow_rate = thrust_.Length() / kExhaustVelocity;
        
        // Считаем, сколько сгорело за шаг времени dt
        double fuel_burned = mass_flow_rate * dt;
        ConsumeFuel( physics::Mass(fuel_burned) );

        // Если топливо закончилось - принудительно отключаем двигатель
        if ( fuel_mass_.in_kg() <= 0.0 ) {
            thrust_ = Vector( 0.0, 0.0 );
        }
    }
}

physics::Mass SpaceShip::GetMaxFuelMass() const {
    return max_fuel_mass_;
}
