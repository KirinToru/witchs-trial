#include <Game/Systems/MouseInteraction.hpp>

//------------[Constructor - Initialize Mouse Interaction Indicator]-------------------
MouseInteraction::MouseInteraction() : mIsDragging(false), mDragPosition(0.f, 0.f) {
  mCursorShape.setRadius(5.f);
  mCursorShape.setOrigin({5.f, 5.f});
  mCursorShape.setFillColor(sf::Color(255, 255, 255, 150));
}
//-------------------------------------------------------

//------------[Init - Setup System State]-------------------
void MouseInteraction::init() {
  mIsDragging = false;
  mDragPosition = {0.f, 0.f};
}
//-------------------------------------------------------

//------------[Handle Event - Process Mouse Press and Release]-------------------
bool MouseInteraction::handleEvent(const sf::Event& event, const sf::RenderWindow& window, const sf::View& view) {
  if (const auto* mPress = event.getIf<sf::Event::MouseButtonPressed>()) {
    if (mPress->button == sf::Mouse::Button::Left) {
      mDragPosition = window.mapPixelToCoords(mPress->position, view);
      mIsDragging = true;
      return true;
    }
  } else if (const auto* mRelease = event.getIf<sf::Event::MouseButtonReleased>()) {
    if (mRelease->button == sf::Mouse::Button::Left) {
      mIsDragging = false;
    }
  }
  return false;
}
//-------------------------------------------------------

//------------[Update - Track Mouse Movement]-------------------
void MouseInteraction::update(float dt, const sf::RenderWindow& window, const sf::View& view) {
  (void)dt;
  if (mIsDragging) {
    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
    mDragPosition = window.mapPixelToCoords(pixelPos, view);
    mCursorShape.setPosition(mDragPosition);
  }
}
//-------------------------------------------------------

//------------[Render - Draw Drag Indicator]-------------------
void MouseInteraction::render(sf::RenderWindow& window) {
  if (mIsDragging) {
    window.draw(mCursorShape);
  }
}
//-------------------------------------------------------

//------------[Is Dragging - Query Drag State]-------------------
bool MouseInteraction::isDragging() const {
  return mIsDragging;
}
//-------------------------------------------------------

//------------[Get Drag Position - Query Dragged World Coordinates]-------------------
sf::Vector2f MouseInteraction::getDragPosition() const {
  return mDragPosition;
}
//-------------------------------------------------------
