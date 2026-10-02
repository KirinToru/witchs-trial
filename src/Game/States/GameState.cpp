#include <Game/States/GameState.hpp>
#include <Game/States/PauseState.hpp>
#include <Game/States/EquipmentState.hpp>
#include <Game/Game.hpp>
#include <Engine/Audio/AudioManager.hpp>
#include <Engine/Graphics/FontManager.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <sstream>

//------------[Constructor - Initialize Game World & Level Assets]-------------------
GameState::GameState(Game *game)
    : State(game),
      mCamera({0.f, 0.f}, {1280.f, 720.f}),
      mBackgroundSprite(mBackgroundTexture),
      mHUD() {

  if (!mBackgroundTexture.loadFromFile("assets/backgrounds/bg.png")) {
    std::cerr << "Failed to load bg.png" << std::endl;
  }
  mBackgroundTexture.setRepeated(true);
  mBackgroundSprite.setTexture(mBackgroundTexture);

  mEndFontLoaded = true;

  mPhysicsWorld.setGravity({0.f, 980.f});

  loadLevel("assets/maps/test.tmx");

  Engine::Audio::AudioManager::getInstance().playBGM("assets/audio/bgm_exploration.ogg", true, 50.f);
}
//-------------------------------------------------------

//------------[Load Level - Load TMX Map and Center Camera]-------------------
void GameState::loadLevel(const std::string &filename) {
  mPhysicsWorld.clear();
  mObjectManager.clear();
  mParticleSystem.clear();
  mHitEnemiesThisSwing.clear();
  mArenaLocked = false;
  mArenaLeftWall = nullptr;
  mArenaRightWall = nullptr;
  mBoss = nullptr;
  mHUD.setBossInfo(false);

  if (mMap.loadFromFile(filename, &mPhysicsWorld)) {
    mPlayer.initPhysics(mPhysicsWorld);
    mPlayer.setObjectManager(&mObjectManager);
    mPlayer.setParticleSystem(&mParticleSystem);
    mPlayer.reset(mMap.getStartPosition());
    sf::Vector2f playerPos = mPlayer.getPosition();
    sf::Vector2f viewSize = mCamera.getSize();
    float mapW = mMap.getWidth();
    float mapH = mMap.getHeight();

    for (const auto &pos : mMap.getEnemySpawns()) {
      mObjectManager.spawnInquisitor(pos);
    }

    for (const auto &pos : mMap.getPendulumTrapSpawns()) {
      mObjectManager.spawnPendulumTrap(pos);
    }

    if (mMap.hasBossSpawn()) {
      mBoss = mObjectManager.spawnBoss(mMap.getBossSpawn());
    }

    if (mMap.hasArenaTrigger()) {
      sf::Vector2f triggerPos = mMap.getArenaTriggerPosition();
      sf::Vector2f arenaCenter = {750.f, 440.f};
      Physics::AABB triggerVolume = Physics::AABB::fromPositionSize(
          {triggerPos.x - 16.f, triggerPos.y - 200.f}, {96.f, 300.f});
      mArenaTrigger = ArenaTrigger(triggerVolume, arenaCenter, viewSize);
    }

    float camX = (mapW < viewSize.x)
                     ? mapW / 2.f
                     : std::clamp(playerPos.x, viewSize.x / 2.f, mapW - viewSize.x / 2.f);
    float camY = (mapH < viewSize.y)
                     ? mapH / 2.f
                     : std::clamp(playerPos.y, viewSize.y / 2.f, mapH - viewSize.y / 2.f);
    mCameraBaseCenter = {camX, camY};
    mCamera.setCenter(mCameraBaseCenter);
  } else {
    std::cerr << "Failed to load level: " << filename << std::endl;
  }
}
//-------------------------------------------------------

//------------[Handle Input - Process Keyboard Navigation & Hotkeys]-------------------
void GameState::handleInput(sf::Event &event) {
  if (const auto *keyPress = event.getIf<sf::Event::KeyPressed>()) {
    if (keyPress->code == sf::Keyboard::Key::Escape) {
      mGame->pushState(std::make_unique<PauseState>(mGame));
    }
    if (keyPress->code == sf::Keyboard::Key::F1) {
      mHUD.toggleHitbox();
    }
    if (keyPress->code == sf::Keyboard::Key::F2) {
      mHUD.toggleInfo();
    }
    if (keyPress->code == sf::Keyboard::Key::F3) {
      mHUD.toggleHitbox();
      mHUD.toggleDebugRaycast();
    }
    if (keyPress->code == sf::Keyboard::Key::I) {
      mGame->pushState(std::make_unique<EquipmentState>(mGame, &mPlayer));
      return;
    }
    if (mGameEndState != GameEndState::None) {
      if (keyPress->code == sf::Keyboard::Key::Enter) {
        restartGame();
        return;
      }
    }
  }

  if (mGameEndState == GameEndState::GameOver) {
    return;
  }

  mPlayer.handleInput(event);
}
//-------------------------------------------------------

