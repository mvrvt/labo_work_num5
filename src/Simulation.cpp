#include "Simulation.hpp"

void Simulation::AddBody( std::shared_ptr<ICelestialBody> body ) {
    universe_.push_back( body );
}

const std::vector<std::shared_ptr<ICelestialBody>>& Simulation::GetUniverse() const {
    return universe_;
}

// Вычисляем суммарное ускорение в заданной точке пространства 
Vector Simulation::CalculateAcceleration( const std::shared_ptr<ICelestialBody>& target, const Vector& current_position ) const {
    Vector total_force = target->GetThrust();

    for ( const auto& other : universe_ ) {
        if ( target == other ) continue;

        Vector direction = other->GetPosition() - current_position;
        double distance_squared = direction.LengthSquared();

        if ( distance_squared < 1.0 ) distance_squared = 1.0; // Защита от деления на 0 при коллизии

        // F = G * (m1 * m2) / r^2
        double force_magnitude = kGravity_ * ( target->GetMass() * other->GetMass() ) / distance_squared;

        Vector force_vector = ( direction / direction.Length() ) * force_magnitude;
        total_force = total_force + force_vector;
    }

    // Возвращаем ускорение (a = F / m)
    return total_force / target->GetMass();
}

void Simulation::Update( double dt ) {
    // Временная структура для хранения новых позиций и скоростей
    // Если мы будем сразу обновлять тела, то следующее тело будет считать гравитацию
    // уже от сдвинувшегося объекта, что сломает физику кадра
    struct State {
        Vector position;
        Vector velocity;
    };
    std::vector<State> new_states( universe_.size() );

    // Даём объектам обновить своё внутреннее состояние (например, потратить топливо)
    for ( auto& body : universe_ ) {
        body->Tick( dt );
    }

    // Шаг №1. Вычисление новых состояний для всех тел
    for ( size_t i = 0; i < universe_.size(); ++i ) {
        const auto& body = universe_[i];

        if ( body->IsStatic() ) {
            new_states[i] = { body->GetPosition(), body->GetVelocity() };
            continue;
        }

        // ---- Интегратор Рунге-Кутты 4-го порядка (RK4) ----

        Vector initial_pos = body->GetPosition();
        Vector initial_vel = body->GetVelocity();

        // k1: Производные в начале шага
        Vector v1 = initial_vel;
        Vector a1 = CalculateAcceleration( body, initial_pos );

        // k2: Производные в середине шага (используем k1)
        Vector p2 = initial_pos + v1 * ( dt / 2.0 );
        Vector v2 = initial_vel + a1 * ( dt / 2.0 );
        Vector a2 = CalculateAcceleration( body, p2 );

        // k3: Производные в середине шага (используем k2)
        Vector p3 = initial_pos + v2 * ( dt / 2.0 );
        Vector v3 = initial_vel + a2 * ( dt / 2.0 );
        Vector a3 = CalculateAcceleration( body, p3 );

        // k4: Производные в конце шага (используем k3)
        Vector p4 = initial_pos + v3 * dt;
        Vector v4 = initial_vel + a3 * dt; 
        Vector a4 = CalculateAcceleration( body, p4 );

        // Итоговое состояние: взвешенное среднее (середина весит больше)
        new_states[i].position = initial_pos + ( v1 + v2 * 2.0 + v3 * 2.0 + v4 ) * ( dt / 6.0 );
        new_states[i].velocity = initial_vel + ( a1 + a2 * 2.0 + a3 * 2.0 + a4 ) * ( dt / 6.0 );
    }

    // Шаг №2: Применяем вычисленные состояния ко всем объектам
    for ( size_t i = 0; i < universe_.size(); ++i ) {
        universe_[i]->UpdateState( new_states[i].position, new_states[i].velocity );
    }
}










// void Simulation::Update( double dt ) {
//     for ( auto& body : universe_ ) {
//         if ( body->IsStatic() ) continue;

//         Vector total_force( 0.0, 0.0 );

//         for ( const auto& other : universe_ ) {
//             if ( body == other ) continue;

//             Vector direction = other->GetPosition() - body->GetPosition();
//             double distance_squared = direction.LengthSquared();

//             if ( distance_squared < 1.0 ) distance_squared = 1.0;

//             double force_magnitude = kGravity_ * ( body->GetMass() * other->GetMass() ) / distance_squared;
//             Vector force_vector = ( direction / direction.Length() ) * force_magnitude;
            
//             total_force = total_force + force_vector;
//         }

//         Vector acceleration = total_force / body->GetMass();

//         // Пока оставляем метод Эйлера, в следующем шаге заменим его на Рунге-Кутту прямо здесь
//         Vector new_velocity = body->GetVelocity() + ( acceleration * dt );
//         Vector new_position = body->GetPosition() + ( new_velocity * dt );

//         body->UpdateState( new_position, new_velocity );
//     }
// }
