#include <Game/UI/HUD.hpp>
#include <Engine/Graphics/FontManager.hpp>
#include <iomanip>
#include <sstream>

HUD::HUD()
    : mShowHitbox(false),
      mShowFPS(true),
      mFPSFontLoaded(true),
      mFrameCount(0),
      mCurrentFPS(0),
      mPlayerSpeed(0.0f),
      mEntityCount(0),
      mFrameTimeMs(0.0f) {
}

void HUD::update(sf::Time dt) {
  mAnimationTimer += dt.asSeconds();
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

  const auto& pixelFont = Engine::Graphics::FontManager::getInstance().getFont(Engine::Graphics::FontType::Pixel);
  const auto& titleFont = Engine::Graphics::FontManager::getInstance().getFont(Engine::Graphics::FontType::Title);

  if (mShowFPS) {
    std::stringstream ss;
    ss << "FPS: " << mCurrentFPS << "\n"
       << "Frame Time: " << std::fixed << std::setprecision(2) << mFrameTimeMs << " ms\n"
       << "Entity Count: " << mEntityCount << "\n"
       << "Speed: " << (int)mPlayerSpeed << " px/s\n"
       << "Form: " << mPlayerForm << " (Q to toggle)\n"
       << "State: " << mPlayerState << "\n"
       << "Debug Hitbox/Laser [F3]: " << (mShowHitbox ? "ON" : "OFF");
       
    sf::Text infoText(pixelFont, ss.str(), 14);
    infoText.setFillColor(sf::Color::Yellow);
    
    infoText.setOutlineColor(sf::Color::Black);
    infoText.setOutlineThickness(1.f);
    
    infoText.setPosition(Engine::Graphics::roundPosition({10.f, 10.f}));
    window.draw(infoText);
  }

  {
    float baseY = (mShowFPS) ? 140.f : 15.f;
    const float barWidth = 180.f;
    const float barHeight = 14.f;
    const float spacing = 22.f;

    auto drawResourceBar = [&](float y, float current, float max, const sf::Color& fillColor, const std::string& label) {
      sf::RectangleShape bg({barWidth, barHeight});
      bg.setPosition(Engine::Graphics::roundPosition({10.f, y}));
      bg.setFillColor(sf::Color(25, 25, 30, 210));
      bg.setOutlineColor(sf::Color(70, 70, 80, 240));
      bg.setOutlineThickness(1.5f);
      window.draw(bg);

      float pct = (max > 0.001f) ? std::clamp(current / max, 0.f, 1.f) : 0.f;
      if (pct > 0.f) {
        sf::RectangleShape fill({barWidth * pct, barHeight});
        fill.setPosition(Engine::Graphics::roundPosition({10.f, y}));
        fill.setFillColor(fillColor);
        window.draw(fill);
      }

      std::string textStr = label + " " + std::to_string(static_cast<int>(current)) + "/" + std::to_string(static_cast<int>(max));
      sf::Text barText(pixelFont, textStr, 11);
      barText.setFillColor(sf::Color::White);
      barText.setOutlineColor(sf::Color::Black);
      barText.setOutlineThickness(1.f);
      barText.setPosition(Engine::Graphics::roundPosition({14.f, y}));
      window.draw(barText);
    };

    drawResourceBar(baseY, mHealth, mMaxHealth, sf::Color(220, 35, 35), "HP");

    if (mPlayerForm == "Witch") {
      drawResourceBar(baseY + spacing, mStamina, mMaxStamina, sf::Color(45, 195, 80), "STA");
      drawResourceBar(baseY + spacing * 2.f, mMana, mMaxMana, sf::Color(40, 140, 240), "MP");
    } else {
      float pct = (mMaxRage > 0.001f) ? (mRage / mMaxRage) : 0.f;
      sf::Color rageFillColor = sf::Color(240, 90, 30);
      if (pct < 0.25f) {
        float pulse = 0.5f + 0.5f * std::sin(mAnimationTimer * 14.f);
        rageFillColor = sf::Color(
            static_cast<uint8_t>(200 + 55 * pulse),
            static_cast<uint8_t>(30 + 40 * (1.0f - pulse)),
            static_cast<uint8_t>(20),
            static_cast<uint8_t>(160 + 95 * pulse)
        );
      }
      drawResourceBar(baseY + spacing, mRage, mMaxRage, rageFillColor, "RAGE");
    }

    if (mComboStep > 0) {
      std::string comboStr = "COMBO x" + std::to_string(mComboStep);
      if (mComboStep >= 3) {
        comboStr += " [FINISHER!]";
      }
      sf::Text comboText(pixelFont, comboStr, 12);
      comboText.setFillColor(mComboStep >= 3 ? sf::Color(255, 130, 40) : sf::Color(255, 215, 60));
      comboText.setOutlineColor(sf::Color::Black);
      comboText.setOutlineThickness(1.5f);
      comboText.setPosition(Engine::Graphics::roundPosition({10.f, baseY + spacing * (mPlayerForm == "Witch" ? 3.f : 2.f) + 2.f}));
      window.draw(comboText);
    }

    // 3. Souls-like Massive Boss Health & Posture Bar
    if (mBossActive && mBossHealth > 0.f) {
      float screenW = static_cast<float>(window.getSize().x);
      float screenH = static_cast<float>(window.getSize().y);
      float barW = 660.f;
      float barH = 16.f;
      float barX = (screenW - barW) * 0.5f;
      float barY = screenH - 58.f;

      // Boss Name Text (Cinzel / Dark Fantasy Title Font)
      sf::Text bossNameText(titleFont, mBossName, 18);
      bossNameText.setFillColor(mBossPhase == 2 ? sf::Color(255, 100, 50) : sf::Color(245, 215, 80));
      bossNameText.setOutlineColor(sf::Color::Black);
      bossNameText.setOutlineThickness(1.5f);
      sf::FloatRect textBounds = bossNameText.getLocalBounds();
      bossNameText.setPosition(Engine::Graphics::roundPosition({barX + (barW - textBounds.size.x) * 0.5f, barY - 26.f}));
      window.draw(bossNameText);

      // Background Frame
      sf::RectangleShape bg({barW + 8.f, barH + 8.f});
      bg.setPosition(Engine::Graphics::roundPosition({barX - 4.f, barY - 4.f}));
      bg.setFillColor(sf::Color(20, 20, 25, 230));
      bg.setOutlineColor(mBossPhase == 2 ? sf::Color(220, 50, 20) : sf::Color(190, 150, 40));
      bg.setOutlineThickness(1.5f);
      window.draw(bg);

      // Depletion Backing (Dark Crimson)
      sf::RectangleShape underFill({barW, barH});
      underFill.setPosition({barX, barY});
      underFill.setFillColor(sf::Color(60, 15, 15));
      window.draw(underFill);

      // Health Fill
      float hpPct = std::clamp(mBossHealth / mBossMaxHealth, 0.f, 1.f);
      if (hpPct > 0.f) {
        sf::RectangleShape hpFill({barW * hpPct, barH});
        hpFill.setPosition({barX, barY});
        if (mBossPhase == 2) {
          hpFill.setFillColor(sf::Color(235, 60, 20));
        } else {
          hpFill.setFillColor(sf::Color(200, 35, 35));
        }
        window.draw(hpFill);
      }

      // Posture / Poise Under-bar
      float postureW = barW * 0.6f;
      float postureH = 3.5f;
      float postureX = (screenW - postureW) * 0.5f;
      float postureY = barY + barH + 7.f;

      sf::RectangleShape postureBg({postureW, postureH});
      postureBg.setPosition({postureX, postureY});
      postureBg.setFillColor(sf::Color(30, 30, 35, 200));
      window.draw(postureBg);

      float posturePct = std::clamp(mBossPosture / mBossMaxPosture, 0.f, 1.f);
      if (posturePct > 0.f) {
        sf::RectangleShape postureFill({postureW * posturePct, postureH});
        postureFill.setPosition({postureX, postureY});
        postureFill.setFillColor(sf::Color(240, 190, 45));
        window.draw(postureFill);
      }
    }
  }

  window.setView(oldView);
}
//-------------------------------------------------------

