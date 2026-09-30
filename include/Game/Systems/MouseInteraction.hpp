#pragma once

#include <SFML/Graphics.hpp>

class MouseInteraction {
public:
  MouseInteraction();
  
  void init();
  bool handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& view);
  void update(float dt, const sf::RenderWindow& window, const sf::View& view);
  void render(sf::RenderWindow& window);

  bool isDragging() const;
  sf::Vector2f getDragPosition() const;

private:
  bool mIsDragging;
  sf::Vector2f mDragPosition;
  sf::CircleShape mCursorShape;
};
