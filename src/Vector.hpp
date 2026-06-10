#pragma once

#include <cmath>

// Структура для двумерного вектора
// Используем struct, т.к. поля х и у логично оставить в public
struct Vector {
    double x;
    double y;

    // Конструктор по умолчанию и с параметрами
    Vector( double x = 0.0, double y = 0.0 ) : x( x ), y( y ) { }

    // Операции с векторами 
    [[nodiscard]] Vector operator+( const Vector& other ) const;
    [[nodiscard]] Vector operator-( const Vector& other ) const;
    [[nodiscard]] Vector operator*( double scalar )         const;
    [[nodiscard]] Vector operator/( double scalar )         const;
    
    [[nodiscard]] double Length() const; 

    [[nodiscard]] Vector Normalized() const;

    // Квадрат длины (нужно будет для закона Ньтона, чтобы не извлекать корень лишний раз)
    [[nodiscard]] double LengthSquared() const;
};
