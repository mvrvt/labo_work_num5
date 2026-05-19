#pragma once

#include "Vector.hpp"
#include <string>

// Буква "I" в начале названия означает интерфейс
// Интерфейс (в С++) - это класс, состоящий из число виртуальных методов
class ICelestialBody {
public:
    // Виртуальный деструктор
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

    // Setter
    virtual void UpdateState( const Vector& new_position, const Vector& new_velocity ) = 0;
};


