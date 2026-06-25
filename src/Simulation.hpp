#pragma once

#include "ICelestialBody.hpp"
#include "PhysicsUnits.hpp"
#include "lab2_files/Sequence.hpp"
#include "lab2_files/ArraySequence.hpp"

class Simulation {
public:
    Simulation();
    ~Simulation(); 

    void AddBody( ICelestialBody* body ); // Добавить тело во вселенную
    void Update( double dt );             // Шаг физики

    const double GetkGravity() const;

    [[nodiscard]] Sequence<ICelestialBody*>* GetUniverse() const;

private:
    Sequence<ICelestialBody*>* universe_;
    const double kGravity_ = 6.67430e-11;

    // Вспомогательный метод для RK4
    // Считает ускорение, которое действовало бы на target, если бы он находился в точке current_position
    [[nodiscard]] Vector CalculateAcceleration( const ICelestialBody* target, const Vector& current_position ) const;
};
