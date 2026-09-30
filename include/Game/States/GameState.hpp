#pragma once

#include <Engine/States/State.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <Game/Entities/Player.hpp>
#include <Game/World/Map.hpp>
#include <Game/UI/HUD.hpp>
#include <SFML/Graphics.hpp>
#include <memory>

#include <Game/Systems/ObjectManager.hpp>
#include <vector>

class GameState : public State {
public:
  GameState(Game *game);

  void handleInput(sf::Event &event) override;
  void fixedUpdate(sf::Time dt) override;
  void update(sf::Time dt) override;
  void render(sf::RenderWindow &window) override;

//------------[Get Physics World - Access Core Custom Physics Simulation]-------------------
  Physics::PhysicsWorld& getPhysicsWorld() {
    return mPhysicsWorld;
  }
//-------------------------------------------------------

//------------[Trigger Hit Stop - Pause Simulation Updates for Impact Freeze]-------------------
  void triggerHitStop(float durationSeconds);
//-------------------------------------------------------

//------------[Trigger Camera Shake - Apply Screen Shake Trauma]-------------------
  void triggerCameraShake(float intensity, float durationSeconds);
//-------------------------------------------------------

private:
  void loadLevel(const std::string &filename);

//------------[Resolve Combat Collisions - Process Player & Enemy Hitbox Overlaps]-------------------
  void resolveCombatCollisions();
//-------------------------------------------------------

  Player mPlayer;
  Map mMap;
  Physics::PhysicsWorld mPhysicsWorld;
  ObjectManager mObjectManager;
  std::vector<Enemy*> mHitEnemiesThisSwing;

  sf::View mCamera;
  sf::Vector2f mCameraBaseCenter{0.f, 0.f};
  float mHitStopTimer{0.f};
  float mShakeIntensity{0.f};
  float mShakeDuration{0.f};
  float mShakeTimer{0.f};

  sf::Texture mBackgroundTexture;
  sf::Sprite mBackgroundSprite;

  HUD mHUD;
};
