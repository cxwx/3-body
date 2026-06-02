#include "body.hh"

Body::Body(sf::Vector2f pos, float m, float r, sf::Color c)
    : position(pos), velocity(0, 0), acceleration(0, 0), mass(m), radius(r), color(c) {
  trail.reserve(MAX_TRAIL_LENGTH);
}

void Body::updateTrail() {
  trail.push_back(position);
  if (trail.size() > MAX_TRAIL_LENGTH) {
    trail.erase(trail.begin());
  }
}

void Body::draw(sf::RenderWindow &window) const {
  sf::CircleShape circle(radius);
  circle.setPosition(sf::Vector2f(position.x - radius, position.y - radius));
  circle.setFillColor(color);
  window.draw(circle);
}