#pragma once

#include <SFML/Graphics.hpp>

class Game;

class State {
protected:
  Game *mGame;

public:
//------------[Constructor - Link State to Game Instance]-------------------
  State(Game *game) : mGame(game) {}
//-------------------------------------------------------

//------------[Destructor - Virtual Clean Up]-------------------
  virtual ~State() = default;
//-------------------------------------------------------

  virtual void handleInput(sf::Event &event) = 0;

//------------[Fixed Update - Default 60Hz Fixed Timestep Step]-------------------
  virtual void fixedUpdate(sf::Time dt) {
    (void)dt;
  }
//-------------------------------------------------------

  virtual void update(sf::Time dt) = 0;
  virtual void render(sf::RenderWindow &window) = 0;
};