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

  mGame->getConsole().setCommandCallback([this](const std::string &commandLine) {
    std::istringstream iss(commandLine);
    std::string cmd;
    iss >> cmd;

    if (cmd == "gravity") {
      float gx, gy;
      if (iss >> gx >> gy) {
        mPhysicsWorld.setGravity({gx, gy});
        mGame->getConsole().addLog("Gravity set to " + std::to_string(gx) + ", " + std::to_string(gy));
      } else {
        mGame->getConsole().addLog("Usage: gravity <x> <y>");
      }
    } else if (cmd == "autojump") {
      int val;
      if (iss >> val) {
        mPlayer.setAutoJump(val != 0);
        mGame->getConsole().addLog("Autojump set to " + std::to_string(val != 0));
      } else {
        mGame->getConsole().addLog("Usage: autojump <0|1>");
      }
    } else if (cmd == "info") {
      int val;
      if (iss >> val) {
        if ((val != 0) != mHUD.isInfoVisible())
          mHUD.toggleInfo();
        mGame->getConsole().addLog("Info display set to " + std::to_string(val != 0));
      } else {
        mGame->getConsole().addLog("Usage: info <0|1>");
      }
    } else if (cmd == "hitbox") {
      int val;
      if (iss >> val) {
        if ((val != 0) != mHUD.isHitboxVisible())
          mHUD.toggleHitbox();
        mGame->getConsole().addLog("Hitboxes set to " + std::to_string(val != 0));
      } else {
        mGame->getConsole().addLog("Usage: hitbox <0|1>");
      }
    } else if (cmd == "respawn") {
      mPlayer.reset(mMap.getStartPosition());
      mGame->getConsole().addLog("Player respawned at start position.");
    } else if (cmd == "tp") {
      float tx, ty;
      if (iss >> tx >> ty) {
        mPlayer.reset({tx, ty});
        mGame->getConsole().addLog("Player teleported to " + std::to_string(tx) + ", " + std::to_string(ty));
      } else {
        mGame->getConsole().addLog("Usage: tp <x> <y>");
      }
    } else if (cmd == "clear") {
      mGame->getConsole().clearLog();
    } else if (cmd == "help") {
      mGame->getConsole().addLog("Available commands:");
      mGame->getConsole().addLog("  help           - Show this help message");
      mGame->getConsole().addLog("  clear          - Clear the console history");
      mGame->getConsole().addLog("  gravity <x> <y>- Set physics gravity vector");
      mGame->getConsole().addLog("  autojump <0|1> - Toggle auto-bunnyhop");
      mGame->getConsole().addLog("  info <0|1>     - Toggle telemetry HUD overlay");
      mGame->getConsole().addLog("  hitbox <0|1>   - Toggle hitbox debug rendering");
      mGame->getConsole().addLog("  respawn        - Reset player to start position");
      mGame->getConsole().addLog("  tp <x> <y>     - Teleport player to coordinates");
    } else {
      mGame->getConsole().addLog("Unknown command: " + cmd);
    }
  });
}
//-------------------------------------------------------

//------------[Load Level - Load TMX Map and Center Camera]-------------------
void GameState::loadLevel(const std::string &filename) {
  mPhysicsWorld.clear();
  if (mMap.loadFromFile(filename, &mPhysicsWorld)) {
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
}
//-------------------------------------------------------

//------------[Fixed Update - Step Custom Physics & Deterministic Movement (60Hz)]-------------------
void GameState::fixedUpdate(sf::Time dt) {
  float dtSec = dt.asSeconds();
  if (!mGame->getConsole().isOpen()) {
    mPhysicsWorld.update(dtSec);
    mPlayer.update(dtSec, mMap, mPhysicsWorld);
  }
}
//-------------------------------------------------------

//------------[Update - Step Camera Tracking & Telemetry]-------------------
void GameState::update(sf::Time dt) {
  float dtSec = dt.asSeconds();

  sf::Vector2f vel = mPlayer.getVelocity();
  mHUD.setPlayerSpeed(std::abs(vel.x));
  mHUD.setEntityCount(static_cast<int>(mPhysicsWorld.getBodies().size()) + 1);
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
