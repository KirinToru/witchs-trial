#include <Game/Game.hpp>
#include <Game/States/MenuState.hpp>
#include <Game/States/GameState.hpp>
#include <Game/States/PauseState.hpp>
#include <iostream>

const sf::Time Game::TimePerFrame = sf::seconds(1.f / 60.f);

//------------[Constructor - Initialize Window, Display Settings, and Initial State]-------------------
Game::Game() : mWindow(sf::VideoMode({1280, 720}), "Witch's Trial") {
  mWindow.setFramerateLimit(60);
  mWindow.setVerticalSyncEnabled(true);
  
  mStates.push_back(std::make_unique<MenuState>(this));
}
//-------------------------------------------------------

//------------[Push State - Schedule State Addition]-------------------
void Game::pushState(std::unique_ptr<State> state) {
  mPendingChanges.push_back({Action::Push, std::move(state)});
}
//-------------------------------------------------------

//------------[Pop State - Schedule State Removal]-------------------
void Game::popState() {
  mPendingChanges.push_back({Action::Pop, nullptr});
}
//-------------------------------------------------------

//------------[Change State - Schedule State Replacement]-------------------
void Game::changeState(std::unique_ptr<State> state) {
  mPendingChanges.push_back({Action::Change, std::move(state)});
}
//-------------------------------------------------------

//------------[Clear States And Push - Schedule Stack Reset With New State]-------------------
void Game::clearStatesAndPush(std::unique_ptr<State> state) {
  mPendingChanges.push_back({Action::ClearAndPush, std::move(state)});
}
//-------------------------------------------------------

//------------[Apply Pending Changes - Execute Queued State Transitions]-------------------
void Game::applyPendingChanges() {
  for (auto &change : mPendingChanges) {
    switch (change.action) {
    case Action::Push:
      mStates.push_back(std::move(change.state));
      break;
    case Action::Pop:
      if (!mStates.empty())
        mStates.pop_back();
      break;
    case Action::Change:
      if (!mStates.empty())
        mStates.pop_back();
      mStates.push_back(std::move(change.state));
      break;
    case Action::ClearAndPush:
      mStates.clear();
      mStates.push_back(std::move(change.state));
      break;
    }
  }
  mPendingChanges.clear();
}
//-------------------------------------------------------

//------------[Run - Main Fixed-Timestep (60Hz) Game Loop]-------------------
void Game::run() {
  sf::Clock clock;
  sf::Time accumulator = sf::Time::Zero;
  const sf::Time maxFrameTime = sf::seconds(0.25f);

  while (mWindow.isOpen()) {
    sf::Time dt = clock.restart();
    if (dt > maxFrameTime) {
      dt = maxFrameTime;
    }
    accumulator += dt;

    processEvents();

    while (accumulator >= TimePerFrame) {
      fixedUpdate(TimePerFrame);
      accumulator -= TimePerFrame;
      applyPendingChanges();
    }

    update(dt);
    render();
  }
}
//-------------------------------------------------------

//------------[Process Events - Poll Window & Input Events]-------------------
void Game::processEvents() {
  while (const std::optional event = mWindow.pollEvent()) {
    if (event->is<sf::Event::Closed>())
      mWindow.close();

    if (const auto *keyPress = event->getIf<sf::Event::KeyPressed>()) {
      if (keyPress->code == sf::Keyboard::Key::F4) {
        if (keyPress->alt)
          mWindow.close();
        else
          cycleWindowMode();
      }
    }
    
    if (!mStates.empty()) {
      sf::Event ev = *event;
      mStates.back()->handleInput(ev);
    }
  }
}
//-------------------------------------------------------

//------------[Fixed Update - Step Deterministic 60Hz Physics & Game Logic]-------------------
void Game::fixedUpdate(sf::Time dt) {
  if (!mStates.empty()) {
    mStates.back()->fixedUpdate(dt);
  }
}
//-------------------------------------------------------

//------------[Update - Variable Timestep Updates For UI & Telemetry]-------------------
void Game::update(sf::Time dt) {
  if (!mStates.empty()) {
    mStates.back()->update(dt);
  }
}
//-------------------------------------------------------

//------------[Render - Draw Active States and UI Elements]-------------------
void Game::render() {
  mWindow.clear(sf::Color::Black);
  for (const auto &state : mStates)
    state->render(mWindow);
  mWindow.display();
}
//-------------------------------------------------------

//------------[Cycle Window Mode - Toggle Windowed, Borderless, and Fullscreen]-------------------
void Game::cycleWindowMode() {
  mWindowMode = (mWindowMode + 1) % 3;

  switch (mWindowMode) {
  case 0:
    mWindow.create(sf::VideoMode({1280, 720}), "Witch's Trial",
                   sf::Style::Default);
    break;
  case 1:
    mWindow.create(sf::VideoMode::getDesktopMode(), "Witch's Trial",
                   sf::Style::None);
    break;
  case 2:
    mWindow.create(sf::VideoMode::getDesktopMode(), "Witch's Trial",
                   sf::State::Fullscreen);
    break;
  }
  mWindow.setFramerateLimit(60);
  mWindow.setVerticalSyncEnabled(true);
}
//-------------------------------------------------------
