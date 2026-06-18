#include "Simulation.hpp"
#include "lab2_files/ArraySequence.hpp" 

Simulation::Simulation() {
    // Выделяем память под конкретный MutableArraySequence
    universe_ = new MutableArraySequence<std::shared_ptr<ICelestialBody>>();
}

Simulation::~Simulation() {
    delete universe_;
}

void Simulation::AddBody( std::shared_ptr<ICelestialBody> body ) {
    universe_->Append( body ); 
}

Sequence<std::shared_ptr<ICelestialBody>>* Simulation::GetUniverse() const {
    return universe_;
}

Vector Simulation::CalculateAcceleration( const std::shared_ptr<ICelestialBody>& target, const Vector& current_position ) const {
    // Реализован закон всемирного тяготения Ньютона

    Vector total_force = target->GetThrust();

    for ( int i = 0; i < universe_->GetLength(); ++i ) {
        std::shared_ptr<ICelestialBody> other = universe_->Get( i );
        
        if ( target == other ) continue;

        Vector direction = other->GetPosition() - current_position;
        double distance_squared = direction.LengthSquared();

        if ( distance_squared < 1.0 ) distance_squared = 1.0; 

        double force_magnitude = kGravity_ * ( target->GetMass() * other->GetMass() ) / distance_squared;
        Vector force_vector = ( direction / direction.Length() ) * force_magnitude;
        total_force = total_force + force_vector;
    }

    return total_force / target->GetMass();
}

const double Simulation::GetkGravity() const {
    return kGravity_;
}

void Simulation::Update( double dt ) {
    MutableArraySequence<Vector> new_positions;
    MutableArraySequence<Vector> new_velocities;

    for ( int i = 0; i < universe_->GetLength(); ++i ) {
        universe_->Get( i )->Tick( dt );
    }

    // Шаг №1. Вычисление новых состояний методом RK4
    for ( int i = 0; i < universe_->GetLength(); ++i ) {
        std::shared_ptr<ICelestialBody> body = universe_->Get( i );

        if ( body->IsStatic() ) {
            new_positions.Append( body->GetPosition() );
            new_velocities.Append( body->GetVelocity() );
            continue;
        }

        Vector initial_pos = body->GetPosition();
        Vector initial_vel = body->GetVelocity();

        Vector v1 = initial_vel;
        Vector a1 = CalculateAcceleration( body, initial_pos );

        Vector p2 = initial_pos + v1 * ( dt / 2.0 );
        Vector v2 = initial_vel + a1 * ( dt / 2.0 );
        Vector a2 = CalculateAcceleration( body, p2 );

        Vector p3 = initial_pos + v2 * ( dt / 2.0 );
        Vector v3 = initial_vel + a2 * ( dt / 2.0 );
        Vector a3 = CalculateAcceleration( body, p3 );

        Vector p4 = initial_pos + v3 * dt;
        Vector v4 = initial_vel + a3 * dt; 
        Vector a4 = CalculateAcceleration( body, p4 );

        Vector new_pos = initial_pos + ( v1 + v2 * 2.0 + v3 * 2.0 + v4 ) * ( dt / 6.0 );
        Vector new_vel = initial_vel + ( a1 + a2 * 2.0 + a3 * 2.0 + a4 ) * ( dt / 6.0 );
        
        new_positions.Append( new_pos );
        new_velocities.Append( new_vel );
    }

    // Шаг №2: Применение состояний
    for ( int i = 0; i < universe_->GetLength(); ++i ) {
        universe_->Get( i )->UpdateState( new_positions.Get( i ), new_velocities.Get( i ) );
    }
}
