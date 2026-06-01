#include <SFML/Graphics.hpp>
#include <cmath>
#include <numbers>
#include <vector>

using namespace std;

// 常量
const float G = 8000.0F;                 // 引力常数
const float TIME_STEP = 0.002F;          // 时间步长
const float SOFTENING = 30.0F;           // 软化参数
const int PHYSICS_STEPS_PER_FRAME = 15;  // 每帧物理计算次数（适中）

// 天体类
class Body {
 public:
  sf::Vector2f position;
  sf::Vector2f velocity;
  sf::Vector2f acceleration;
  float mass;
  float radius;
  sf::Color color;
  vector<sf::Vector2f> trail;  // 轨迹
  static constexpr size_t MAX_TRAIL_LENGTH = 5000;  // 最大轨迹长度（10倍）

  Body(sf::Vector2f pos, float m, float r, sf::Color c) : position(pos), velocity(0, 0), acceleration(0, 0), mass(m), radius(r), color(c) {
    trail.reserve(MAX_TRAIL_LENGTH);
  }

  void updateTrail() {
    trail.push_back(position);
    if (trail.size() > MAX_TRAIL_LENGTH) {
      trail.erase(trail.begin());
    }
  }

  void draw(sf::RenderWindow &window) const {
    sf::CircleShape circle(radius);
    circle.setPosition(sf::Vector2f(position.x - radius, position.y - radius));
    circle.setFillColor(color);
    window.draw(circle);
  }
};

// 3体系统类
class ThreeBodySystem {
 private:
  vector<Body> bodies;
  bool isConfiguring{};
  int currentBody{};
  bool settingVelocity{};
  sf::Vector2f velocityStart;
  float zoom = 1.0F;  // 缩放系数，默认为1.0
  bool isPaused{};   // 暂停状态
  int selectedBodyForEdit{-1};  // 选中的天体用于编辑（-1表示无选中）
  string editingText;  // 当前编辑的文本
  bool isEditingVelocityX{}, isEditingVelocityY{}, isEditingMass{};  // 编辑状态
  sf::Vector2f panOffset{};  // 平移偏移量

  // 按钮区域定义
  struct Button {
    sf::FloatRect rect;
    string label;
    bool isHovered{};
  };

 public:
  static constexpr float CONTROL_PANEL_WIDTH = 200.0F;  // 控制面板宽度

  ThreeBodySystem() {
    bodies.clear();
    // 创建稳定的3体配置（正三角形轨道）
    createStableConfiguration();
  }

  void createStableConfiguration() {
    bodies.clear();

    // 正三角形配置，边长300（更大尺度）
    float side = 300.0F;
    float centerX = 800.0F;  // 调整中心位置
    float centerY = 600.0F;
    float radius = side / std::numbers::sqrt3_v<float>;

    // 3个天体的位置（正三角形顶点）
    sf::Vector2f pos1(centerX, centerY - radius);
    sf::Vector2f pos2(centerX + (side * 0.5F), centerY + (radius * 0.5F));
    sf::Vector2f pos3(centerX - (side * 0.5F), centerY + (radius * 0.5F));

    sf::Color colors[] = {sf::Color(255, 100, 100), sf::Color(100, 255, 100), sf::Color(100, 100, 255)};

    // harper: ignore
    // 计算圆周运动速度 v = sqrt(G*M/r)
    float mass = 500.0F;
    float totalMass = 3 * mass;
    float orbitalSpeed = sqrt(G * totalMass / (3 * radius)) * 0.85F;  // 调整速度系数

    // 添加天体，初始速度垂直于位置向量（圆周运动）
    bodies.emplace_back(pos1, mass, 20.0F, colors[0]);
    bodies[0].velocity = sf::Vector2f(orbitalSpeed, 0);

    bodies.emplace_back(pos2, mass, 20.0F, colors[1]);
    bodies[1].velocity = sf::Vector2f(-orbitalSpeed * 0.5F, orbitalSpeed * std::numbers::sqrt3_v<float> * 0.5F);

    bodies.emplace_back(pos3, mass, 20.0F, colors[2]);
    bodies[2].velocity = sf::Vector2f(0.5F * -orbitalSpeed, -orbitalSpeed * std::numbers::sqrt3_v<float> * 0.5F);

    currentBody = 3;
    isConfiguring = false;
    settingVelocity = false;

    // 修正质心速度，确保系统不动
    fixCenterOfMassVelocity();
  }

