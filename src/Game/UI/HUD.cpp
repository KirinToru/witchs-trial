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

  // Combat Resource Gauges (Souls-like style)
  if (mFPSFontLoaded) {
    float baseY = (mShowFPS) ? 140.f : 15.f;
    const float barWidth = 180.f;
    const float barHeight = 14.f;
    const float spacing = 22.f;

    auto drawResourceBar = [&](float y, float current, float max, const sf::Color& fillColor, const std::string& label) {
      // 1. Background frame
      sf::RectangleShape bg({barWidth, barHeight});
      bg.setPosition({10.f, y});
      bg.setFillColor(sf::Color(25, 25, 30, 210));
      bg.setOutlineColor(sf::Color(70, 70, 80, 240));
      bg.setOutlineThickness(1.5f);
      window.draw(bg);

      // 2. Filled gauge
      float pct = (max > 0.001f) ? std::clamp(current / max, 0.f, 1.f) : 0.f;
      if (pct > 0.f) {
        sf::RectangleShape fill({barWidth * pct, barHeight});
        fill.setPosition({10.f, y});
        fill.setFillColor(fillColor);
        window.draw(fill);
      }

      // 3. Status text
      std::string textStr = label + " " + std::to_string(static_cast<int>(current)) + "/" + std::to_string(static_cast<int>(max));
      sf::Text barText(mFPSFont, textStr, 11);
      barText.setFillColor(sf::Color::White);
      barText.setOutlineColor(sf::Color::Black);
      barText.setOutlineThickness(1.f);
      barText.setPosition({14.f, y});
      window.draw(barText);
    };

    if (mPlayerForm == "Witch") {
      // Witch Form: Stamina (Green) and Mana (Cyan/Blue)
      drawResourceBar(baseY, mStamina, mMaxStamina, sf::Color(45, 195, 80), "STA");
      drawResourceBar(baseY + spacing, mMana, mMaxMana, sf::Color(40, 140, 240), "MP");
    } else {
      // Beast Form: Rage Meter (Crimson)
      drawResourceBar(baseY, mRage, mMaxRage, sf::Color(230, 45, 35), "RAGE");
    }
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

//------------[Set Stamina - Update Stamina Gauge Display]-------------------
void HUD::setStamina(float current, float max) {
  mStamina = current;
  mMaxStamina = max;
}
//-------------------------------------------------------

//------------[Set Mana - Update Mana Gauge Display]-------------------
void HUD::setMana(float current, float max) {
  mMana = current;
  mMaxMana = max;
}
//-------------------------------------------------------

//------------[Set Rage - Update Beast Rage Gauge Display]-------------------
void HUD::setRage(float current, float max) {
  mRage = current;
  mMaxRage = max;
}
//-------------------------------------------------------