//------------[Fixed Update - Step Custom Physics & Deterministic Movement (60Hz)]-------------------
void GameState::fixedUpdate(sf::Time dt) {
  float dtSec = dt.asSeconds();

  if (mPlayer.isDead() && mGameEndState == GameEndState::None) {
    triggerGameOver();
  }

  if (mGameEndState == GameEndState::GameOver) {
    mEndStateTimer += dtSec;
    mPhysicsWorld.update(dtSec * 0.25f);
    mPlayer.fixedUpdate(dtSec * 0.25f, mMap, mPhysicsWorld);
    return;
  }

  if (mGameEndState == GameEndState::Victory) {
    mEndStateTimer += dtSec;
  }

  if (mHitStopTimer > 0.f) {
    mHitStopTimer = std::max(0.f, mHitStopTimer - dtSec);
    return;
  }

  mPhysicsWorld.update(dtSec);
  mPlayer.fixedUpdate(dtSec, mMap, mPhysicsWorld);
  mObjectManager.updateEnemies(dtSec, mPlayer, mPhysicsWorld);
  mObjectManager.updateProjectiles(dtSec, mPhysicsWorld, &mParticleSystem, &mPlayer);
  mObjectManager.updateIceWalls(dtSec, &mParticleSystem);
  mObjectManager.updatePendulumTraps(dtSec);
  resolveCombatCollisions();
  mObjectManager.cleanupDestroyed();

  if (mPlayer.consumeGroundSmashImpact()) {
    triggerCameraShake(10.f, 0.4f);
    Engine::Audio::AudioManager::getInstance().playSound("assets/audio/beast_smash.wav", 95.f);
    Physics::AABB smashAABB = Physics::AABB::fromCenterHalfExtents(
        mPlayer.getPosition() + sf::Vector2f(0.f, 20.f),
        sf::Vector2f(100.f, 50.f)
    );
    for (auto& prop : mObjectManager.getProps()) {
      if (!prop || prop->isDestroyed() || !prop->isDestructible()) continue;
      if (smashAABB.intersects(prop->getAABB())) {
        bool destroyed = prop->takeDamage(200.f);
        if (destroyed) {
          mParticleSystem.emitDebrisBurst(prop->getPosition() + prop->getSize() * 0.5f);
        }
      }
    }
  }

  if (!mArenaTrigger.isCompleted() && !mArenaLocked) {
    if (mArenaTrigger.check(mPlayer.getAABB())) {
      engageArenaLock();
    }
  }

  for (const auto &sp : mMap.getSavePoints()) {
    Physics::AABB spAABB = Physics::AABB::fromPositionSize(sp, {32.f, 32.f});
    if (mPlayer.getAABB().intersects(spAABB)) {
      if (mPlayer.getMana() < mPlayer.getMaxMana()) {
        mPlayer.restoreMana(mPlayer.getMaxMana());
        Engine::Audio::AudioManager::getInstance().playSound("assets/audio/save_point.wav", 80.f);
      }
    }
  }

  // Update Boss State and Live Telemetry in Arena
  if (mArenaLocked && mBoss) {
    if (mBoss->consumePhaseTransitionShake()) {
      triggerCameraShake(12.f, 0.7f);
      triggerHitStop(0.12f);
    }

    if (mBoss->isDead()) {
      releaseArenaLock();
      if (mGameEndState == GameEndState::None) {
        triggerVictory();
      }
    } else {
      mHUD.setBossInfo(true, mBoss->getBossName(), mBoss->getHealth(), mBoss->getMaxHealth(),
                       mBoss->getPosture(), mBoss->getMaxPosture(), mBoss->getPhaseNumber());
    }
  }
}
//-------------------------------------------------------

//------------[Engage Arena Lock - Restrict Camera and Spawn Boundary Colliders]-------------------
void GameState::engageArenaLock() {
  if (mArenaLocked) return;
  mArenaLocked = true;

  sf::Vector2f center = mArenaTrigger.getArenaCenter();
  sf::Vector2f viewSize = mCamera.getSize();
  float halfW = viewSize.x * 0.5f;

  // 1. Invisible static left boundary wall
  Physics::RigidBodyDef leftDef;
  leftDef.type = Physics::BodyType::Static;
  leftDef.tag = Physics::ColliderTag::SolidWall;
  leftDef.isOneWay = false;
  leftDef.position = {center.x - halfW - 24.f, center.y - 500.f};
  leftDef.localAABB = Physics::AABB::fromPositionSize({0.f, 0.f}, {24.f, 1000.f});
  mArenaLeftWall = mPhysicsWorld.createBody(leftDef);

  // 2. Invisible static right boundary wall
  Physics::RigidBodyDef rightDef;
  rightDef.type = Physics::BodyType::Static;
  rightDef.tag = Physics::ColliderTag::SolidWall;
  rightDef.isOneWay = false;
  rightDef.position = {center.x + halfW, center.y - 500.f};
  rightDef.localAABB = Physics::AABB::fromPositionSize({0.f, 0.f}, {24.f, 1000.f});
  mArenaRightWall = mPhysicsWorld.createBody(rightDef);

  if (mBoss) {
    mBoss->engage();
  }

  Engine::Audio::AudioManager::getInstance().playBGM("assets/audio/bgm_boss.ogg", true, 65.f);
  Engine::Audio::AudioManager::getInstance().playSound("assets/audio/arena_lock.wav", 85.f);

  triggerCameraShake(8.f, 0.45f);
  triggerHitStop(0.08f);
}
//-------------------------------------------------------

//------------[Release Arena Lock - Remove Boundary Colliders and Unlock Camera]-------------------
void GameState::releaseArenaLock() {
  if (!mArenaLocked) return;
  mArenaLocked = false;

  if (mArenaLeftWall) {
    mPhysicsWorld.removeBody(mArenaLeftWall);
    mArenaLeftWall = nullptr;
  }
  if (mArenaRightWall) {
    mPhysicsWorld.removeBody(mArenaRightWall);
    mArenaRightWall = nullptr;
  }

  mArenaTrigger.setCompleted(true);
  mHUD.setBossInfo(false);

  Engine::Audio::AudioManager::getInstance().playSound("assets/audio/boss_defeat.wav", 100.f);

  triggerCameraShake(14.f, 0.8f);
  triggerHitStop(0.2f);
}
//-------------------------------------------------------