  void addBody(sf::Vector2f pos) {
    sf::Color colors[] = {sf::Color(255, 100, 100), sf::Color(100, 255, 100), sf::Color(100, 100, 255)};
    float mass = 500.0F;   // 统一质量
    float radius = 20.0F;  // 统一半径
    bodies.emplace_back(pos, mass, radius, colors[currentBody]);
  }

  // 修正质心速度，确保系统总动量为0
  void fixCenterOfMassVelocity() {
    if (bodies.size() < 2) return;

    // 计算质心速度
    sf::Vector2f totalMomentum(0, 0);
    float totalMass = 0.0F;

    for (const auto &body : bodies) {
      totalMomentum += body.velocity * body.mass;
      totalMass += body.mass;
    }

    sf::Vector2f centerVelocity = totalMomentum / totalMass;

    // 从每个天体减去质心速度
    for (auto &body : bodies) {
      body.velocity -= centerVelocity;
    }
  }

  // 修正质心位置到窗口中心
  void fixCenterOfMassPosition() {
    if (bodies.size() < 2) return;

    // 计算质心位置
    sf::Vector2f centerPosition(0, 0);
    float totalMass = 0.0F;

    for (const auto &body : bodies) {
      centerPosition += body.position * body.mass;
      totalMass += body.mass;
    }

    centerPosition /= totalMass;

    // 将质心移到窗口中心
    sf::Vector2f windowCenter(800.0F, 600.0F);  // 1600x1200窗口的中心
    sf::Vector2f offset = windowCenter - centerPosition;

    for (auto &body : bodies) {
      body.position += offset;
    }
  }

  void calculateGravity() {
    for (unsigned i = 0; i < bodies.size(); i++) {
      for (unsigned j = i + 1; j < bodies.size(); j++) {
        sf::Vector2f diff = bodies[j].position - bodies[i].position;
        float dist = sqrt((diff.x * diff.x) + (diff.y * diff.y));

        // 避免除零错误
        if (dist < 0.001F) {
          continue;  // 距离太近，跳过此对天体的引力计算
        }

        // 使用软化引力公式
        float force = G * bodies[i].mass * bodies[j].mass / (dist * dist + SOFTENING * SOFTENING);

        sf::Vector2f forceVec = diff / dist * force;

        bodies[i].acceleration += forceVec / bodies[i].mass;
        bodies[j].acceleration -= forceVec / bodies[j].mass;
      }
    }
  }

  void update(float dt) {
    if (isPaused) return;  // 暂停时不更新物理

    if (!isConfiguring && bodies.size() == 3) {
      // 每帧执行多次物理计算，提高模拟速度同时保持精度
      for (int step = 0; step < PHYSICS_STEPS_PER_FRAME; step++) {
        // 重新计算当前加速度
        calculateGravity();

        // Leapfrog积分
        for (auto &body : bodies) {
          // 第一步：速度半步更新
          body.velocity += body.acceleration * (dt * 0.5F);

          // 第二步：位置完整步更新
          body.position += body.velocity * dt;
        }

        // 第三步：重新计算加速度
        for (auto &body : bodies) { body.acceleration = sf::Vector2f(0, 0); }
        calculateGravity();

        // 第四步：完成速度半步更新
        for (auto &body : bodies) { body.velocity += body.acceleration * (dt * 0.5F); }
      }

      // 每帧更新一次轨迹（避免在物理步骤中频繁更新）
      for (auto &body : bodies) {
        body.updateTrail();
      }
    }
  }

  void draw(sf::RenderWindow &window) {
    // 绘制模拟区域（左侧）
    float simWidth = window.getSize().x - CONTROL_PANEL_WIDTH;

    // 计算屏幕中心（基于模拟区域）
    sf::Vector2f center(simWidth / 2.0F, window.getSize().y / 2.0F);

    for (auto &body : bodies) {
      // 绘制轨迹（应用缩放和平移）
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

      // 绘制天体（应用缩放和平移）
      sf::Vector2f relPos = body.position - center;
      sf::Vector2f scaledPos = center + relPos * zoom + panOffset;

      sf::CircleShape circle(body.radius * zoom);
      circle.setPosition(sf::Vector2f(scaledPos.x - (body.radius * zoom), scaledPos.y - (body.radius * zoom)));
      circle.setFillColor(body.color);
      window.draw(circle);
    }

    // 绘制速度设置线（应用缩放）
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

    // 绘制控制面板（右侧）
    drawControlPanel(window);
  }

