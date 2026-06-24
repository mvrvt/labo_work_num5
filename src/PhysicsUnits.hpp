#pragma once

// Собираем типы в пространство имён, чтобы не было конфликтов 
namespace physics {

// АТД "Масса"
class Mass {
public:
    explicit Mass(double v = 0.0) : value_kg(v) {}
    
    [[nodiscard]] double in_kg() const { return value_kg; }
    
    Mass operator+(const Mass& other) const { return Mass(value_kg + other.value_kg); }
    Mass operator-(const Mass& other) const { return Mass(value_kg - other.value_kg); }
    
    // Операторы сравнения (нужны для std::max и проверок)
    bool operator>(const Mass& other) const { return value_kg > other.value_kg; }
    bool operator<(const Mass& other) const { return value_kg < other.value_kg; }
    bool operator==(const Mass& other) const { return value_kg == other.value_kg; }
    bool operator>=(const Mass& other) const { return value_kg >= other.value_kg; }
    bool operator<=(const Mass& other) const { return value_kg <= other.value_kg; }

private:
    double value_kg;
};

// АТД "Дистанция"
class Distance {
public:
    explicit Distance(double v = 0.0) : value_m(v) {}
    [[nodiscard]] double in_meters() const { return value_m; }
    [[nodiscard]] double in_km() const { return value_m / 1000.0; }

private:
    double value_m;
};


// АТД "Сила (Тяга)"
class Force {
public:
    explicit Force(double v = 0.0) : value_newtons(v) {}
    [[nodiscard]] double in_newtons() const { return value_newtons; }

private:
    double value_newtons;
};

// --- Пользовательские литералы ---
// Позволяет писать: 1000.0_kg или 384000.0_km прямо в коде
inline Mass operator"" _kg(long double val) { return Mass(static_cast<double>(val)); }
inline Distance operator"" _m(long double val) { return Distance(static_cast<double>(val)); }
inline Distance operator"" _km(long double val) { return Distance(static_cast<double>(val) * 1000.0); }
inline Force operator"" _N(long double val) { return Force(static_cast<double>(val)); }
inline Force operator"" _kN(long double val) { return Force(static_cast<double>(val) * 1000.0); }

}

