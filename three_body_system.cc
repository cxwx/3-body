#include "three_body_system.hh"
#include <cmath>
#include <numbers>

using namespace std;

ThreeBodySystem::ThreeBodySystem() {
  // 加载并缓存字体
  if (!cachedFont.openFromFile("/System/Library/Fonts/Helvetica.ttc")) {
    // 如果加载失败，可以在这里处理错误
  }

  bodies.clear();
  createStableConfiguration();
}

void ThreeBodySystem::createStableConfiguration() {
  bodies.clear();

  float radius = TRIANGLE_SIDE_LENGTH / std::numbers::sqrt3_v<float>;

  // 3个天体的位置（正三角形顶点）
  sf::Vector2f pos1(WINDOW_CENTER_X, WINDOW_CENTER_Y - radius);
  sf::Vector2f pos2(WINDOW_CENTER_X + (TRIANGLE_SIDE_LENGTH * 0.5F), WINDOW_CENTER_Y + (radius * 0.5F));
  sf::Vector2f pos3(WINDOW_CENTER_X - (TRIANGLE_SIDE_LENGTH * 0.5F), WINDOW_CENTER_Y + (radius * 0.5F));

  float totalMass = 3 * DEFAULT_BODY_MASS;
  float orbitalSpeed = sqrt(G * totalMass / (3 * radius)) * ORBITAL_SPEED_FACTOR;

  // 添加天体，初始速度垂直于位置向量（圆周运动）
  bodies.emplace_back(pos1, DEFAULT_BODY_MASS, DEFAULT_BODY_RADIUS, BODY_COLORS[0]);
  bodies[0].velocity = sf::Vector2f(orbitalSpeed, 0);

  bodies.emplace_back(pos2, DEFAULT_BODY_MASS, DEFAULT_BODY_RADIUS, BODY_COLORS[1]);
  bodies[1].velocity = sf::Vector2f(-orbitalSpeed * 0.5F, orbitalSpeed * std::numbers::sqrt3_v<float> * 0.5F);

  bodies.emplace_back(pos3, DEFAULT_BODY_MASS, DEFAULT_BODY_RADIUS, BODY_COLORS[2]);
  bodies[2].velocity = sf::Vector2f(0.5F * -orbitalSpeed, -orbitalSpeed * std::numbers::sqrt3_v<float> * 0.5F);

  currentBody = 3;
  isConfiguring = false;
  settingVelocity = false;

  fixCenterOfMassVelocity();
}

void ThreeBodySystem::addBody(sf::Vector2f pos) {
  bodies.emplace_back(pos, DEFAULT_BODY_MASS, DEFAULT_BODY_RADIUS, BODY_COLORS[currentBody]);
}

void ThreeBodySystem::calculateGravity() {
  for (unsigned i = 0; i < bodies.size(); i++) {
    for (unsigned j = i + 1; j < bodies.size(); j++) {
      sf::Vector2f diff = bodies[j].position - bodies[i].position;
      float dist = sqrt((diff.x * diff.x) + (diff.y * diff.y));

      if (dist < 0.001F) {
        continue;
      }

      float force = G * bodies[i].mass * bodies[j].mass / (dist * dist + SOFTENING * SOFTENING);
      sf::Vector2f forceVec = diff / dist * force;

      bodies[i].acceleration += forceVec / bodies[i].mass;
      bodies[j].acceleration -= forceVec / bodies[j].mass;
    }
  }
}

void ThreeBodySystem::update(float dt) {
  if (isPaused) return;

  if (!isConfiguring && bodies.size() == 3) {
    for (int step = 0; step < PHYSICS_STEPS_PER_FRAME; step++) {
      calculateGravity();

      for (auto &body : bodies) {
        body.velocity += body.acceleration * (dt * 0.5F);
        body.position += body.velocity * dt;
      }

      for (auto &body : bodies) { body.acceleration = sf::Vector2f(0, 0); }
      calculateGravity();

      for (auto &body : bodies) { body.velocity += body.acceleration * (dt * 0.5F); }
    }

    for (auto &body : bodies) {
      body.updateTrail();
    }
  }
}

