cmake_minimum_required( VERSION 3.15 )

project( SpaceSimulation )

# Cтандарт С++17
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic" ) 

# Указываем Cmake где Homebrew хранит библиотеки на Apple Silicon Mac
set( CMAKE_PREFIX_PATH "/opt/homebrew" )

# Поиск библиотеки SFML
find_package( SFML 3 COMPONENTS Graphics Window System REQUIRED )

# === ИНТЕГРАЦИЯ IMGUI И IMGUI-SFML ===
set( BUILD_SHARED_LIBS ON )
set( SFML_STATIC_LIBRARIES OFF )

include(FetchContent)

# 1. Скачиваем ядро Dear ImGui 
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.5
)
FetchContent_GetProperties( imgui )
if ( NOT imgui_POPULATED )
    FetchContent_Populate( imgui )
endif()

# Передаем путь к скачанному ядру в обязательную переменную IMGUI_DIR
set( IMGUI_DIR ${imgui_SOURCE_DIR} )

# 2. Теперь скачиваем мост ImGui-SFML
FetchContent_Declare(
    ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG master 
)
FetchContent_MakeAvailable(ImGui-SFML)
# ====================================

# Указатель на место, где лежат заголовочные файлы
include_directories( src )

set( SOURCES
    main.cpp
    src/Vector.cpp
    src/Planet.cpp
    src/SpaceShip.cpp
    src/Simulation.cpp
    src/Renderer.cpp
    src/Moon.cpp
)

# Создание исполняемого файла (программы)
add_executable( SpaceSimulation ${SOURCES} )

# Линкуем (привязываем) SFML и ImGui-SFML к программе 
target_link_libraries( SpaceSimulation PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::System 
    ImGui-SFML::ImGui-SFML 
)

file( COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )
