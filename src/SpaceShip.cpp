#include "SpaceShip.hpp"

SpaceShip::SpaceShip( const std::string& name, double dry_mass, double fuel_mass, double radius, const Vector& position, const Vector& velocity )
    : name_( name ), dry_mass_( dry_mass ), fuel_mass_( fuel_mass ), radius_( radius ), position_( position ), velocity_( velocity ), thrust_( 0.0, 0.0 ) { }

std::string SpaceShip::GetName() const { return name_; }

double SpaceShip::GetMass() const {
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

double SpaceShip::GetFuelMass() const {
    return fuel_mass_;
}

void SpaceShip::ConsumeFuel( double amount ) {
    fuel_mass_ = std::max( 0.0, fuel_mass_ - amount );
}

void SpaceShip::SetThrust( const Vector& thrust ) {
    thrust_ = thrust;
}

Vector SpaceShip::GetThrust() const {
    return thrust_;
}

void SpaceShip::Tick( double dt ) {
    // Если двигатель работает (вектор тяги не нулевой) 
    if ( thrust_.LengthSquared() > 0.0 ){
        // Коэффициент расхода топлива. Чем больше тяга, тем больше сгорает топлива
        const double kFuelConsumptionRate = 0.05;

        double fuel_burned = thrust_.Length() * kFuelConsumptionRate * dt;
        ConsumeFuel( fuel_burned );

        // Если топливо закончилось - отключаем двигатель
        if ( fuel_mass_ <= 0.0 ) {
            thrust_ = Vector( 0.0, 0.0 );
        }
    }
}