void ThreeBodySystem::draw(sf::RenderWindow &window) {
  float simWidth = window.getSize().x - CONTROL_PANEL_WIDTH;
  sf::Vector2f center(simWidth / 2.0F, window.getSize().y / 2.0F);

  for (auto &body : bodies) {
    if (body.trail.size() > 1) {
      for (size_t i = 1; i < body.trail.size(); i++) {
        sf::Vertex line[2];
        sf::Vector2f relPos1 = body.trail[i - 1] - center;
        sf::Vector2f relPos2 = body.trail[i] - center;
        line[0].position = center + relPos1 * zoom + panOffset;
        line[0].color = body.color;
        line[1].position = center + relPos2 * zoom + panOffset;
        line[1].color = body.color;
        window.draw(line, 2, sf::PrimitiveType::Lines);
      }
    }

    sf::Vector2f relPos = body.position - center;
    sf::Vector2f scaledPos = center + relPos * zoom + panOffset;

    sf::CircleShape circle(body.radius * zoom);
    circle.setPosition(sf::Vector2f(scaledPos.x - (body.radius * zoom), scaledPos.y - (body.radius * zoom)));
    circle.setFillColor(body.color);
    window.draw(circle);
  }

  if (settingVelocity && static_cast<std::size_t>(currentBody) > 0 && bodies.size() + 1 >= static_cast<std::size_t>(currentBody)) {
    if (static_cast<std::size_t>(currentBody) - 1 < bodies.size()) {
      sf::Vertex line[2];
      sf::Vector2f relPos1 = bodies[currentBody - 1].position - center;
      sf::Vector2f relPos2 = velocityStart - center;
      line[0].position = center + relPos1 * zoom + panOffset;
      line[0].color = sf::Color::White;
      line[1].position = center + relPos2 * zoom + panOffset;
      line[1].color = sf::Color::Yellow;
      window.draw(line, 2, sf::PrimitiveType::Lines);
    }
  }

  drawControlPanel(window);
}

void ThreeBodySystem::drawControlPanel(sf::RenderWindow &window) {
  float panelX = window.getSize().x - CONTROL_PANEL_WIDTH;

  sf::RectangleShape panelBg(sf::Vector2f(CONTROL_PANEL_WIDTH, window.getSize().y));
  panelBg.setPosition(sf::Vector2f(panelX, 0));
  panelBg.setFillColor(sf::Color(40, 40, 50));
  window.draw(panelBg);

  drawButton(window, panelX + 20, 50, 160, 40, isPaused ? "Resume" : "Pause", isPaused);
  drawButton(window, panelX + 20, 120, 160, 40, "Reset", false);

  float yPos = 200;
  for (size_t i = 0; i < bodies.size(); i++) {
    const auto &body = bodies[i];

    sf::Text bodyLabel(cachedFont, "Body " + to_string(i + 1), 14);
    bodyLabel.setFillColor(body.color);
    bodyLabel.setPosition(sf::Vector2f(panelX + 10, yPos));
    window.draw(bodyLabel);
    yPos += 25;

    drawEditableField(window, cachedFont, panelX + 10, yPos, "Vx:",
                     to_string(body.velocity.x), i == static_cast<size_t>(selectedBodyForEdit) && isEditingVelocityX,
                     static_cast<size_t>(selectedBodyForEdit) == i && !isEditingVelocityX && !isEditingVelocityY && !isEditingMass);
    yPos += 25;

    drawEditableField(window, cachedFont, panelX + 10, yPos, "Vy:",
                     to_string(body.velocity.y), i == static_cast<size_t>(selectedBodyForEdit) && isEditingVelocityY,
                     static_cast<size_t>(selectedBodyForEdit) == i && !isEditingVelocityX && !isEditingVelocityY && !isEditingMass);
    yPos += 25;

    float totalSpeed = sqrt(body.velocity.x * body.velocity.x + body.velocity.y * body.velocity.y);
    sf::Text speedLabel(cachedFont, "|V|: " + to_string(totalSpeed), 12);
    speedLabel.setFillColor(sf::Color(180, 180, 180));
    speedLabel.setPosition(sf::Vector2f(panelX + 10, yPos));
    window.draw(speedLabel);
    yPos += 25;

    drawEditableField(window, cachedFont, panelX + 10, yPos, "Mass:",
                     to_string(body.mass), i == static_cast<size_t>(selectedBodyForEdit) && isEditingMass,
                     static_cast<size_t>(selectedBodyForEdit) == i && !isEditingVelocityX && !isEditingVelocityY && !isEditingMass);
    yPos += 35;
  }

  yPos += 20;

  sf::Text cmTitle(cachedFont, "Center of Mass", 14);
  cmTitle.setFillColor(sf::Color(255, 215, 0));
  cmTitle.setPosition(sf::Vector2f(panelX + 10, yPos));
  window.draw(cmTitle);
  yPos += 25;

  sf::Vector2f cmVelocity = getCenterOfMassVelocity();
  float cmSpeed = sqrt(cmVelocity.x * cmVelocity.x + cmVelocity.y * cmVelocity.y);

  sf::Text cmSpeedLabel(cachedFont, "Speed: " + to_string(cmSpeed), 12);
  cmSpeedLabel.setFillColor(sf::Color(180, 180, 180));
  cmSpeedLabel.setPosition(sf::Vector2f(panelX + 10, yPos));
  window.draw(cmSpeedLabel);
  yPos += 25;

  if (cmSpeed > 0.001F) {
    float arrowX = panelX + 90;
    float arrowY = yPos + 15;
    float arrowLength = 50.0F;

    sf::Vector2f dir = cmVelocity / cmSpeed;

    sf::Vertex arrowLine[2];
    arrowLine[0].position = sf::Vector2f(arrowX, arrowY);
    arrowLine[0].color = sf::Color(255, 215, 0);
    arrowLine[1].position = sf::Vector2f(arrowX + dir.x * arrowLength, arrowY + dir.y * arrowLength);
    arrowLine[1].color = sf::Color(255, 215, 0);
    window.draw(arrowLine, 2, sf::PrimitiveType::Lines);

    float arrowHeadSize = 8.0F;
    sf::Vector2f arrowTip = sf::Vector2f(arrowX + dir.x * arrowLength, arrowY + dir.y * arrowLength);

    sf::Vector2f perp(-dir.y, dir.x);

    sf::Vertex arrowHead[3];
    arrowHead[0].position = arrowTip;
    arrowHead[0].color = sf::Color(255, 215, 0);
    arrowHead[1].position = sf::Vector2f(arrowTip.x - dir.x * arrowHeadSize + perp.x * arrowHeadSize * 0.5F,
                                      arrowTip.y - dir.y * arrowHeadSize + perp.y * arrowHeadSize * 0.5F);
    arrowHead[1].color = sf::Color(255, 215, 0);
    arrowHead[2].position = sf::Vector2f(arrowTip.x - dir.x * arrowHeadSize - perp.x * arrowHeadSize * 0.5F,
                                      arrowTip.y - dir.y * arrowHeadSize - perp.y * arrowHeadSize * 0.5F);
    arrowHead[2].color = sf::Color(255, 215, 0);
    window.draw(arrowHead, 3, sf::PrimitiveType::Triangles);

    float angle = atan2(cmVelocity.y, cmVelocity.x) * 180.0F / 3.14159F;
    sf::Text angleLabel(cachedFont, "Angle: " + to_string(angle) + "°", 12);
    angleLabel.setFillColor(sf::Color(150, 150, 150));
    angleLabel.setPosition(sf::Vector2f(panelX + 10, yPos + 35));
    window.draw(angleLabel);
  } else {
    sf::Text stationaryLabel(cachedFont, "Stationary", 12);
    stationaryLabel.setFillColor(sf::Color(150, 150, 150));
    stationaryLabel.setPosition(sf::Vector2f(panelX + 10, yPos));
    window.draw(stationaryLabel);
  }
}

