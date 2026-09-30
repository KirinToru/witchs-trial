#include <Game/UI/HUD.hpp>
#include <iomanip>
#include <sstream>

HUD::HUD()
    : mShowHitbox(false),
      mShowFPS(true),
      mFPSFontLoaded(false),
      mFrameCount(0),
      mCurrentFPS(0),
      mPlayerSpeed(0.0f),
      mEntityCount(0),
      mFrameTimeMs(0.0f) {
      
  if (mFPSFont.openFromFile("C:/Windows/Fonts/arial.ttf")) {
    mFPSFontLoaded = true;
  } else {
    std::cerr << "Failed to load Arial font for HUD." << std::endl;
  }
}

void HUD::update(sf::Time dt) {
  if (mShowFPS) {
    mFrameCount++;
    mFrameTimeMs = dt.asSeconds() * 1000.f;
    if (mFPSClock.getElapsedTime().asSeconds() >= 1.0f) {
      mCurrentFPS = mFrameCount;
      mFrameCount = 0;
      mFPSClock.restart();
    }
  }
}

//------------[Render - Draw Screen-Space Telemetry HUD]-------------------
void HUD::render(sf::RenderWindow& window) {
  sf::View oldView = window.getView();
  window.setView(window.getDefaultView()); // Draw HUD in screen space

  if (mShowFPS && mFPSFontLoaded) {
    std::stringstream ss;
    ss << "FPS: " << mCurrentFPS << "\n"
       << "Frame Time: " << std::fixed << std::setprecision(2) << mFrameTimeMs << " ms\n"
       << "Entity Count: " << mEntityCount << "\n"
       << "Speed: " << (int)mPlayerSpeed << " px/s\n"
       << "Form: " << mPlayerForm << " (Q to toggle)\n"
       << "State: " << mPlayerState;
       
    sf::Text infoText(mFPSFont, ss.str(), 16);
    infoText.setFillColor(sf::Color::Yellow);
    
    infoText.setOutlineColor(sf::Color::Black);
    infoText.setOutlineThickness(1.f);
    
    infoText.setPosition({10.f, 10.f});
    window.draw(infoText);
  }

  window.setView(oldView);
}
//-------------------------------------------------------

void HUD::toggleHitbox() {
  mShowHitbox = !mShowHitbox;
}

void HUD::toggleInfo() {
  mShowFPS = !mShowFPS;
}

bool HUD::isHitboxVisible() const {
  return mShowHitbox;
}

bool HUD::isInfoVisible() const {
  return mShowFPS;
}

void HUD::setPlayerSpeed(float speed) {
  mPlayerSpeed = speed;
}

void HUD::setEntityCount(int count) {
  mEntityCount = count;
}

void HUD::setFrameTime(float ms) {
  mFrameTimeMs = ms;
}

//------------[Set Player Form - Update Displayed Dual Form]-------------------
void HUD::setPlayerForm(std::string_view form) {
  mPlayerForm = form;
}
//-------------------------------------------------------

//------------[Set Player State - Update Displayed FSM State]-------------------
void HUD::setPlayerState(std::string_view state) {
  mPlayerState = state;
}
//-------------------------------------------------------
