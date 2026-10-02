#include <Game/Entities/PendulumTrap.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <cmath>

sf::Texture PendulumTrap::sBobTexture;
bool PendulumTrap::sTextureInitialized = false;

//------------[Ensure Shared Texture - Initialize Spiked Bob Sprite Texture]-------------------
void PendulumTrap::ensureSharedTexture() {
    if (sTextureInitialized) return;

    const unsigned int width = 40;
    const unsigned int height = 40;
    sf::Image image({width, height}, sf::Color(0, 0, 0, 0));

    const float centerX = 20.f;
    const float centerY = 20.f;
    const float bodyRadius = 13.f;

    for (unsigned int y = 0; y < height; ++y) {
        for (unsigned int x = 0; x < width; ++x) {
            float dx = static_cast<float>(x) + 0.5f - centerX;
            float dy = static_cast<float>(y) + 0.5f - centerY;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= bodyRadius) {
                float shade = 1.0f - (dist / bodyRadius) * 0.4f;
                auto val = static_cast<uint8_t>(std::clamp(70.f * shade, 0.f, 255.f));
                image.setPixel({x, y}, sf::Color(val + 30, val + 20, val + 20, 255));
            } else if (dist <= bodyRadius + 1.2f) {
                image.setPixel({x, y}, sf::Color(30, 25, 25, 255));
            }
        }
    }

    const int numSpikes = 8;
    const float pi = 3.14159265359f;
    for (int i = 0; i < numSpikes; ++i) {
        float angle = static_cast<float>(i) * (2.f * pi / static_cast<float>(numSpikes));
        float dirX = std::cos(angle);
        float dirY = std::sin(angle);

        for (float r = bodyRadius; r <= 19.f; r += 0.5f) {
            float factor = (19.f - r) / (19.f - bodyRadius);
            float perp = factor * 2.2f;

            for (float p = -perp; p <= perp; p += 0.5f) {
                float px = centerX + dirX * r - dirY * p;
                float py = centerY + dirY * r + dirX * p;
                int ix = static_cast<int>(std::round(px));
                int iy = static_cast<int>(std::round(py));
                if (ix >= 0 && ix < static_cast<int>(width) && iy >= 0 && iy < static_cast<int>(height)) {
                    image.setPixel({static_cast<unsigned int>(ix), static_cast<unsigned int>(iy)}, sf::Color(190, 45, 45, 255));
                }
            }
        }
    }

    (void)sBobTexture.loadFromImage(image);
    sTextureInitialized = true;
}
//-------------------------------------------------------

//------------[Constructor - Initialize Pendulum Trap Physics Parameters & Visuals]-------------------
PendulumTrap::PendulumTrap(sf::Vector2f pivot, float length, float initialAngle)
    : mPivot(pivot),
      mLength(length),
      mAngle(initialAngle),
      mAngularVelocity(0.f),
      mAngularAcceleration(0.f),
      mGravity(980.f),
      mDamping(0.003f),
      mBobPosition(pivot.x + length * std::sin(initialAngle), pivot.y + length * std::cos(initialAngle)),
      mBobVelocity(0.f, 0.f),
      mBobRadius(18.f),
      mDamage(25.f),
      mBobSprite(sBobTexture) {
    ensureSharedTexture();
    mBobSprite.setTexture(sBobTexture, true);
    mBobSprite.setOrigin({20.f, 20.f});
    mBobSprite.setPosition(mBobPosition);
    mBobSprite.setRotation(sf::radians(mAngle));
}
//-------------------------------------------------------

//------------[Update - Step Euler Physical Pendulum Integration]-------------------
void PendulumTrap::update(float dt) {
    mAngularAcceleration = -(mGravity / mLength) * std::sin(mAngle) - mDamping * mAngularVelocity;
    mAngularVelocity += mAngularAcceleration * dt;
    mAngle += mAngularVelocity * dt;

    float sinA = std::sin(mAngle);
    float cosA = std::cos(mAngle);

    mBobPosition.x = mPivot.x + mLength * sinA;
    mBobPosition.y = mPivot.y + mLength * cosA;

    mBobVelocity.x = mLength * mAngularVelocity * cosA;
    mBobVelocity.y = -mLength * mAngularVelocity * sinA;

    mBobSprite.setPosition(mBobPosition);
    mBobSprite.setRotation(sf::radians(mAngle));
}
//-------------------------------------------------------

//------------[Render - Draw Suspension Chain and Spiked Bob Sprite]-------------------
void PendulumTrap::render(sf::RenderWindow& window, bool showHitbox) const {
    sf::VertexArray chain(sf::PrimitiveType::Lines, 2);
    chain[0] = sf::Vertex{mPivot, sf::Color(160, 160, 170, 220)};
    chain[1] = sf::Vertex{mBobPosition, sf::Color(160, 160, 170, 220)};
    window.draw(chain);

    sf::CircleShape anchor(4.f);
    anchor.setOrigin({4.f, 4.f});
    anchor.setPosition(mPivot);
    anchor.setFillColor(sf::Color(80, 80, 90));
    window.draw(anchor);

    window.draw(mBobSprite);

    if (showHitbox) {
        Physics::AABB box = getAABB();
        sf::RectangleShape debugBox;
        debugBox.setPosition(box.min);
        debugBox.setSize(box.getSize());
        debugBox.setFillColor(sf::Color(255, 0, 0, 70));
        debugBox.setOutlineColor(sf::Color::Red);
        debugBox.setOutlineThickness(1.f);
        window.draw(debugBox);
    }
}
//-------------------------------------------------------

//------------[Get AABB - Calculate World Space Hazard Bounding Box]-------------------
Physics::AABB PendulumTrap::getAABB() const {
    return Physics::AABB::fromCenterHalfExtents(mBobPosition, {mBobRadius, mBobRadius});
}
//-------------------------------------------------------
