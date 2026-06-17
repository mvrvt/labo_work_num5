#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <memory> 
#include <iostream>
#include <cmath>
#include <optional> 

#include "imgui.h"
#include "imgui-SFML.h"

#include "Vector.hpp"
#include "Planet.hpp"
#include "Moon.hpp"
#include "SpaceShip.hpp"
#include "Simulation.hpp"
#include "Renderer.hpp"
#include "StateMachine.hpp" 

// Данные телеметрии для автомата
struct Telemetry {
    Vector earth_pos;
    Vector moon_pos;
    Vector ship_pos;
    Vector ship_vel;
    Vector moon_vel; 

    double dist_to_earth;
    double dist_to_moon;

    Telemetry() : dist_to_earth(0.0), dist_to_moon(0.0) { }

    Telemetry( Vector e, Vector m, Vector s, Vector sv, Vector mv ) 
        : earth_pos( e ), moon_pos( m ), ship_pos( s ), ship_vel( sv ), moon_vel( mv ) { 
        dist_to_earth = (s - e).Length();
        dist_to_moon = (s - m).Length();
    }
};

// 1. Ждем правильного фазового угла. Берем чуть раньше (1.50 - 1.60 рад), 
// чтобы Луна своей огромной массой сама затянула нас.
struct ConditionStartBurn {
    bool operator()( const Telemetry& t ) const {
        Vector to_ship = t.ship_pos - t.earth_pos;
        Vector to_moon = t.moon_pos - t.earth_pos;

        double dot = to_ship.x * to_moon.x + to_ship.y * to_moon.y;
        double det = to_ship.x * to_moon.y - to_ship.y * to_moon.x;
        double angle = std::atan2( det, dot );

        return angle > 1.50 && angle < 1.60;
    }
};

// 2. Разгоняемся ровно до 480. Этого хватит, чтобы долететь до орбиты Луны.
struct ConditionStopBurn {
    bool operator()( const Telemetry& t ) const {
        return t.ship_vel.Length() >= 480.0; 
    }
};

// 3. Зона захвата (Sphere of Influence). Включаем умный контроллер сильно заранее!
struct ConditionStartCapture {
    bool operator()( const Telemetry& t ) const {
        return t.dist_to_moon <= 180.0;
    }
};

// 4. Считаем, что мы на орбите, если дистанция около 45, а радиальная скорость (падение) близка к нулю.
struct ConditionOrbitEntered {
    bool operator()( const Telemetry& t ) const {
        if (t.dist_to_moon > 50.0 || t.dist_to_moon < 40.0) return false;
        
        Vector to_moon = t.moon_pos - t.ship_pos;
        Vector dir = to_moon.Normalized();
        Vector rel_vel = t.ship_vel - t.moon_vel;
        
        // Радиальная скорость (насколько быстро мы отдаляемся/приближаемся к центру Луны)
        double radial_v = std::abs(rel_vel.x * dir.x + rel_vel.y * dir.y);
        return radial_v < 5.0; // Орбита стабилизировалась
    }
};

