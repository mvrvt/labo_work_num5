#pragma once

namespace my_utils {

// Замена std::pair
template <typename T1, typename T2>
struct Pair {
public:
    T1 first;
    T2 second;

    Pair() : first( T1() ), second( T2() ) { }
    Pair( const T1& f , const T2& s ) : first( f ), second ( s ) { }  
};

// Замена std::optional (для безопасного возврата значений)
template <typename T>
class Optional {
public:
    Optional() : has_value_( false ) { }
    explicit Optional( const T& val ) : value_( val ), has_value_( true ) { }

    bool HasValue() const { return has_value_; }

    const T& Value() const {
        if ( !has_value_ )
            throw std::runtime_error( "Optional object contains no value" );
        return value_;
    }

private:
    T    value_;
    bool has_value_;
};

// Структура для описания мощности (размера) LazySequence 
struct Cardinality {
    int infinite_parts;  // Кол-во бесконечных генераторов
    int finite_elements; // Кол-во обычнях элементов

    Cardinality( int inf = 0, int fin = 0 ) : infinite_parts( inf ), finite_elements( fin ) { }

    bool IsInfinite() const {
        return infinite_parts > 0;
    }

    // Перегрузка сложения для конкатенации
    Cardinality operator+( const Cardinality& other ) const {
        return Cardinality( 
            this->infinite_parts + other.infinite_parts,
            this->finite_elements + other.finite_elements
        );
    }
};

}
