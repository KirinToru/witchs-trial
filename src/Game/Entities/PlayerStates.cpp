#include <Game/Entities/PlayerStates.hpp>
#include <Game/Entities/Player.hpp>
#include <Game/Combat/CombatBoxes.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <Engine/Audio/AudioManager.hpp>
#include <Engine/Graphics/ParticleSystem.hpp>
#include <Game/Systems/ObjectManager.hpp>
#include <Game/Entities/Enemy.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
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
    player.getAnimator().play("Idle");
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

    float f = player.isOnIce() ? (player.getFriction() * 0.03f) : player.getFriction();
    if (vel.x > 0.f) {
        vel.x -= f * dt;
        if (vel.x < 0.f) vel.x = 0.f;
    } else if (vel.x < 0.f) {
        vel.x += f * dt;
        if (vel.x > 0.f) vel.x = 0.f;
    }
    player.setVelocity(vel);

    // Jump detection (W)
    bool jumpPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
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
    bool dashPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
    if (dashPressed && player.getDashCooldownTimer() <= 0.f) {
        if (player.getForm() == PlayerForm::Witch) {
            player.changeState(PlayerStateType::Dash);
        } else {
            player.changeState(PlayerStateType::Pounce);
        }
        return;
    }

    // Parry action: C
    bool parryPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C);
    if (parryPressed && player.getForm() == PlayerForm::Witch) {
        player.changeState(PlayerStateType::Parry);
        return;
    }

    // Combat action: Melee J (Witch Light Slash / Beast Heavy Strike)
    bool attackPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J);
    if (attackPressed) {
        if (player.getForm() == PlayerForm::Witch) {
            if (player.canMeleeAttack() && player.consumeStamina(20.f)) {
                player.changeState(PlayerStateType::MeleeAttack);
                return;
            }
        } else {
            player.changeState(PlayerStateType::HeavyStrike);
            return;
        }
    }

    // Gun action: K (Witch Form)
    bool gunPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K);
    if (gunPressed && player.getForm() == PlayerForm::Witch && player.hasMana(15.f) && player.canFireGun()) {
        player.changeState(PlayerStateType::Gun);
        return;
    }

    // Magic Spell action / Roar: L
    bool spellPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L);
    if (spellPressed) {
        if (player.getForm() == PlayerForm::Witch && player.canCastSpell()) {
            player.castActiveSpell();
        } else if (player.getForm() == PlayerForm::Beast && player.canCastSpell() && player.getRage() >= 25.f) {
            player.changeState(PlayerStateType::Roar);
            return;
        }
    }

    // Horizontal movement input & facing direction
    bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
    if (left && !right) {
        player.setFacingRight(false);
    } else if (right && !left) {
        player.setFacingRight(true);
    }

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
    player.getAnimator().play("Run");
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

    // Jump detection (W)
    bool jumpPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);
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
    bool dashPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
    if (dashPressed && player.getDashCooldownTimer() <= 0.f) {
        if (player.getForm() == PlayerForm::Witch) {
            player.changeState(PlayerStateType::Dash);
        } else {
            player.changeState(PlayerStateType::Pounce);
        }
        return;
    }

    // Parry action: C
    bool parryPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C);
    if (parryPressed && player.getForm() == PlayerForm::Witch) {
        player.changeState(PlayerStateType::Parry);
        return;
    }

    // Combat action: Melee J (Witch Light Slash / Beast Heavy Strike)
    bool attackPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J);
    if (attackPressed) {
        if (player.getForm() == PlayerForm::Witch) {
            if (player.canMeleeAttack() && player.consumeStamina(20.f)) {
                player.changeState(PlayerStateType::MeleeAttack);
                return;
            }
        } else {
            player.changeState(PlayerStateType::HeavyStrike);
            return;
        }
    }

    // Gun action: K (Witch Form)
    bool gunPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K);
    if (gunPressed && player.getForm() == PlayerForm::Witch && player.hasMana(15.f) && player.canFireGun()) {
        player.changeState(PlayerStateType::Gun);
        return;
    }

    // Magic Spell action / Roar: L
    bool spellPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L);
    if (spellPressed) {
        if (player.getForm() == PlayerForm::Witch && player.canCastSpell()) {
            player.castActiveSpell();
        } else if (player.getForm() == PlayerForm::Beast && player.canCastSpell() && player.getRage() >= 25.f) {
            player.changeState(PlayerStateType::Roar);
            return;
        }
    }

    // Horizontal movement acceleration
    bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);

    if (left && !right) {
        player.setFacingRight(false);
    } else if (right && !left) {
        player.setFacingRight(true);
    }

    if (left && !right) {
        vel.x -= player.getAcceleration() * dt;
    } else if (right && !left) {
        vel.x += player.getAcceleration() * dt;
    } else {
        float f = player.isOnIce() ? (player.getFriction() * 0.03f) : player.getFriction();
        if (vel.x > 0.f) {
            vel.x -= f * dt;
            if (vel.x < 0.f) vel.x = 0.f;
        } else if (vel.x < 0.f) {
            vel.x += f * dt;
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
    if (player.getVelocity().y < 0.f) {
        player.getAnimator().play("Jump");
    } else {
        player.getAnimator().play("Fall");
    }
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

    // Aerial Dash / Pounce (Shift)
    bool dashPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
    if (dashPressed && player.getDashCooldownTimer() <= 0.f && player.getHasAirDash()) {
        player.setHasAirDash(false);
        if (player.getForm() == PlayerForm::Witch) {
            player.changeState(PlayerStateType::Dash);
        } else {
            player.changeState(PlayerStateType::Pounce);
        }
        return;
    }

    // Parry action in air: C
    bool parryPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::C);
    if (parryPressed && player.getForm() == PlayerForm::Witch) {
        player.changeState(PlayerStateType::Parry);
        return;
    }

    // Aerial combat action: Melee J (Witch Light Slash / Beast Heavy Strike)
    bool attackPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J);
    if (attackPressed) {
        if (player.getForm() == PlayerForm::Witch) {
            if (player.canMeleeAttack() && player.consumeStamina(20.f)) {
                player.changeState(PlayerStateType::MeleeAttack);
                return;
            }
        } else {
            player.changeState(PlayerStateType::HeavyStrike);
            return;
        }
    }

    // Gun action in air: K (Witch Form)
    bool gunPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::K);
    if (gunPressed && player.getForm() == PlayerForm::Witch && player.hasMana(15.f) && player.canFireGun()) {
        player.changeState(PlayerStateType::Gun);
        return;
    }

    // Magic Spell action / Roar in air: L
    bool spellPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::L);
    if (spellPressed) {
        if (player.getForm() == PlayerForm::Witch && player.canCastSpell()) {
            player.castActiveSpell();
        } else if (player.getForm() == PlayerForm::Beast && player.canCastSpell() && player.getRage() >= 25.f) {
            player.changeState(PlayerStateType::Roar);
            return;
        }
    }

    bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
    bool down = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S);
    bool jumpPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W);

    if (left && !right) {
        player.setFacingRight(false);
    } else if (right && !left) {
        player.setFacingRight(true);
    }

    // Plunge / Drop down mechanic: S in air
    if (down) {
        vel.y = std::max(vel.y, 500.f);
        vel.y += 2200.f * dt;
        if (vel.y > 1150.f) vel.y = 1150.f;
    }

    // Horizontal air control (with slightly reduced air acceleration)
    float airAccel = player.getAcceleration() * 0.75f;
    if (left && !right) {
        vel.x -= airAccel * dt;
    } else if (right && !left) {
        vel.x += airAccel * dt;
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
        float grav = player.getGravity();

        if (!player.isExternalImpulseActive()) {
            if (player.getIsJumping() && std::abs(vel.y) < 60.f) {
                grav *= 0.5f;
            } else if (vel.y > 0.f) {
                grav *= 1.25f;
            } else if (!jumpPressed && vel.y < 0.f) {
                grav *= 2.0f;
            }
        } else {
            if (vel.y > 0.f) {
                grav *= 1.25f;
            }
        }

        if (vel.y < 0.f) {
            player.getAnimator().play("Jump");
        } else {
            player.getAnimator().play("Fall");
        }

        vel.y += grav * dt;
        if (vel.y > 850.f) vel.y = 850.f;

        if (!player.isExternalImpulseActive()) {
            if ((player.getJumpBufferTimer() > 0.f || (player.isAutoJumpEnabled() && jumpPressed)) && player.getCoyoteTimer() > 0.f) {
                vel.y = -player.getJumpStrength();
                player.setCoyoteTimer(0.f);
                player.setIsJumping(true);
                player.setJumpBufferTimer(0.f);
            } else if (player.getJumpBufferTimer() > 0.f && player.getHasAirJump()) {
                vel.y = -player.getJumpStrength();
                player.setHasAirJump(false);
                player.setIsJumping(true);
                player.setJumpBufferTimer(0.f);
                player.getAnimator().play("Jump");
                Engine::Audio::AudioManager::getInstance().playSound("assets/audio/jump.wav", 80.f);
            }
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
    // Air-Dash consumes stamina in Witch Form
    if (!player.consumeStamina(25.f)) {
        player.setIsDashing(false);
        if (player.getIsGrounded()) {
            player.changeState(PlayerStateType::Idle);
        } else {
            player.changeState(PlayerStateType::Airborne);
        }
        return;
    }

    // Enable invulnerability frames (i-frames) during dodge
    player.getHurtbox().invulnerable = true;

    player.setIsDashing(true);
    player.setDashTimer(0.15f);
    player.setDashFreezeTimer(0.07f);
    player.setDashCooldownTimer(0.5f);
    player.getAnimator().play("Dash");

    bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);

    sf::Vector2f dir{0.f, 0.f};
    if (left && !right) {
        dir.x = -1.f;
        player.setFacingRight(false);
    } else if (right && !left) {
        dir.x = 1.f;
        player.setFacingRight(true);
    } else {
        dir.x = player.isFacingRight() ? 1.f : -1.f;
    }
    dir.y = 0.f;

    player.setDashDirection(dir);
    player.setVelocity({0.f, 0.f});
    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/player_dash.wav", 75.f);
}
//-------------------------------------------------------

