#include <Game/States/GameState.hpp>
#include <Game/States/PauseState.hpp>
#include <Game/Game.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
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
  if (mMap.loadFromFile(filename, &mPhysicsWorld)) {
    mPlayer.initPhysics(mPhysicsWorld);
    mPlayer.reset(mMap.getStartPosition());
    sf::Vector2f playerPos = mPlayer.getPosition();
    sf::Vector2f viewSize = mCamera.getSize();
    float mapW = mMap.getWidth();
    float mapH = mMap.getHeight();

    float camX = (mapW < viewSize.x)
                     ? mapW / 2.f
                     : std::clamp(playerPos.x, viewSize.x / 2.f, mapW - viewSize.x / 2.f);
    float camY = (mapH < viewSize.y)
                     ? mapH / 2.f
                     : std::clamp(playerPos.y, viewSize.y / 2.f, mapH - viewSize.y / 2.f);
    mCamera.setCenter({camX, camY});
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
  mPhysicsWorld.update(dtSec);
  mPlayer.fixedUpdate(dtSec, mMap, mPhysicsWorld);
}
//-------------------------------------------------------

//------------[Update - Step Camera Tracking & Telemetry]-------------------
void GameState::update(sf::Time dt) {
  float dtSec = dt.asSeconds();

  sf::Vector2f vel = mPlayer.getVelocity();
  mHUD.setPlayerSpeed(std::abs(vel.x));
  mHUD.setEntityCount(static_cast<int>(mPhysicsWorld.getBodies().size()));
  mHUD.setPlayerForm(mPlayer.getForm() == PlayerForm::Witch ? "Witch" : "Beast");
  mHUD.setPlayerState(mPlayer.getStateName());
  mHUD.update(dt);

  sf::Vector2f playerPos = mPlayer.getPosition();
  sf::Vector2f viewSize = mCamera.getSize();
  sf::Vector2f currentCenter = mCamera.getCenter();
  float mapW = mMap.getWidth();
  float mapH = mMap.getHeight();

  float targetX = (mapW < viewSize.x)
                      ? mapW / 2.f
                      : std::clamp(playerPos.x, viewSize.x / 2.f, mapW - viewSize.x / 2.f);
  float targetY = (mapH < viewSize.y)
                      ? mapH / 2.f
                      : std::clamp(playerPos.y, viewSize.y / 2.f, mapH - viewSize.y / 2.f);

  float lerpSpeed = 5.0f;
  float newX = currentCenter.x + (targetX - currentCenter.x) * lerpSpeed * dtSec;
  float newY = currentCenter.y + (targetY - currentCenter.y) * lerpSpeed * dtSec;
  mCamera.setCenter({std::round(newX), std::round(newY)});
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
  mPlayer.render(window, mHUD.isHitboxVisible());

  if (mHUD.isHitboxVisible()) {
    mPhysicsWorld.renderDebug(window);
  }

  mHUD.render(window);
}
//-------------------------------------------------------
