#pragma once

#include <string>
#include <functional>
#include <stdexcept>
#include <memory>
#include "lab2_files/Sequence.hpp"
#include "lab2_files/ArraySequence.hpp"
#include "MyUtils.hpp"

// Просторанство имён для автомата, чтобы отделить логику FSM (Finite State Machine)
namespace fsm {
    
// Структура, описывающая состояние автомата
struct State {
    std::string id;
    bool        is_final; // Является ли состояние финальным (конечным)

    State( const std::string& state_id, bool final_state = false ) : id( state_id ), is_final( final_state ) { }
};

// Класс перехода между состояниями
template <typename T>
class Transition {
public:
    using PredicateFunc = std::function<bool( const T& )>;

    // Нужен для массивов
    Transition() : from_state(""), to_state(""), condition_( nullptr ) { }

    Transition( const std::string& from, const std::string& to, PredicateFunc condition )
        : from_state( from ), to_state( to ), condition_( condition ) { }

    // Проверка можно ли совершить переход по данному входному эл-ту
    bool CanTransition( const std::string& current_state, const T& input ) const {
        return ( current_state == from_state ) && condition_( input );
    }
    
    std::string GetNextState() const {
        return to_state;
    }

private:
    std::string   from_state;
    std::string   to_state;
    PredicateFunc condition_;
};

// Главный класс конечного автомата
template <typename T>
class StateMachine {
public:
    StateMachine() : current_state_id_( "" ) { }

    // Добавление нового состояния
    void AddState( const std::string& id, bool is_final = false ) {
        if ( FindState( id ) != -1 )
            throw std::invalid_argument( "StateMachine: State already exists" );
        states_.Append( std::make_shared<State>( id, is_final ) );
    }

    // Установка начального состояния
    void SetInitialState( const std::string& id ) {
        if ( FindState( id ) == -1 )
            throw std::invalid_argument( "StateMachine: State not found" );
        
        initial_state_id_ = id;
        current_state_id_ = id;
    }

    // Добавление правила перехода
    void AddTransition( const std::string& from, const std::string& to, std::function<bool( const T&)> condition ) {
        if ( FindState( from ) == -1 || FindState( to ) == -1 )
            throw std::invalid_argument( "StateMachine: invalid from/to state" );
        transitions_.Append( Transition<T>( from, to, condition ) );
    }

    // Запуск автомата на пос-ти (в т.ч. на LazySequence)
    bool Process( const Sequence<T>* input_stream ) {
        if ( initial_state_id_.empty() )
            throw std::logic_error( "StateMachine: initial state isn't set" );

        Reset();
        IEnumerator<T>* enumerator = input_stream->GetEnumerator();

        while ( enumerator->MoveNext() ) {
            const T& current_input = enumerator->Current();
            bool transitioned = false;

            // Ищем подходящий переход
            for ( int i = 0; i < transitions_.GetLength(); ++i ) {
                const Transition<T>& transition = transitions_.Get( i );
                if ( transition.CanTransition( current_state_id_, current_input ) ) {
                    current_state_id_ = transition.GetNextState();
                    transitioned = true;
                    break; 
                }
            }

            // Если для текущего символа нет перехода, автомат останавливается с ошибкой
            if ( !transitioned ) {
                delete enumerator;
                return false;
            }
        }

        delete enumerator;
        return IsInFinalState();
    }

    void Reset() { current_state_id_ = initial_state_id_; }

    std::string GetCurrentState() const {
        return current_state_id_;
    }

    // --- Дополнение для ЛР-5 (Игровой ИИ)

    // Принудительная установка текущего состояния (например, для вкл/выкл автопилота кнопкой)
    void SetCurrentState( const std::string& id ) {
        if ( FindState( id ) == -1 )
            throw std::invalid_argument( "StateMachine: state not found" );
        current_state_id_ = id;
    }

    // Покадровый шаг автомата
    // Принимает текущие данные и проверяет, нужно ли перейти в другое состояние 
    bool Step( const T& current_input ) {
        if ( current_state_id_.empty() ) return false; 

        for ( int i = 0; i < transitions_.GetLength(); ++i ) {
            const Transition<T>& transition = transitions_.Get( i );
            if ( transition.CanTransition( current_state_id_, current_input ) ) {
                current_state_id_ = transition.GetNextState();
                return true; // Произшёл переход
            }
        }
        return false; // Остаёмся в текущем состоянии
    }

private:
    MutableArraySequence<std::shared_ptr<State>> states_;
    MutableArraySequence<Transition<T>>          transitions_;

    std::string                                  initial_state_id_;
    std::string                                  current_state_id_;

    // Вспомогательный метод поиска состояния
    int FindState( const std::string& id ) const {
        for ( int i = 0; i < states_.GetLength(); ++i ) 
            if ( states_.Get( i )->id == id ) return i;
        return -1;
    }

    bool IsInFinalState() const {
        int idx = FindState( current_state_id_ );
        if ( idx != -1 ) 
            return states_.Get( idx )->is_final;
        return false;
    }
};

}