//------------[Exit - Restore Normal Physics & Retain Momentum]-------------------
void PlayerDashState::exit(Player& player) {
    player.getHurtbox().invulnerable = false;
    player.setIsDashing(false);
}
//-------------------------------------------------------

//------------[Fixed Update - Step Air-Dash Freeze, Burst & CCD Sweep]-------------------
void PlayerDashState::fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    (void)map;

    float freeze = player.getDashFreezeTimer();
    if (freeze > 0.f) {
        player.setDashFreezeTimer(freeze - dt);
        player.setVelocity({0.f, 0.f});

        bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
        bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);

        if (left && !right) {
            player.setDashDirection({-1.f, 0.f});
            player.setFacingRight(false);
        } else if (right && !left) {
            player.setDashDirection({1.f, 0.f});
            player.setFacingRight(true);
        }
        return;
    }

    float timer = player.getDashTimer() - dt;
    player.setDashTimer(timer);

    sf::Vector2f burstVel = player.getDashDirection() * player.getDashSpeed();
    player.setVelocity(burstVel);
    player.setCurrentMaxSpeed(player.getDashSpeed() * 0.8f);

    if (auto* ps = player.getParticleSystem()) {
        ps->emitDashTrail(player.getPosition() + player.getBounds().size * 0.5f, true);
    }

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

    if (mIsGroundSmash) {
        player.getAnimator().play("Fall");
    } else {
        player.getAnimator().play("Jump");
    }

    Combat::Hitbox pounceBox;
    pounceBox.damage = mIsGroundSmash ? 60.f : 40.f;
    pounceBox.poiseDamage = mIsGroundSmash ? 70.f : 40.f;
    pounceBox.knockback = mIsGroundSmash ? sf::Vector2f(0.f, 200.f) : (player.isFacingRight() ? sf::Vector2f(300.f, -120.f) : sf::Vector2f(-300.f, -120.f));
    pounceBox.localBounds = Physics::AABB::fromPositionSize({-10.f, -5.f}, {player.getBounds().size.x + 20.f, player.getBounds().size.y + 10.f});
    pounceBox.active = true;
    player.setAttackHitbox(pounceBox);
    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/beast_pounce.wav", 85.f);
}
//-------------------------------------------------------

