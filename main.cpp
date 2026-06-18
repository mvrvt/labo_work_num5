#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <memory> 
#include <iostream>
#include <cmath>
#include <optional> 
#include <string>

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
    Vector target_pos;
    Vector ship_pos;
    Vector ship_vel;
    Vector target_vel; 

    double dist_to_earth;
    double dist_to_target;

    // Динамические параметры выбранной цели
    double target_mass;
    double target_radius;
    double target_soi; // Сфера влияния (Сфера Лапласа)

    Telemetry()  { }

    Telemetry( Vector e, Vector tp, Vector s, Vector sv, Vector tv, double t_mass, double t_rad, double t_soi ) 
        : earth_pos( e ), target_pos( tp ), ship_pos( s ), ship_vel( sv ), target_vel( tv ), 
          target_mass( t_mass ), target_radius( t_rad ), target_soi( t_soi ) { 
        dist_to_earth = (s - e).Length();
        dist_to_target = (s - tp).Length();
    }
};

// Универсальный динамический расчет фазового угла по законам Кеплера
struct ConditionStartBurn {
    // Подсчёт фазового угла для перелёта Гомана
        
    double mu;
    ConditionStartBurn( double mu_val ) : mu( mu_val ) {}
    
    bool operator()( const Telemetry& t ) const {
        Vector to_ship = t.ship_pos - t.earth_pos;
        Vector to_target = t.target_pos - t.earth_pos;

        double r1 = t.dist_to_earth;
        double r2 = to_target.Length();

        // Физика: Считаем время перелета (Гоман) и движение цели
        double a_transfer = ( r1 + r2 ) / 2.0;
        double time_of_flight = M_PI * std::sqrt( ( a_transfer * a_transfer * a_transfer ) / mu );
        double target_angular_vel = std::sqrt( mu / ( r2 * r2 * r2 ) );
        double target_travel_angle = target_angular_vel * time_of_flight;
        
        // Идеальный угол опережения
        double ideal_phase_angle = M_PI - target_travel_angle;

        double dot = to_ship.x * to_target.x + to_ship.y * to_target.y;
        double det = to_ship.x * to_target.y - to_ship.y * to_target.x;
        double current_angle = std::atan2( det, dot ); // арктангенс от отношения двух чисел 
        if ( current_angle < 0 ) current_angle += 2 * M_PI; // Нормализация 0..2PI

        // Даем окно погрешности в 0.05 радиан
        return std::abs( current_angle - ideal_phase_angle ) < 0.05;
    }
};

// Умный разгон: вычисляем нужную скорость динамически через уравнение Виз-Вива
struct ConditionStopBurn {
    double mu;

    // Конструктор принимает гравитационный параметр (мю)
    ConditionStopBurn( double mu_val ) : mu( mu_val ) { }

    bool operator()( const Telemetry& t ) const {
        // Текущее фактическое расстояние до Земли (r1)
        double r_current = t.dist_to_earth;
        double target_orbit_radius = ( t.target_pos - t.earth_pos ).Length();

        // Уравнение Виз-Вива для эллиптической орбиты перелета Гомана.
        // v = sqrt( GM * (2/r_current - 1/a) ), где a = (r_current + target_orbit_radius) / 2
        double v_required = std::sqrt( mu * ( 2.0 / r_current - 2.0 / (r_current + target_orbit_radius) ) );
        
        // Отключаем двигатель ровно в тот момент, когда набрали нужную скорость для текущего радиуса
        return t.ship_vel.Length() >= v_required; 
    }
};

// 3. Зона захвата (Sphere of Influence). Включаем умный контроллер сильно заранее
struct ConditionStartCapture {
    bool operator()( const Telemetry& t ) const {
        return t.dist_to_target <= t.target_soi;
    }
};