  void drawControlPanel(sf::RenderWindow &window) {
    float panelX = window.getSize().x - CONTROL_PANEL_WIDTH;

    // 绘制面板背景
    sf::RectangleShape panelBg(sf::Vector2f(CONTROL_PANEL_WIDTH, window.getSize().y));
    panelBg.setPosition(sf::Vector2f(panelX, 0));
    panelBg.setFillColor(sf::Color(40, 40, 50));
    window.draw(panelBg);

    // 加载字体
    sf::Font font;
    if (!font.openFromFile("/System/Library/Fonts/Helvetica.ttc")) {
      return;
    }

    // 绘制控制按钮
    drawButton(window, panelX + 20, 50, 160, 40, isPaused ? "Resume" : "Pause", isPaused);
    drawButton(window, panelX + 20, 120, 160, 40, "Reset", false);

    // 绘制天体信息
    float yPos = 200;
    for (size_t i = 0; i < bodies.size(); i++) {
      const auto &body = bodies[i];

      // 天体编号和颜色
      sf::Text bodyLabel(font, "Body " + to_string(i + 1), 14);
      bodyLabel.setFillColor(body.color);
      bodyLabel.setPosition(sf::Vector2f(panelX + 10, yPos));
      window.draw(bodyLabel);
      yPos += 25;

      // 速度X
      drawEditableField(window, font, panelX + 10, yPos, "Vx:",
                       to_string(body.velocity.x), i == selectedBodyForEdit && isEditingVelocityX,
                       selectedBodyForEdit == static_cast<int>(i) && !isEditingVelocityX && !isEditingVelocityY && !isEditingMass);
      yPos += 25;

      // 速度Y
      drawEditableField(window, font, panelX + 10, yPos, "Vy:",
                       to_string(body.velocity.y), i == selectedBodyForEdit && isEditingVelocityY,
                       selectedBodyForEdit == static_cast<int>(i) && !isEditingVelocityX && !isEditingVelocityY && !isEditingMass);
      yPos += 25;

      // 总速度
      float totalSpeed = sqrt(body.velocity.x * body.velocity.x + body.velocity.y * body.velocity.y);
      sf::Text speedLabel(font, "|V|: " + to_string(totalSpeed), 12);
      speedLabel.setFillColor(sf::Color(180, 180, 180));
      speedLabel.setPosition(sf::Vector2f(panelX + 10, yPos));
      window.draw(speedLabel);
      yPos += 25;

      // 质量
      drawEditableField(window, font, panelX + 10, yPos, "Mass:",
                       to_string(body.mass), i == selectedBodyForEdit && isEditingMass,
                       selectedBodyForEdit == static_cast<int>(i) && !isEditingVelocityX && !isEditingVelocityY && !isEditingMass);
      yPos += 35;
    }
  }

  void drawEditableField(sf::RenderWindow &window, sf::Font &font, float x, float y,
                        const string &label, const string &value, bool isEditing, bool isSelected) {
    // 标签
    sf::Text labelText(font, label, 12);
    labelText.setFillColor(sf::Color(150, 150, 170));
    labelText.setPosition(sf::Vector2f(x, y));
    window.draw(labelText);

    // 值背景
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

    // 值文本
    string displayValue = isEditing ? editingText : value;
    sf::Text valueText(font, displayValue, 12);
    valueText.setFillColor(isEditing ? sf::Color(100, 255, 100) : sf::Color(200, 200, 200));
    valueText.setPosition(sf::Vector2f(x + 45, y - 1));
    window.draw(valueText);
  }

  void drawButton(sf::RenderWindow &window, float x, float y, float width, float height, const string &label, bool isActive) {
    // 检查鼠标悬停
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    bool isHovered = (mousePos.x >= x && mousePos.x <= x + width && mousePos.y >= y && mousePos.y <= y + height);

    // 绘制按钮背景
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

    // 绘制按钮边框
    sf::RectangleShape border(sf::Vector2f(width - 2, height - 2));
    border.setPosition(sf::Vector2f(x + 1, y + 1));
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineThickness(1);
    border.setOutlineColor(sf::Color(150, 150, 170));
    window.draw(border);

    // 绘制文字
    sf::Font font;
    if (font.openFromFile("/System/Library/Fonts/Helvetica.ttc")) {
      sf::Text text(font, label, 16);
      text.setFillColor(sf::Color::White);
      // 简单的文字居中
      text.setPosition(sf::Vector2f(x + 10, y + 10));
      window.draw(text);
    }
  }