//------------[Exit - Restore Normal Physics & Retain Beast Momentum]-------------------
void PlayerPounceState::exit(Player& player) {
    player.deactivateAttackHitbox();
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

        if (auto* ps = player.getParticleSystem()) {
            ps->emitDashTrail(player.getPosition() + player.getBounds().size * 0.5f, false);
        }

        player.moveWithSweptCCD(vel * dt, physicsWorld, false);

        if (player.getIsGrounded()) {
            mSmashRecoveryTimer = 0.12f;
            player.setVelocity({0.f, 0.f});
            player.setCurrentMaxSpeed(player.getMoveSpeed());
            player.triggerGroundSmashImpact();
        }
    } else {
        // Pounce leap trajectory
        mDurationTimer -= dt;
        sf::Vector2f vel = player.getVelocity();
        vel.y += player.getGravity() * 0.75f * dt;
        player.setVelocity(vel);

        if (auto* ps = player.getParticleSystem()) {
            ps->emitDashTrail(player.getPosition() + player.getBounds().size * 0.5f, false);
        }

        player.moveWithSweptCCD(vel * dt, physicsWorld, false);

        if (player.getIsGrounded()) {
            player.changeState(PlayerStateType::Run);
        } else if (mDurationTimer <= 0.f) {
            player.changeState(PlayerStateType::Airborne);
        }
    }
}
//-------------------------------------------------------

//=============================================================================
// PlayerMeleeAttackState (Witch Form Fast Light Slash)
//=============================================================================