// 4. Динамическая орбита парковки
struct ConditionOrbitEntered {
    bool operator()( const Telemetry& t ) const {
        // Жесткая целевая орбита
        double park_radius = t.target_radius + 20.0; 

        if ( t.dist_to_target > park_radius + 3.0 || t.dist_to_target < park_radius - 3.0 ) return false;
        
        Vector to_target = t.target_pos - t.ship_pos;
        Vector direction = to_target.Normalized();
        Vector rel_vel = t.ship_vel - t.target_vel;
        
        // Радиальная скорость (насколько быстро мы отдаляемся/приближаемся к центру спутника)
        double radial_velocity = std::abs( rel_vel.x * direction.x + rel_vel.y * direction.y );
        return radial_velocity < 1.5; // Орбита стабилизировалась (орбита почти идеально круглая)
    }
};

int main() {
    unsigned int window_width  = 1600;
    unsigned int window_height = 1000;

    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8; // Восьмикратное сглаживание 

    sf::RenderWindow window( sf::VideoMode( { window_width, window_height} ), "Space Simulation: Lab 5", sf::State::Windowed, settings );
    window.setFramerateLimit( 120 );

    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Не удалось инициализировать ImGui-SFML!" << std::endl;
        return -1;
    }

    // Создаем и настраиваем камеру (View) для отдаления "вселенной"
    sf::View camera = window.getDefaultView();
    camera.zoom( 1.6f ); 

    // Явно ставим центр камеры в центр экрана
    camera.setCenter( sf::Vector2f( window_width / 2.0f, window_height / 2.0f) );

    window.setView( camera ); // Применяем камеру к окну

    Simulation simulation;

    // Координаты центра экрана (для расположения в них Земли)
    double center_x = window_width / 2.0;
    double center_y = window_height / 2.0;

    double selene_orbit_r = 800.0;

    // Расставляем объекты относительно центра экрана
    // Земля строго в центре (её масса 100 млн кг)
    auto earth = std::make_shared<Planet>( "Earth", 100000000.0, 40.0, Vector( center_x, center_y ) );

    // Луна (1.2 млн кг).
    auto moon = std::make_shared<Moon>( "Moon", 1200000.0, 15.0, Vector( center_x, center_y - 350.0 ), Vector( 169.0, 0.0 ) );

    // Вектор скорости меняем на отрицательный (-111.8), чтобы она летела по кругу правильно
    // Селена теперь весит 0.5 млн (легкий дальний спутник). Орбита 800. Скорость -111.8.
    auto selene = std::make_shared<Moon>( "Selene", 500000.0, 10.0, Vector( center_x, center_y + selene_orbit_r ), Vector( -111.8, 0.0 ) );

    // Корабль: парковочная орбита 85 (center_x + 85.0)
    auto player_ship = std::make_shared<SpaceShip>( "Soyuz", 2100.0, 6000.0, 5.0, Vector( center_x + 85.0, center_y ), Vector( 0.0, 343.0 ) );
    simulation.AddBody( earth );
    simulation.AddBody( moon );
    simulation.AddBody( selene );
    simulation.AddBody( player_ship );

    // --- ПЕРЕМЕННЫЕ ВЫБОРА ЦЕЛИ АВТОПИЛОТА ---
    std::shared_ptr<ICelestialBody> current_target = moon; // По умолчанию целью будет Луна
    int target_index = 0; // 0 - Луна, 1 - Селена

    Renderer renderer;
    sf::Clock physics_clock;
    sf::Clock imgui_clock; // Таймер специально для ImGui
    
    // Делаем мощную тягу, чтобы контроллер успевал выруливать в критических ситуациях
    const double kEnginePower = 4000000.0; // 4 млн

    fsm::StateMachine<Telemetry> autopilot; // Инициализация автопилота
    
    autopilot.AddState( "Off", true );
    autopilot.AddState( "WaitAlignment" );  // Ожидание выравнивания
    autopilot.AddState( "ProgradeBurn" );   // Импульс/Разгон по направлению движения
    autopilot.AddState( "Coast" );          // Полёт по инерции
    autopilot.AddState( "Capture" );        // Захват
    autopilot.AddState( "OrbitMoon", true); // Полёт по орбите спутника
    autopilot.SetInitialState( "Off" );

    // Вычисляем гравитационный параметр Земли (мю = G * Mass)
    double mu = simulation.GetkGravity() * earth->GetMass();
    // Передаём мю и радиус орбиты цели в условие остановки двигателя
    autopilot.AddTransition( "WaitAlignment", "ProgradeBurn", ConditionStartBurn( mu ) );
    // Автопилот сам вычислит орбиту цели.
    autopilot.AddTransition( "ProgradeBurn", "Coast", ConditionStopBurn( mu ) );
    autopilot.AddTransition( "Coast", "Capture", ConditionStartCapture() );
    autopilot.AddTransition( "Capture", "OrbitMoon", ConditionOrbitEntered() );

    // Флаги состояния симуляции
    bool is_game_over = false;
    std::string game_over_reason = "";

    while ( window.isOpen() ) {
        while ( const std::optional<sf::Event> event = window.pollEvent() ) {
            // Передаем события в ImGui
            ImGui::SFML::ProcessEvent( window, event.value() );

            if ( event->is<sf::Event::Closed>() ) {
                window.close();
            }
        }

        // Обновление логики ImGui на каждый кадр
        ImGui::SFML::Update( window, imgui_clock.restart() );

        Vector current_thrust( 0.0, 0.0 );

        if ( player_ship->GetFuelMass() > 0.0 ) {

            // Считаем динамическую сферу влияния (сферу Лапласа) для выбранной цели
            double target_orbit_radius = ( current_target->GetPosition() - earth->GetPosition() ).Length();
            double target_soi = target_orbit_radius * std::pow( current_target->GetMass() / earth->GetMass(), 0.4 ) * 1.5;
            
            Telemetry t(
                earth->GetPosition(), 
                current_target->GetPosition(), 
                player_ship->GetPosition(),
                player_ship->GetVelocity(), 
                current_target->GetVelocity(),
                current_target->GetMass(),
                current_target->GetRadius(),
                target_soi
            );

            bool state_changed = autopilot.Step( t );
            if (state_changed) {
                std::cout << ">> Autopilot transition to state: [" << autopilot.GetCurrentState() << "]\n";
            }

            std::string current_state = autopilot.GetCurrentState();
            
            // --- Логика маневров ---
            if ( current_state == "WaitAlignment" || current_state == "Coast" ) {
                current_thrust = Vector( 0.0, 0.0 ); 
            }
            else if ( current_state == "OrbitMoon" ) {
                // СИСТЕМА УДЕРЖАНИЯ ОРБИТЫ (Station Keeping / RCS)
                // Компенсируем гравитационные возмущения от Земли непрерывно (иначе корабль может улететь/врезаться в неб. тело)
                Vector to_target = current_target->GetPosition() - player_ship->GetPosition(); 
                double dist = to_target.Length();
                double target_radius = current_target->GetRadius() + 20.0; 

                // Считаем ошибку расстояния до жесткой орбиты парковки
                double dist_error = dist - target_radius; 

                Vector dir_to_target = to_target.Normalized(); 
                Vector relative_vel = player_ship->GetVelocity() - current_target->GetVelocity();

                // Пропорциональный расчет желаемой скорости сближения.
                double desired_approach_v = dist_error * 5.0;
                if ( desired_approach_v > 50.0 ) desired_approach_v = 50.0; 
                if ( desired_approach_v < -50.0 ) desired_approach_v = -50.0; 

                double v_circ = std::sqrt( simulation.GetkGravity() * current_target->GetMass() / dist );

                Vector tangent( -dir_to_target.y, dir_to_target.x );
                if (tangent.x * relative_vel.x + tangent.y * relative_vel.y < 0) {
                    tangent = Vector(dir_to_target.y, -dir_to_target.x);
                }

                Vector ideal_rel_vel = (dir_to_target * desired_approach_v) + (tangent * v_circ);
                Vector target_vel_global = current_target->GetVelocity() + ideal_rel_vel;

                // Считаем ошибку скорости (разницу между идеальной скоростью ИИ и реальной скоростью корабля
                Vector vel_error = target_vel_global - player_ship->GetVelocity();
                
                // Непрерывный мощный импульс для удержания
                current_thrust = vel_error * 500000.0;
                
                // Разрешаем ИИ использовать до 100% мощности в случае экстренного срыва с орбиты
                if ( current_thrust.Length() > kEnginePower ) {
                    current_thrust = current_thrust.Normalized() * kEnginePower;
                }
            }
            else if ( current_state == "ProgradeBurn" ) {
                // Разгон по вектору скорости (Prograde)
                Vector prograde = player_ship->GetVelocity().Normalized();
                current_thrust = prograde * kEnginePower;
            } 
            else if ( current_state == "Capture" ) {
                Vector to_target = current_target->GetPosition() - player_ship->GetPosition(); 
                double dist = to_target.Length();
                Vector dir_to_target = to_target.Normalized(); 
                Vector relative_vel = player_ship->GetVelocity() - current_target->GetVelocity();

                // Динамическая целевая орбита
                double target_radius = current_target->GetRadius() + 20.0; 
                double dist_error = dist - target_radius; 

                double desired_approach_v = dist_error * 1.5; // Чуть более агрессивное сближение при захвате
                if (desired_approach_v > 80.0) desired_approach_v = 80.0; 
                if (desired_approach_v < -15.0) desired_approach_v = -15.0; 

                double safe_dist = dist;
                if (safe_dist < current_target->GetRadius() + 5.0) safe_dist = current_target->GetRadius() + 5.0; 

                double v_circ = std::sqrt( simulation.GetkGravity() * current_target->GetMass() / safe_dist );

                Vector tangent(-dir_to_target.y, dir_to_target.x);
                if (tangent.x * relative_vel.x + tangent.y * relative_vel.y < 0) { 
                    tangent = Vector(dir_to_target.y, -dir_to_target.x);
                }

                Vector ideal_rel_vel = (dir_to_target * desired_approach_v) + (tangent * v_circ);
                Vector target_vel_global = current_target->GetVelocity() + ideal_rel_vel;

                Vector vel_error = target_vel_global - player_ship->GetVelocity();
                current_thrust = vel_error * 200000.0; // Более сильное торможение
                
                if ( current_thrust.Length() > kEnginePower ) {
                    current_thrust = current_thrust.Normalized() * kEnginePower;
                }
            }
        }

        player_ship->SetThrust( current_thrust );

        double dt = physics_clock.restart().asSeconds();
        if ( dt > 0.1 ) dt = 0.1; 
        // === ЛОГИКА СТОЛКНОВЕНИЙ И ВЫЛЕТА ЗА ПРЕДЕЛЫ ===
        if ( !is_game_over ) {
            double dist_to_earth = ( player_ship->GetPosition() - earth->GetPosition() ).Length();
            double dist_to_moon = ( player_ship->GetPosition() - moon->GetPosition() ).Length();
            double dist_to_selene = ( player_ship->GetPosition() - selene->GetPosition() ).Length();

            // 1. Проверка на столкновение с Землей (Используем "мягкий хитбокс" 75% от радиуса)
            if ( dist_to_earth <= earth->GetRadius() * 0.75 ) {
                is_game_over = true;
                game_over_reason = "CRITICAL FAILURE: Spaceship crashed into Earth!";
            }
            // 2. Проверка на столкновение с Луной (Даем такую же поблажку)
            else if ( dist_to_moon <= moon->GetRadius() * 0.75 ) {
                is_game_over = true;
                game_over_reason = "CRITICAL FAILURE: Spaceship crashed into the Moon!";
            }
            else if ( dist_to_selene <= selene->GetRadius() * 0.75 ) {
                is_game_over = true;
                game_over_reason = "CRITICAL FAILURE: Spaceship crashed into Selene!";
            }
            // 4. Проверка на вылет за пределы симуляции
            else if ( dist_to_earth > 1200.0 ) {
                is_game_over = true;
                game_over_reason = "MISSION FAILED: Spaceship lost in deep space!";
            }

            // Обновляем физику ТОЛЬКО если всё в порядке
            if ( !is_game_over ) {
                simulation.Update( dt );
            } else {
                player_ship->SetThrust(Vector(0.0, 0.0));
                autopilot.SetCurrentState("Off");
            }
        }

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
        // Мы вы вынесли это сюда, чтобы использовать и для статуса, и для спидометра
        std::string current_orbit;
        double dist_to_earth = (player_ship->GetPosition() - earth->GetPosition()).Length();
        
        // Динамически вычисляем дистанцию до выбранной в UI цели
        double dist_to_target = (player_ship->GetPosition() - current_target->GetPosition()).Length();
        
        // Вычисляем SOI для интерфейса
        double ui_target_orbit_radius = (current_target->GetPosition() - earth->GetPosition()).Length();
        double ui_target_soi = ui_target_orbit_radius * std::pow(current_target->GetMass() / earth->GetMass(), 0.4) * 1.5;

        double display_speed = 0.0;

        // Теперь интерфейс адаптируется под любую выбранную цель
        if (dist_to_target < ui_target_soi) { 
            current_orbit = "Target Orbit";
            display_speed = (player_ship->GetVelocity() - current_target->GetVelocity()).Length();
        } else if (dist_to_earth < 450.0) { // Слегка расширили зону Земли
            current_orbit = "Earth Parking Orbit";
            display_speed = player_ship->GetVelocity().Length();
        } else {
            current_orbit = "Transfer Trajectory"; 
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
        ImGui::SetNextWindowPos( ImVec2( window_width - 270.f, window_height - 180.f ), ImGuiCond_Once );
        ImGuiWindowFlags auto_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

        ImGui::Begin("Autopilot Control", nullptr, auto_flags);
            // Чтобы не было конфликта имен переменных внутри окна, используем текущий статус автопилота
            std::string autopilot_ui_state = autopilot.GetCurrentState();
            ImGui::Text("AI State: %s", autopilot_ui_state.c_str());
            ImGui::Spacing();
            
            // --- СЕКЦИЯ: ВЫБОР ЦЕЛИ ---
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Select Mission Target:");
            
            // Если автопилот запущен, блокируем выбор другой цели, чтобы не сломать полет
            if (autopilot_ui_state != "Off" && autopilot_ui_state != "OrbitMoon") ImGui::BeginDisabled();
            
            if (ImGui::RadioButton("Moon", &target_index, 0)) {
                current_target = moon;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Selene", &target_index, 1)) {
                current_target = selene;
            }
            
            if (autopilot_ui_state != "Off" && autopilot_ui_state != "OrbitMoon") ImGui::EndDisabled();
            // ---------------------------------
            
            ImGui::Spacing();
            
            // --- ВОЗВРАЩАЕМ ЦВЕТА КНОПКИ (ИСПРАВЛЕНИЕ ОШИБКИ) ---
            if (autopilot_ui_state == "Off") {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.0f)); // Серый цвет, если выключен
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.6f, 0.1f, 1.0f)); // Зеленый, если работает
            }

            if (ImGui::Button("ENGAGE AUTOPILOT", ImVec2(250.f, 40.f))) {
                if (autopilot_ui_state == "Off" || autopilot_ui_state == "OrbitMoon") {
                    autopilot.SetCurrentState("WaitAlignment");
                    std::cout << ">> Autopilot: ENGAGED. Waiting for phase alignment...\n";
                } else {
                    autopilot.SetCurrentState("Off");
                    std::cout << ">> Autopilot: DISABLED. Manual control.\n";
                }
            }
            
            // Теперь этой команде есть что "снимать" со стека!
            ImGui::PopStyleColor(); 
            // ---------------------------------------------------
        ImGui::End();
        // ===================================

        // --- ОКНО ПОРАЖЕНИЯ (GAME OVER) ---
        if ( is_game_over ) {
            // Центрируем окно ровно по середине экрана
            ImGui::SetNextWindowPos(ImVec2(window_width / 2.0f, window_height / 2.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGuiWindowFlags over_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
            
            // Задаем красный цвет для рамки окна, чтобы выглядело как тревога
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.0f, 0.0f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

            ImGui::Begin( "SYSTEM ALERT", nullptr, over_flags );
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", game_over_reason.c_str());
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                // Добавляем кнопку выхода
                if (ImGui::Button("EXIT SIMULATION", ImVec2(300.f, 40.f))) {
                    window.close(); // Закрываем программу
                }
            ImGui::End();

            ImGui::PopStyleColor(2);
        }

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
