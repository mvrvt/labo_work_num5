#pragma once

#include "ICelestialBody.hpp"
#include <algorithm>
#include <string>

class SpaceShip : public ICelestialBody {
public:
    SpaceShip( const std::string& name, double dry_mass, double fuel_mass, double radius, const Vector& position, const Vector& velocity );

    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] double GetMass()      const override; // Масса корабля непостоянна. Она равна сумме сухой массы и оставшегося топлива
    [[nodiscard]] double GetRadius()    const override;
    [[nodiscard]] Vector GetPosition()  const override;
    [[nodiscard]] Vector GetVelocity()  const override;

    [[nodiscard]] bool IsStatic() const override;
    void UpdateState( const Vector& new_position, const Vector& new_velocity ) override;

    // ---- Специфичные методы корабля ----
    [[nodiscard]] double GetFuelMass() const;

    // Метод для траты топлива
    void ConsumeFuel( double amount );

    [[nodiscard]] Vector GetThrust() const override;
    void Tick( double dt ) override;

    // Setter для управления с клавиатуры 
    void SetThrust( const Vector& thrust );

    [[nodiscard]] double GetMaxFuelMass() const;

private:
    std::string name_;
    double      dry_mass_;
    double      fuel_mass_;
    double      max_fuel_mass_; 
    double      radius_;
    Vector      position_;
    Vector      velocity_;
    Vector      thrust_; // Тякущая тяга двигателя
};
