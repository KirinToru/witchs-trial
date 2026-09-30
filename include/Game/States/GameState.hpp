#pragma once

#include <Engine/States/State.hpp>
#include <Game/Entities/Player.hpp>
#include <Game/World/Map.hpp>
#include <Game/UI/HUD.hpp>
#include <SFML/Graphics.hpp>
#include <memory>

class GameState : public State {
public:
  GameState(Game *game);

  void handleInput(sf::Event &event) override;
  void update(sf::Time dt) override;
  void render(sf::RenderWindow &window) override;

private:
  void loadLevel(const std::string &filename);

  Player mPlayer;
  Map mMap;

  sf::View mCamera;
  sf::Texture mBackgroundTexture;
  sf::Sprite mBackgroundSprite;

  HUD mHUD;
};
