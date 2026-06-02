#ifndef BODY_HH
#define BODY_HH

#include <SFML/Graphics.hpp>
#include <vector>

class Body {
 public:
  sf::Vector2f position;
  sf::Vector2f velocity;
  sf::Vector2f acceleration;
  float mass;
  float radius;
  sf::Color color;
  std::vector<sf::Vector2f> trail;
  static constexpr size_t MAX_TRAIL_LENGTH = 5000;

  Body(sf::Vector2f pos, float m, float r, sf::Color c);
  void updateTrail();
  void draw(sf::RenderWindow &window) const;
};

#endif  // BODY_HH