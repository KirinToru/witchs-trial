#pragma once

#include <cstdint>
#include <string_view>

namespace Physics {
class PhysicsWorld;
}

class Enemy;
class Player;

enum class EnemyStateType : uint8_t {
    Idle,
    Chase,
    TelegraphAttack,
    ActiveAttack,
    Staggered,
    Dead
};

class EnemyState {
public:
//------------[Virtual Destructor - Allow Polymorphic Cleanup]-------------------
    virtual ~EnemyState() = default;
//-------------------------------------------------------

//------------[Enter - Invoked When State Becomes Active]-------------------
    virtual void enter(Enemy& enemy) {
        (void)enemy;
    }
//-------------------------------------------------------

//------------[Exit - Invoked Before State Transitions Away]-------------------
    virtual void exit(Enemy& enemy) {
        (void)enemy;
    }
//-------------------------------------------------------

//------------[Fixed Update - Step State Logic & Physics (60Hz)]-------------------
    virtual void fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) = 0;
//-------------------------------------------------------

//------------[Get Name - Return Human-Readable State Identifier]-------------------
    virtual std::string_view getName() const = 0;
//-------------------------------------------------------

//------------[Get Type - Return Concrete State Type Enum]-------------------
    virtual EnemyStateType getType() const = 0;
//-------------------------------------------------------
};