  void handleMouseWheel(float delta) {
    // 鼠标滚轮缩放
    zoom *= (1.0F + (delta * 0.1F));
    zoom = std::max(0.1F, std::min(zoom, 5.0F));  // 限制缩放范围
  }

  // 将屏幕坐标转换为世界坐标（考虑缩放）
  auto screenToWorld(sf::Vector2i screenPos, sf::RenderWindow &window) const -> sf::Vector2f {
    float simWidth = window.getSize().x - CONTROL_PANEL_WIDTH;
    sf::Vector2f center(simWidth / 2.0F, window.getSize().y / 2.0F);
    sf::Vector2f relPos = sf::Vector2f(screenPos) - center;
    return center + relPos / zoom;
  }

  // 检查是否点击了控制面板按钮或字段
  bool handleControlPanelClick(sf::Vector2i mousePos, sf::RenderWindow &window) {
    float panelX = window.getSize().x - CONTROL_PANEL_WIDTH;

    // 检查是否点击了天体信息字段
    float yPos = 200;
    for (size_t i = 0; i < bodies.size(); i++) {
      // 天体编号（不处理点击）
      yPos += 25;

      // 速度X字段
      if (mousePos.x >= panelX + 50 && mousePos.x <= panelX + 170 && mousePos.y >= yPos && mousePos.y <= yPos + 18) {
        selectedBodyForEdit = static_cast<int>(i);
        isEditingVelocityX = true;
        isEditingVelocityY = false;
        isEditingMass = false;
        editingText = to_string(bodies[i].velocity.x);
        return true;
      }
      yPos += 25;

      // 速度Y字段
      if (mousePos.x >= panelX + 50 && mousePos.x <= panelX + 170 && mousePos.y >= yPos && mousePos.y <= yPos + 18) {
        selectedBodyForEdit = static_cast<int>(i);
        isEditingVelocityX = false;
        isEditingVelocityY = true;
        isEditingMass = false;
        editingText = to_string(bodies[i].velocity.y);
        return true;
      }
      yPos += 25;

      // 总速度（不处理点击）
      yPos += 25;

      // 质量字段
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

    // 暂停按钮
    if (mousePos.x >= panelX + 20 && mousePos.x <= panelX + 180 && mousePos.y >= 50 && mousePos.y <= 90) {
      isPaused = !isPaused;
      return true;
    }

    // 重置按钮
    if (mousePos.x >= panelX + 20 && mousePos.x <= panelX + 180 && mousePos.y >= 120 && mousePos.y <= 160) {
      reset();
      return true;
    }

    return false;
  }

  // 处理文本输入
  void handleTextInput(uint32_t unicode) {
    if (selectedBodyForEdit < 0 || selectedBodyForEdit >= static_cast<int>(bodies.size())) {
      return;
    }

    // 处理数字、小数点、负号
    if ((unicode >= '0' && unicode <= '9') || unicode == '.' || unicode == '-' || unicode == 8 || unicode == 127) {
      // 8 = Backspace, 127 = Delete
      if (unicode == 8) {  // Backspace
        if (!editingText.empty()) {
          editingText.pop_back();
        }
      } else if (unicode == 127) {  // Delete
        // 暂不处理
      } else if (unicode == '-') {
        // 只在开头或e/E后允许负号（科学计数法）
        if (editingText.empty() || editingText.back() == 'e' || editingText.back() == 'E') {
          editingText += unicode;
        }
      } else if (unicode == '.') {
        // 防止多个小数点
        if (editingText.find('.') == string::npos) {
          editingText += unicode;
        }
      } else {
        editingText += unicode;
      }

      // 实时更新值
      updateEditedValue();
    } else if (unicode == 13) {  // Enter键确认
      confirmEdit();
      selectedBodyForEdit = -1;
      isEditingVelocityX = false;
      isEditingVelocityY = false;
      isEditingMass = false;
      editingText.clear();
    } else if (unicode == 27) {  // Escape键取消
      selectedBodyForEdit = -1;
      isEditingVelocityX = false;
      isEditingVelocityY = false;
      isEditingMass = false;
      editingText.clear();
    }
  }

  void updateEditedValue() {
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

  void confirmEdit() {
    updateEditedValue();
  }

  void cancelEdit() {
    selectedBodyForEdit = -1;
    isEditingVelocityX = false;
    isEditingVelocityY = false;
    isEditingMass = false;
    editingText.clear();
  }

  // 处理方向键平移
  void handlePan(sf::Keyboard::Key key) {
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

  void handleMousePress(sf::Vector2f mousePos) {
    if (isConfiguring && currentBody < 3) {
      addBody(mousePos);
      currentBody++;
      settingVelocity = true;
      velocityStart = mousePos;
    }
  }

  void handleMouseDrag(sf::Vector2f mousePos) {
    if (settingVelocity) { velocityStart = mousePos; }
  }

  void handleMouseRelease(sf::Vector2f mousePos) {
    if (settingVelocity && currentBody > 0 && static_cast<std::size_t>(currentBody) <= bodies.size() + 1) {
      if (static_cast<std::size_t>(currentBody) - 1 < bodies.size()) {
        sf::Vector2f velocity = (bodies[currentBody - 1].position - mousePos) * 1.0F;
        bodies[currentBody - 1].velocity = velocity;
      }
      settingVelocity = false;
    }
  }

  void reset() {
    bodies.clear();
    isConfiguring = true;
    currentBody = 0;
    settingVelocity = false;
  }

  [[nodiscard]] auto isConfiguringMode() const -> bool { return isConfiguring; }

  [[nodiscard]] auto isEditing() const -> bool {
    return isEditingVelocityX || isEditingVelocityY || isEditingMass;
  }

  void startSimulation() {
    if (bodies.size() == 3) {
      isConfiguring = false;
      settingVelocity = false;
      // 修正质心速度和位置，确保系统围绕固定质心运动
      fixCenterOfMassVelocity();
      fixCenterOfMassPosition();
      // 清空轨迹，因为位置被调整了
      for (auto &body : bodies) {
        body.trail.clear();
      }
    }
  }
};

auto main() -> int {  // NOLINT
  sf::RenderWindow window(sf::VideoMode({1600, 1200}), "Three Body Problem");
  window.setFramerateLimit(60);

  ThreeBodySystem system;

  while (window.isOpen()) {
    while (auto event = window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        window.close();
      } else if (const auto *keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        // 处理方向键或WASD平移（在非编辑模式下）
        if (!system.isEditing()) {
          if (keyPressed->code == sf::Keyboard::Key::Escape) {
            window.close();
          } else if (keyPressed->code == sf::Keyboard::Key::R) {
            system.reset();
          } else if (keyPressed->code == sf::Keyboard::Key::Enter && system.isConfiguringMode()) {
            system.startSimulation();
          } else {
            // 处理方向键平移
            system.handlePan(keyPressed->code);
          }
        } else {
          // 编辑模式下只处理Esc确认/取消编辑
          if (keyPressed->code == sf::Keyboard::Key::Escape) {
            system.cancelEdit();
          } else if (keyPressed->code == sf::Keyboard::Key::Enter) {
            system.confirmEdit();
          }
        }
      } else if (const auto *mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mousePressed->button == sf::Mouse::Button::Left) {
          // 先检查是否点击了控制面板按钮
          if (!system.handleControlPanelClick(sf::Vector2i(mousePressed->position), window)) {
            // 如果不是控制面板，则处理模拟区域
            sf::Vector2f mousePos = system.screenToWorld(sf::Vector2i(mousePressed->position), window);
            system.handleMousePress(mousePos);
          }
        }
      } else if (const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
          // 只在模拟区域处理拖拽
          if (mouseMoved->position.x < window.getSize().x - ThreeBodySystem::CONTROL_PANEL_WIDTH) {
            sf::Vector2f mousePos = system.screenToWorld(sf::Vector2i(mouseMoved->position), window);
            system.handleMouseDrag(mousePos);
          }
        }
      } else if (const auto *mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
        if (mouseReleased->button == sf::Mouse::Button::Left) {
          // 只在模拟区域处理释放
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
