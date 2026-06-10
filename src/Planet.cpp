#include "Planet.hpp"

Planet::Planet( const std::string& name, double mass, double radius, const Vector& position ) 
    : name_( name ), mass_( mass ), radius_( radius ), position_( position ) { }

std::string Planet::GetName() const { return name_; }
double Planet::GetMass() const { return mass_; }
double Planet::GetRadius() const { return radius_; }
Vector Planet::GetPosition() const { return position_; }

Vector Planet::GetVelocity() const {
    // Планета является неподвижной, её скорость всегда равна нулю
    return Vector( 0.0, 0.0 );
}

bool Planet::IsStatic() const {
    return true;
}

Vector Planet::GetThrust() const {
    return Vector( 0.0, 0.0 ); // У планеты нет двигателей 
}

void Planet::Tick( double dt ) {
    // Планета со временем не меняется, топливо не сжигает
}

void Planet::UpdateState( const Vector& new_position, const Vector& new_velocity ) {
    // Полькольку планета статичка, мы игнорируем попытки изменить её состояние
    // В будущем можно будет выкидывать исключение, но пока просто ничего не произойдет при вызове
}
