#include <Game/Entities/PlayerStates.hpp>
#include <Game/Entities/Player.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <algorithm>
#include <cmath>

//=============================================================================
// PlayerIdleState
//=============================================================================

//------------[Enter - Initialize Idle State Transition]-------------------
void PlayerIdleState::enter(Player& player) {
    player.setIsGrounded(true);
    player.setHasAirDash(true);
    player.setHasAirJump(true);
    player.setIsWallSliding(false);
    player.setIsJumping(false);
}
//-------------------------------------------------------

//------------[Handle Input - Process Idle Key Events]-------------------
void PlayerIdleState::handleInput(Player& player, const sf::Event& event) {
    (void)player;
    (void)event;
}
//-------------------------------------------------------

//------------[Fixed Update - Step Idle Physics & Ground Check]-------------------
void PlayerIdleState::fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    (void)map;

    sf::Vector2f vel = player.getVelocity();

    // Apply ground friction to bring residual horizontal momentum to a halt
    if (vel.x > 0.f) {
        vel.x -= player.getFriction() * dt;
        if (vel.x < 0.f) vel.x = 0.f;
    } else if (vel.x < 0.f) {
        vel.x += player.getFriction() * dt;
        if (vel.x > 0.f) vel.x = 0.f;
    }
    player.setVelocity(vel);

    // Jump detection
    bool jumpPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
    if (player.getJumpBufferTimer() > 0.f || (player.isAutoJumpEnabled() && jumpPressed)) {
        vel.y = -player.getJumpStrength();
        player.setVelocity(vel);
        player.setIsJumping(true);
        player.setIsGrounded(false);
        player.setJumpBufferTimer(0.f);
        player.changeState(PlayerStateType::Airborne);
        return;
    }

    // Special action: Dash (Witch) / Pounce (Beast)
    bool dashPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);
    if (dashPressed && player.getDashCooldownTimer() <= 0.f) {
        if (player.getForm() == PlayerForm::Witch) {
            player.changeState(PlayerStateType::Dash);
        } else {
            player.changeState(PlayerStateType::Pounce);
        }
        return;
    }

    // Horizontal movement input
    bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
    if (left != right) {
        player.changeState(PlayerStateType::Run);
        return;
    }

    // One-way platform drop through
    player.checkOneWayDropThrough(physicsWorld);

    // Downward floor probe using CCD sweep
    player.moveWithSweptCCD({0.f, 15.f * dt}, physicsWorld, true);

    if (!player.getIsGrounded()) {
        player.changeState(PlayerStateType::Airborne);
    }
}
//-------------------------------------------------------


//=============================================================================
// PlayerRunState
//=============================================================================

//------------[Enter - Initialize Run State Transition]-------------------
void PlayerRunState::enter(Player& player) {
    player.setIsGrounded(true);
    player.setHasAirDash(true);
    player.setHasAirJump(true);
    player.setIsWallSliding(false);
}
//-------------------------------------------------------

//------------[Handle Input - Process Run Key Events]-------------------
void PlayerRunState::handleInput(Player& player, const sf::Event& event) {
    (void)player;
    (void)event;
}
//-------------------------------------------------------

