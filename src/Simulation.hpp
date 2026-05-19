#pragma once

#include <vector>
#include <memory>
#include "ICelestialBody.hpp"

class Simulation {
public:
    // Добавить тело во вселенную
    void AddBody( std::shared_ptr<ICelestialBody> body );

    // Шаг физики
    void Update( double dt );

    // Getter для получения списка тел (возвращаем по константной ссылке, чтобы не копировать массив)
    [[nodiscard]] const std::vector<std::shared_ptr<ICelestialBody> >& GetUniverse() const;

private:
    std::vector<std::shared_ptr<ICelestialBody> > universe_;

    const double kGravity_ = 1000.0;
};