//------------[Enter - Initialize Witch Light Slash Attack]-------------------
void PlayerMeleeAttackState::enter(Player& player) {
    mAttackTimer = 0.f;
    mHitboxActivated = false;
    mHitboxDeactivated = false;
    mAttackBuffered = false;
    player.deactivateAttackHitbox();
    player.getAnimator().play("Attack", true);

    if (player.getComboWindowTimer() <= 0.f) {
        player.setComboStep(0);
    }
    mCurrentComboStep = player.getComboStep();

    sf::Vector2f vel = player.getVelocity();
    bool facingRight = player.isFacingRight();
    float forwardPush = 0.f;

    if (mCurrentComboStep == 0) {
        forwardPush = facingRight ? 45.f : -45.f;
        player.setComboStep(1);
        player.setComboWindowTimer(0.70f);
    } else if (mCurrentComboStep == 1) {
        forwardPush = facingRight ? 75.f : -75.f;
        player.setComboStep(2);
        player.setComboWindowTimer(0.70f);
    } else {
        forwardPush = facingRight ? 140.f : -140.f;
        player.setMeleeCooldownTimer(0.6f);
        player.setComboStep(0);
        player.setComboWindowTimer(0.f);
        player.setHasAirJump(true);
        player.setHasAirDash(true);
        player.setDashCooldownTimer(0.f);
    }

    if (auto* objMgr = player.getObjectManager()) {
        sf::Vector2f pCenter = player.getPosition() + sf::Vector2f(player.getBounds().size.x * 0.5f, player.getBounds().size.y * 0.5f);
        float bestDist = 160.f;
        bool foundTarget = false;

        for (const auto& enemy : objMgr->getEnemies()) {
            if (!enemy || enemy->isDead()) continue;
            sf::Vector2f eCenter = enemy->getPosition() + enemy->getSize() * 0.5f;
            float dx = eCenter.x - pCenter.x;
            float dy = std::abs(eCenter.y - pCenter.y);

            if (dy < 65.f) {
                if (facingRight && dx > 0.f && dx <= bestDist) {
                    foundTarget = true;
                    bestDist = dx;
                } else if (!facingRight && dx < 0.f && -dx <= bestDist) {
                    foundTarget = true;
                    bestDist = -dx;
                }
            }
        }

        if (foundTarget) {
            float targetPush = (mCurrentComboStep == 2) ? 350.f : ((mCurrentComboStep == 1) ? 280.f : 240.f);
            forwardPush = facingRight ? targetPush : -targetPush;
        }
    }

    if (!player.getIsGrounded()) {
        vel.y = 0.f;
    }
    vel.x = forwardPush;
    player.setVelocity(vel);
    if (auto* rb = player.getRigidBody()) {
        rb->setVelocity(vel);
    }
    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/player_slash.wav", 80.f);
}
//-------------------------------------------------------

//------------[Exit - Reset Attack Hitbox]-------------------
void PlayerMeleeAttackState::exit(Player& player) {
    player.deactivateAttackHitbox();
    mHitboxActivated = false;
    mHitboxDeactivated = false;
}
//-------------------------------------------------------

//------------[Handle Input - Process Attack Key Events]-------------------
void PlayerMeleeAttackState::handleInput(Player& player, const sf::Event& event) {
    (void)player;
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::J) {
            mAttackBuffered = true;
        }
    }
}
//-------------------------------------------------------

//------------[Fixed Update - Step Attack Windup, Active Hitbox Window & Recovery]-------------------
void PlayerMeleeAttackState::fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    (void)map;
    mAttackTimer += dt;

    sf::Vector2f vel = player.getVelocity();
    if (player.getIsGrounded()) {
        if (vel.x > 0.f) {
            vel.x -= player.getFriction() * 1.5f * dt;
            if (vel.x < 0.f) vel.x = 0.f;
        } else if (vel.x < 0.f) {
            vel.x += player.getFriction() * 1.5f * dt;
            if (vel.x > 0.f) vel.x = 0.f;
        }
    } else {
        vel.y = 0.f;
        if (vel.x > 0.f) {
            vel.x -= 400.f * dt;
            if (vel.x < 0.f) vel.x = 0.f;
        } else if (vel.x < 0.f) {
            vel.x += 400.f * dt;
            if (vel.x > 0.f) vel.x = 0.f;
        }
    }
    player.setVelocity(vel);
    if (auto* rb = player.getRigidBody()) {
        rb->setVelocity(vel);
    }
    player.moveWithSweptCCD(vel * dt, physicsWorld, false);

    std::size_t frame = player.getAnimator().getCurrentFrame();
    if (frame >= 1 && frame < 3 && !mHitboxActivated) {
        mHitboxActivated = true;
        Combat::Hitbox slash;
        slash.active = true;

        if (mCurrentComboStep == 0) {
            slash.damage = 25.f;
            slash.poiseDamage = 15.f;
            slash.knockback = player.isFacingRight() ? sf::Vector2f(200.f, -80.f) : sf::Vector2f(-200.f, -80.f);
            if (player.isFacingRight()) {
                slash.localBounds = Physics::AABB::fromPositionSize({20.f, -2.f}, {36.f, 38.f});
            } else {
                slash.localBounds = Physics::AABB::fromPositionSize({-26.f, -2.f}, {36.f, 38.f});
            }
        } else if (mCurrentComboStep == 1) {
            slash.damage = 30.f;
            slash.poiseDamage = 20.f;
            slash.knockback = player.isFacingRight() ? sf::Vector2f(260.f, -90.f) : sf::Vector2f(-260.f, -90.f);
            if (player.isFacingRight()) {
                slash.localBounds = Physics::AABB::fromPositionSize({20.f, -4.f}, {40.f, 40.f});
            } else {
                slash.localBounds = Physics::AABB::fromPositionSize({-30.f, -4.f}, {40.f, 40.f});
            }
        } else {
            slash.damage = 50.f;
            slash.poiseDamage = 40.f;
            slash.knockback = player.isFacingRight() ? sf::Vector2f(420.f, -140.f) : sf::Vector2f(-420.f, -140.f);
            if (player.isFacingRight()) {
                slash.localBounds = Physics::AABB::fromPositionSize({18.f, -6.f}, {48.f, 46.f});
            } else {
                slash.localBounds = Physics::AABB::fromPositionSize({-36.f, -6.f}, {48.f, 46.f});
            }
        }

        player.setAttackHitbox(slash);
    } else if (frame >= 3 && !mHitboxDeactivated) {
        mHitboxDeactivated = true;
        player.deactivateAttackHitbox();
    }

    if (mHitboxActivated && !mHitboxDeactivated) {
        if (auto* objMgr = player.getObjectManager()) {
            sf::Vector2f pPos = player.getPosition();
            Physics::AABB hitAABB = player.getAttackHitbox().getWorldAABB(pPos);
            hitAABB.min.y -= 12.f;
            hitAABB.max.y += 18.f;
            if (player.isFacingRight()) {
                hitAABB.max.x += 18.f;
            } else {
                hitAABB.min.x -= 18.f;
            }

            for (auto& proj : objMgr->getProjectiles()) {
                if (!proj || proj->isDead() || !proj->isPogoOrb() || proj->isPogoStruck()) continue;
                if (hitAABB.intersects(proj->getAABB())) {
                    float launchDir = player.isFacingRight() ? 960.f : -960.f;
                    proj->strikePogo(launchDir);
                    if (auto* ps = player.getParticleSystem()) {
                        ps->emitPogoBurst(proj->getPosition());
                    }

                    sf::Vector2f pogoVel = player.getVelocity();
                    pogoVel.y = -820.f;
                    player.setVelocity(pogoVel);
                    player.setIsGrounded(false);
                    player.setIsJumping(true);
                    if (auto* rb = player.getRigidBody()) {
                        rb->setVelocity(pogoVel);
                    }

                    player.setHasAirJump(true);
                    player.setHasAirDash(true);
                    player.setDashCooldownTimer(0.f);

                    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/pogo_hit.wav", 95.f);
                    player.changeState(PlayerStateType::Airborne);
                    return;
                }
            }
        }
    }

    if (mHitboxActivated && mAttackTimer >= 0.30f && player.canMeleeAttack() && player.getComboWindowTimer() > 0.f) {
        bool attackPressed = mAttackBuffered || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::J);
        if (attackPressed && player.consumeStamina(20.f)) {
            mAttackBuffered = false;
            player.changeState(PlayerStateType::MeleeAttack, true);
            return;
        }
    }

    if (player.getAnimator().isFinished() || mAttackTimer >= 0.38f) {
        if (player.getIsGrounded()) {
            player.changeState(PlayerStateType::Idle);
        } else {
            player.changeState(PlayerStateType::Airborne);
        }
    }
}
//-------------------------------------------------------


