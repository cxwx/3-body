#include "constants.hh"
#include "three_body_system.hh"

auto main() -> int {
  sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Three Body Problem");
  window.setFramerateLimit(60);

  ThreeBodySystem system;

  while (window.isOpen()) {
    while (auto event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      } else if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        if (!system.isEditing()) {
          if (keyPressed->code == sf::Keyboard::Key::Escape) {
            window.close();
          } else if (keyPressed->code == sf::Keyboard::Key::R) {
            system.reset();
          } else if (keyPressed->code == sf::Keyboard::Key::Enter && system.isConfiguringMode()) {
            system.startSimulation();
          } else {
            system.handlePan(keyPressed->code);
          }
        } else {
          if (keyPressed->code == sf::Keyboard::Key::Escape) {
            system.cancelEdit();
          } else if (keyPressed->code == sf::Keyboard::Key::Enter) {
            system.confirmEdit();
          }
        }
      } else if (const auto *mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mousePressed->button == sf::Mouse::Button::Left) {
          if (!system.handleControlPanelClick(sf::Vector2i(mousePressed->position), window)) {
            sf::Vector2f mousePos = system.screenToWorld(sf::Vector2i(mousePressed->position), window);
            system.handleMousePress(mousePos);
          }
        }
      } else if (const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
          if (mouseMoved->position.x < window.getSize().x - ThreeBodySystem::CONTROL_PANEL_WIDTH) {
            sf::Vector2f mousePos = system.screenToWorld(sf::Vector2i(mouseMoved->position), window);
            system.handleMouseDrag(mousePos);
          }
        }
      } else if (const auto *mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
        if (mouseReleased->button == sf::Mouse::Button::Left) {
          if (mouseReleased->position.x < window.getSize().x - ThreeBodySystem::CONTROL_PANEL_WIDTH) {
            sf::Vector2f mousePos = system.screenToWorld(sf::Vector2i(mouseReleased->position), window);
            system.handleMouseRelease(mousePos);
          }
        }
      } else if (const auto *mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
        system.handleMouseWheel(mouseWheel->delta);
      } else if (const auto *textEntered = event->getIf<sf::Event::TextEntered>()) {
        system.handleTextInput(textEntered->unicode);
      }
    }

    system.update(TIME_STEP);

    window.clear(sf::Color(20, 20, 30));
    system.draw(window);
    window.display();
  }

  return 0;
}