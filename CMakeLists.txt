cmake_minimum_required( VERSION 3.15 )

# Название проекта
project( SpaceSimulation )

# Требуется стандарт С++17
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic" )

# Указываем Cmake где Homebrew хранит библиотеки на Apple Silicon Mac
set( CMAKE_PREFIX_PATH "/opt/homebrew" )

# Поиск библиотеки SFML (нужны модули графики, окна и системы)
find_package( SFML 3 COMPONENTS Graphics Window System REQUIRED )

# Указатель на место, где лежат заголовочные файлы
include_directories( src )

set( SOURCES
    main.cpp
    src/Vector.cpp
    src/Planet.cpp
    src/SpaceShip.cpp
    src/Simulation.cpp
    src/Renderer.cpp
)

# Создание исполняемого файла (программы)
add_executable( SpaceSimulation ${SOURCES} )

# Линкуем (привязываем) SFML к программе 
target_link_libraries( SpaceSimulation PRIVATE SFML::Graphics SFML::Window SFML::System )
