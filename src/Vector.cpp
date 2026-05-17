#include "Vector.hpp"
#include <stdexcept> // Для генерации исключений

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
    // Программа не должна "умирать молча" при делении на ноль
    if ( scalar == 0.0 ) 
        throw std::invalid_argument( "Vector2D: Division by zero isn't allowed" );
    return { x / scalar, y / scalar };
}

double Vector::length() const {
    return std::sqrt( x * x + y * y );
}

double Vector::lengthSquared() const {
    return x * x + y * y;
}
