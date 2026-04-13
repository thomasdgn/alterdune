cmake_minimum_required(VERSION 3.18)
project(ALTERDUNE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ALTERDUNE_CORE_SOURCES
    src/ActAction.cpp
    src/BestiaryEntry.cpp
    src/Entity.cpp
    src/Game.cpp
    src/Item.cpp
    src/Monster.cpp
    src/Player.cpp
)

add_executable(alterdune_console
    src/main.cpp
    ${ALTERDUNE_CORE_SOURCES}
)

target_include_directories(alterdune_console PRIVATE include)

find_package(SFML 3 QUIET COMPONENTS Graphics Window System Audio CONFIG)

if(SFML_FOUND)
    add_executable(alterdune_frontend
        src/frontend_main.cpp
        src/FrontendApp.cpp
        ${ALTERDUNE_CORE_SOURCES}
    )

    target_include_directories(alterdune_frontend PRIVATE include)
    target_link_libraries(alterdune_frontend PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Audio)
endif()