//------------[Toggle Hitbox - Flip Debug Hitbox Display State]-------------------
void HUD::toggleHitbox() {
  mShowHitbox = !mShowHitbox;
}
//-------------------------------------------------------

//------------[Toggle Info - Flip Telemetry Display State]-------------------
void HUD::toggleInfo() {
  mShowFPS = !mShowFPS;
}
//-------------------------------------------------------

//------------[Toggle Debug Raycast - Flip Gun Raycast Debug Line Display State]-------------------
void HUD::toggleDebugRaycast() {
  mShowDebugRaycast = !mShowDebugRaycast;
}
//-------------------------------------------------------

//------------[Is Hitbox Visible - Query Debug Hitbox Display State]-------------------
bool HUD::isHitboxVisible() const {
  return mShowHitbox;
}
//-------------------------------------------------------

//------------[Is Info Visible - Query Telemetry Display State]-------------------
bool HUD::isInfoVisible() const {
  return mShowFPS;
}
//-------------------------------------------------------

//------------[Is Debug Raycast Visible - Query Gun Raycast Debug Line Display State]-------------------
bool HUD::isDebugRaycastVisible() const {
  return mShowDebugRaycast;
}
//-------------------------------------------------------

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

//------------[Set Health - Update Health Gauge Display]-------------------
void HUD::setHealth(float current, float max) {
  mHealth = current;
  mMaxHealth = max;
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

//------------[Set Boss Info - Update Boss Encounter Telemetry]-------------------
void HUD::setBossInfo(bool active, std::string_view name, float health, float maxHealth, float posture, float maxPosture, int phase) {
  mBossActive = active;
  mBossName = std::string(name);
  mBossHealth = health;
  mBossMaxHealth = maxHealth > 0.f ? maxHealth : 1.f;
  mBossPosture = posture;
  mBossMaxPosture = maxPosture > 0.f ? maxPosture : 1.f;
  mBossPhase = phase;
}
//-------------------------------------------------------

//------------[Set Combo Step - Update Displayed Melee Combo Counter]-------------------
void HUD::setComboStep(int step) {
  mComboStep = step;
}
//-------------------------------------------------------

