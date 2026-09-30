#include <Game/Systems/ObjectManager.hpp>
#include <cmath>

//------------[Constructor - Initialize Empty Manager]-------------------
ObjectManager::ObjectManager() = default;
//-------------------------------------------------------

//------------[Spawn Box - Create Rectangle Prop]-------------------
void ObjectManager::spawnBox(sf::Vector2f position) {
  auto prop = std::make_unique<InteractiveProp2D>();
  PropPhysicsSettings settings;
  settings.density = 2.0f;
  settings.friction = 0.4f;
  settings.restitution = 0.1f;
  
  sf::Vector2f size = {40.f, 40.f};
  prop->init(position, size, settings, InteractiveProp2D::ShapeType::Box);
  
  auto shape = std::make_unique<sf::RectangleShape>(size);
  shape->setOrigin({size.x / 2.f, size.y / 2.f});
  shape->setPosition(position);
  shape->setFillColor(sf::Color(180, 100, 50));
  shape->setOutlineThickness(1.f);
  shape->setOutlineColor(sf::Color::White);
  
  mSpawnedProps.push_back(std::move(prop));
  mSpawnedShapes.push_back(std::move(shape));
}
//-------------------------------------------------------

//------------[Spawn Ball - Create Circle Prop]-------------------
void ObjectManager::spawnBall(sf::Vector2f position) {
  auto prop = std::make_unique<InteractiveProp2D>();
  PropPhysicsSettings settings;
  settings.density = 1.0f;
  settings.friction = 0.2f;
  settings.restitution = 0.7f;
  
  sf::Vector2f size = {40.f, 40.f};
  prop->init(position, size, settings, InteractiveProp2D::ShapeType::Circle);
  
  auto shape = std::make_unique<sf::CircleShape>(size.x / 2.f);
  shape->setOrigin({size.x / 2.f, size.y / 2.f});
  shape->setPosition(position);
  shape->setFillColor(sf::Color(50, 150, 220));
  shape->setOutlineThickness(1.f);
  shape->setOutlineColor(sf::Color::White);
  
  mSpawnedProps.push_back(std::move(prop));
  mSpawnedShapes.push_back(std::move(shape));
}
//-------------------------------------------------------

//------------[Spawn Triangle - Create Triangular Convex Prop]-------------------
void ObjectManager::spawnTriangle(sf::Vector2f position) {
  auto prop = std::make_unique<InteractiveProp2D>();
  PropPhysicsSettings settings;
  settings.density = 1.5f;
  settings.friction = 0.5f;
  settings.restitution = 0.1f;
  
  sf::Vector2f size = {50.f, 45.f};
  prop->init(position, size, settings, InteractiveProp2D::ShapeType::Triangle);
  
  auto shape = std::make_unique<sf::ConvexShape>(3);
  shape->setPoint(0, {-size.x / 2.f, size.y / 2.f});
  shape->setPoint(1, {size.x / 2.f, size.y / 2.f});
  shape->setPoint(2, {0.f, -size.y / 2.f});
  shape->setPosition(position);
  shape->setFillColor(sf::Color(220, 180, 50));
  shape->setOutlineThickness(1.f);
  shape->setOutlineColor(sf::Color::White);
  
  mSpawnedProps.push_back(std::move(prop));
  mSpawnedShapes.push_back(std::move(shape));
}
//-------------------------------------------------------

//------------[Spawn Star - Create Star Shaped Convex Prop]-------------------
void ObjectManager::spawnStar(sf::Vector2f position) {
  auto prop = std::make_unique<InteractiveProp2D>();
  PropPhysicsSettings settings;
  settings.density = 0.8f;
  settings.friction = 0.3f;
  settings.restitution = 0.4f;
  
  sf::Vector2f size = {45.f, 45.f};
  prop->init(position, size, settings, InteractiveProp2D::ShapeType::Star);
  
  auto shape = std::make_unique<sf::ConvexShape>(10);
  float outerR = size.x / 2.f;
  float innerR = outerR * 0.4f;
  for (int i = 0; i < 10; ++i) {
    float angle = i * (3.14159265f / 5.f) - (3.14159265f / 2.f);
    float r = (i % 2 == 0) ? outerR : innerR;
    shape->setPoint(i, {r * std::cos(angle), r * std::sin(angle)});
  }
  shape->setPosition(position);
  shape->setFillColor(sf::Color(255, 105, 180));
  shape->setOutlineThickness(1.f);
  shape->setOutlineColor(sf::Color::White);
  
  mSpawnedProps.push_back(std::move(prop));
  mSpawnedShapes.push_back(std::move(shape));
}
//-------------------------------------------------------

//------------[Clear - Remove All Spawned Props]-------------------
void ObjectManager::clear() {
  mSpawnedProps.clear();
  mSpawnedShapes.clear();
}
//-------------------------------------------------------

//------------[Render - Draw All Active Props]-------------------
void ObjectManager::render(sf::RenderWindow& window) {
  for (size_t i = 0; i < mSpawnedProps.size() && i < mSpawnedShapes.size(); ++i) {
    mSpawnedShapes[i]->setPosition(mSpawnedProps[i]->getPosition());
    mSpawnedShapes[i]->setRotation(sf::degrees(mSpawnedProps[i]->getRotation()));
    window.draw(*mSpawnedShapes[i]);
  }
}
//-------------------------------------------------------