void ThreeBodySystem::drawButton(sf::RenderWindow &window, float x, float y, float width, float height,
                             const std::string &label, bool isActive) {
  sf::Vector2i mousePos = sf::Mouse::getPosition(window);
  bool isHovered = (mousePos.x >= x && mousePos.x <= x + width && mousePos.y >= y && mousePos.y <= y + height);

  sf::RectangleShape button(sf::Vector2f(width, height));
  button.setPosition(sf::Vector2f(x, y));

  if (isActive) {
    button.setFillColor(sf::Color(100, 200, 100));
  } else if (isHovered) {
    button.setFillColor(sf::Color(80, 80, 100));
  } else {
    button.setFillColor(sf::Color(60, 60, 80));
  }

  window.draw(button);

  sf::RectangleShape border(sf::Vector2f(width - 2, height - 2));
  border.setPosition(sf::Vector2f(x + 1, y + 1));
  border.setFillColor(sf::Color::Transparent);
  border.setOutlineThickness(1);
  border.setOutlineColor(sf::Color(150, 150, 170));
  window.draw(border);

  sf::Text text(cachedFont, label, 16);
  text.setFillColor(sf::Color::White);
  text.setPosition(sf::Vector2f(x + 10, y + 10));
  window.draw(text);
}

void ThreeBodySystem::drawEditableField(sf::RenderWindow &window, sf::Font &font, float x, float y,
                                  const std::string &label, const std::string &value, bool isEditing, bool isSelected) {
  sf::Text labelText(font, label, 12);
  labelText.setFillColor(sf::Color(150, 150, 170));
  labelText.setPosition(sf::Vector2f(x, y));
  window.draw(labelText);

  sf::RectangleShape valueBg(sf::Vector2f(120, 18));
  valueBg.setPosition(sf::Vector2f(x + 40, y - 2));
  if (isEditing) {
    valueBg.setFillColor(sf::Color(60, 80, 60));
  } else if (isSelected) {
    valueBg.setFillColor(sf::Color(60, 60, 80));
  } else {
    valueBg.setFillColor(sf::Color(30, 30, 40));
  }
  window.draw(valueBg);

  std::string displayValue = isEditing ? editingText : value;
  sf::Text valueText(font, displayValue, 12);
  valueText.setFillColor(isEditing ? sf::Color(100, 255, 100) : sf::Color(200, 200, 200));
  valueText.setPosition(sf::Vector2f(x + 45, y - 1));
  window.draw(valueText);
}

