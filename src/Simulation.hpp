#pragma once

#include <memory>
#include "ICelestialBody.hpp"
#include "lab2_files/Sequence.hpp"
#include "lab2_files/ArraySequence.hpp"

class Simulation {
public:
    Simulation();
    ~Simulation(); 

    void AddBody( std::shared_ptr<ICelestialBody> body ); // Добавить тело во вселенную
    void Update( double dt );                             // Шаг физики

    [[nodiscard]] Sequence<std::shared_ptr<ICelestialBody>>* GetUniverse() const;

private:
    // std::vector<std::shared_ptr<ICelestialBody> > universe_;
    Sequence<std::shared_ptr<ICelestialBody>>* universe_;
    const double kGravity_ = 1000.0;

    // Вспомогательный метод для RK4
    // Считает ускорение, которое действовало бы на target, если бы он находился в точке current_position
    [[nodiscard]] Vector CalculateAcceleration( const std::shared_ptr<ICelestialBody>& target, const Vector& current_position ) const;
};
