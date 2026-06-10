#include "Vector.hpp"
#include <stdexcept> // Для исключений

Vector Vector::operator+( const Vector& other ) const {
    return { x + other.x, y + other.y };
}

Vector Vector::operator-( const Vector& other ) const {
    return { x - other.x, y - other.y };
}

Vector Vector::operator*( double scalar ) const {
    return { x * scalar, y * scalar };
}

Vector Vector::operator/( double scalar ) const {
    if ( scalar == 0.0 ) {
        throw std::invalid_argument( "Vector: Division by zero isn't allowed" );
    }
    return { x / scalar, y / scalar };
}

double Vector::Length() const {
    return std::sqrt( x * x + y * y );
}

double Vector::LengthSquared() const {
    return x * x + y * y;
}

Vector Vector::Normalized() const {
    double len = Length();
    if ( len == 0 ) return { 0.0, 0.0 };
    return { x / len, y / len };
}
