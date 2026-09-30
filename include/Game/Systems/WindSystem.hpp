#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

struct WindParticle {
  sf::Vector2f position;
  sf::Vector2f velocity;
  float rotation;
  float rotationSpeed;
  float life;
  float maxLife;
  sf::Color color;
};

class WindSystem {
public:
  WindSystem();
  
  void update(float dtSec, const sf::View& view);
  void render(sf::RenderWindow& window);
  
  void setWindForce(sf::Vector2f force);
  sf::Vector2f getWindForce() const;

private:
  sf::Vector2f mWindForce;
  std::vector<WindParticle> mParticles;
  float mSpawnTimer;
};