void ThreeBodySystem::handleMouseWheel(float delta) {
  zoom *= (1.0F + (delta * 0.1F));
  zoom = std::max(0.1F, std::min(zoom, 5.0F));
}

auto ThreeBodySystem::screenToWorld(sf::Vector2i screenPos, sf::RenderWindow &window) const -> sf::Vector2f {
  float simWidth = window.getSize().x - CONTROL_PANEL_WIDTH;
  sf::Vector2f center(simWidth / 2.0F, window.getSize().y / 2.0F);
  sf::Vector2f relPos = sf::Vector2f(screenPos) - center;
  return center + relPos / zoom;
}

bool ThreeBodySystem::handleControlPanelClick(sf::Vector2i mousePos, sf::RenderWindow &window) {
  float panelX = window.getSize().x - CONTROL_PANEL_WIDTH;

  float yPos = 200;
  for (size_t i = 0; i < bodies.size(); i++) {
    yPos += 25;

    if (mousePos.x >= panelX + 50 && mousePos.x <= panelX + 170 && mousePos.y >= yPos && mousePos.y <= yPos + 18) {
      selectedBodyForEdit = static_cast<int>(i);
      isEditingVelocityX = true;
      isEditingVelocityY = false;
      isEditingMass = false;
      editingText = to_string(bodies[i].velocity.x);
      return true;
    }
    yPos += 25;

    if (mousePos.x >= panelX + 50 && mousePos.x <= panelX + 170 && mousePos.y >= yPos && mousePos.y <= yPos + 18) {
      selectedBodyForEdit = static_cast<int>(i);
      isEditingVelocityX = false;
      isEditingVelocityY = true;
      isEditingMass = false;
      editingText = to_string(bodies[i].velocity.y);
      return true;
    }
    yPos += 25;

    yPos += 25;

    if (mousePos.x >= panelX + 50 && mousePos.x <= panelX + 170 && mousePos.y >= yPos && mousePos.y <= yPos + 18) {
      selectedBodyForEdit = static_cast<int>(i);
      isEditingVelocityX = false;
      isEditingVelocityY = false;
      isEditingMass = true;
      editingText = to_string(bodies[i].mass);
      return true;
    }
    yPos += 35;
  }

  if (mousePos.x >= panelX + 20 && mousePos.x <= panelX + 180 && mousePos.y >= 50 && mousePos.y <= 90) {
    isPaused = !isPaused;
    return true;
  }

  if (mousePos.x >= panelX + 20 && mousePos.x <= panelX + 180 && mousePos.y >= 120 && mousePos.y <= 160) {
    reset();
    return true;
  }

  return false;
}

void ThreeBodySystem::handleMousePress(sf::Vector2f mousePos) {
  if (isConfiguring && currentBody < 3) {
    addBody(mousePos);
    currentBody++;
    settingVelocity = true;
    velocityStart = mousePos;
  }
}

void ThreeBodySystem::handleMouseDrag(sf::Vector2f mousePos) {
  if (settingVelocity) { velocityStart = mousePos; }
}

void ThreeBodySystem::handleMouseRelease(sf::Vector2f mousePos) {
  if (settingVelocity && currentBody > 0 && static_cast<std::size_t>(currentBody) <= bodies.size() + 1) {
    if (static_cast<std::size_t>(currentBody) - 1 < bodies.size()) {
      sf::Vector2f velocity = (bodies[currentBody - 1].position - mousePos) * 1.0F;
      bodies[currentBody - 1].velocity = velocity;
    }
    settingVelocity = false;
  }
}

