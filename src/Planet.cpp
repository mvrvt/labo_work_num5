#include "Planet.hpp"

Planet::Planet( const std::string& name, physics::Mass mass, double radius, const Vector& position, const Vector& velocity, bool is_static ) 
    : name_( name ), mass_( mass ), radius_( radius ), position_( position ), velocity_( velocity ), is_static_( is_static ) { }

std::string Planet::GetName() const { return name_; }
physics::Mass Planet::GetMass() const { return mass_; }
double Planet::GetRadius() const { return radius_; }
Vector Planet::GetPosition() const { return position_; }
Vector Planet::GetVelocity() const { return velocity_; }

bool Planet::IsStatic() const {
    return is_static_;
}

Vector Planet::GetThrust() const {
    return Vector( 0.0, 0.0 ); 
}

void Planet::Tick( double dt ) {}

void Planet::UpdateState( const Vector& new_position, const Vector& new_velocity ) {
    if ( !is_static_ ) {
        position_ = new_position;
        velocity_ = new_velocity;
    }
}
    
