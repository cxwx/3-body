cmake_minimum_required(VERSION 3.10)
project(ThreeBodySimulation)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 查找SFML库
find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)

# 添加可执行文件
add_executable(three_body
    main.cc
    body.cc
    three_body_system.cc
)

# 链接SFML库
target_link_libraries(three_body PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
)

# 设置输出目录
set_target_properties(three_body PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