void ThreeBodySystem::handlePan(sf::Keyboard::Key key) {
  const float panSpeed = 20.0F;

  switch (key) {
    case sf::Keyboard::Key::Left:
    case sf::Keyboard::Key::A:
      panOffset.x -= panSpeed;
      break;
    case sf::Keyboard::Key::Right:
    case sf::Keyboard::Key::D:
      panOffset.x += panSpeed;
      break;
    case sf::Keyboard::Key::Up:
    case sf::Keyboard::Key::W:
      panOffset.y -= panSpeed;
      break;
    case sf::Keyboard::Key::Down:
    case sf::Keyboard::Key::S:
      panOffset.y += panSpeed;
      break;
    default:
      break;
  }
}

void ThreeBodySystem::handleTextInput(uint32_t unicode) {
  if (selectedBodyForEdit < 0 || selectedBodyForEdit >= static_cast<int>(bodies.size())) {
    return;
  }

  if ((unicode >= '0' && unicode <= '9') || unicode == '.' || unicode == '-' || unicode == 8 || unicode == 127) {
    if (unicode == 8) {
      if (!editingText.empty()) {
        editingText.pop_back();
      }
    } else if (unicode == 127) {
      // 暂不处理
    } else if (unicode == '-') {
      if (editingText.empty() || editingText.back() == 'e' || editingText.back() == 'E') {
        editingText += static_cast<char>(unicode);
      }
    } else if (unicode == '.') {
      if (editingText.find('.') == std::string::npos) {
        editingText += static_cast<char>(unicode);
      }
    } else {
      editingText += static_cast<char>(unicode);
    }

    updateEditedValue();
  } else if (unicode == 13) {
    confirmEdit();
  } else if (unicode == 27) {
    cancelEdit();
  }
}

void ThreeBodySystem::updateEditedValue() {
  if (selectedBodyForEdit < 0 || selectedBodyForEdit >= static_cast<int>(bodies.size())) {
    return;
  }

  try {
    float value = std::stof(editingText);
    if (isEditingVelocityX) {
      bodies[selectedBodyForEdit].velocity.x = value;
    } else if (isEditingVelocityY) {
      bodies[selectedBodyForEdit].velocity.y = value;
    } else if (isEditingMass) {
      bodies[selectedBodyForEdit].mass = value;
    }
  } catch (...) {
    // 忽略解析错误
  }
}

void ThreeBodySystem::confirmEdit() {
  updateEditedValue();
}

void ThreeBodySystem::cancelEdit() {
  selectedBodyForEdit = -1;
  isEditingVelocityX = false;
  isEditingVelocityY = false;
  isEditingMass = false;
  editingText.clear();
}

void ThreeBodySystem::reset() {
  bodies.clear();
  isConfiguring = true;
  currentBody = 0;
  settingVelocity = false;
}

auto ThreeBodySystem::isConfiguringMode() const -> bool {
  return isConfiguring;
}

auto ThreeBodySystem::isEditing() const -> bool {
  return isEditingVelocityX || isEditingVelocityY || isEditingMass;
}

void ThreeBodySystem::startSimulation() {
  if (bodies.size() == 3) {
    isConfiguring = false;
    settingVelocity = false;
    fixCenterOfMassVelocity();
    fixCenterOfMassPosition();
    for (auto &body : bodies) {
      body.trail.clear();
    }
  }
}

void ThreeBodySystem::fixCenterOfMassVelocity() {
  if (bodies.size() < 2) return;

  sf::Vector2f totalMomentum(0, 0);
  float totalMass = 0.0F;

  for (const auto &body : bodies) {
    totalMomentum += body.velocity * body.mass;
    totalMass += body.mass;
  }

  sf::Vector2f centerVelocity = totalMomentum / totalMass;

  for (auto &body : bodies) {
    body.velocity -= centerVelocity;
  }
}

void ThreeBodySystem::fixCenterOfMassPosition() {
  if (bodies.size() < 2) return;

  sf::Vector2f centerPosition(0, 0);
  float totalMass = 0.0F;

  for (const auto &body : bodies) {
    centerPosition += body.position * body.mass;
    totalMass += body.mass;
  }

  centerPosition /= totalMass;

  sf::Vector2f windowCenter(WINDOW_CENTER_X, WINDOW_CENTER_Y);
  sf::Vector2f offset = windowCenter - centerPosition;

  for (auto &body : bodies) {
    body.position += offset;
  }
}

auto ThreeBodySystem::getCenterOfMassVelocity() const -> sf::Vector2f {
  if (bodies.empty()) return sf::Vector2f(0, 0);

  sf::Vector2f centerVelocity(0, 0);
  float totalMass = 0.0F;

  for (const auto &body : bodies) {
    centerVelocity += body.velocity * body.mass;
    totalMass += body.mass;
  }

  if (totalMass > 0) {
    centerVelocity /= totalMass;
  }

  return centerVelocity;
}