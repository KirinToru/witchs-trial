#include <Game/Entities/EnemyStates.hpp>
#include <Game/Entities/Enemy.hpp>
#include <Game/Entities/Player.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <algorithm>

//=============================================================================
// EnemyIdleState
//=============================================================================

//------------[Enter - Initialize Idle State & Zero Velocity]-------------------
void EnemyIdleState::enter(Enemy& enemy) {
    sf::Vector2f vel = enemy.getVelocity();
    vel.x = 0.f;
    enemy.setVelocity(vel);
}
//-------------------------------------------------------

//------------[Exit - Cleanup Idle State]-------------------
void EnemyIdleState::exit(Enemy& enemy) {
    (void)enemy;
}
//-------------------------------------------------------

//------------[Fixed Update - Check Line Of Sight & Detection Range (60Hz)]-------------------
void EnemyIdleState::fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) {
    // 1. Settle on ground with gravity
    sf::Vector2f vel = enemy.getVelocity();
    vel.y += 980.f * dt;
    enemy.setVelocity(vel);
    enemy.moveWithSweptCCD({0.f, vel.y * dt}, physicsWorld, player.getRigidBody());

    // 2. Query target proximity & Line of Sight
    float dist = enemy.getDistanceToPlayer(player);
    if (dist <= enemy.getDetectionRange()) {
        if (enemy.hasLineOfSightToPlayer(player, physicsWorld)) {
            float dir = enemy.getDirectionToPlayer(player);
            enemy.setFacingRight(dir > 0.f);
            enemy.changeState(EnemyStateType::Chase);
        }
    }
}
//-------------------------------------------------------

//=============================================================================
// EnemyChaseState
//=============================================================================

//------------[Enter - Initialize Chase Behavior]-------------------
void EnemyChaseState::enter(Enemy& enemy) {
    (void)enemy;
    mLostLoSTimer = 0.f;
}
//-------------------------------------------------------

//------------[Exit - Cleanup Chase State]-------------------
void EnemyChaseState::exit(Enemy& enemy) {
    (void)enemy;
}
//-------------------------------------------------------

//------------[Fixed Update - Pursue Player & Check Attack Range (60Hz)]-------------------
void EnemyChaseState::fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) {
    sf::Vector2f vel = enemy.getVelocity();
    vel.y += 980.f * dt;

    float dist = enemy.getDistanceToPlayer(player);
    bool hasLoS = enemy.hasLineOfSightToPlayer(player, physicsWorld);

    if (!hasLoS) {
        mLostLoSTimer += dt;
        if (mLostLoSTimer >= 2.5f || dist > 550.f) {
            enemy.changeState(EnemyStateType::Idle);
            return;
        }
    } else {
        mLostLoSTimer = 0.f;
    }

    float dir = enemy.getDirectionToPlayer(player);
    enemy.setFacingRight(dir > 0.f);

    // Check if within attack range
    if (dist <= enemy.getAttackRange() && hasLoS) {
        if (enemy.getAttackCooldownTimer() <= 0.f) {
            enemy.changeState(EnemyStateType::TelegraphAttack);
            return;
        } else {
            // Player is in attack range but attack is on cooldown; hold position
            vel.x = 0.f;
        }
    } else {
        // Move towards player
        vel.x = dir * enemy.getMoveSpeed();
    }

    enemy.setVelocity(vel);
    enemy.moveWithSweptCCD(vel * dt, physicsWorld, player.getRigidBody());
}
//-------------------------------------------------------

//=============================================================================
// EnemyTelegraphAttackState
//=============================================================================

//------------[Enter - Lock Facing Direction & Start Windup Timer]-------------------
void EnemyTelegraphAttackState::enter(Enemy& enemy) {
    enemy.setTelegraphTimer(0.f);
    enemy.deactivateAttackHitbox();

    sf::Vector2f vel = enemy.getVelocity();
    vel.x = 0.f;
    enemy.setVelocity(vel);
}
//-------------------------------------------------------

//------------[Exit - Cleanup Telegraph Visuals]-------------------
void EnemyTelegraphAttackState::exit(Enemy& enemy) {
    (void)enemy;
}
//-------------------------------------------------------

//------------[Fixed Update - Step Windup Progress & Trigger Attack (60Hz)]-------------------
void EnemyTelegraphAttackState::fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) {
    (void)player;

    sf::Vector2f vel = enemy.getVelocity();
    vel.y += 980.f * dt;
    vel.x = 0.f;
    enemy.setVelocity(vel);
    enemy.moveWithSweptCCD({0.f, vel.y * dt}, physicsWorld, player.getRigidBody());

    float t = enemy.getTelegraphTimer() + dt;
    enemy.setTelegraphTimer(t);

    if (t >= enemy.getTelegraphDuration()) {
        enemy.changeState(EnemyStateType::ActiveAttack);
    }
}
//-------------------------------------------------------

