#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>
#include <vector>

namespace Engine::Graphics {

struct Particle {
    sf::Vector2f position{0.f, 0.f};
    sf::Vector2f velocity{0.f, 0.f};
    sf::Color startColor{sf::Color::White};
    sf::Color endColor{sf::Color::Transparent};
    float lifetime{0.f};
    float maxLifetime{1.f};
    float startScale{4.f};
    float endScale{0.f};
    float gravity{0.f};
    float drag{0.f};
    bool active{false};
};

class ParticleSystem {
public:
//------------[Constructor - Initialize Pre-Allocated Particle Pool]-------------------
    explicit ParticleSystem(size_t maxParticles = 2048);
//-------------------------------------------------------

//------------[Destructor - Cleanup Particle Resources]-------------------
    ~ParticleSystem() = default;
//-------------------------------------------------------

//------------[Update - Step Particle Positions, Gravity, Velocity Decay and Alpha Fade]-------------------
    void update(float dt);
//-------------------------------------------------------

//------------[Render - Draw Batched Triangles for Active Particles]-------------------
    void render(sf::RenderWindow& window);
//-------------------------------------------------------

//------------[Clear - Deactivate All Particles in Pool]-------------------
    void clear();
//-------------------------------------------------------

//------------[Emit - Allocate and Activate Single Particle]-------------------
    void emit(const Particle& particle);
//-------------------------------------------------------

//------------[Emit Muzzle Flash - Spawn Directional Spark and Smoke Particles]-------------------
    void emitMuzzleFlash(sf::Vector2f position, sf::Vector2f direction);
//-------------------------------------------------------

//------------[Emit Pogo Trail - Spawn Floating Magical Sparkles]-------------------
    void emitPogoTrail(sf::Vector2f position);
//-------------------------------------------------------

//------------[Emit Pogo Burst - Spawn Massive Radial Shockwave and Sparkle Burst]-------------------
    void emitPogoBurst(sf::Vector2f position);
//-------------------------------------------------------

//------------[Emit Dash Trail - Spawn Fading Silhouette Ghost Particles]-------------------
    void emitDashTrail(sf::Vector2f position, bool isWitch);
//-------------------------------------------------------

//------------[Emit Blood Splatter - Spawn Directional Blood Droplets on Impact]-------------------
    void emitBloodSplatter(sf::Vector2f position, sf::Vector2f impactNormal);
//-------------------------------------------------------

//------------[Emit Debris Burst - Spawn Wood Splinter and Dust Particles]-------------------
    void emitDebrisBurst(sf::Vector2f position);
//-------------------------------------------------------

//------------[Emit Parry Sparks - Spawn Sharp Metallic Deflection Spark Particles]-------------------
    void emitParrySparks(sf::Vector2f position);
//-------------------------------------------------------

//------------[Emit Roar Shockwave - Spawn Expanding Radial Shockwave Ring]-------------------
    void emitRoarShockwave(sf::Vector2f position);
//-------------------------------------------------------

private:
    std::vector<Particle> mParticles;
    size_t mPoolIndex{0};
    sf::VertexArray mVertices;
};

} // namespace Engine::Graphics
