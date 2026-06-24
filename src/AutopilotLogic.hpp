#pragma once

#include "Vector.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Данные телеметрии для автомата
struct Telemetry {
    Vector earth_pos;
    Vector target_pos;
    Vector ship_pos;
    Vector ship_vel;
    Vector target_vel;

    double dist_to_earth;
    double dist_to_target;

    // Динамические параметры выбранной цели
    double target_mass;
    double target_radius;
    double target_soi; // Сфера влияния (сфера Лапласа)
    
    Telemetry() = default;

    Telemetry( Vector e, Vector tp, Vector s, Vector sv, Vector tv, double t_mass, double t_rad, double t_soi ) 
        : earth_pos( e ), target_pos( tp ), ship_pos( s ), ship_vel( sv ), target_vel( tv ), 
          target_mass( t_mass ), target_radius( t_rad ), target_soi( t_soi ) { 
        dist_to_earth = ( s - e ).Length();
        dist_to_target = ( s - tp ).Length();
    }
};

// Универсальный динамический расчет фазового угла по законам Кеплера (Условие начала манёвра)
struct ConditionStartBurn {
    double mu;
    explicit ConditionStartBurn( double mu_val ) : mu( mu_val ) {}
    
    bool operator()( const Telemetry& t ) const {
        Vector to_ship = t.ship_pos - t.earth_pos;
        Vector to_target = t.target_pos - t.earth_pos;

        double r1 = t.dist_to_earth;
        double r2 = to_target.Length();

        double a_transfer = ( r1 + r2 ) / 2.0; 
        double time_of_flight = M_PI * std::sqrt( ( a_transfer * a_transfer * a_transfer ) / mu );
        double target_angular_vel = std::sqrt( mu / ( r2 * r2 * r2 ) );
        double target_travel_angle = target_angular_vel * time_of_flight;
        
        double ideal_phase_angle = M_PI - target_travel_angle;

        double dot = to_ship.x * to_target.x + to_ship.y * to_target.y;
        double det = to_ship.x * to_target.y - to_ship.y * to_target.x;
        double current_angle = std::atan2( det, dot ); 
        if ( current_angle < 0 ) current_angle += 2 * M_PI; 

        return std::abs( current_angle - ideal_phase_angle ) < 0.05;
    }
};

// Умный разгон: вычисляем нужную скорость динамически через уравнение Виз-Вива
struct ConditionStopBurn {
    double mu;
    explicit ConditionStopBurn( double mu_val ) : mu( mu_val ) { }

    bool operator()( const Telemetry& t ) const {
        double r_current = t.dist_to_earth;
        double target_orbit_radius = ( t.target_pos - t.earth_pos ).Length();

        double v_required = std::sqrt( mu * ( 2.0 / r_current - 2.0 / (r_current + target_orbit_radius) ) );
        return t.ship_vel.Length() >= v_required; 
    }
};

// Зона захвата (Sphere of Influence)
struct ConditionStartCapture {
    bool operator()( const Telemetry& t ) const {
        return t.dist_to_target <= t.target_soi;
    }
};

// Динамическая орбита парковки (Условие успешной парковки)
struct ConditionOrbitEntered {
    bool operator()( const Telemetry& t ) const {
        double park_radius = t.target_radius * 2.0;

        // Широкая зона погрешности (1000 км) для успешного перехода
        if ( t.dist_to_target > park_radius + 1000000.0 || t.dist_to_target < park_radius - 1000000.0 ) return false;
        
        Vector to_target = t.target_pos - t.ship_pos;
        Vector direction = to_target.Normalized();
        Vector rel_vel = t.ship_vel - t.target_vel;
        
        double radial_velocity = std::abs( rel_vel.x * direction.x + rel_vel.y * direction.y );
        return radial_velocity < 25.0; // Орбита круглая, если мы почти не сближаемся с центром
    }
};