//=============================================================================
// PlayerHeavyStrikeState (Beast Form Heavy Poise-Breaking Claw Strike)
//=============================================================================

//------------[Enter - Initialize Beast Heavy Claw Strike]-------------------
void PlayerHeavyStrikeState::enter(Player& player) {
    mAttackTimer = 0.f;
    mHitboxActivated = false;
    mHitboxDeactivated = false;
    player.deactivateAttackHitbox();
    player.getAnimator().play("HeavyStrike", true);

    sf::Vector2f vel = player.getVelocity();
    vel.x = player.isFacingRight() ? 220.f : -220.f;
    player.setVelocity(vel);
    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/beast_smash.wav", 90.f);
}
//-------------------------------------------------------

//------------[Exit - Reset Heavy Strike Hitbox]-------------------
void PlayerHeavyStrikeState::exit(Player& player) {
    player.deactivateAttackHitbox();
    mHitboxActivated = false;
    mHitboxDeactivated = false;
}
//-------------------------------------------------------

//------------[Handle Input - Process Heavy Strike Key Events]-------------------
void PlayerHeavyStrikeState::handleInput(Player& player, const sf::Event& event) {
    (void)player;
    (void)event;
}
//-------------------------------------------------------

//------------[Fixed Update - Step Heavy Strike Poise-Breaking Arc & Rage Generation]-------------------
void PlayerHeavyStrikeState::fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    (void)map;
    mAttackTimer += dt;

    sf::Vector2f vel = player.getVelocity();
    if (player.getIsGrounded()) {
        if (vel.x > 0.f) {
            vel.x -= player.getFriction() * 1.2f * dt;
            if (vel.x < 0.f) vel.x = 0.f;
        } else if (vel.x < 0.f) {
            vel.x += player.getFriction() * 1.2f * dt;
            if (vel.x > 0.f) vel.x = 0.f;
        }
    } else {
        vel.y += player.getGravity() * dt;
    }
    player.setVelocity(vel);
    player.moveWithSweptCCD(vel * dt, physicsWorld, false);

    std::size_t frame = player.getAnimator().getCurrentFrame();
    if (frame >= 1 && frame < 3 && !mHitboxActivated) {
        mHitboxActivated = true;
        Combat::Hitbox claw;
        claw.damage = 50.f;
        claw.poiseDamage = 45.f;
        claw.knockback = player.isFacingRight() ? sf::Vector2f(350.f, -140.f) : sf::Vector2f(-350.f, -140.f);
        claw.active = true;

        if (player.isFacingRight()) {
            claw.localBounds = Physics::AABB::fromPositionSize({26.f, -6.f}, {48.f, 52.f});
        } else {
            claw.localBounds = Physics::AABB::fromPositionSize({-32.f, -6.f}, {48.f, 52.f});
        }
        player.setAttackHitbox(claw);
    } else if (frame >= 3 && !mHitboxDeactivated) {
        mHitboxDeactivated = true;
        player.deactivateAttackHitbox();
    }

    if (player.getAnimator().isFinished() || mAttackTimer >= 0.46f) {
        if (player.getIsGrounded()) {
            player.changeState(PlayerStateType::Idle);
        } else {
            player.changeState(PlayerStateType::Airborne);
        }
    }
}
//-------------------------------------------------------