int main() {
    unsigned int window_width  = 1600;
    unsigned int window_height = 1000;
    sf::RenderWindow window( sf::VideoMode( { window_width, window_height} ), "Space Simulation: Lab 5" );
    window.setFramerateLimit( 120 );

    // Инициализация ImGui
    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Не удалось инициализировать ImGui-SFML!" << std::endl;
        return -1;
    }

    Simulation simulation;

    // Координаты центра экрана (для расположения в них Земли)
    double center_x = window_width / 2.0;
    double center_y = window_height / 2.0;

    // Расставляем объекты относительно центра экрана
    // Земля строго в центре
    auto earth = std::make_shared<Planet>( "Earth", 10000.0, 40.0, Vector( center_x, center_y ) );

    // Луна: орбита 350, значит ставим её на 350 пикселей выше центра экрана (center_y - 350.0)
    auto moon = std::make_shared<Moon>( "Moon", 2500.0, 15.0, Vector( center_x, center_y - 350.0 ), Vector( 169.0, 0.0 ) );

    // Корабль: парковочная орбита 70, ставим на 70 пикселей правее центра (center_x + 70.0)
    auto player_ship = std::make_shared<SpaceShip>( "Soyuz", 0.01, 0.8, 5.0, Vector( center_x + 70.0, center_y ), Vector( 0.0, 378.0 ) );

    simulation.AddBody( earth );
    simulation.AddBody( moon );
    simulation.AddBody( player_ship );

    Renderer renderer;
    sf::Clock physics_clock;
    sf::Clock imgui_clock; // Таймер специально для ImGui
    
    // Делаем тягу мощнее, чтобы контроллер успевал выруливать в критических ситуациях
    const double kEnginePower = 400.0; 

    fsm::StateMachine<Telemetry> autopilot;
    
    autopilot.AddState( "Off", true );
    autopilot.AddState( "WaitAlignment" ); 
    autopilot.AddState( "ProgradeBurn" );  
    autopilot.AddState( "Coast" );         
    autopilot.AddState( "Capture" );       
    autopilot.AddState( "OrbitMoon", true); 
    autopilot.SetInitialState( "Off" );

    autopilot.AddTransition( "WaitAlignment", "ProgradeBurn", ConditionStartBurn() );
    autopilot.AddTransition( "ProgradeBurn", "Coast", ConditionStopBurn() );
    autopilot.AddTransition( "Coast", "Capture", ConditionStartCapture() );
    autopilot.AddTransition( "Capture", "OrbitMoon", ConditionOrbitEntered() );

    while ( window.isOpen() ) {
        while ( const std::optional<sf::Event> event = window.pollEvent() ) {
            // Передаем события в ImGui
            ImGui::SFML::ProcessEvent(window, event.value());

            if ( event->is<sf::Event::Closed>() ) {
                window.close();
            }
        }

        // Обновление логики ImGui на каждый кадр
        ImGui::SFML::Update(window, imgui_clock.restart());

        Vector current_thrust( 0.0, 0.0 );

        if ( player_ship->GetFuelMass() > 0.0 ) {
            
            Telemetry t(
                earth->GetPosition(), 
                moon->GetPosition(), 
                player_ship->GetPosition(),
                player_ship->GetVelocity(), 
                moon->GetVelocity()
            );

            bool state_changed = autopilot.Step(t);
            if (state_changed) {
                std::cout << ">> Autopilot transition to state: [" << autopilot.GetCurrentState() << "]\n";
            }

            std::string current_state = autopilot.GetCurrentState();
            
            // --- Логика маневров ---
            if ( current_state == "WaitAlignment" || current_state == "Coast" || current_state == "OrbitMoon" ) {
                current_thrust = Vector( 0.0, 0.0 ); 
            }
            else if ( current_state == "ProgradeBurn" ) {
                // Разгон по вектору скорости (Prograde)
                Vector prograde = player_ship->GetVelocity().Normalized();
                current_thrust = prograde * kEnginePower;
            } 
            else if ( current_state == "Capture" ) {
                // --- УМНЫЙ АЛГОРИТМ ЗАХВАТА ---
                Vector to_moon = moon->GetPosition() - player_ship->GetPosition(); 
                double dist = to_moon.Length();
                Vector dir_to_moon = to_moon.Normalized(); // Вектор ОТ корабля К Луне
                Vector relative_vel = player_ship->GetVelocity() - moon->GetVelocity();

                // Целевая орбита
                double target_radius = 45.0; 
                double dist_error = dist - target_radius; // Насколько мы далеко от нужной орбиты

                // Желаемая скорость сближения (если далеко - сближаемся, если близко - выравниваемся)
                double desired_approach_v = dist_error * 1.2;
                if (desired_approach_v > 80.0) desired_approach_v = 80.0; // Ограничиваем скорость падения
                if (desired_approach_v < -15.0) desired_approach_v = -15.0; // Позволяем чуть-чуть отлететь, если промахнулись

                // Идеальная круговая скорость на целевой орбите
                double v_circ = std::sqrt( 1000.0 * 2500.0 / target_radius );

                // Выбираем вектор касательной (по направлению движения)
                Vector tangent(-dir_to_moon.y, dir_to_moon.x);
                if (tangent.x * relative_vel.x + tangent.y * relative_vel.y < 0) {
                    tangent = Vector(dir_to_moon.y, -dir_to_moon.x);
                }

                // Строим идеальный вектор скорости (сближение + закручивание по орбите)
                Vector ideal_rel_vel = (dir_to_moon * desired_approach_v) + (tangent * v_circ);
                Vector target_vel_global = moon->GetVelocity() + ideal_rel_vel;

                // Вычисляем ошибку и даем тягу
                Vector vel_error = target_vel_global - player_ship->GetVelocity();
                current_thrust = vel_error * 15.0; // Сильный PD-коэффициент для четкого управления
                
                if ( current_thrust.Length() > kEnginePower ) {
                    current_thrust = current_thrust.Normalized() * kEnginePower;
                }
            } 
            else if ( current_state == "Off" ) {
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Up ) )    current_thrust.y -= kEnginePower;
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Down ) )  current_thrust.y += kEnginePower;
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Left ) )  current_thrust.x -= kEnginePower;
                if ( sf::Keyboard::isKeyPressed( sf::Keyboard::Key::Right ) ) current_thrust.x += kEnginePower;
            }
        }

        player_ship->SetThrust( current_thrust );

        double dt = physics_clock.restart().asSeconds();
        if ( dt > 0.1 ) dt = 0.1; 
        simulation.Update( dt );

        // ===== ОТРИСОВКА ИНТЕРФЕЙСА IMGUI =====

        // --- ОКНО 1: ТОПЛИВО (Слева сверху) ---
        // Фиксируем начальное положение окна в координатах x=20, y=20
        ImGui::SetNextWindowPos(ImVec2(20.f, 20.f), ImGuiCond_Once);
        // Запрещаем сворачивать, двигать мышкой и менять размер окна, чтобы оно было частью HUD игры
        ImGuiWindowFlags fuel_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        ImGui::Begin("Fuel System", nullptr, fuel_flags); 
            ImGui::Text("Fuel Level:"); 
            float fuel_ratio = static_cast<float>(player_ship->GetFuelMass() / player_ship->GetMaxFuelMass());
            
            if (fuel_ratio < 0.25f) {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.8f, 0.1f, 0.1f, 1.0f)); // Красный, если мало
            } else {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.1f, 0.8f, 0.1f, 1.0f)); // Зеленый в норме
            }
            ImGui::ProgressBar(fuel_ratio, ImVec2(250.f, 25.f));
            ImGui::PopStyleColor(); 
        ImGui::End();

        // === ЛОГИКА ОПРЕДЕЛЕНИЯ ТЕКУЩЕЙ ОРБИТЫ И ОТНОСИТЕЛЬНОЙ СКОРОСТИ ===
        // Мы вынесли это сюда, чтобы использовать и для статуса, и для спидометра
        std::string current_orbit;
        double dist_to_earth = (player_ship->GetPosition() - earth->GetPosition()).Length();
        double dist_to_moon = (player_ship->GetPosition() - moon->GetPosition()).Length();
        
        double display_speed = 0.0;

        if (dist_to_moon < 180.0) { 
            current_orbit = "Moon Orbit";
            // В зоне Луны показываем скорость относительно движущейся Луны (векторная разность)
            display_speed = (player_ship->GetVelocity() - moon->GetVelocity()).Length();
        } else if (dist_to_earth < 400.0) {
            current_orbit = "Earth Parking Orbit";
            // Земля у нас статична, ее скорость 0, поэтому берем абсолютную скорость корабля
            display_speed = player_ship->GetVelocity().Length();
        } else {
            current_orbit = "Transfer Trajectory"; 
            // В глубоком космосе показываем абсолютную скорость
            display_speed = player_ship->GetVelocity().Length();
        }

        // --- ОКНО 3: ТЕЛЕМЕТРИЯ КОРАБЛЯ (Слева, под топливом) ---
        ImGui::SetNextWindowPos(ImVec2(20.f, 100.f), ImGuiCond_Once);
        ImGuiWindowFlags telemetry_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
        
        ImGui::Begin("Ship Telemetry", nullptr, telemetry_flags);
            double current_mass = player_ship->GetMass();
            
            // Выводим нашу "умную" относительную скорость
            ImGui::Text("Rel. Speed: %.2f m/s", display_speed);
            ImGui::Text("Mass:       %.2f kg", current_mass);
        ImGui::End();


        // --- ОКНО 4: СТАТУС ПОЛЕТА (По центру снизу) ---
        // Логика нумерации фаз автопилота
        std::string state_str = autopilot.GetCurrentState();
        std::string phase_text;
        if (state_str == "Off")                phase_text = "0 - Manual Control";
        else if (state_str == "WaitAlignment") phase_text = "1 - Await Alignment";
        else if (state_str == "ProgradeBurn")  phase_text = "2 - Prograde Burn";
        else if (state_str == "Coast")         phase_text = "3 - Coasting";
        else if (state_str == "Capture")       phase_text = "4 - Capture Burn";
        else if (state_str == "OrbitMoon")     phase_text = "5 - Orbit Stabilized";

        // ИСПРАВЛЕНИЕ ПОЗИЦИИ:
        // Окно автопилота у тебя строится от координаты Y = window_height - 110.f,
        // но оно высокое из-за кнопки. Центральное окно низкое. 
        // Чтобы их нижние края выровнялись, мы опускаем центральное окно ниже, например до -75.f
        ImGui::SetNextWindowPos(ImVec2((window_width / 2.0f) - 200.f, window_height - 75.f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(400.f, 0.f), ImGuiCond_Once); 
        
        ImGuiWindowFlags status_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
        
        ImGui::Begin("Flight Status", nullptr, status_flags);
            ImGui::Columns(2, "status_columns", true); 
            
            ImGui::Text("Current Location:");
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", current_orbit.c_str());
            
            ImGui::NextColumn(); 
            
            ImGui::Text("Autopilot Phase:");
            if (state_str == "Off") {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", phase_text.c_str()); 
            } else {
                ImGui::TextColored(ImVec4(0.1f, 0.8f, 0.1f, 1.0f), "%s", phase_text.c_str()); 
            }
            
            ImGui::Columns(1); 
        ImGui::End();

        // --- ОКНО 2: АВТОПИЛОТ (Справа снизу) ---
        ImGui::SetNextWindowPos( ImVec2( window_width - 270.f, window_height - 110.f ), ImGuiCond_Once );
        ImGuiWindowFlags auto_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        ImGui::Begin("Autopilot Control", nullptr, auto_flags);
            std::string current_state = autopilot.GetCurrentState();
            ImGui::Text("AI State: %s", current_state.c_str());
            ImGui::Spacing();

            if (current_state == "Off") {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.0f)); // Серый цвет, если выключен
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.1f, 1.0f)); // Зеленый, если работает
            }

            if (ImGui::Button("ENGAGE AUTOPILOT", ImVec2(250.f, 40.f))) {
                if (current_state == "Off" || current_state == "OrbitMoon") {
                    autopilot.SetCurrentState("WaitAlignment");
                    std::cout << ">> Autopilot: ENGAGED. Waiting for phase alignment...\n";
                } else {
                    autopilot.SetCurrentState("Off");
                    std::cout << ">> Autopilot: DISABLED. Manual control.\n";
                }
            }
            ImGui::PopStyleColor();
        ImGui::End();
        // ===================================

        window.clear( sf::Color::Black );
        renderer.Draw( window, simulation.GetUniverse() );
        
        // Рендерим окна ImGui поверх графики SFML
        ImGui::SFML::Render(window);
        window.display();
    }

    // Завершение работы ImGui
    ImGui::SFML::Shutdown();
    return 0;
}
