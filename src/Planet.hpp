#pragma once

#include "ICelestialBody.hpp"
#include "PhysicsUnits.hpp"
#include <string>

class Planet : public ICelestialBody {
public:
    Planet( const std::string& name, physics::Mass mass, double radius, const Vector& position, const Vector& velocity = Vector(0.0, 0.0), bool is_static = true );

    [[nodiscard]] std::string   GetName()     const override;
    [[nodiscard]] physics::Mass GetMass()     const override; 
    [[nodiscard]] double        GetRadius()   const override;
    [[nodiscard]] Vector        GetPosition() const override;
    [[nodiscard]] Vector        GetVelocity() const override;

    [[nodiscard]] bool IsStatic() const override;

    [[nodiscard]] Vector GetThrust() const override;
    void Tick( double dt ) override; 

    void UpdateState( const Vector& new_position, const Vector& new_velocity ) override;

private:
    std::string   name_;
    physics::Mass mass_;
    double        radius_;
    Vector        position_;
    Vector        velocity_;
    bool          is_static_; 
};