//=============================================================================
// PlayerCastSpellState (Witch Form Ranged Magic)
//=============================================================================

//------------[Enter - Initialize Witch Spell Cast]-------------------
void PlayerCastSpellState::enter(Player& player) {
    mCastTimer = 0.f;
    mProjectileSpawned = false;
    player.deactivateAttackHitbox();
    player.getAnimator().play("CastSpell", true);

    sf::Vector2f vel = player.getVelocity();
    vel.x *= 0.5f;
    player.setVelocity(vel);
}
//-------------------------------------------------------

//------------[Exit - Reset Spell Casting Flags]-------------------
void PlayerCastSpellState::exit(Player& player) {
    mProjectileSpawned = false;
}
//-------------------------------------------------------

//------------[Handle Input - Process Spell Key Events]-------------------
void PlayerCastSpellState::handleInput(Player& player, const sf::Event& event) {
    (void)player;
    (void)event;
}
//-------------------------------------------------------

//------------[Fixed Update - Step Spell Cast Animation, Mana Consumption & Projectile Spawn]-------------------
void PlayerCastSpellState::fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    (void)map;
    mCastTimer += dt;

    bool left = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A);
    bool right = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
    if (!mProjectileSpawned) {
        if (left && !right) {
            player.setFacingRight(false);
        } else if (right && !left) {
            player.setFacingRight(true);
        }
    }

    sf::Vector2f vel = player.getVelocity();
    if (player.getIsGrounded()) {
        if (vel.x > 0.f) {
            vel.x -= player.getFriction() * 2.0f * dt;
            if (vel.x < 0.f) vel.x = 0.f;
        } else if (vel.x < 0.f) {
            vel.x += player.getFriction() * 2.0f * dt;
            if (vel.x > 0.f) vel.x = 0.f;
        }
    } else {
        vel.y += player.getGravity() * dt;
    }
    player.setVelocity(vel);
    player.moveWithSweptCCD(vel * dt, physicsWorld, false);

    std::size_t frame = player.getAnimator().getCurrentFrame();
    if (frame >= 2 && !mProjectileSpawned) {
        mProjectileSpawned = true;
        if (player.consumeMana(25.f)) {
            sf::Vector2f pPos = player.getPosition();
            sf::FloatRect bounds = player.getBounds();
            bool facingRight = player.isFacingRight();
            float spawnX = facingRight ? (pPos.x + bounds.size.x + 12.f) : (pPos.x - 12.f);
            float spawnY = pPos.y + bounds.size.y * 0.45f;
            float projSpeed = 650.f;
            sf::Vector2f projVel = {facingRight ? projSpeed : -projSpeed, 0.f};

            player.spawnProjectile({spawnX, spawnY}, projVel, 30.f, 25.f);
            Engine::Audio::AudioManager::getInstance().playSound("assets/audio/player_cast.wav", 85.f);
        }
    }

    if (player.getAnimator().isFinished() || mCastTimer >= 0.38f) {
        if (player.getIsGrounded()) {
            player.changeState(PlayerStateType::Idle);
        } else {
            player.changeState(PlayerStateType::Airborne);
        }
    }
}
//-------------------------------------------------------

//=============================================================================
// PlayerGunState (Witch Form Auto-Aim Gun with Recoil Impulse)
//=============================================================================

