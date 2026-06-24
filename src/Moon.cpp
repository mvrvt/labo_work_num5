#include "Moon.hpp"

Moon::Moon( const std::string& name, physics::Mass mass, double radius, const Vector& position, const Vector& velocity )
    : name_( name ), mass_( mass ), radius_( radius ), position_( position ), velocity_( velocity ) { }

std::string Moon::GetName() const { return name_; }
physics::Mass Moon::GetMass() const { return mass_; }
double Moon::GetRadius() const { return radius_; }
Vector Moon::GetPosition() const { return position_; }
Vector Moon::GetVelocity() const { return velocity_; }

bool Moon::IsStatic() const {
    return false; // пересчитываем координаты Луны через RK4
}

Vector Moon::GetThrust() const {
    return Vector( 0.0, 0.0 );
}

void Moon::Tick( double dt ) {
    // Внутренних изменений нет
}

void Moon::UpdateState( const Vector& new_position, const Vector& new_velocity ) {
    position_ = new_position;
    velocity_ = new_velocity;
}
