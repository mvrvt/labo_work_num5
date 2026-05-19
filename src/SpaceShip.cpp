#include "SpaceShip.hpp"

SpaceShip::SpaceShip( const std::string& name, double dry_mass, double fuel_mass, double radius, const Vector& position, const Vector& velocity )
    : name_( name ), dry_mass_( dry_mass ), fuel_mass_( fuel_mass ), radius_( radius ), position_( position ), velocity_( velocity ) { }

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
