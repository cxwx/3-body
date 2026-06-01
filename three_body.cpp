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

  Body(sf::Vector2f pos, float m, float r, sf::Color c) : position(pos), velocity(0, 0), acceleration(0, 0), mass(m), radius(r), color(c) {}

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
  float zoom{};  // 缩放系数

 public:
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
  }

  void addBody(sf::Vector2f pos) {
    sf::Color colors[] = {sf::Color(255, 100, 100), sf::Color(100, 255, 100), sf::Color(100, 100, 255)};
    float mass = 500.0F;   // 统一质量
    float radius = 20.0F;  // 统一半径
    bodies.emplace_back(pos, mass, radius, colors[currentBody]);
  }

  void calculateGravity() {
    for (unsigned i = 0; i < bodies.size(); i++) {
      for (unsigned j = i + 1; j < bodies.size(); j++) {
        sf::Vector2f diff = bodies[j].position - bodies[i].position;
        float dist = sqrt((diff.x * diff.x) + (diff.y * diff.y));

        // 使用软化引力公式
        float force = G * bodies[i].mass * bodies[j].mass / (dist * dist + SOFTENING * SOFTENING);

        sf::Vector2f forceVec = diff / dist * force;

        bodies[i].acceleration += forceVec / bodies[i].mass;
        bodies[j].acceleration -= forceVec / bodies[j].mass;
      }
    }
  }

  void update(float dt) {
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
    }
  }

  void draw(sf::RenderWindow &window) {
    // 计算屏幕中心
    sf::Vector2f center(window.getSize().x / 2.0F, window.getSize().y / 2.0F);

    for (auto &body : bodies) {
      // 手动应用缩放变换
      sf::Vector2f relPos = body.position - center;
      sf::Vector2f scaledPos = center + relPos * zoom;

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
        line[0].position = center + relPos1 * zoom;
        line[0].color = sf::Color::White;
        line[1].position = center + relPos2 * zoom;
        line[1].color = sf::Color::Yellow;
        window.draw(line, 2, sf::PrimitiveType::Lines);
      }
    }
  }

  void handleMouseWheel(float delta) {
    // 鼠标滚轮缩放
    zoom *= (1.0F + (delta * 0.1F));
    zoom = std::max(0.1F, std::min(zoom, 5.0F));  // 限制缩放范围
  }

  // 将屏幕坐标转换为世界坐标（考虑缩放）
  auto screenToWorld(sf::Vector2i screenPos, sf::RenderWindow &window) const -> sf::Vector2f {
    sf::Vector2f center(window.getSize().x / 2.0F, window.getSize().y / 2.0F);
    sf::Vector2f relPos = sf::Vector2f(screenPos) - center;
    return center + relPos / zoom;
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

  void startSimulation() {
    if (bodies.size() == 3) {
      isConfiguring = false;
      settingVelocity = false;
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
        if (keyPressed->code == sf::Keyboard::Key::Escape) {
          window.close();
        } else if (keyPressed->code == sf::Keyboard::Key::R) {
          system.reset();
        } else if (keyPressed->code == sf::Keyboard::Key::Enter && system.isConfiguringMode()) {
          system.startSimulation();
        }
      } else if (const auto *mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mousePressed->button == sf::Mouse::Button::Left) {
          sf::Vector2f mousePos = system.screenToWorld(sf::Vector2i(mousePressed->position), window);
          system.handleMousePress(mousePos);
        }
      } else if (const auto *mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
          sf::Vector2f mousePos = system.screenToWorld(sf::Vector2i(mouseMoved->position), window);
          system.handleMouseDrag(mousePos);
        }
      } else if (const auto *mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
        if (mouseReleased->button == sf::Mouse::Button::Left) {
          sf::Vector2f mousePos = system.screenToWorld(sf::Vector2i(mouseReleased->position), window);
          system.handleMouseRelease(mousePos);
        }
      } else if (const auto *mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
        system.handleMouseWheel(mouseWheel->delta);
      }
    }

    system.update(TIME_STEP);

    window.clear(sf::Color(20, 20, 30));
    system.draw(window);
    window.display();
  }

  return 0;
}