//------------[Resolve Combat Collisions - Process Player & Enemy Hitbox Overlaps]-------------------
void GameState::resolveCombatCollisions() {
  const auto& playerHitbox = mPlayer.getAttackHitbox();
  sf::Vector2f playerPos = mPlayer.getPosition();
  auto& enemies = mObjectManager.getEnemies();

  if (mPlayer.getIsDashing()) {
    Physics::AABB playerBox = mPlayer.getAABB();
    for (auto& enemy : enemies) {
      if (!enemy || enemy->isDead()) continue;
      if (enemy->getHurtbox().active && playerBox.intersects(enemy->getHurtbox().getWorldAABB(enemy->getPosition()))) {
        bool broken = enemy->takeDamage(35.f, 25.f, mPlayer.getDashDirection() * 320.f);
        sf::Vector2f hitPos = (playerPos + enemy->getPosition() + enemy->getSize() * 0.5f) * 0.5f;
        sf::Vector2f hitNormal = -mPlayer.getDashDirection();
        mParticleSystem.emitBloodSplatter(hitPos, hitNormal);
        spawnDamagePopup(enemy->getPosition() + enemy->getSize() * 0.5f, 35.f, false);
        triggerHitStop(0.08f);
        triggerCameraShake(6.f, 0.25f);
        Engine::Audio::AudioManager::getInstance().playSound("assets/audio/enemy_hit.wav", 90.f);
        if (broken) {
          Engine::Audio::AudioManager::getInstance().playSound("assets/audio/posture_break.wav", 100.f);
        }

        sf::Vector2f dashDir = mPlayer.getDashDirection();
        float bounceX = (dashDir.x >= 0.f) ? -380.f : 380.f;
        float bounceY = -700.f;
        sf::Vector2f bounceVel{bounceX, bounceY};
        mPlayer.changeState(PlayerStateType::Airborne);
        mPlayer.setIsDashing(false);
        mPlayer.setVelocity(bounceVel);
        if (auto* rb = mPlayer.getRigidBody()) {
          rb->setVelocity(bounceVel);
        }
        mPlayer.setHasAirJump(true);
        mPlayer.setHasAirDash(true);
        mPlayer.setDashCooldownTimer(0.f);
        mPlayer.triggerExternalImpulse(0.4f);
        if (mPlayer.getForm() == PlayerForm::Beast) {
          mPlayer.addRage(15.f);
        } else {
          mPlayer.deductStamina(25.f);
          if (mPlayer.getMana() >= 10.f) {
            mPlayer.consumeMana(10.f);
          }
        }
        break;
      }
    }
  }

  if (!playerHitbox.active) {
    mHitEnemiesThisSwing.clear();
  } else {
    for (auto& enemy : enemies) {
      if (!enemy || enemy->isDead()) continue;

      if (std::find(mHitEnemiesThisSwing.begin(), mHitEnemiesThisSwing.end(), enemy.get()) != mHitEnemiesThisSwing.end()) {
        continue;
      }

      if (Combat::checkOverlap(playerHitbox, playerPos, enemy->getHurtbox(), enemy->getPosition())) {
        bool postureBroken = enemy->takeDamage(playerHitbox.damage, playerHitbox.poiseDamage, playerHitbox.knockback);
        mHitEnemiesThisSwing.push_back(enemy.get());
        spawnDamagePopup(enemy->getPosition() + enemy->getSize() * 0.5f, playerHitbox.damage, (mPlayer.getComboStep() == 3 || mPlayer.getForm() == PlayerForm::Beast));

        if (mPlayer.getForm() == PlayerForm::Beast) {
          mPlayer.addRage(25.f);
        }

        if (mPlayer.getStateType() == PlayerStateType::MeleeAttack) {
          sf::Vector2f vel = mPlayer.getVelocity();
          vel.x = 0.f;
          mPlayer.setVelocity(vel);
          if (auto* rb = mPlayer.getRigidBody()) {
            rb->setVelocity(vel);
          }
          if (mPlayer.getMeleeCooldownTimer() > 0.f) {
            mPlayer.setHasAirJump(true);
            mPlayer.setHasAirDash(true);
            mPlayer.setDashCooldownTimer(0.f);
          }
        }

        sf::Vector2f hitNormal = {mPlayer.isFacingRight() ? 1.f : -1.f, -0.2f};
        sf::Vector2f hitPos = (playerPos + enemy->getPosition() + enemy->getSize() * 0.5f) * 0.5f;
        mParticleSystem.emitBloodSplatter(hitPos, hitNormal);

        if (postureBroken) {
          triggerHitStop(0.10f);
          triggerCameraShake(10.f, 0.4f);
          Engine::Audio::AudioManager::getInstance().playSound("assets/audio/posture_break.wav", 100.f);
        } else if (mPlayer.getForm() == PlayerForm::Beast) {
          triggerHitStop(0.08f);
          triggerCameraShake(6.f, 0.25f);
          Engine::Audio::AudioManager::getInstance().playSound("assets/audio/enemy_hit.wav", 85.f);
        } else {
          triggerHitStop(0.03f);
          triggerCameraShake(3.f, 0.15f);
          Engine::Audio::AudioManager::getInstance().playSound("assets/audio/enemy_hit.wav", 80.f);
        }
      }
    }

    Physics::AABB playerHitAABB = playerHitbox.getWorldAABB(playerPos);
    for (auto& prop : mObjectManager.getProps()) {
      if (!prop || prop->isDestroyed() || !prop->isDestructible()) continue;
      if (playerHitAABB.intersects(prop->getAABB())) {
        float dmg = (mPlayer.getForm() == PlayerForm::Beast) ? 120.f : 25.f;
        bool destroyed = prop->takeDamage(dmg);
        if (destroyed) {
          mParticleSystem.emitDebrisBurst(prop->getPosition() + prop->getSize() * 0.5f);
          triggerHitStop(mPlayer.getForm() == PlayerForm::Beast ? 0.08f : 0.03f);
          triggerCameraShake(mPlayer.getForm() == PlayerForm::Beast ? 6.f : 2.f, 0.2f);
          Engine::Audio::AudioManager::getInstance().playSound("assets/audio/prop_destroy.wav", 80.f);
        } else {
          Engine::Audio::AudioManager::getInstance().playSound("assets/audio/prop_hit.wav", 60.f);
        }
      }
    }

    for (auto& wall : mObjectManager.getIceWalls()) {
      if (!wall || wall->isDead()) continue;
      if (playerHitAABB.intersects(wall->getAABB())) {
        wall->destroy();
        mParticleSystem.emitDebrisBurst(wall->getPosition());
        Engine::Audio::AudioManager::getInstance().playSound("assets/audio/ice_break.wav", 90.f);
      }
    }

    for (auto& proj : mObjectManager.getProjectiles()) {
      if (!proj || proj->isDead() || !proj->isPogoOrb() || proj->isPogoStruck()) continue;
      if (playerHitAABB.intersects(proj->getAABB())) {
        float launchDir = mPlayer.isFacingRight() ? 920.f : -920.f;
        proj->strikePogo(launchDir);
        mParticleSystem.emitPogoBurst(proj->getPosition());

        sf::Vector2f vel = mPlayer.getVelocity();
        vel.y = -820.f;
        mPlayer.setVelocity(vel);
        mPlayer.setIsGrounded(false);
        mPlayer.setIsJumping(true);
        if (auto* rb = mPlayer.getRigidBody()) {
          rb->setVelocity(vel);
        }

        mPlayer.setHasAirJump(true);
        mPlayer.setHasAirDash(true);
        mPlayer.setDashCooldownTimer(0.f);
        mPlayer.triggerExternalImpulse(0.4f);

        triggerHitStop(0.06f);
        triggerCameraShake(4.f, 0.2f);
        Engine::Audio::AudioManager::getInstance().playSound("assets/audio/pogo_hit.wav", 95.f);
        break;
      }
    }
  }

  auto& projectiles = mObjectManager.getProjectiles();
  for (auto& proj : projectiles) {
    if (!proj || proj->isDead()) continue;
    const auto& projHitbox = proj->getHitbox();
    sf::Vector2f projPos = proj->getPosition();

    bool hitTarget = false;
    for (auto& enemy : enemies) {
      if (!enemy || enemy->isDead()) continue;
      if (Combat::checkOverlap(projHitbox, projPos, enemy->getHurtbox(), enemy->getPosition())) {
        bool postureBroken = enemy->takeDamage(proj->getDamage(), proj->getPoiseDamage(), proj->getKnockback());
        sf::Vector2f hitNormal = {proj->getVelocity().x >= 0.f ? 1.f : -1.f, -0.2f};
        mParticleSystem.emitBloodSplatter(projPos, hitNormal);
        spawnDamagePopup(enemy->getPosition() + enemy->getSize() * 0.5f, proj->getDamage(), proj->isPogoStruck());
        proj->destroy();
        hitTarget = true;

        if (proj->getType() == ProjectileType::Thunder) {
          enemy->setElectrified(3.5f);
          enemy->changeState(EnemyStateType::Staggered);
          enemy->setStaggerTimer(0.f);
          Engine::Audio::AudioManager::getInstance().playSound("assets/audio/thunder_hit.wav", 95.f);
        }

        if (postureBroken) {
          triggerHitStop(0.10f);
          triggerCameraShake(8.f, 0.35f);
          Engine::Audio::AudioManager::getInstance().playSound("assets/audio/posture_break.wav", 100.f);
        } else {
          triggerHitStop(0.04f);
          triggerCameraShake(3.f, 0.15f);
          Engine::Audio::AudioManager::getInstance().playSound("assets/audio/magic_impact.wav", 80.f);
        }
        break;
      }
    }

    if (hitTarget) continue;

    for (auto& wall : mObjectManager.getIceWalls()) {
      if (!wall || wall->isDead()) continue;
      if (proj->getAABB().intersects(wall->getAABB())) {
        wall->destroy();
        proj->destroy();
        mParticleSystem.emitDebrisBurst(wall->getPosition());
        Engine::Audio::AudioManager::getInstance().playSound("assets/audio/ice_break.wav", 90.f);
        hitTarget = true;
        break;
      }
    }

    if (hitTarget) continue;

    for (auto& prop : mObjectManager.getProps()) {
      if (!prop || prop->isDestroyed() || !prop->isDestructible()) continue;
      if (proj->getAABB().intersects(prop->getAABB())) {
        bool destroyed = prop->takeDamage(proj->getDamage());
        proj->destroy();
        if (destroyed) {
          mParticleSystem.emitDebrisBurst(prop->getPosition() + prop->getSize() * 0.5f);
          triggerHitStop(0.04f);
          triggerCameraShake(3.f, 0.15f);
          Engine::Audio::AudioManager::getInstance().playSound("assets/audio/prop_destroy.wav", 80.f);
        }
        break;
      }
    }
  }

  Physics::AABB playerBodyAABB = mPlayer.getAABB();
  for (auto& enemy : enemies) {
    if (!enemy || enemy->isDead() || !enemy->isElectrified()) continue;
    if (playerBodyAABB.intersects(enemy->getAABB())) {
      enemy->setElectrified(0.f);
      mPlayer.takeDamage(5.f, {0.f, 0.f});
      sf::Vector2f shockVel{mPlayer.getVelocity().x, -850.f};
      mPlayer.setVelocity(shockVel);
      if (auto* rb = mPlayer.getRigidBody()) {
        rb->setVelocity(shockVel);
      }
      mPlayer.setIsGrounded(false);
      mPlayer.setIsJumping(true);
      mPlayer.changeState(PlayerStateType::Airborne);
      mPlayer.setHasAirJump(true);
      mPlayer.setHasAirDash(true);
      mPlayer.setDashCooldownTimer(0.f);

      mParticleSystem.emitPogoBurst(playerPos + mPlayer.getBounds().size * 0.5f);
      triggerHitStop(0.06f);
      triggerCameraShake(6.f, 0.25f);
      Engine::Audio::AudioManager::getInstance().playSound("assets/audio/shock_jump.wav", 95.f);
      break;
    }
  }

  const auto& playerHurtbox = mPlayer.getHurtbox();
  for (auto& enemy : enemies) {
    if (!enemy || enemy->isDead()) continue;

    const auto& enemyHitbox = enemy->getAttackHitbox();
    if (enemyHitbox.active) {
      Physics::AABB enemyHitAABB = enemyHitbox.getWorldAABB(enemy->getPosition());
      for (auto& wall : mObjectManager.getIceWalls()) {
        if (!wall || wall->isDead()) continue;
        if (enemyHitAABB.intersects(wall->getAABB())) {
          wall->destroy();
          mParticleSystem.emitDebrisBurst(wall->getPosition());
          Engine::Audio::AudioManager::getInstance().playSound("assets/audio/ice_break.wav", 90.f);
        }
      }

      if (Combat::checkOverlap(enemyHitbox, enemy->getPosition(), playerHurtbox, playerPos)) {
        if (mPlayer.isParrying()) {
          enemy->takeDamage(30.f, 100.f, {mPlayer.isFacingRight() ? 280.f : -280.f, -140.f});
          enemy->setStaggerDuration(2.0f);
          enemy->setStaggerTimer(0.f);
          enemy->resetPosture();
          enemy->changeState(EnemyStateType::Staggered);
          mPlayer.addRage(40.f);
          sf::Vector2f hitPos = (playerPos + enemy->getPosition() + enemy->getSize() * 0.5f) * 0.5f;
          mParticleSystem.emitParrySparks(hitPos);
          spawnDamagePopup(enemy->getPosition() + enemy->getSize() * 0.5f, 30.f, true);
          triggerHitStop(0.12f);
          triggerCameraShake(8.f, 0.3f);
          Engine::Audio::AudioManager::getInstance().playSound("assets/audio/posture_break.wav", 100.f);
        } else {
          bool threatOnRight = enemy->getPosition().x >= playerPos.x;
          mPlayer.setFacingRight(threatOnRight);
          sf::Vector2f backwardKnockback = threatOnRight ? sf::Vector2f(-340.f, -440.f) : sf::Vector2f(340.f, -440.f);
          if (mPlayer.takeDamage(enemyHitbox.damage, backwardKnockback)) {
            sf::Vector2f hitNormal = {threatOnRight ? 1.f : -1.f, -0.3f};
            mParticleSystem.emitBloodSplatter(playerPos + mPlayer.getBounds().size * 0.5f, hitNormal);
            triggerHitStop(0.10f);
            triggerCameraShake(5.f, 0.25f);
          }
        }
      }
    }
  }

  for (const auto& trap : mObjectManager.getPendulumTraps()) {
    if (!trap) continue;
    if (playerHurtbox.active && !playerHurtbox.invulnerable && mPlayer.getAABB().intersects(trap->getAABB())) {
      sf::Vector2f bobVel = trap->getBobVelocity();
      float speed = std::sqrt(bobVel.x * bobVel.x + bobVel.y * bobVel.y);
      sf::Vector2f knockback = (speed > 20.f)
          ? (bobVel / speed * 380.f + sf::Vector2f(0.f, -180.f))
          : sf::Vector2f(mPlayer.isFacingRight() ? -340.f : 340.f, -280.f);

      if (mPlayer.takeDamage(trap->getDamage(), knockback)) {
        sf::Vector2f hitNormal = {knockback.x >= 0.f ? 1.f : -1.f, -0.3f};
        mParticleSystem.emitBloodSplatter(trap->getBobPosition(), hitNormal);
        triggerHitStop(0.08f);
        triggerCameraShake(6.f, 0.25f);
        Engine::Audio::AudioManager::getInstance().playSound("assets/audio/player_hurt.wav", 85.f);
      }
    }
  }
}
//-------------------------------------------------------

