#pragma once

#include <SFML/Window/Event.hpp>
#include <cstdint>
#include <string_view>

class Map;

namespace Physics {
class PhysicsWorld;
}

class Player;

enum class PlayerForm : uint8_t {
    Witch,
    Beast
};

enum class PlayerStateType : uint8_t {
    Idle,
    Run,
    Airborne,
    Dash,
    Pounce,
    MeleeAttack,
    HeavyStrike,
    CastSpell,
    Gun,
    Parry,
    Roar
};

class PlayerState {
public:
//------------[Virtual Destructor - Allow Polymorphic Cleanup]-------------------
    virtual ~PlayerState() = default;
//-------------------------------------------------------

//------------[Enter - Invoked When State Becomes Active]-------------------
    virtual void enter(Player& player) {
        (void)player;
    }
//-------------------------------------------------------

//------------[Exit - Invoked Before State Transitions Away]-------------------
    virtual void exit(Player& player) {
        (void)player;
    }
//-------------------------------------------------------

//------------[Handle Input - Process Discrete Input Events]-------------------
    virtual void handleInput(Player& player, const sf::Event& event) {
        (void)player;
        (void)event;
    }
//-------------------------------------------------------

//------------[Fixed Update - Step State Physics & Logic (60Hz)]-------------------
    virtual void fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) = 0;
//-------------------------------------------------------

//------------[Get Name - Return Human-Readable State Identifier]-------------------
    virtual std::string_view getName() const = 0;
//-------------------------------------------------------

//------------[Get Type - Return Enumerated State Classification]-------------------
    virtual PlayerStateType getType() const = 0;
//-------------------------------------------------------
};
