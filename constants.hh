#ifndef CONSTANTS_HH
#define CONSTANTS_HH

#include <SFML/Graphics.hpp>

// 物理常量
inline constexpr float G = 8000.0F;                 // 引力常数
inline constexpr float TIME_STEP = 0.002F;          // 时间步长
inline constexpr float SOFTENING = 30.0F;           // 软化参数
inline constexpr int PHYSICS_STEPS_PER_FRAME = 15;  // 每帧物理计算次数

// 天体默认参数
inline constexpr float DEFAULT_BODY_MASS = 500.0F;   // 默认天体质量
inline constexpr float DEFAULT_BODY_RADIUS = 20.0F;   // 默认天体半径
inline constexpr float TRIANGLE_SIDE_LENGTH = 300.0F; // 正三角形边长
inline constexpr float ORBITAL_SPEED_FACTOR = 0.85F;   // 轨道速度系数

// 窗口设置
inline constexpr float WINDOW_CENTER_X = 800.0F;       // 窗口中心X
inline constexpr float WINDOW_CENTER_Y = 600.0F;       // 窗口中心Y
inline constexpr int WINDOW_WIDTH = 1600;              // 窗口宽度
inline constexpr int WINDOW_HEIGHT = 1200;             // 窗口高度

// 天体颜色
inline const sf::Color BODY_COLORS[] = {
    sf::Color(255, 100, 100),  // 红色
    sf::Color(100, 255, 100),  // 绿色
    sf::Color(100, 100, 255)   // 蓝色
};
inline constexpr size_t BODY_COLORS_COUNT = 3;

#endif  // CONSTANTS_HH