//------------[Trigger Hit Stop - Pause Simulation Updates for Impact Freeze]-------------------
void GameState::triggerHitStop(float durationSeconds) {
  mHitStopTimer = std::max(mHitStopTimer, durationSeconds);
}
//-------------------------------------------------------

//------------[Trigger Camera Shake - Apply Screen Shake Trauma]-------------------
void GameState::triggerCameraShake(float intensity, float durationSeconds) {
  float currentRemaining = (mShakeDuration > 0.f) ? (mShakeIntensity * (mShakeTimer / mShakeDuration)) : 0.f;
  if (intensity >= currentRemaining) {
    mShakeIntensity = intensity;
    mShakeDuration = durationSeconds;
    mShakeTimer = durationSeconds;
  }
}
//-------------------------------------------------------

//------------[Update - Step Camera Tracking & Telemetry]-------------------
void GameState::update(sf::Time dt) {
  float dtSec = dt.asSeconds();

  bool rHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R);
  if (rHeld) {
    mHoldRTimer += dtSec;
    mResetFadeAlpha = std::min(255.f, mHoldRTimer * 255.f);
    if (mResetFadeAlpha >= 255.f) {
      mResetRepeatTimer += dtSec;
      if (mResetRepeatTimer >= 0.25f || (mHoldRTimer - dtSec < 1.0f)) {
        restartGame();
        mResetRepeatTimer = 0.f;
      }
    }
  } else {
    mHoldRTimer = 0.f;
    mResetRepeatTimer = 0.f;
    if (mResetFadeAlpha > 0.f) {
      mResetFadeAlpha = std::max(0.f, mResetFadeAlpha - dtSec * 500.f);
    }
  }

  mParticleSystem.update(dtSec);

  for (auto& popup : mDamagePopups) {
    popup.timer -= dtSec;
    popup.position.y -= 40.f * dtSec;
  }
  std::erase_if(mDamagePopups, [](const DamagePopup& p) { return p.timer <= 0.f; });

  sf::Vector2f vel = mPlayer.getVelocity();
  mHUD.setPlayerSpeed(std::abs(vel.x));
  mHUD.setEntityCount(static_cast<int>(mPhysicsWorld.getBodies().size() + mObjectManager.getEntityCount()));
  mHUD.setPlayerForm(mPlayer.getForm() == PlayerForm::Witch ? "Witch" : "Beast");
  mHUD.setPlayerState(mPlayer.getStateName());
  mHUD.setComboStep(mPlayer.getComboStep());
  mHUD.setHealth(mPlayer.getHealth(), mPlayer.getMaxHealth());
  mHUD.setStamina(mPlayer.getStamina(), mPlayer.getMaxStamina());
  mHUD.setMana(mPlayer.getMana(), mPlayer.getMaxMana());
  mHUD.setRage(mPlayer.getRage(), mPlayer.getMaxRage());
  mHUD.update(dt);

  sf::Vector2f playerPos = mPlayer.getPosition();
  sf::Vector2f viewSize = mCamera.getSize();
  float mapW = mMap.getWidth();
  float mapH = mMap.getHeight();

  float targetX = 0.f;
  float targetY = 0.f;

  if (mArenaLocked) {
    targetX = mArenaTrigger.getArenaCenter().x;
    targetY = mArenaTrigger.getArenaCenter().y;
  } else {
    targetX = (mapW < viewSize.x)
                  ? mapW / 2.f
                  : std::clamp(playerPos.x, viewSize.x / 2.f, mapW - viewSize.x / 2.f);
    targetY = (mapH < viewSize.y)
                  ? mapH / 2.f
                  : std::clamp(playerPos.y, viewSize.y / 2.f, mapH - viewSize.y / 2.f);
  }

  float lerpSpeed = 5.0f;
  mCameraBaseCenter.x += (targetX - mCameraBaseCenter.x) * lerpSpeed * dtSec;
  mCameraBaseCenter.y += (targetY - mCameraBaseCenter.y) * lerpSpeed * dtSec;

  sf::Vector2f shakeOffset{0.f, 0.f};
  if (mShakeTimer > 0.f) {
    mShakeTimer = std::max(0.f, mShakeTimer - dtSec);
    float progress = (mShakeDuration > 0.f) ? (mShakeTimer / mShakeDuration) : 0.f;
    float currentIntensity = mShakeIntensity * (progress * progress);

    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(-1.f, 1.f);
    shakeOffset.x = dist(rng) * currentIntensity;
    shakeOffset.y = dist(rng) * currentIntensity;
  }

  mCamera.setCenter({std::round(mCameraBaseCenter.x + shakeOffset.x),
                     std::round(mCameraBaseCenter.y + shakeOffset.y)});
}
//-------------------------------------------------------

