#include <Game/States/GameState.hpp>
#include <Game/States/PauseState.hpp>
#include <Game/Game.hpp>
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

  mPhysicsWorld.setGravity({0.f, 980.f});

  loadLevel("assets/maps/test.tmx");
}
//-------------------------------------------------------

//------------[Load Level - Load TMX Map and Center Camera]-------------------
void GameState::loadLevel(const std::string &filename) {
  mPhysicsWorld.clear();
  mObjectManager.clear();
  mHitEnemiesThisSwing.clear();
  mArenaLocked = false;
  mArenaLeftWall = nullptr;
  mArenaRightWall = nullptr;
  mBoss = nullptr;
  mHUD.setBossInfo(false);

  if (mMap.loadFromFile(filename, &mPhysicsWorld)) {
    mPlayer.initPhysics(mPhysicsWorld);
    mPlayer.setObjectManager(&mObjectManager);
    mPlayer.reset(mMap.getStartPosition());
    sf::Vector2f playerPos = mPlayer.getPosition();
    sf::Vector2f viewSize = mCamera.getSize();
    float mapW = mMap.getWidth();
    float mapH = mMap.getHeight();

    // 1. Bottom Floor - Enemies and props around start position (y ≈ 1540-1568)
    mObjectManager.spawnInquisitor({450.f, 1540.f});
    mObjectManager.spawnInquisitor({1150.f, 1540.f});
    mObjectManager.spawnCrate({350.f, 1540.f});
    mObjectManager.spawnBarrel({650.f, 1540.f});
    mObjectManager.spawnBarrel({980.f, 1540.f});
    mObjectManager.spawnCrate({1250.f, 1540.f});

    // 2. Middle Floor - Enemies and props on platforms (y ≈ 1030-1056)
    mObjectManager.spawnInquisitor({200.f, 1030.f});
    mObjectManager.spawnCrate({280.f, 1030.f});
    mObjectManager.spawnInquisitor({800.f, 1030.f});
    mObjectManager.spawnBarrel({730.f, 1030.f});
    mObjectManager.spawnCrate({870.f, 1030.f});
    mObjectManager.spawnInquisitor({1350.f, 1030.f});
    mObjectManager.spawnCrate({1260.f, 1030.f});

    // 3. Top Roof Arena - Grand Inquisitor Boss & Arena Trigger (y ≈ 600-640)
    sf::Vector2f arenaCenter = {750.f, 440.f};
    Physics::AABB triggerVolume = Physics::AABB::fromPositionSize({1260.f, 300.f}, {90.f, 350.f});
    mArenaTrigger = ArenaTrigger(triggerVolume, arenaCenter, viewSize);
    mBoss = mObjectManager.spawnBoss({750.f, 600.f});

    mObjectManager.spawnBarrel({400.f, 610.f});
    mObjectManager.spawnCrate({1100.f, 610.f});

    mObjectManager.settleProps(mPhysicsWorld);

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
  }

  mPlayer.handleInput(event);
}
//-------------------------------------------------------

//------------[Fixed Update - Step Custom Physics & Deterministic Movement (60Hz)]-------------------
void GameState::fixedUpdate(sf::Time dt) {
  float dtSec = dt.asSeconds();

  if (mHitStopTimer > 0.f) {
    mHitStopTimer = std::max(0.f, mHitStopTimer - dtSec);
    return;
  }

  mPhysicsWorld.update(dtSec);
  mPlayer.fixedUpdate(dtSec, mMap, mPhysicsWorld);
  mObjectManager.updateEnemies(dtSec, mPlayer, mPhysicsWorld);
  mObjectManager.updateProjectiles(dtSec, mPhysicsWorld);
  resolveCombatCollisions();
  mObjectManager.cleanupDestroyed();

  if (mPlayer.consumeGroundSmashImpact()) {
    triggerCameraShake(10.f, 0.4f);
    Physics::AABB smashAABB = Physics::AABB::fromCenterHalfExtents(
        mPlayer.getPosition() + sf::Vector2f(0.f, 20.f),
        sf::Vector2f(100.f, 50.f)
    );
    for (auto& prop : mObjectManager.getProps()) {
      if (!prop || prop->isDestroyed() || !prop->isDestructible()) continue;
      if (smashAABB.intersects(prop->getAABB())) {
        prop->takeDamage(200.f);
      }
    }
  }

  // Check Boss Arena Lock Trigger
  if (!mArenaTrigger.isCompleted() && !mArenaLocked) {
    if (mArenaTrigger.check(mPlayer.getAABB())) {
      engageArenaLock();
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

  triggerCameraShake(14.f, 0.8f);
  triggerHitStop(0.2f);
}
//-------------------------------------------------------

//------------[Resolve Combat Collisions - Process Player & Enemy Hitbox Overlaps]-------------------
void GameState::resolveCombatCollisions() {
  const auto& playerHitbox = mPlayer.getAttackHitbox();
  sf::Vector2f playerPos = mPlayer.getPosition();
  auto& enemies = mObjectManager.getEnemies();

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

        if (postureBroken) {
          triggerHitStop(0.10f);
          triggerCameraShake(10.f, 0.4f);
        } else if (mPlayer.getForm() == PlayerForm::Beast) {
          triggerHitStop(0.08f);
          triggerCameraShake(6.f, 0.25f);
        } else {
          triggerHitStop(0.03f);
          triggerCameraShake(3.f, 0.15f);
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
          triggerHitStop(mPlayer.getForm() == PlayerForm::Beast ? 0.08f : 0.03f);
          triggerCameraShake(mPlayer.getForm() == PlayerForm::Beast ? 6.f : 2.f, 0.2f);
        }
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
        proj->destroy();
        hitTarget = true;

        if (postureBroken) {
          triggerHitStop(0.10f);
          triggerCameraShake(8.f, 0.35f);
        } else {
          triggerHitStop(0.04f);
          triggerCameraShake(3.f, 0.15f);
        }
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
          triggerHitStop(0.04f);
          triggerCameraShake(3.f, 0.15f);
        }
        break;
      }
    }
  }

  const auto& playerHurtbox = mPlayer.getHurtbox();
  for (auto& enemy : enemies) {
    if (!enemy || enemy->isDead()) continue;

    const auto& enemyHitbox = enemy->getAttackHitbox();
    if (enemyHitbox.active) {
      if (Combat::checkOverlap(enemyHitbox, enemy->getPosition(), playerHurtbox, playerPos)) {
        if (mPlayer.takeDamage(enemyHitbox.damage, enemyHitbox.knockback)) {
          triggerHitStop(0.06f);
          triggerCameraShake(4.f, 0.2f);
        }
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

  sf::Vector2f vel = mPlayer.getVelocity();
  mHUD.setPlayerSpeed(std::abs(vel.x));
  mHUD.setEntityCount(static_cast<int>(mPhysicsWorld.getBodies().size() + mObjectManager.getEntityCount()));
  mHUD.setPlayerForm(mPlayer.getForm() == PlayerForm::Witch ? "Witch" : "Beast");
  mHUD.setPlayerState(mPlayer.getStateName());
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
  mObjectManager.renderEnemies(window, mHUD.isHitboxVisible());
  mObjectManager.renderProjectiles(window, mHUD.isHitboxVisible());
  mPlayer.render(window, mHUD.isHitboxVisible());

  if (mHUD.isHitboxVisible()) {
    mPhysicsWorld.renderDebug(window);
  }

  mHUD.render(window);
}
//-------------------------------------------------------