//------------[Fixed Update - Step Ground Acceleration and Kinematic Swept CCD]-------------------
void PlayerRunState::fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    (void)map;

    sf::Vector2f vel = player.getVelocity();

    // Jump detection
    bool jumpPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
    if (player.getJumpBufferTimer() > 0.f || (player.isAutoJumpEnabled() && jumpPressed)) {
        vel.y = -player.getJumpStrength();
        player.setVelocity(vel);
        player.setIsJumping(true);
        player.setIsGrounded(false);
        player.setJumpBufferTimer(0.f);
        player.changeState(PlayerStateType::Airborne);
        return;
    }

    // Special action: Dash (Witch) / Pounce (Beast)
    bool dashPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);
    if (dashPressed && player.getDashCooldownTimer() <= 0.f) {
        if (player.getForm() == PlayerForm::Witch) {
            player.changeState(PlayerStateType::Dash);
        } else {
            player.changeState(PlayerStateType::Pounce);
        }
        return;
    }

    // Horizontal movement acceleration
    bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);

    if (left && !right) {
        vel.x -= player.getAcceleration() * dt;
        player.setFacingRight(false);
    } else if (right && !left) {
        vel.x += player.getAcceleration() * dt;
        player.setFacingRight(true);
    } else {
        // No horizontal input, apply ground friction
        if (vel.x > 0.f) {
            vel.x -= player.getFriction() * dt;
            if (vel.x < 0.f) vel.x = 0.f;
        } else if (vel.x < 0.f) {
            vel.x += player.getFriction() * dt;
            if (vel.x > 0.f) vel.x = 0.f;
        }

        if (std::abs(vel.x) < 5.f) {
            vel.x = 0.f;
            player.setVelocity(vel);
            player.changeState(PlayerStateType::Idle);
            return;
        }
    }

    // Clamp horizontal velocity to current max speed
    float maxSpd = player.getCurrentMaxSpeed();
    if (vel.x > maxSpd) vel.x = maxSpd;
    if (vel.x < -maxSpd) vel.x = -maxSpd;
    player.setVelocity(vel);

    // One-way platform drop through
    player.checkOneWayDropThrough(physicsWorld);

    // Move player with Swept AABB CCD
    player.moveWithSweptCCD({vel.x * dt, 15.f * dt}, physicsWorld, true);

    if (!player.getIsGrounded()) {
        player.changeState(PlayerStateType::Airborne);
    }
}
//-------------------------------------------------------


//=============================================================================
// PlayerAirborneState
//=============================================================================

//------------[Enter - Initialize Airborne State Transition]-------------------
void PlayerAirborneState::enter(Player& player) {
    player.setIsGrounded(false);
}
//-------------------------------------------------------

//------------[Handle Input - Process Aerial Key Events]-------------------
void PlayerAirborneState::handleInput(Player& player, const sf::Event& event) {
    (void)player;
    (void)event;
}
//-------------------------------------------------------