//------------[Render - Draw World, Map, Player, Physics Debug, and HUD]-------------------
void GameState::render(sf::RenderWindow &window) {
  window.setView(mCamera);

  sf::Vector2f cameraCenter = mCamera.getCenter();
  sf::Vector2f viewSize = mCamera.getSize();

  mBackgroundSprite.setPosition(
      {cameraCenter.x - viewSize.x / 2.f, cameraCenter.y - viewSize.y / 2.f});

  float parallaxFactorX = 0.2f;
  float parallaxFactorY = 0.1f;

  int texX = static_cast<int>(cameraCenter.x * parallaxFactorX);
  int texY = static_cast<int>(cameraCenter.y * parallaxFactorY);
  mBackgroundSprite.setTextureRect(
      sf::IntRect({texX, texY}, {static_cast<int>(viewSize.x) + 2,
                                 static_cast<int>(viewSize.y) + 2}));

  window.draw(mBackgroundSprite);
  mMap.render(window, mPlayer.getPosition(), mHUD.isHitboxVisible());
  mObjectManager.render(window);
  mObjectManager.renderIceWalls(window, mHUD.isHitboxVisible());
  mObjectManager.renderPendulumTraps(window, mHUD.isHitboxVisible());
  mObjectManager.renderEnemies(window, mHUD.isHitboxVisible());
  mObjectManager.renderProjectiles(window, mHUD.isHitboxVisible());
  mPlayer.render(window, mHUD.isHitboxVisible());

  if (mHUD.isHitboxVisible()) {
    mPhysicsWorld.renderDebug(window);
  }

  if (mHUD.isHitboxVisible()) {
    sf::Vector2f playerPos = mPlayer.getPosition();
    sf::FloatRect bounds = mPlayer.getBounds();
    sf::Vector2f center = {playerPos.x + bounds.size.x * 0.5f, playerPos.y + bounds.size.y * 0.45f};

    Enemy* targetEnemy = nullptr;
    const float maxRadius = 550.f;
    const float maxDistSq = maxRadius * maxRadius;

    struct Candidate {
      Enemy* enemy;
      float distSq;
      sf::Vector2f pos;
    };
    std::vector<Candidate> candidates;

    for (const auto& enemy : mObjectManager.getEnemies()) {
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
      Physics::SweptHit hit = mPhysicsWorld.raycast(center, cand.pos, mPlayer.getRigidBody(), false);
      if (hit.hit && hit.body && hit.body->getType() == Physics::BodyType::Static &&
          (hit.body->getTag() == Physics::ColliderTag::SolidWall || !hit.body->isOneWay()) &&
          hit.toi < 0.99f) {
        continue;
      }
      targetEnemy = cand.enemy;
      break;
    }

    sf::Vector2f aimDir = {mPlayer.isFacingRight() ? 1.f : -1.f, 0.f};
    sf::Vector2f laserEnd = center + aimDir * maxRadius;
    bool locked = false;

    if (targetEnemy) {
      laserEnd = targetEnemy->getPosition() + targetEnemy->getSize() * 0.5f;
      locked = true;
    } else {
      Physics::SweptHit hit = mPhysicsWorld.raycast(center, laserEnd, mPlayer.getRigidBody(), false);
      if (hit.hit && hit.body && hit.body->getType() == Physics::BodyType::Static &&
          (hit.body->getTag() == Physics::ColliderTag::SolidWall || !hit.body->isOneWay()) &&
          hit.toi < 0.99f) {
        laserEnd = center + (laserEnd - center) * hit.toi;
      }
    }

    sf::Color laserColor = locked ? sf::Color(255, 40, 40, 220) : sf::Color(100, 200, 255, 140);
    sf::VertexArray line(sf::PrimitiveType::Lines, 2);
    line[0] = sf::Vertex{center, laserColor};
    line[1] = sf::Vertex{laserEnd, laserColor};
    window.draw(line);

    sf::CircleShape endMarker(locked ? 5.f : 3.f);
    endMarker.setOrigin({endMarker.getRadius(), endMarker.getRadius()});
    endMarker.setPosition(laserEnd);
    endMarker.setFillColor(laserColor);
    window.draw(endMarker);
  }

  mParticleSystem.render(window);

  const auto& pixelFont = Engine::Graphics::FontManager::getInstance().getFont(Engine::Graphics::FontType::Pixel);
  for (const auto& popup : mDamagePopups) {
    float alpha = std::clamp(popup.timer / popup.maxDuration, 0.f, 1.f);
    std::uint8_t a = static_cast<std::uint8_t>(alpha * 255.f);
    sf::Text text(pixelFont, popup.text, popup.isCrit ? 15 : 12);
    sf::Color col = popup.color;
    col.a = a;
    text.setFillColor(col);
    text.setOutlineColor(sf::Color(0, 0, 0, a));
    text.setOutlineThickness(1.5f);
    sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(Engine::Graphics::roundPosition({bounds.size.x * 0.5f, bounds.size.y * 0.5f}));
    text.setPosition(Engine::Graphics::roundPosition(popup.position));
    window.draw(text);
  }

  mHUD.render(window);
  renderEndScreen(window);

  if (mResetFadeAlpha > 0.f) {
    sf::View defaultView = window.getDefaultView();
    window.setView(defaultView);
    sf::RectangleShape fadeOverlay(defaultView.getSize());
    fadeOverlay.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(std::clamp(mResetFadeAlpha, 0.f, 255.f))));
    window.draw(fadeOverlay);
  }
}
//-------------------------------------------------------

