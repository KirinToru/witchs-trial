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
  
  bool isHitboxVisible() const;
  bool isInfoVisible() const;

  void setPlayerSpeed(float speed);
  void setEntityCount(int count);
  void setFrameTime(float ms);
  void setPlayerForm(std::string_view form);
  void setPlayerState(std::string_view state);
  void setHealth(float current, float max);
  void setStamina(float current, float max);
  void setMana(float current, float max);
  void setRage(float current, float max);

private:
  bool mShowHitbox;
  bool mShowFPS;

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
};