//------------[Fixed Update - Step Airborne Physics, Gravity & Wall Slide]-------------------
void PlayerAirborneState::fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    (void)map;

    sf::Vector2f vel = player.getVelocity();

    // Aerial Dash / Pounce
    bool dashPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift);
    if (dashPressed && player.getDashCooldownTimer() <= 0.f && player.getHasAirDash()) {
        player.setHasAirDash(false);
        if (player.getForm() == PlayerForm::Witch) {
            player.changeState(PlayerStateType::Dash);
        } else {
            player.changeState(PlayerStateType::Pounce);
        }
        return;
    }

    bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
    bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
    bool jumpPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);

    // Horizontal air control (with slightly reduced air acceleration)
    float airAccel = player.getAcceleration() * 0.75f;
    if (left && !right) {
        vel.x -= airAccel * dt;
        player.setFacingRight(false);
    } else if (right && !left) {
        vel.x += airAccel * dt;
        player.setFacingRight(true);
    }

    float maxSpd = player.getCurrentMaxSpeed();
    if (vel.x > maxSpd) vel.x = maxSpd;
    if (vel.x < -maxSpd) vel.x = -maxSpd;

    // Wall slide query using narrow lateral sweep probe
    bool wallSliding = false;
    int wallSide = 0;
    if (vel.y > 0.f) {
        Physics::AABB currentAABB = player.getAABB();
        if (left) {
            Physics::SweptHit hit = physicsWorld.sweepTest(currentAABB, {-3.f, 0.f}, player.getRigidBody(), false);
            if (hit.hit && hit.normal.x > 0.5f) {
                wallSliding = true;
                wallSide = -1;
            }
        }
        if (!wallSliding && right) {
            Physics::SweptHit hit = physicsWorld.sweepTest(currentAABB, {3.f, 0.f}, player.getRigidBody(), false);
            if (hit.hit && hit.normal.x < -0.5f) {
                wallSliding = true;
                wallSide = 1;
            }
        }
    }

    player.setIsWallSliding(wallSliding);
    player.setWallDir(wallSide);

    if (wallSliding) {
        player.setHasAirDash(true); // Wall touching restores air dash

        float maxSlide = down ? player.getFastWallSlideSpeed() : player.getWallSlideSpeed();
        if (vel.y > maxSlide) {
            vel.y = maxSlide;
        }

        // Wall jump
        if (player.getJumpBufferTimer() > 0.f || (player.isAutoJumpEnabled() && jumpPressed)) {
            sf::Vector2f wjForce = player.getWallJumpForce();
            vel.x = (wallSide == 1) ? -wjForce.x : wjForce.x;
            vel.y = -wjForce.y;
            player.setFacingRight(vel.x > 0.f);
            player.setIsWallSliding(false);
            player.setIsJumping(true);
            player.setJumpBufferTimer(0.f);
        }
    } else {
        // Normal aerial gravity simulation
        float grav = player.getGravity();

        // Apex hang time (reduced gravity near jump apex)
        if (player.getIsJumping() && std::abs(vel.y) < 60.f) {
            grav *= 0.5f;
        } else if (vel.y > 0.f) {
            grav *= 1.25f; // Fall faster for snappy platforming feel
        } else if (!jumpPressed && vel.y < 0.f) {
            grav *= 2.0f; // Variable jump height cut-off when releasing Space
        }

        vel.y += grav * dt;
        if (vel.y > 850.f) vel.y = 850.f; // Terminal velocity

        // Coyote jump handling
        if ((player.getJumpBufferTimer() > 0.f || (player.isAutoJumpEnabled() && jumpPressed)) && player.getCoyoteTimer() > 0.f) {
            vel.y = -player.getJumpStrength();
            player.setCoyoteTimer(0.f);
            player.setIsJumping(true);
            player.setJumpBufferTimer(0.f);
        }
    }

    player.setVelocity(vel);

    // Step displacement with Swept AABB CCD
    player.moveWithSweptCCD(player.getVelocity() * dt, physicsWorld, false);

    // Landing transition check
    if (player.getIsGrounded()) {
        sf::Vector2f landVel = player.getVelocity();
        landVel.y = 0.f;
        player.setVelocity(landVel);
        player.setIsJumping(false);
        player.setHasAirDash(true);

        if (std::abs(landVel.x) > 20.f) {
            player.changeState(PlayerStateType::Run);
        } else {
            player.changeState(PlayerStateType::Idle);
        }
    }
}
//-------------------------------------------------------


//=============================================================================
// PlayerDashState (Witch Form Air-Dash)
//=============================================================================

//------------[Enter - Initialize Witch Air-Dash Burst & Freeze Timer]-------------------
void PlayerDashState::enter(Player& player) {
    player.setIsDashing(true);
    player.setDashTimer(0.15f);
    player.setDashFreezeTimer(0.07f);
    player.setDashCooldownTimer(0.5f);

    bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
    bool up = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
    bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);

    sf::Vector2f dir{0.f, 0.f};
    if (up) dir.y = -1.f;
    else if (down) dir.y = 1.f;
    if (left) dir.x = -1.f;
    else if (right) dir.x = 1.f;

    if (dir.x == 0.f && dir.y == 0.f) {
        dir.x = player.isFacingRight() ? 1.f : -1.f;
    }

    float len = std::hypot(dir.x, dir.y);
    if (len > 0.001f) {
        dir /= len;
    }

    player.setDashDirection(dir);
    player.setVelocity({0.f, 0.f});
}
//-------------------------------------------------------

//------------[Exit - Restore Normal Physics & Retain Momentum]-------------------
void PlayerDashState::exit(Player& player) {
    player.setIsDashing(false);
    sf::Vector2f vel = player.getVelocity();
    if (player.getDashDirection().y < 0.f) {
        vel.y *= 0.85f;
    }
    player.setVelocity(vel);
}
//-------------------------------------------------------