//------------[Trigger Game Over - Initiate Death Screen and Audio Transition]-------------------
void GameState::triggerGameOver() {
  if (mGameEndState != GameEndState::None) return;
  mGameEndState = GameEndState::GameOver;
  mEndStateTimer = 0.f;
  triggerHitStop(0.25f);
  triggerCameraShake(8.f, 0.5f);
  Engine::Audio::AudioManager::getInstance().stopBGM();
  Engine::Audio::AudioManager::getInstance().playSound("assets/audio/player_death.wav", 90.f);
  Engine::Audio::AudioManager::getInstance().playSound("assets/audio/game_over.wav", 100.f);
}
//-------------------------------------------------------

//------------[Trigger Victory - Initiate Victory Screen and Audio Celebration]-------------------
void GameState::triggerVictory() {
  if (mGameEndState != GameEndState::None) return;
  mGameEndState = GameEndState::Victory;
  mEndStateTimer = 0.f;
  triggerCameraShake(12.f, 0.8f);
  Engine::Audio::AudioManager::getInstance().stopBGM();
  Engine::Audio::AudioManager::getInstance().playSound("assets/audio/victory.wav", 100.f);
}
//-------------------------------------------------------

//------------[Restart Game - Completely Reset Level State, Player, Enemies and Camera]-------------------
void GameState::restartGame() {
  mGameEndState = GameEndState::None;
  mEndStateTimer = 0.f;
  mHitStopTimer = 0.f;
  mShakeTimer = 0.f;
  mParticleSystem.clear();
  mDamagePopups.clear();
  loadLevel("assets/maps/test.tmx");
  Engine::Audio::AudioManager::getInstance().stopAllSounds();
  Engine::Audio::AudioManager::getInstance().playBGM("assets/audio/bgm_exploration.ogg", true, 50.f);
  Engine::Audio::AudioManager::getInstance().playSound("assets/audio/restart.wav", 85.f);
}
//-------------------------------------------------------