//=============================================================================
// EnemyActiveAttackState
//=============================================================================

//------------[Enter - Initialize Weapon Swing & Forward Momentum]-------------------
void EnemyActiveAttackState::enter(Enemy& enemy) {
    mAttackTimer = 0.f;
    mHitboxActivated = false;
    mHitboxDeactivated = false;
    enemy.deactivateAttackHitbox();

    // Minor forward lunge impulse during swing
    sf::Vector2f vel = enemy.getVelocity();
    vel.x = enemy.isFacingRight() ? 90.f : -90.f;
    enemy.setVelocity(vel);
}
//-------------------------------------------------------

//------------[Exit - Deactivate Weapon Hitbox]-------------------
void EnemyActiveAttackState::exit(Enemy& enemy) {
    enemy.deactivateAttackHitbox();
    mHitboxActivated = false;
    mHitboxDeactivated = false;
}
//-------------------------------------------------------

//------------[Fixed Update - Step Hitbox Activation Window & Recovery (60Hz)]-------------------
void EnemyActiveAttackState::fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) {
    (void)player;
    mAttackTimer += dt;

    sf::Vector2f vel = enemy.getVelocity();
    vel.y += 980.f * dt;
    vel.x *= std::max(0.f, 1.f - 4.f * dt); // Smooth lunge friction
    enemy.setVelocity(vel);
    enemy.moveWithSweptCCD(vel * dt, physicsWorld, player.getRigidBody());

    // Hitbox window: 0.06s to 0.22s
    if (mAttackTimer >= 0.06f && !mHitboxActivated) {
        mHitboxActivated = true;
        enemy.spawnAttackHitbox();
    } else if (mAttackTimer >= 0.22f && !mHitboxDeactivated) {
        mHitboxDeactivated = true;
        enemy.deactivateAttackHitbox();
    }

    if (mAttackTimer >= 0.45f) {
        enemy.setAttackCooldownTimer(0.9f);
        enemy.changeState(EnemyStateType::Chase);
    }
}
//-------------------------------------------------------

//=============================================================================
// EnemyStaggeredState
//=============================================================================

//------------[Enter - Trigger Posture Break Stun & Disable Movement]-------------------
void EnemyStaggeredState::enter(Enemy& enemy) {
    enemy.setStaggerTimer(0.f);
    enemy.deactivateAttackHitbox();

    sf::Vector2f vel = enemy.getVelocity();
    vel.x = 0.f;
    enemy.setVelocity(vel);
}
//-------------------------------------------------------

//------------[Exit - Restore Enemy Poise On Wakeup]-------------------
void EnemyStaggeredState::exit(Enemy& enemy) {
    enemy.resetPosture();
}
//-------------------------------------------------------

//------------[Fixed Update - Count Down Stagger Stun Duration (60Hz)]-------------------
void EnemyStaggeredState::fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) {
    sf::Vector2f vel = enemy.getVelocity();
    vel.y += 980.f * dt;
    vel.x = 0.f;
    enemy.setVelocity(vel);
    enemy.moveWithSweptCCD({0.f, vel.y * dt}, physicsWorld, player.getRigidBody());

    float t = enemy.getStaggerTimer() + dt;
    enemy.setStaggerTimer(t);

    if (t >= enemy.getStaggerDuration()) {
        enemy.resetPosture();
        enemy.changeState(EnemyStateType::Chase);
    }
}
//-------------------------------------------------------

//=============================================================================
// EnemyDeadState
//=============================================================================

//------------[Enter - Deactivate Collision & Play Defeat Sequence]-------------------
void EnemyDeadState::enter(Enemy& enemy) {
    enemy.deactivateAttackHitbox();
    enemy.getHurtbox().active = false;
    enemy.setVelocity({0.f, 0.f});
}
//-------------------------------------------------------

//------------[Exit - Finalize Enemy Removal]-------------------
void EnemyDeadState::exit(Enemy& enemy) {
    (void)enemy;
}
//-------------------------------------------------------

//------------[Fixed Update - Settle Corpse On Ground (60Hz)]-------------------
void EnemyDeadState::fixedUpdate(Enemy& enemy, float dt, const Player& player, const Physics::PhysicsWorld& physicsWorld) {
    sf::Vector2f vel = enemy.getVelocity();
    vel.y += 980.f * dt;
    vel.x = 0.f;
    enemy.setVelocity(vel);
    enemy.moveWithSweptCCD({0.f, vel.y * dt}, physicsWorld, player.getRigidBody());
}
//-------------------------------------------------------
