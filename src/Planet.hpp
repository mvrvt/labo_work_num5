#pragma once

#include "ICelestialBody.hpp"
#include <string>

// Planet наследуется от интерфейса ICelestialBody 
class Planet : public ICelestialBody {
public:
    // Конструктор 
    Planet( const std::string& name, double mass, double radius, const Vector& position );

    [[nodiscard]] std::string GetName()     const override;
    [[nodiscard]] double      GetMass()     const override; 
    [[nodiscard]] double      GetRadius()   const override;
    [[nodiscard]] Vector      GetPosition() const override;
    [[nodiscard]] Vector      GetVelocity() const override;

    [[nodiscard]] bool IsStatic() const override;

    void UpdateState( const Vector& new_position, const Vector& new_velocity ) override;

private:
    std::string name_;
    double      mass_;
    double      radius_;
    Vector      position_;
};
