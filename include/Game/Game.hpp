#pragma once

#include <Engine/States/State.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/Context.hpp>
#include <memory>
#include <vector>

class Game {
public:
  Game();
  void run();

  void pushState(std::unique_ptr<State> state);
  void popState();
  void changeState(std::unique_ptr<State> state);

  void clearStatesAndPush(std::unique_ptr<State> state);

//------------[Get Window - Access Render Window]-------------------
  const sf::RenderWindow &getWindow() const { return mWindow; }
//-------------------------------------------------------

//------------[Get Window Mode - Access Current Display Mode]-------------------
  int getWindowMode() const { return mWindowMode; }
//-------------------------------------------------------

  void cycleWindowMode();

private:
  void processEvents();
  void fixedUpdate(sf::Time dt);
  void update(sf::Time dt);
  void render();

  void applyPendingChanges();

  sf::RenderWindow mWindow;
  sf::Context mContext;
  std::vector<std::unique_ptr<State>> mStates;

  static const sf::Time TimePerFrame;

  int mWindowMode = 0; // 0=windowed, 1=maximized, 2=fullscreen

  enum class Action {
    Push,
    Pop,
    Change,
    ClearAndPush
  };

  struct PendingChange {
    Action action;
    std::unique_ptr<State> state;
  };

  std::vector<PendingChange> mPendingChanges;
};
