#include <Engine/Graphics/ParticleSystem.hpp>
#include <algorithm>
#include <cmath>
#include <random>

namespace Engine::Graphics {

namespace {
thread_local std::mt19937 gRandomEngine{1337};

//------------[Helper - Generate Random Float in Range]-------------------
float randomFloat(float minVal, float maxVal) {
    std::uniform_real_distribution<float> dist(minVal, maxVal);
    return dist(gRandomEngine);
}
//-------------------------------------------------------

//------------[Helper - Generate Random Int in Range]-------------------
int randomInt(int minVal, int maxVal) {
    std::uniform_int_distribution<int> dist(minVal, maxVal);
    return dist(gRandomEngine);
}
//-------------------------------------------------------

//------------[Helper - Linearly Interpolate Colors]-------------------
sf::Color lerpColor(sf::Color a, sf::Color b, float t) {
    auto r = static_cast<uint8_t>(static_cast<float>(a.r) + t * (static_cast<float>(b.r) - static_cast<float>(a.r)));
    auto g = static_cast<uint8_t>(static_cast<float>(a.g) + t * (static_cast<float>(b.g) - static_cast<float>(a.g)));
    auto bl = static_cast<uint8_t>(static_cast<float>(a.b) + t * (static_cast<float>(b.b) - static_cast<float>(a.b)));
    auto al = static_cast<uint8_t>(static_cast<float>(a.a) + t * (static_cast<float>(b.a) - static_cast<float>(a.a)));
    return sf::Color(r, g, bl, al);
}
//-------------------------------------------------------
}

//------------[Constructor - Initialize Pre-Allocated Particle Pool]-------------------
ParticleSystem::ParticleSystem(size_t maxParticles)
    : mParticles(maxParticles),
      mVertices(sf::PrimitiveType::Triangles) {
    mVertices.resize(maxParticles * 6);
}
//-------------------------------------------------------

//------------[Update - Step Particle Positions, Gravity, Velocity Decay and Alpha Fade]-------------------
void ParticleSystem::update(float dt) {
    for (auto& p : mParticles) {
        if (!p.active) continue;

        p.lifetime += dt;
        if (p.lifetime >= p.maxLifetime) {
            p.active = false;
            continue;
        }

        p.velocity.y += p.gravity * dt;
        if (p.drag > 0.f) {
            float decay = std::max(0.f, 1.f - p.drag * dt);
            p.velocity *= decay;
        }

        p.position += p.velocity * dt;
    }
}
//-------------------------------------------------------

//------------[Render - Draw Batched Triangles for Active Particles]-------------------
void ParticleSystem::render(sf::RenderWindow& window) {
    mVertices.clear();

    for (const auto& p : mParticles) {
        if (!p.active) continue;

        float t = std::clamp(p.lifetime / p.maxLifetime, 0.f, 1.f);
        sf::Color color = lerpColor(p.startColor, p.endColor, t);
        float scale = p.startScale + t * (p.endScale - p.startScale);
        float half = scale * 0.5f;

        sf::Vector2f p0{p.position.x - half, p.position.y - half};
        sf::Vector2f p1{p.position.x + half, p.position.y - half};
        sf::Vector2f p2{p.position.x + half, p.position.y + half};
        sf::Vector2f p3{p.position.x - half, p.position.y + half};

        mVertices.append(sf::Vertex{p0, color});
        mVertices.append(sf::Vertex{p1, color});
        mVertices.append(sf::Vertex{p2, color});

        mVertices.append(sf::Vertex{p0, color});
        mVertices.append(sf::Vertex{p2, color});
        mVertices.append(sf::Vertex{p3, color});
    }

    if (mVertices.getVertexCount() > 0) {
        window.draw(mVertices);
    }
}
//-------------------------------------------------------

//------------[Clear - Deactivate All Particles in Pool]-------------------
void ParticleSystem::clear() {
    for (auto& p : mParticles) {
        p.active = false;
    }
    mPoolIndex = 0;
}
//-------------------------------------------------------

//------------[Emit - Allocate and Activate Single Particle]-------------------
void ParticleSystem::emit(const Particle& particle) {
    mParticles[mPoolIndex] = particle;
    mParticles[mPoolIndex].active = true;
    mParticles[mPoolIndex].lifetime = 0.f;
    mPoolIndex = (mPoolIndex + 1) % mParticles.size();
}
//-------------------------------------------------------

//------------[Emit Muzzle Flash - Spawn Directional Spark and Smoke Particles]-------------------
void ParticleSystem::emitMuzzleFlash(sf::Vector2f position, sf::Vector2f direction) {
    float baseAngle = std::atan2(direction.y, direction.x);

    for (int i = 0; i < 18; ++i) {
        float angle = baseAngle + randomFloat(-0.45f, 0.45f);
        float speed = randomFloat(250.f, 650.f);

        Particle p;
        p.position = position;
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.startColor = sf::Color(255, randomInt(180, 240), randomInt(60, 120), 255);
        p.endColor = sf::Color(255, 60, 0, 0);
        p.maxLifetime = randomFloat(0.10f, 0.22f);
        p.startScale = randomFloat(3.5f, 6.0f);
        p.endScale = 0.5f;
        p.gravity = 150.f;
        p.drag = 4.f;
        emit(p);
    }

    for (int i = 0; i < 8; ++i) {
        float angle = baseAngle + randomFloat(-0.7f, 0.7f);
        float speed = randomFloat(60.f, 180.f);

        Particle p;
        p.position = position;
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.startColor = sf::Color(180, 180, 210, 160);
        p.endColor = sf::Color(70, 70, 90, 0);
        p.maxLifetime = randomFloat(0.25f, 0.45f);
        p.startScale = randomFloat(4.f, 8.f);
        p.endScale = randomFloat(10.f, 18.f);
        p.gravity = -50.f;
        p.drag = 2.f;
        emit(p);
    }
}
//-------------------------------------------------------

//------------[Emit Pogo Trail - Spawn Floating Fire Sparkles]-------------------
void ParticleSystem::emitPogoTrail(sf::Vector2f position) {
    for (int i = 0; i < 3; ++i) {
        Particle p;
        p.position = position + sf::Vector2f{randomFloat(-8.f, 8.f), randomFloat(-8.f, 8.f)};
        p.velocity = {randomFloat(-30.f, 30.f), randomFloat(-40.f, 10.f)};
        p.startColor = sf::Color(255, randomInt(110, 200), randomInt(10, 40), 220);
        p.endColor = sf::Color(200, randomInt(20, 60), 0, 0);
        p.maxLifetime = randomFloat(0.35f, 0.65f);
        p.startScale = randomFloat(4.f, 7.f);
        p.endScale = 1.f;
        p.gravity = -30.f;
        p.drag = 1.5f;
        emit(p);
    }
}
//-------------------------------------------------------

//------------[Emit Pogo Burst - Spawn Massive Radial Fire Burst]-------------------
void ParticleSystem::emitPogoBurst(sf::Vector2f position) {
    for (int i = 0; i < 55; ++i) {
        float angle = randomFloat(0.f, 6.2831853f);
        float speed = randomFloat(180.f, 650.f);

        Particle p;
        p.position = position;
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        if (i % 2 == 0) {
            p.startColor = sf::Color(255, 235, 120, 255);
            p.endColor = sf::Color(255, 100, 20, 0);
        } else {
            p.startColor = sf::Color(255, 140, 30, 240);
            p.endColor = sf::Color(200, 30, 10, 0);
        }
        p.maxLifetime = randomFloat(0.28f, 0.60f);
        p.startScale = randomFloat(5.f, 9.f);
        p.endScale = 0.5f;
        p.gravity = 100.f;
        p.drag = 2.5f;
        emit(p);
    }

    for (int i = 0; i < 20; ++i) {
        float angle = randomFloat(0.f, 6.2831853f);
        float speed = randomFloat(80.f, 240.f);

        Particle p;
        p.position = position;
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.startColor = sf::Color(255, 255, 200, 240);
        p.endColor = sf::Color(255, 160, 40, 0);
        p.maxLifetime = randomFloat(0.4f, 0.8f);
        p.startScale = randomFloat(6.f, 12.f);
        p.endScale = 1.f;
        p.gravity = 0.f;
        p.drag = 3.f;
        emit(p);
    }
}
//-------------------------------------------------------

//------------[Emit Dash Trail - Spawn Fading Silhouette Ghost Particles]-------------------
void ParticleSystem::emitDashTrail(sf::Vector2f position, bool isWitch) {
    for (int i = 0; i < 4; ++i) {
        Particle p;
        p.position = position + sf::Vector2f{randomFloat(-6.f, 6.f), randomFloat(-12.f, 12.f)};
        p.velocity = {randomFloat(-25.f, 25.f), randomFloat(-25.f, 25.f)};
        if (isWitch) {
            p.startColor = sf::Color(170, 110, 255, 180);
            p.endColor = sf::Color(60, 20, 150, 0);
        } else {
            p.startColor = sf::Color(245, 60, 40, 190);
            p.endColor = sf::Color(120, 10, 10, 0);
        }
        p.maxLifetime = randomFloat(0.20f, 0.38f);
        p.startScale = randomFloat(6.f, 10.f);
        p.endScale = 2.f;
        p.gravity = 0.f;
        p.drag = 2.f;
        emit(p);
    }
}
//-------------------------------------------------------

//------------[Emit Blood Splatter - Spawn Directional Blood Droplets on Impact]-------------------
void ParticleSystem::emitBloodSplatter(sf::Vector2f position, sf::Vector2f impactNormal) {
    float baseAngle = std::atan2(impactNormal.y, impactNormal.x);

    for (int i = 0; i < 22; ++i) {
        float angle = baseAngle + randomFloat(-0.85f, 0.85f);
        float speed = randomFloat(120.f, 420.f);

        Particle p;
        p.position = position;
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.startColor = sf::Color(randomInt(160, 220), randomInt(10, 30), randomInt(10, 30), 240);
        p.endColor = sf::Color(90, 0, 0, 0);
        p.maxLifetime = randomFloat(0.25f, 0.55f);
        p.startScale = randomFloat(3.f, 6.f);
        p.endScale = randomFloat(1.f, 2.5f);
        p.gravity = 750.f;
        p.drag = 1.2f;
        emit(p);
    }
}
//-------------------------------------------------------

//------------[Emit Debris Burst - Spawn Wood Splinter and Dust Particles]-------------------
void ParticleSystem::emitDebrisBurst(sf::Vector2f position) {
    for (int i = 0; i < 30; ++i) {
        float angle = randomFloat(0.f, 6.2831853f);
        float speed = randomFloat(150.f, 500.f);

        Particle p;
        p.position = position;
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        if (i % 3 == 0) {
            p.startColor = sf::Color(160, 110, 65, 255);
            p.endColor = sf::Color(100, 65, 35, 0);
        } else if (i % 3 == 1) {
            p.startColor = sf::Color(190, 145, 90, 255);
            p.endColor = sf::Color(120, 80, 45, 0);
        } else {
            p.startColor = sf::Color(120, 120, 130, 200);
            p.endColor = sf::Color(60, 60, 70, 0);
        }
        p.maxLifetime = randomFloat(0.35f, 0.70f);
        p.startScale = randomFloat(4.f, 8.f);
        p.endScale = 1.5f;
        p.gravity = 850.f;
        p.drag = 1.8f;
        emit(p);
    }
}
//-------------------------------------------------------

} // namespace Engine::Graphics
