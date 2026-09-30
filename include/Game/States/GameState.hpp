#pragma once

#include <Engine/States/State.hpp>
#include <Engine/Physics/PhysicsWorld.hpp>
#include <Game/Entities/Player.hpp>
#include <Game/World/Map.hpp>
#include <Game/UI/HUD.hpp>
#include <SFML/Graphics.hpp>
#include <memory>

#include <Game/Systems/ObjectManager.hpp>
#include <Game/World/ArenaTrigger.hpp>
#include <Game/Entities/GrandInquisitor.hpp>
#include <vector>

enum class GameEndState {
  None,
  GameOver,
  Victory
};

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

//------------[Trigger Game Over - Initiate Death Screen and Audio Transition]-------------------
  void triggerGameOver();
//-------------------------------------------------------

//------------[Trigger Victory - Initiate Victory Screen and Audio Celebration]-------------------
  void triggerVictory();
//-------------------------------------------------------

//------------[Restart Game - Completely Reset Level State, Player, Enemies and Camera]-------------------
  void restartGame();
//-------------------------------------------------------

private:
  void loadLevel(const std::string &filename);

//------------[Resolve Combat Collisions - Process Player & Enemy Hitbox Overlaps]-------------------
  void resolveCombatCollisions();
//-------------------------------------------------------

//------------[Engage Arena Lock - Restrict Camera and Spawn Boundary Colliders]-------------------
  void engageArenaLock();
//-------------------------------------------------------

//------------[Release Arena Lock - Remove Boundary Colliders and Unlock Camera]-------------------
  void releaseArenaLock();
//-------------------------------------------------------

//------------[Render End Screen - Draw Cinematic Dark Red Death or Golden Victory Overlay]-------------------
  void renderEndScreen(sf::RenderWindow &window);
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

  ArenaTrigger mArenaTrigger;
  bool mArenaLocked{false};
  Physics::RigidBody* mArenaLeftWall{nullptr};
  Physics::RigidBody* mArenaRightWall{nullptr};
  GrandInquisitor* mBoss{nullptr};

  GameEndState mGameEndState{GameEndState::None};
  float mEndStateTimer{0.f};
  sf::Font mEndFont;
  bool mEndFontLoaded{false};
};
