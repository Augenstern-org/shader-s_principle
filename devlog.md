# 开发日志 — Shader's Principle

## 项目概述

用 C++20 在 CPU 上模拟 GPU 渲染管线（软渲染器）。使用 GLFW + OpenGL 仅作为显示输出，核心渲染逻辑全部在 CPU 端实现。

技术栈：CMake + C++20 + OpenGL + GLFW + GLM

---

## 开发总规划

分 5 个阶段，每个阶段结束时都能在屏幕上看到可见的结果。核心原则：

> **每一步都要有可见的输出，只引入当前需要的抽象。**

| 阶段 | 目标 | 屏幕结果 |
|------|------|----------|
| 1. 搭骨架 | CMake 编译通过，GLFW 窗口出现，能清屏 | 纯色窗口 |
| 2. 画第一个三角形 | 硬编码顶点，手写光栅化，写像素到 Framebuffer | 一个三角形 |
| 3. 引入顶点着色器 | 顶点变换抽成可替换函数，加上 MVP 矩阵 | 三角形可移动/旋转 |
| 4. 引入片段着色器 | 像素颜色逻辑抽成可替换函数 | 彩色三角形 |
| 5. 扩展功能 | 纹理、深度测试等 | 更丰富的画面 |

### 代码模块划分

| 模块 | 文件 | 职责 |
|------|------|------|
| 基础类型 | `include/Types.hpp` | Color、Vertex 等基础数据结构和数学类型定义 |
| 帧缓冲 | `include/Framebuffer.hpp` `src/Framebuffer.cpp` | 像素缓冲区，支持清屏和读写像素 |
| 显示输出 | `include/Screen.hpp` `src/Screen.cpp` | GLFW 窗口管理，把 Framebuffer 贴到窗口 |
| 着色器 | `include/Shader.hpp` `src/Shader.cpp` | 顶点/片段着色器的接口和实现 |
| 管线 | `include/Pipeline.hpp` `src/Pipeline.cpp` | 把顶点→着色器→光栅化→帧缓冲串起来 |
| 入口 | `src/main.cpp` | 程序入口，主循环 |

### 为什么这么拆分

上一版只有 3 个头文件（Data.hpp、ComputeUnit.hpp、Screen.hpp），Data.hpp 塞了几百行，ComputeUnit.hpp 塞了五个类。当模块职责不清晰时：
- 改一个功能要在同一个文件里上下翻找
- 类之间的依赖关系模糊，不知道谁调用谁
- 新人（包括未来的自己）打开代码不知道从哪读起

按职责拆分后，每个文件只有一个核心职责。比如想看"像素怎么存"，直接打开 Framebuffer.hpp 就行。

---

## 阶段 1：搭骨架

**目标**：CMake 编译通过，GLFW 窗口弹出来，能清屏为纯色。

**为什么这是第一步**：
上一版的问题在于一次性写了几百行数据结构（DescriptorSet、DescriptorPool 等），但没有一个能跑的入口。这次反过来——先让程序跑起来，再逐步往里加东西。窗口和清屏是最小的可运行单元，后续所有渲染结果都通过它来看。

### 需要创建的文件

| 文件 | 职责 |
|------|------|
| `CMakeLists.txt` | 构建配置 |
| `include/Types.hpp` | 基础类型（本阶段只有 Color） |
| `include/Framebuffer.hpp` | Framebuffer 类声明 |
| `src/Framebuffer.cpp` | Framebuffer 实现 |
| `include/Screen.hpp` | Screen 类声明 |
| `src/Screen.cpp` | Screen 实现 |
| `src/main.cpp` | 程序入口 |

### 各模块规格

#### CMakeLists.txt
- 项目名 `shader_s_principle`，C++20
- 依赖：OpenGL (MODULE)、glfw3 (CONFIG)、glm (CONFIG)
- 注意：GLFW 的 CMake target 名可能是 `glfw` 也可能是 `glfw3`，取决于你系统上的包。先用 `glfw`，如果找不到就试 `glfw3`

#### Types.hpp
- 只放 `Color` 结构体：三个 `float` 成员 `r, g, b`
- 提供默认构造函数（初始化为黑色 0,0,0）和带参构造函数
- 使用 `#pragma once` 或传统的 include guard

#### Framebuffer
- 成员：`width`、`height`、像素存储（`std::vector<Color>`）
- 构造函数接收 `(int w, int h)`，分配 `w * h` 个像素
- `clear(Color c)` 方法：把所有像素设为颜色 c
- `setPixel(int x, int y, Color c)` 方法：在 (x, y) 位置写入颜色（将来光栅化用）
- `const std::vector<Color>& pixels() const`：只读访问像素数据（Screen 需要读它上传纹理）

#### Screen
- 构造函数接收 `(int width, int height)`
- 在构造函数中：初始化 GLFW、创建窗口、创建 OpenGL 纹理（用于把 CPU 像素贴到窗口上）
- `present(const Framebuffer& fb)`：把 Framebuffer 的像素上传到 OpenGL 纹理，画全屏四边形显示
- `bool isOpen() const`：检查窗口是否关闭
- `GLFWwindow* window()`：获取窗口句柄（主循环需要）
- 析构函数：清理 GLFW 资源

#### main.cpp
- 创建 `Screen(800, 600)` 和 `Framebuffer(800, 600)`
- 主循环：清屏 → present → 处理窗口事件
- 检查窗口创建是否成功，失败则返回 -1

### 关键注意事项

1. **OpenGL 纹理上传的数据格式**：`glTexImage2D` 期望 `GL_RGB` + `GL_FLOAT` 对应 `Color` 的 `{r, g, b}` 内存布局。因为 Color 是三个连续的 float，所以直接 `reinterpret_cast<const float*>(pixels.data())` 就可以传给 OpenGL。
2. **纹理坐标**：OpenGL 的纹理坐标原点在左下角，而屏幕/图像通常原点在左上角。如果贴出来画面上下颠倒，需要在四边形顶点里翻转 y 轴。
3. **上一版的 bug**：`Screen::present()` 里用的是成员变量 `frame_buffer` 而不是参数 `fb`。这次注意参数用起来。
4. **GLFW 初始化**：必须在创建窗口之前调用 `glfwInit()`，程序退出前调用 `glfwTerminate()`。

### 本阶段完成后项目的文件结构

```
shader-s_principle/
├── CMakeLists.txt
├── devlog.md
├── include/
│   ├── Types.hpp
│   ├── Framebuffer.hpp
│   └── Screen.hpp
└── src/
    ├── Framebuffer.cpp
    ├── Screen.cpp
    └── main.cpp
```

---
