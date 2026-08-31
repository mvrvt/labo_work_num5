#pragma once

#include "ICelestialBody.hpp"
#include <string>

class Moon : public ICelestialBody {
public:
    Moon( const std::string& name, physics::Mass mass, double radius, const Vector& position, const Vector& velocity );

    [[nodiscard]] std::string   GetName()     const override;
    [[nodiscard]] physics::Mass GetMass()     const override;
    [[nodiscard]] double        GetRadius()   const override;
    [[nodiscard]] Vector        GetPosition() const override; 
    [[nodiscard]] Vector        GetVelocity() const override; 

    [[nodiscard]] bool IsStatic() const override; // false, Луна подвижна

    [[nodiscard]] Vector GetThrust() const override;
    void Tick( double dt ) override; 
    
    void UpdateState( const Vector& new_position, const Vector& new_velocity ) override;

private:
    std::string   name_;
    physics::Mass mass_;
    double        radius_;
    Vector        position_;
    Vector        velocity_;
};

