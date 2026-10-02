#pragma once

#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>

class HUD {
public:
  HUD();
  
  void update(sf::Time dt);
  void render(sf::RenderWindow& window);

  void toggleHitbox();
  void toggleInfo();
  void toggleDebugRaycast();
  
  bool isHitboxVisible() const;
  bool isInfoVisible() const;
  bool isDebugRaycastVisible() const;

  void setPlayerSpeed(float speed);
  void setEntityCount(int count);
  void setFrameTime(float ms);
  void setPlayerForm(std::string_view form);
  void setPlayerState(std::string_view state);
  void setHealth(float current, float max);
  void setStamina(float current, float max);
  void setMana(float current, float max);
  void setRage(float current, float max);

//------------[Set Boss Info - Update Boss Encounter Telemetry]-------------------
  void setBossInfo(bool active, std::string_view name = "", float health = 0.f, float maxHealth = 1.f, float posture = 0.f, float maxPosture = 1.f, int phase = 1);
//-------------------------------------------------------

//------------[Set Combo Step - Update Displayed Melee Combo Counter]-------------------
  void setComboStep(int step);
//-------------------------------------------------------

private:
  bool mShowHitbox;
  bool mShowFPS;
  bool mShowDebugRaycast{true};
  int mComboStep{0};

  sf::Font mFPSFont;
  bool mFPSFontLoaded;
  sf::Clock mFPSClock;
  int mFrameCount;
  int mCurrentFPS;
  float mPlayerSpeed;
  int mEntityCount;
  float mFrameTimeMs;
  std::string mPlayerForm;
  std::string mPlayerState;

  float mHealth{100.f};
  float mMaxHealth{100.f};
  float mStamina{100.f};
  float mMaxStamina{100.f};
  float mMana{100.f};
  float mMaxMana{100.f};
  float mRage{0.f};
  float mMaxRage{100.f};
  float mAnimationTimer{0.f};

  bool mBossActive{false};
  std::string mBossName;
  float mBossHealth{0.f};
  float mBossMaxHealth{1.f};
  float mBossPosture{0.f};
  float mBossMaxPosture{1.f};
  int mBossPhase{1};
};