//------------[Enter - Initialize Witch Gun Shot & Auto-Aim Recoil]-------------------
void PlayerGunState::enter(Player& player) {
    mGunTimer = 0.f;
    mAirStaggerTimer = 0.07f;
    mRecoilApplied = false;
    player.deactivateAttackHitbox();
    player.getAnimator().play("CastSpell", true);

    player.setGunCooldownTimer(0.5f);
    if (!player.getIsGrounded()) {
        player.setHasAirJump(true);
        player.setHasAirDash(true);
        player.setDashCooldownTimer(0.f);
    }

    sf::Vector2f playerPos = player.getPosition();
    sf::FloatRect bounds = player.getBounds();
    sf::Vector2f center = {playerPos.x + bounds.size.x * 0.5f, playerPos.y + bounds.size.y * 0.45f};

    Enemy* targetEnemy = nullptr;
    const float maxRadius = 550.f;
    const float maxDistSq = maxRadius * maxRadius;

    if (auto* objMgr = player.getObjectManager()) {
        auto* pw = player.getPhysicsWorld();
        struct Candidate {
            Enemy* enemy;
            float distSq;
            sf::Vector2f pos;
        };
        std::vector<Candidate> candidates;

        for (const auto& enemy : objMgr->getEnemies()) {
            if (!enemy || enemy->isDead()) continue;
            sf::Vector2f ePos = enemy->getPosition() + enemy->getSize() * 0.5f;
            float dx = ePos.x - center.x;
            float dy = ePos.y - center.y;
            float distSq = dx * dx + dy * dy;
            if (distSq <= maxDistSq) {
                candidates.push_back({enemy.get(), distSq, ePos});
            }
        }

        std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
            return a.distSq < b.distSq;
        });

        for (const auto& cand : candidates) {
            if (pw) {
                Physics::SweptHit hit = pw->raycast(center, cand.pos, player.getRigidBody(), false);
                if (hit.hit && hit.body && hit.body->getType() == Physics::BodyType::Static &&
                    (hit.body->getTag() == Physics::ColliderTag::SolidWall || !hit.body->isOneWay()) &&
                    hit.toi < 0.99f) {
                    continue;
                }
            }
            targetEnemy = cand.enemy;
            break;
        }
    }

    sf::Vector2f aimDir = {player.isFacingRight() ? 1.f : -1.f, 0.f};
    sf::Vector2f rayStart = center;
    sf::Vector2f rayEnd = center + aimDir * maxRadius;
    bool rayBlocked = false;

    if (targetEnemy) {
        sf::Vector2f ePos = targetEnemy->getPosition() + targetEnemy->getSize() * 0.5f;
        sf::Vector2f diff = ePos - center;
        float len = std::sqrt(diff.x * diff.x + diff.y * diff.y);
        if (len > 0.001f) {
            aimDir = diff / len;
        }
        rayEnd = ePos;
    } else if (auto* pw = player.getPhysicsWorld()) {
        Physics::SweptHit hit = pw->raycast(center, rayEnd, player.getRigidBody(), false);
        if (hit.hit && hit.body && hit.body->getType() == Physics::BodyType::Static &&
            (hit.body->getTag() == Physics::ColliderTag::SolidWall || !hit.body->isOneWay()) &&
            hit.toi < 0.99f) {
            rayEnd = center + (rayEnd - center) * hit.toi;
            rayBlocked = true;
        }
    }
    player.setFacingRight(aimDir.x >= 0.f);
    player.consumeMana(15.f);

    sf::Vector2f spawnPos = center + aimDir * 24.f;
    sf::Vector2f projVel = aimDir * 1150.f;
    player.spawnGunProjectile(spawnPos, projVel, 40.f, 30.f);
    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/gun_shot.wav", 95.f);

    if (auto* ps = player.getParticleSystem()) {
        ps->emitMuzzleFlash(spawnPos, aimDir);
    }

    sf::Vector2f recoil = -aimDir * 460.f;
    recoil.y = std::min(recoil.y - 190.f, -280.f);
    mRecoil = recoil;

    player.setVelocity({0.f, 0.f});
    if (auto* rb = player.getRigidBody()) {
        rb->setVelocity({0.f, 0.f});
    }
}
//-------------------------------------------------------

//------------[Exit - Reset Gun State Properties]-------------------
void PlayerGunState::exit(Player& player) {
    (void)player;
}
//-------------------------------------------------------

//------------[Handle Input - Process Gun Key Events]-------------------
void PlayerGunState::handleInput(Player& player, const sf::Event& event) {
    (void)player;
    (void)event;
}
//-------------------------------------------------------

//------------[Fixed Update - Step Gun Recovery & Aerial Momentum]-------------------
void PlayerGunState::fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    (void)map;
    mGunTimer += dt;

    if (mAirStaggerTimer > 0.f) {
        mAirStaggerTimer -= dt;
        player.setVelocity({0.f, 0.f});
        if (auto* rb = player.getRigidBody()) {
            rb->setVelocity({0.f, 0.f});
        }
        if (mAirStaggerTimer <= 0.f && !mRecoilApplied) {
            mRecoilApplied = true;
            player.setVelocity(mRecoil);
            player.setIsGrounded(false);
            player.setIsJumping(true);
            if (auto* rb = player.getRigidBody()) {
                rb->setVelocity(mRecoil);
            }
        }
        return;
    }

    if (!mRecoilApplied) {
        mRecoilApplied = true;
        player.setVelocity(mRecoil);
        player.setIsGrounded(false);
        player.setIsJumping(true);
        if (auto* rb = player.getRigidBody()) {
            rb->setVelocity(mRecoil);
        }
    }

    sf::Vector2f vel = player.getVelocity();
    vel.y += player.getGravity() * dt;
    if (vel.y > 850.f) vel.y = 850.f;
    player.setVelocity(vel);

    player.moveWithSweptCCD(vel * dt, physicsWorld, false);

    if (mGunTimer >= 0.22f) {
        if (player.getIsGrounded()) {
            player.changeState(PlayerStateType::Idle);
        } else {
            player.changeState(PlayerStateType::Airborne);
        }
    }
}
//-------------------------------------------------------

