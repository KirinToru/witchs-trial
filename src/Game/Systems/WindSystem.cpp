#include <Game/Systems/WindSystem.hpp>
#include <cmath>
#include <cstdlib>

//------------[Constructor - Initialize Wind System Defaults]-------------------
WindSystem::WindSystem()
    : mWindForce(100.f, 0.f), mSpawnTimer(0.f) {}
//-------------------------------------------------------

//------------[Update - Particle Simulation & Force Calculation]-------------------
void WindSystem::update(float dtSec, const sf::View& view) {
  mSpawnTimer += dtSec;
  if (mSpawnTimer > 0.05f) {
    mSpawnTimer = 0.f;
    if (mParticles.size() < 100) {
      sf::Vector2f center = view.getCenter();
      sf::Vector2f size = view.getSize();

      float startX = (mWindForce.x >= 0.f) ? (center.x - size.x * 0.6f) : (center.x + size.x * 0.6f);
      float startY = center.y + ((std::rand() % 1000) / 1000.f - 0.5f) * size.y * 1.2f;

      WindParticle p;
      p.position = {startX, startY};
      p.velocity = {mWindForce.x + ((std::rand() % 100) - 50.f), mWindForce.y + ((std::rand() % 40) - 20.f)};
      p.rotation = static_cast<float>(std::rand() % 360);
      p.rotationSpeed = ((std::rand() % 200) - 100.f);
      p.life = 0.f;
      p.maxLife = 2.0f + ((std::rand() % 100) / 50.f);
      p.color = sf::Color(200, 220, 255, 120);

      mParticles.push_back(p);
    }
  }

  for (auto it = mParticles.begin(); it != mParticles.end();) {
    it->life += dtSec;
    if (it->life >= it->maxLife) {
      it = mParticles.erase(it);
    } else {
      it->position += it->velocity * dtSec;
      it->rotation += it->rotationSpeed * dtSec;
      float alphaRatio = 1.0f - (it->life / it->maxLife);
      it->color.a = static_cast<uint8_t>(120.f * alphaRatio);
      ++it;
    }
  }
}
//-------------------------------------------------------

//------------[Render - Draw Wind Streak Particles]-------------------
void WindSystem::render(sf::RenderWindow& window) {
  for (const auto& p : mParticles) {
    sf::RectangleShape line({16.f, 2.f});
    line.setOrigin({8.f, 1.f});
    line.setPosition(p.position);
    line.setRotation(sf::degrees(p.rotation));
    line.setFillColor(p.color);
    window.draw(line);
  }
}
//-------------------------------------------------------

//------------[Set Wind Force - Update Wind Vector]-------------------
void WindSystem::setWindForce(sf::Vector2f force) {
  mWindForce = force;
}
//-------------------------------------------------------

//------------[Get Wind Force - Retrieve Wind Vector]-------------------
sf::Vector2f WindSystem::getWindForce() const {
  return mWindForce;
}
//-------------------------------------------------------
