#pragma once

#include "Vector.hpp"
#include <string>

class ICelestialBody {
public:
    // Если мы будем удалять объект производного класса через указатель на базовый,
    // то без virtual деструктора вызовется только деструктор базового класса, т.е. будет утечка памяти
    virtual ~ICelestialBody() = default;
    
    // Getters
    [[nodiscard]] virtual std::string GetName()     const = 0;
    [[nodiscard]] virtual double      GetMass()     const = 0;
    [[nodiscard]] virtual double      GetRadius()   const = 0;
    [[nodiscard]] virtual Vector      GetPosition() const = 0;
    [[nodiscard]] virtual Vector      GetVelocity() const = 0; 

    // Является ли объект статичным (звезда или что-то двигающееся)
    // Пригодится для оптимизации: статичным телам не нужно считать Рунге-Кутту
    [[nodiscard]] virtual bool IsStatic() const = 0;

    // Возвращает вектор тяги двигателя (для планет будет просто 0,0)
    [[nodiscard]] virtual Vector GetThrust() const = 0;

    // Setter
    virtual void UpdateState( const Vector& new_position, const Vector& new_velocity ) = 0;

    // Внутреннее обновление объекта (например, сжигание топлива)
    virtual void Tick( double dt ) = 0;
};


