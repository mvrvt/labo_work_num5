#pragma once

#include "ICelestialBody.hpp"
#include "PhysicsUnits.hpp"
#include <algorithm>
#include <string>

class SpaceShip : public ICelestialBody {
public:
    SpaceShip( const std::string& name, physics::Mass dry_mass, physics::Mass fuel_mass, double radius, const Vector& position, const Vector& velocity );

    [[nodiscard]] std::string   GetName()     const override;
    [[nodiscard]] physics::Mass GetMass()     const override;
    [[nodiscard]] double        GetRadius()   const override;
    [[nodiscard]] Vector        GetPosition() const override;
    [[nodiscard]] Vector        GetVelocity() const override;

    [[nodiscard]] bool IsStatic() const override;
    void UpdateState( const Vector& new_position, const Vector& new_velocity ) override;

    // ---- Специфичные методы корабля ----
    [[nodiscard]] physics::Mass GetFuelMass() const;

    // Метод для траты топлива
    void ConsumeFuel( physics::Mass amount );

    [[nodiscard]] Vector GetThrust() const override;
    void Tick( double dt ) override;

    // Setter для управления с клавиатуры 
    void SetThrust( const Vector& thrust );

    [[nodiscard]] physics::Mass GetMaxFuelMass() const;

private:
    std::string   name_;
    physics::Mass dry_mass_;
    physics::Mass fuel_mass_;
    physics::Mass max_fuel_mass_; 
    double        radius_;
    Vector        position_;
    Vector        velocity_;
    Vector        thrust_; // Тякущая тяга двигателя
};