//------------[Render End Screen - Draw Cinematic Dark Red Death or Golden Victory Overlay]-------------------
void GameState::renderEndScreen(sf::RenderWindow &window) {
  if (mGameEndState == GameEndState::None) return;

  sf::View defaultView = window.getDefaultView();
  window.setView(defaultView);
  sf::Vector2f viewSize = defaultView.getSize();

  float alphaFactor = std::min(1.0f, mEndStateTimer / 1.2f);

  if (mGameEndState == GameEndState::GameOver) {
    sf::RectangleShape overlay(viewSize);
    overlay.setFillColor(sf::Color(22, 4, 6, static_cast<std::uint8_t>(alphaFactor * 210.f)));
    window.draw(overlay);

    sf::RectangleShape topBar({viewSize.x, 80.f});
    topBar.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alphaFactor * 255.f)));
    window.draw(topBar);

    sf::RectangleShape botBar({viewSize.x, 80.f});
    botBar.setPosition({0.f, viewSize.y - 80.f});
    botBar.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alphaFactor * 255.f)));
    window.draw(botBar);

    const auto& titleFont = Engine::Graphics::FontManager::getInstance().getFont(Engine::Graphics::FontType::Title);
    const auto& pixelFont = Engine::Graphics::FontManager::getInstance().getFont(Engine::Graphics::FontType::Pixel);

    if (mGameEndState == GameEndState::GameOver) {
      sf::Text shadowText(titleFont, "YOU DIED", 76);
      shadowText.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alphaFactor * 220.f)));
      sf::FloatRect sBounds = shadowText.getLocalBounds();
      shadowText.setPosition(Engine::Graphics::roundPosition({(viewSize.x - sBounds.size.x) * 0.5f + 3.f, viewSize.y * 0.5f - 62.f}));
      window.draw(shadowText);

      sf::Text titleText(titleFont, "YOU DIED", 76);
      titleText.setFillColor(sf::Color(180, 20, 20, static_cast<std::uint8_t>(alphaFactor * 255.f)));
      titleText.setOutlineColor(sf::Color(40, 0, 0, static_cast<std::uint8_t>(alphaFactor * 255.f)));
      titleText.setOutlineThickness(3.f);
      sf::FloatRect bounds = titleText.getLocalBounds();
      titleText.setPosition(Engine::Graphics::roundPosition({(viewSize.x - bounds.size.x) * 0.5f, viewSize.y * 0.5f - 65.f}));
      window.draw(titleText);
    } else if (mGameEndState == GameEndState::Victory) {
      sf::Text shadowText(titleFont, "VICTORY ACHIEVED", 62);
      shadowText.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alphaFactor * 220.f)));
      sf::FloatRect sBounds = shadowText.getLocalBounds();
      shadowText.setPosition(Engine::Graphics::roundPosition({(viewSize.x - sBounds.size.x) * 0.5f + 3.f, viewSize.y * 0.5f - 57.f}));
      window.draw(shadowText);

      sf::Text titleText(titleFont, "VICTORY ACHIEVED", 62);
      titleText.setFillColor(sf::Color(240, 210, 85, static_cast<std::uint8_t>(alphaFactor * 255.f)));
      titleText.setOutlineColor(sf::Color(50, 40, 10, static_cast<std::uint8_t>(alphaFactor * 255.f)));
      titleText.setOutlineThickness(3.f);
      sf::FloatRect bounds = titleText.getLocalBounds();
      titleText.setPosition(Engine::Graphics::roundPosition({(viewSize.x - bounds.size.x) * 0.5f, viewSize.y * 0.5f - 60.f}));
      window.draw(titleText);
    }

    if (mEndStateTimer > 0.8f) {
      float promptAlpha = std::min(1.0f, (mEndStateTimer - 0.8f) / 0.6f);
      sf::Text promptText(pixelFont, "PRESS [R] OR [ENTER] TO RESTART", 18);
      promptText.setFillColor(sf::Color(220, 220, 220, static_cast<std::uint8_t>(promptAlpha * 240.f)));
      promptText.setOutlineColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(promptAlpha * 240.f)));
      promptText.setOutlineThickness(1.5f);
      sf::FloatRect pBounds = promptText.getLocalBounds();
      promptText.setPosition(Engine::Graphics::roundPosition({(viewSize.x - pBounds.size.x) * 0.5f, viewSize.y * 0.5f + 70.f}));
      window.draw(promptText);
    }
  }
}
//-------------------------------------------------------

//------------[Spawn Damage Popup - Spawn Floating Damage Text Indicator]-------------------
void GameState::spawnDamagePopup(sf::Vector2f worldPos, float damage, bool isCrit) {
  DamagePopup popup;
  static float jitter = 0.f;
  jitter = std::fmod(jitter + 14.f, 28.f) - 14.f;
  popup.position = {worldPos.x + jitter, worldPos.y - 14.f};
  popup.text = std::to_string(static_cast<int>(damage));
  popup.timer = 0.5f;
  popup.maxDuration = 0.5f;
  popup.isCrit = isCrit;
  if (isCrit) {
    popup.text += "!";
    popup.color = sf::Color(255, 120, 30);
  } else {
    popup.color = sf::Color(255, 235, 120);
  }
  mDamagePopups.push_back(popup);
}
//-------------------------------------------------------