//=============================================================================
// PlayerParryState
//=============================================================================

//------------[Enter - Initialize Parry Stance & Counter Window]-------------------
void PlayerParryState::enter(Player& player) {
    mParryTimer = 0.f;
    player.setParrying(true);
    player.deactivateAttackHitbox();
    player.getAnimator().play("Attack", true);
    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/parry_ready.wav", 80.f);
}
//-------------------------------------------------------

//------------[Exit - Reset Parry Stance]-------------------
void PlayerParryState::exit(Player& player) {
    player.setParrying(false);
}
//-------------------------------------------------------

//------------[Handle Input - Process Parry Key Events]-------------------
void PlayerParryState::handleInput(Player& player, const sf::Event& event) {
    (void)player;
    (void)event;
}
//-------------------------------------------------------

//------------[Fixed Update - Step Parry Duration & Friction]-------------------
void PlayerParryState::fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    (void)map;
    mParryTimer += dt;

    if (mParryTimer > mParryWindow) {
        player.setParrying(false);
    }

    sf::Vector2f vel = player.getVelocity();
    if (player.getIsGrounded()) {
        vel.x *= 0.82f;
        player.setVelocity(vel);
        player.moveWithSweptCCD(vel * dt, physicsWorld, true);
    } else {
        vel.y += player.getGravity() * dt;
        player.setVelocity(vel);
        player.moveWithSweptCCD(vel * dt, physicsWorld, false);
    }

    if (mParryTimer >= mTotalDuration) {
        if (player.getIsGrounded()) {
            player.changeState(PlayerStateType::Idle);
        } else {
            player.changeState(PlayerStateType::Airborne);
        }
    }
}
//-------------------------------------------------------

//=============================================================================
// PlayerRoarState
//=============================================================================

//------------[Enter - Initialize Beast Roar Stagger AoE]-------------------
void PlayerRoarState::enter(Player& player) {
    mRoarTimer = 0.f;
    mRoarTriggered = false;
    player.consumeRage(25.f);
    player.setSpellCooldownTimer(1.0f);
    player.deactivateAttackHitbox();
    player.getAnimator().play("HeavyStrike", true);

    sf::Vector2f vel = player.getVelocity();
    vel.x = 0.f;
    player.setVelocity(vel);
    if (auto* rb = player.getRigidBody()) {
        rb->setVelocity(vel);
    }

    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/beast_smash.wav", 100.f);
}
//-------------------------------------------------------

//------------[Exit - Reset Roar State]-------------------
void PlayerRoarState::exit(Player& player) {
    (void)player;
    mRoarTriggered = false;
}
//-------------------------------------------------------

//------------[Handle Input - Process Roar Key Events]-------------------
void PlayerRoarState::handleInput(Player& player, const sf::Event& event) {
    (void)player;
    (void)event;
}
//-------------------------------------------------------

//------------[Fixed Update - Step Roar Duration, Poise Break Sweep & Shockwave]-------------------
void PlayerRoarState::fixedUpdate(Player& player, float dt, const Map& map, const Physics::PhysicsWorld& physicsWorld) {
    (void)map;
    mRoarTimer += dt;

    if (!mRoarTriggered && mRoarTimer >= 0.08f) {
        mRoarTriggered = true;
        sf::Vector2f pCenter = player.getPosition() + sf::Vector2f(player.getBounds().size.x * 0.5f, player.getBounds().size.y * 0.5f);
        const float roarRadius = 240.f;

        if (auto* ps = player.getParticleSystem()) {
            ps->emitRoarShockwave(pCenter);
        }

        if (auto* objMgr = player.getObjectManager()) {
            for (auto& enemy : objMgr->getEnemies()) {
                if (!enemy || enemy->isDead()) continue;
                sf::Vector2f eCenter = enemy->getPosition() + enemy->getSize() * 0.5f;
                float dx = eCenter.x - pCenter.x;
                float dy = eCenter.y - pCenter.y;
                if ((dx * dx + dy * dy) <= roarRadius * roarRadius) {
                    enemy->setStaggerDuration(3.5f);
                    enemy->setStaggerTimer(0.f);
                    enemy->resetPosture();
                    enemy->changeState(EnemyStateType::Staggered);
                    enemy->takeDamage(15.f, 100.f, {dx >= 0.f ? 220.f : -220.f, -100.f});
                }
            }
        }

        Engine::Audio::AudioManager::getInstance().playSound("assets/audio/posture_break.wav", 95.f);
    }

    sf::Vector2f vel = player.getVelocity();
    if (player.getIsGrounded()) {
        vel.x *= 0.85f;
        player.setVelocity(vel);
        player.moveWithSweptCCD(vel * dt, physicsWorld, true);
    } else {
        vel.y += player.getGravity() * dt;
        player.setVelocity(vel);
        player.moveWithSweptCCD(vel * dt, physicsWorld, false);
    }

    if (mRoarTimer >= 0.45f) {
        if (player.getIsGrounded()) {
            player.changeState(PlayerStateType::Idle);
        } else {
            player.changeState(PlayerStateType::Airborne);
        }
    }
}
//-------------------------------------------------------



