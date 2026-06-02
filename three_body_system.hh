#ifndef THREE_BODY_SYSTEM_HH
#define THREE_BODY_SYSTEM_HH

#include "body.hh"
#include "constants.hh"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

class ThreeBodySystem {
 private:
  std::vector<Body> bodies;
  bool isConfiguring{};
  int currentBody{};
  bool settingVelocity{};
  sf::Vector2f velocityStart;
  float zoom = 1.0F;  // 缩放系数，默认为1.0
  bool isPaused{};   // 暂停状态
  int selectedBodyForEdit{-1};  // 选中的天体用于编辑（-1表示无选中）
  std::string editingText;  // 当前编辑的文本
  bool isEditingVelocityX{}, isEditingVelocityY{}, isEditingMass{};  // 编辑状态
  sf::Vector2f panOffset{};  // 平移偏移量
  sf::Font cachedFont;      // 缓存的字体

 public:
  static constexpr float CONTROL_PANEL_WIDTH = 200.0F;  // 控制面板宽度

  ThreeBodySystem();
  void createStableConfiguration();
  void addBody(sf::Vector2f pos);
  void calculateGravity();
  void update(float dt);
  void draw(sf::RenderWindow &window);
  void drawControlPanel(sf::RenderWindow &window);
  void drawButton(sf::RenderWindow &window, float x, float y, float width, float height,
                  const std::string &label, bool isActive);
  void drawEditableField(sf::RenderWindow &window, sf::Font &font, float x, float y,
                        const std::string &label, const std::string &value, bool isEditing, bool isSelected);
  void handleMouseWheel(float delta);
  auto screenToWorld(sf::Vector2i screenPos, sf::RenderWindow &window) const -> sf::Vector2f;
  bool handleControlPanelClick(sf::Vector2i mousePos, sf::RenderWindow &window);
  void handleMousePress(sf::Vector2f mousePos);
  void handleMouseDrag(sf::Vector2f mousePos);
  void handleMouseRelease(sf::Vector2f mousePos);
  void handlePan(sf::Keyboard::Key key);
  void handleTextInput(uint32_t unicode);
  void updateEditedValue();
  void confirmEdit();
  void cancelEdit();
  void reset();
  [[nodiscard]] auto isConfiguringMode() const -> bool;
  [[nodiscard]] auto isEditing() const -> bool;
  void startSimulation();
  void fixCenterOfMassVelocity();
  void fixCenterOfMassPosition();
  [[nodiscard]] auto getCenterOfMassVelocity() const -> sf::Vector2f;
};

#endif  // THREE_BODY_SYSTEM_HH