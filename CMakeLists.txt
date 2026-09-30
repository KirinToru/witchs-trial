cmake_minimum_required(VERSION 3.28)
project(WitchsTrial)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_BUILD_RPATH "$ORIGIN/lib")
set(CMAKE_INSTALL_RPATH "$ORIGIN/lib")

include(FetchContent)

message(STATUS "Fetching SFML 3.1.0...")
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SFML)

if(MSVC)
    add_compile_options("/Zc:__cplusplus")
endif()

set(SOURCES
    "src/Game/main.cpp"
    "src/Game/Game.cpp"
    "src/Game/Entities/Player.cpp"
    "src/Game/Entities/PlayerStates.cpp"
    "src/Game/Entities/EnemyStates.cpp"
    "src/Game/Entities/Enemy.cpp"
    "src/Game/Entities/InquisitorFootman.cpp"
    "src/Game/Entities/GrandInquisitor.cpp"
    "src/Game/Entities/SourceMovementController2D.cpp"
    "src/Game/Entities/InteractiveProp2D.cpp"
    "src/Game/World/Map.cpp"
    "src/Game/States/MenuState.cpp"
    "src/Game/States/PauseState.cpp"
    "src/Game/States/GameState.cpp"
    "src/Engine/GUI/Button.cpp"
    "src/Game/UI/SpawnMenu.cpp"
    "src/Game/UI/HUD.cpp"
    "src/Game/Systems/MouseInteraction.cpp"
    "src/Game/Systems/WindSystem.cpp"
    "src/Game/Systems/ObjectManager.cpp"
    "src/Engine/Physics/PhysicsWorld.cpp"
)
add_executable(WitchsTrial ${SOURCES})

target_include_directories(WitchsTrial PRIVATE include)
target_link_libraries(WitchsTrial PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)

if(WIN32)
    add_custom_command(TARGET WitchsTrial POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/lib"
        "$<TARGET_FILE_DIR:WitchsTrial>"
    )
endif()

add_custom_command(TARGET WitchsTrial POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:WitchsTrial>/assets"
)