//------------[Fixed Update - Step Air-Dash Freeze, Burst & CCD Sweep]-------------------
void PlayerDashState::fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    (void)map;

    float freeze = player.getDashFreezeTimer();
    if (freeze > 0.f) {
        player.setDashFreezeTimer(freeze - dt);
        player.setVelocity({0.f, 0.f});

        // Allow directional redirection during freeze pause
        bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
        bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
        bool up = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
        bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);

        sf::Vector2f newDir{0.f, 0.f};
        if (up) newDir.y = -1.f;
        else if (down) newDir.y = 1.f;
        if (left) newDir.x = -1.f;
        else if (right) newDir.x = 1.f;

        if (newDir.x != 0.f || newDir.y != 0.f) {
            float len = std::hypot(newDir.x, newDir.y);
            player.setDashDirection(newDir / len);
        }
        return;
    }

    float timer = player.getDashTimer() - dt;
    player.setDashTimer(timer);

    sf::Vector2f burstVel = player.getDashDirection() * player.getDashSpeed();
    player.setVelocity(burstVel);
    player.setCurrentMaxSpeed(player.getDashSpeed() * 0.8f);

    player.moveWithSweptCCD(burstVel * dt, physicsWorld, false);

    if (timer <= 0.f) {
        if (player.getIsGrounded()) {
            player.changeState(PlayerStateType::Run);
        } else {
            player.changeState(PlayerStateType::Airborne);
        }
    }
}
//-------------------------------------------------------


//=============================================================================
// PlayerPounceState (Beast Form Pounce / Ground Smash)
//=============================================================================

//------------[Enter - Initialize Beast Pounce or Ground Smash]-------------------
void PlayerPounceState::enter(Player& player) {
    player.setIsDashing(true);
    player.setDashCooldownTimer(0.6f);

    bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);

    // If holding down while airborne, trigger Ground Smash
    if (down && !player.getIsGrounded()) {
        mIsGroundSmash = true;
        mDurationTimer = 0.6f;
        mSmashRecoveryTimer = 0.f;
        player.setVelocity({0.f, 1000.f}); // Ferocious downward smash velocity
    } else {
        // Forward leaping Beast Pounce
        mIsGroundSmash = false;
        mDurationTimer = 0.4f;
        mSmashRecoveryTimer = 0.f;

        bool up = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
        float pounceX = player.isFacingRight() ? 650.f : -650.f;
        float pounceY = up ? -360.f : -200.f;

        player.setVelocity({pounceX, pounceY});
        player.setCurrentMaxSpeed(750.f);
    }
}
//-------------------------------------------------------

//------------[Exit - Restore Normal Physics & Retain Beast Momentum]-------------------
void PlayerPounceState::exit(Player& player) {
    player.setIsDashing(false);
    mIsGroundSmash = false;
    mSmashRecoveryTimer = 0.f;
}
//-------------------------------------------------------

//------------[Fixed Update - Step Beast Pounce Velocity & Ground Smash Impact]-------------------
void PlayerPounceState::fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    (void)map;

    // Ground Smash landing recovery pause
    if (mSmashRecoveryTimer > 0.f) {
        mSmashRecoveryTimer -= dt;
        player.setVelocity({0.f, 0.f});
        if (mSmashRecoveryTimer <= 0.f) {
            player.changeState(PlayerStateType::Idle);
        }
        return;
    }

    if (mIsGroundSmash) {
        sf::Vector2f vel = player.getVelocity();
        vel.y += 1800.f * dt; // Heavy downward acceleration
        if (vel.y > 1200.f) vel.y = 1200.f;
        player.setVelocity(vel);

        player.moveWithSweptCCD(vel * dt, physicsWorld, false);

        if (player.getIsGrounded()) {
            // High-impact ground impact pause
            mSmashRecoveryTimer = 0.12f;
            player.setVelocity({0.f, 0.f});
            player.setCurrentMaxSpeed(player.getMoveSpeed());
        }
    } else {
        // Pounce leap trajectory
        mDurationTimer -= dt;
        sf::Vector2f vel = player.getVelocity();
        vel.y += player.getGravity() * 0.75f * dt;
        player.setVelocity(vel);

        player.moveWithSweptCCD(vel * dt, physicsWorld, false);

        if (player.getIsGrounded()) {
            player.changeState(PlayerStateType::Run);
        } else if (mDurationTimer <= 0.f) {
            player.changeState(PlayerStateType::Airborne);
        }
    }
}
//-------------------------------------------------------
