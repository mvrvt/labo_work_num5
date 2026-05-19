#include "Simulation.hpp"

void Simulation::AddBody( std::shared_ptr<ICelestialBody> body ) {
    universe_.push_back( body );
}

const std::vector<std::shared_ptr<ICelestialBody>>& Simulation::GetUniverse() const {
    return universe_;
}

void Simulation::Update( double dt ) {
    for ( auto& body : universe_ ) {
        if ( body->IsStatic() ) continue;

        Vector total_force( 0.0, 0.0 );

        for ( const auto& other : universe_ ) {
            if ( body == other ) continue;

            Vector direction = other->GetPosition() - body->GetPosition();
            double distance_squared = direction.LengthSquared();

            if ( distance_squared < 1.0 ) distance_squared = 1.0;

            double force_magnitude = kGravity_ * ( body->GetMass() * other->GetMass() ) / distance_squared;
            Vector force_vector = ( direction / direction.Length() ) * force_magnitude;
            
            total_force = total_force + force_vector;
        }

        Vector acceleration = total_force / body->GetMass();

        // Пока оставляем метод Эйлера, в следующем шаге заменим его на Рунге-Кутту прямо здесь
        Vector new_velocity = body->GetVelocity() + ( acceleration * dt );
        Vector new_position = body->GetPosition() + ( new_velocity * dt );

        body->UpdateState( new_position, new_velocity );
    }
}
