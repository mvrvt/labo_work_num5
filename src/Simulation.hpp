#pragma once

#include "ICelestialBody.hpp"
#include "PhysicsUnits.hpp"
#include "lab2_files/Sequence.hpp"
#include "lab2_files/ArraySequence.hpp" 

class Simulation {
public:
    Simulation();
    ~Simulation(); 

    void AddBody( ICelestialBody* body ); 
    void Update( double dt );                             

    const double GetkGravity() const;

    [[nodiscard]] Sequence<ICelestialBody*>* GetUniverse() const;

private:
    Sequence<ICelestialBody*>* universe_;
    const double kGravity_ = 6.67430e-11;

    [[nodiscard]] Vector CalculateAcceleration( const ICelestialBody* target, const Vector& current_position ) const;
};
