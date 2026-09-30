#pragma once

#include <SFML/Graphics.hpp>
#include <Game/Entities/InteractiveProp2D.hpp>
#include <vector>
#include <memory>

class ObjectManager {
public:
  ObjectManager();
  
  void spawnBox(sf::Vector2f position);
  void spawnBall(sf::Vector2f position);
  void spawnTriangle(sf::Vector2f position);
  void spawnStar(sf::Vector2f position);
  
  void clear();
  void render(sf::RenderWindow& window);
  
  const std::vector<std::unique_ptr<InteractiveProp2D>>& getProps() const { return mSpawnedProps; }
  size_t getEntityCount() const { return mSpawnedProps.size(); }

private:
  std::vector<std::unique_ptr<InteractiveProp2D>> mSpawnedProps;
  std::vector<std::unique_ptr<sf::Shape>> mSpawnedShapes;
};
