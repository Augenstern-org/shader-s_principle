# 开发日志 — Shader's Principle

## 项目概述

用 C++20 在 CPU 上模拟 GPU 渲染管线（软渲染器）。使用 GLFW + OpenGL 仅作为显示输出，核心渲染逻辑全部在 CPU 端实现。

技术栈：CMake + C++20 + OpenGL + GLFW + GLM + GLAD

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
| 基础类型 | `include/Types.h` | Color、Vertex 等基础数据结构和数学类型定义 |
| 帧缓冲 | `include/Framebuffer.h` `src/Framebuffer.cpp` | 像素缓冲区，支持清屏和读写像素 |
| 显示输出 | `include/Screen.h` `src/Screen.cpp` | GLFW 窗口管理，把 Framebuffer 贴到窗口 |
| 着色器 | `include/Shader.h` `src/Shader.cpp` | 顶点/片段着色器的接口和实现 |
| 管线 | `include/Pipeline.h` `src/Pipeline.cpp` | 把顶点→着色器→光栅化→帧缓冲串起来 |
| 入口 | `src/main.cpp` | 程序入口，主循环 |

### 为什么这么拆分

上一版只有 3 个头文件（Data.hpp、ComputeUnit.hpp、Screen.hpp），Data.hpp 塞了几百行，ComputeUnit.hpp 塞了五个类。当模块职责不清晰时：
- 改一个功能要在同一个文件里上下翻找
- 类之间的依赖关系模糊，不知道谁调用谁
- 新人（包括未来的自己）打开代码不知道从哪读起

按职责拆分后，每个文件只有一个核心职责。比如想看"像素怎么存"，直接打开 Framebuffer.h 就行。

---

## 阶段 1：搭骨架

**目标**：CMake 编译通过，GLFW 窗口弹出来，能清屏为纯色。

**为什么这是第一步**：
上一版的问题在于一次性写了几百行数据结构（DescriptorSet、DescriptorPool 等），但没有一个能跑的入口。这次反过来——先让程序跑起来，再逐步往里加东西。窗口和清屏是最小的可运行单元，后续所有渲染结果都通过它来看。

### 需要创建的文件

| 文件 | 职责 |
|------|------|
| `CMakeLists.txt` | 构建配置（根目录 + src/ 子目录） |
| `include/Types.h` | 基础类型（本阶段只有 Color） |
| `include/Framebuffer.h` | Framebuffer 类声明 |
| `src/Framebuffer.cpp` | Framebuffer 实现 |
| `include/Screen.h` | Screen 类声明 |
| `src/Screen.cpp` | Screen 实现 |
| `src/main.cpp` | 程序入口 |

### 各模块规格

#### CMakeLists.txt
- 项目名 `shader-s_principle`，C++20
- 根 `CMakeLists.txt`：`include_directories(include)`，`add_subdirectory(src)`
- `src/CMakeLists.txt`：`add_executable` + `target_link_libraries`
- 依赖：OpenGL、glfw3（CONFIG）、glad（CONFIG）
- 注意：GLFW 的 CMake target 名可能是 `glfw` 也可能是 `glfw3`，取决于你系统上的包。如果找不到就两个都试试

#### Types.h
- 只放 `Color` 结构体：三个 `float` 成员 `r, g, b`
- 提供默认构造函数（初始化为黑色 0,0,0）和带参构造函数
- `Color` 的内存布局是连续的三个 float，可以直接当 `const float*` 传给 OpenGL
- 使用 `#pragma once` 或传统的 include guard

#### Framebuffer
- 成员：`width`（int）、`height`（int）、像素存储 `pixels`（`std::vector<Color>`）
- 构造函数接收 `(int w, int h)`，初始化列表里分配 `w * h` 个像素（默认黑色）
- `clear(Color c)`：把所有像素设为颜色 c（`pixels.assign(size, c)`）
- `setPixel(int x, int y, Color c)`：在 (x, y) 位置写入颜色。内部用 `pixels[y * width + x] = c` 做一维索引
- `getPixels()`：返回 `const std::vector<Color>&`，只读访问像素数据（Screen 需要读它上传纹理）
- `getWidth()`：返回帧缓冲宽度
- `getHeight()`：返回帧缓冲高度

#### Screen
- 成员：`width`、`height`、`window`（`GLFWwindow*`）、`title`（`const char*`）、`textureID`（`GLuint`）
- 构造函数接收 `(int w, int h, const char* title)`
- 在构造函数中：
  1. 调用 `glfwInit()` 初始化 GLFW
  2. `glfwCreateWindow(width, height, title, nullptr, nullptr)` 创建窗口
  3. `glfwMakeContextCurrent(window)` 设置 OpenGL 上下文
  4. **通过 GLAD 加载 OpenGL 函数**：`gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)`。GLAD 是一个 OpenGL 函数加载器，必须在调用任何 OpenGL 函数之前初始化。如果 GLAD 加载失败，销毁窗口并终止 GLFW
  5. `glGenTextures` + `glBindTexture` + `glTexParameteri` 创建一个 OpenGL 纹理，用于后续把 CPU 像素上传到 GPU 显示
  6. `glPixelStorei(GL_UNPACK_ALIGNMENT, 1)` 确保 RGB 三个 float（12 字节）的像素行不会被 OpenGL 做字节对齐填充
- `present(const Framebuffer& fb)`：把 Framebuffer 的像素通过 `glTexImage2D` 上传到纹理，然后画一个全屏四边形显示。纹理坐标原点在 OpenGL 中是左下角，而屏幕原点在左上角，所以四边形顶点需要翻转 y 轴——注释里标明了"左下/右下/右上/左上"
- `isOpen()`：返回 `!glfwWindowShouldClose(window)`，封装窗口关闭检查
- `getWindow()`：返回 `GLFWwindow*` 指针，供主循环使用
- 析构函数：调用 `glfwTerminate()` 清理资源

#### main.cpp
- 创建 `Screen screen(WIDTH, HEIGHT, "Shader's principle")` 和 `Framebuffer(WIDTH, HEIGHT)`
- 主循环：清屏 → present → `glfwPollEvents()` 处理窗口事件
- 条件用 `glfwWindowShouldClose(screen.getWindow())` 检测窗口关闭
- 检查窗口创建是否成功，失败则返回 -1

### 关键注意事项

1. **GLAD 的作用**：现代 OpenGL 的函数不是直接可用的——需要通过函数加载器（loader）在运行时获取函数指针。GLAD 就是这个加载器。`#include <glad/glad.h>` 必须在所有 OpenGL 相关 include 之前（包括 GLFW），否则会编译报错。OpenGL 本身只是一个规范，真正的函数实现在显卡驱动里，所以需要 GLAD 去"问"驱动要函数地址。
2. **OpenGL 纹理上传的数据格式**：`glTexImage2D` 期望 `GL_RGB` + `GL_FLOAT` 对应 `Color` 的 `{r, g, b}` 内存布局。因为 Color 是三个连续的 float，所以直接把 `getPixels().data()`（`const Color*`）传给 OpenGL 即可，内存布局天然匹配。不需要 reinterpret_cast。
3. **纹理坐标**：OpenGL 的纹理坐标原点在左下角，而屏幕/图像通常原点在左上角。如果贴出来画面上下颠倒，需要在四边形顶点里翻转 y 轴。本项目的 `present()` 已经处理好了——纹理坐标 y 值做了翻转。
4. **上一版的 bug**：上一版的 `Screen::present()` 里用的是成员变量而不是参数。这次注意参数 `fb` 用起来。
5. **GLFW 初始化**：必须在创建窗口之前调用 `glfwInit()`，程序退出前调用 `glfwTerminate()`。`glfwInit` 失败时 `glfwCreateWindow` 会返回 `nullptr`。
6. **构造函数失败处理**：Screen 的构造函数可能失败（GLFW 初始化失败、窗口创建失败、GLAD 加载失败）。失败时析构函数仍会调用 `glfwTerminate()`。目前的实现是静默处理——失败时 `window` 成员为 `nullptr`，主循环通过 `glfwWindowShouldClose` 检测到并退出。

### 本阶段完成后项目的文件结构

```
shader-s_principle/
├── CMakeLists.txt
├── devlog.md
├── plan.md
├── include/
│   ├── Types.h
│   ├── Framebuffer.h
│   └── Screen.h
└── src/
    ├── CMakeLists.txt
    ├── Framebuffer.cpp
    ├── Screen.cpp
    └── main.cpp
```

---

## 阶段 2：画第一个三角形

**目标**：在屏幕中央画一个白色的三角形。三个顶点坐标硬编码在 NDC 空间中，经视口变换后用手写光栅化算法写入 Framebuffer。

**为什么这是第二步**：
阶段 1 有了可运行的窗口和像素缓冲区，但屏幕上只有一个纯色背景——什么都没画。阶段 2 是整个软渲染器最核心的一步：**光栅化**。光栅化回答了"怎么把三个顶点变成一堆像素"这个问题，理解了它就理解了 GPU 渲染管线最关键的环节。

本阶段故意把顶点坐标和三角形颜色都硬编码，不引入着色器抽象。目的是让光栅化逻辑完整、清晰、可验证，后续阶段再逐步把硬编码的部分抽成可替换的函数。

### 本阶段的数据流

```
[3 个 NDC 顶点]  →  [视口变换]  →  [屏幕坐标顶点]  →  [光栅化]  →  [像素写入 Framebuffer]
    Vertex         viewport          vec2         边缘函数         setPixel()
```

整个数据流在 `Pipeline::drawTriangle()` 一个函数里完成。不引入着色器、不引入顶点缓冲区——一切都显式地写在一个函数里，方便理解和调试。

### 需要修改/创建的文件

| 文件 | 操作 | 职责 |
|------|------|------|
| `include/Types.h` | 修改 | 新增 `#include <glm/glm.hpp>`，新增 `Vertex` 结构体 |
| `include/Framebuffer.h` | 已在阶段 1 补齐 | `getWidth()`、`getHeight()` 已添加 |
| `include/Pipeline.h` | **新建** | Pipeline 类声明 |
| `src/Pipeline.cpp` | **新建** | Pipeline 实现：视口变换 + 光栅化 |
| `src/main.cpp` | 修改 | 硬编码三个顶点，调用 Pipeline 画三角形 |
| `CMakeLists.txt`（根） | 修改 | 新增 `find_package(glm CONFIG REQUIRED)` |
| `src/CMakeLists.txt` | 修改 | 新增 `Pipeline.cpp` 编译，新增 `glm::glm` 链接 |

### 各模块规格

#### Types.h 新增内容

引入 GLM 数学库，新增顶点类型。

- 新增 `#include <glm/glm.hpp>`
- 新增 `Vertex` 结构体：
  - 成员：
    - `glm::vec4 position` — 顶点在 NDC 空间的位置。NDC（归一化设备坐标）是渲染管线中的一个标准坐标空间，x 和 y 的有效范围是 [-1, 1]，分别对应屏幕的左/右边界和下/上边界。z 分量在阶段 2 暂不使用（填 0），w 分量固定为 1.0（表示这是一个点位置，不是方向向量）
  - 成员函数：
    - `Vertex()`：默认构造函数，position 初始化为 (0, 0, 0, 1)
    - `Vertex(const glm::vec4& pos)`：带参构造函数，直接设置 position

```cpp
// Vertex 内存布局示意
// |  x  |  y  |  z  |  w  |
// |  4 bytes each (float) |
// |  total = 16 bytes     |
```

#### Pipeline（新建）

Pipeline 是本阶段的**核心新模块**。它负责把三个顶点坐标转换为屏幕上的像素。

**为什么 Pipeline 是独立的类而不是写在 main 里**：
如果现在把光栅化逻辑写在 main.cpp 里，20 行代码能搞定。但阶段 3 引入顶点着色器、阶段 4 引入片段着色器后，管线逻辑会膨胀到上百行。届时再拆分会比现在麻烦——因为你在调试时已经习惯了"三角形代码在 main 里"这个假象。提前给管线一个独立的"家"，后续扩展只是往家里添家具，而不是搬家。

**头文件 `include/Pipeline.h`**：
- `class Pipeline`：使用 `#pragma once` 或 include guard
  - 需要 include `Types.h`（Vertex 类型）和 `Framebuffer.h`（Framebuffer 引用参数需要完整类型定义，不能只前向声明）
  - 公开方法：
    - `void drawTriangle(Framebuffer& fb, const Vertex& v0, const Vertex& v1, const Vertex& v2)`  
      接收一个 Framebuffer 引用和三个顶点，执行视口变换和光栅化，将三角形像素写入 Framebuffer
  - 注意：本阶段 Pipeline 没有成员变量。它可以是一个无状态的"函数对象"（其实就是把一堆函数打包在一个类里）。

**实现文件 `src/Pipeline.cpp`**：

需要的 include：
```cpp
#include "Pipeline.h"
#include "Framebuffer.h"   // Framebuffer::getWidth(), getHeight(), setPixel()
#include <glm/glm.hpp>     // glm::vec2, glm::vec4
#include <cmath>           // std::floor, std::ceil
#include <algorithm>       // std::min, std::max
```

包含两个静态辅助函数和一个公开方法。

- **静态函数** `glm::vec2 viewportTransform(const glm::vec4& ndc, int width, int height)`
  - 职责：将 NDC 坐标转换为屏幕像素坐标
  - 输入：ndc 顶点的 position（glm::vec4），帧缓冲的 width 和 height
  - 输出：屏幕空间的 2D 浮点坐标（glm::vec2）
  - 变换公式：
    ```
    screen_x = (ndc.x + 1.0) * 0.5 * width
    screen_y = (ndc.y + 1.0) * 0.5 * height
    ```
  - 解释：`(ndc + 1.0) * 0.5` 把 [-1, 1] 映射到 [0, 1]，再乘以宽/高映射到像素坐标。注意 ndc.y = -1 对应屏幕底部（screen_y = 0），ndc.y = 1 对应屏幕顶部（screen_y = height）
  - 返回类型是 float vec2（不是 int），保留子像素精度

- **静态函数** `float edgeFunction(const glm::vec2& a, const glm::vec2& b, const glm::vec2& p)`
  - 职责：计算点 p 相对于有向边 AB 的"边函数"值（2D 叉积）
  - 公式：`E(A, B, P) = (P.x - A.x) * (B.y - A.y) - (P.y - A.y) * (B.x - A.x)`
  - 几何含义：正值表示 P 在边 AB 的左侧，负值表示在右侧，零表示在边上
  - 这是光栅化的核心数学工具——用三次边函数就能判断一个像素是否在三角形内部

- **公开方法** `void Pipeline::drawTriangle(Framebuffer& fb, const Vertex& v0, const Vertex& v1, const Vertex& v2)`
  - 执行流程（5 步）：
    1. **视口变换**：调用 `viewportTransform` 将三个 NDC 顶点转为屏幕坐标 `p0, p1, p2`
    2. **计算包围盒**：取三个屏幕坐标的 min/max，得到包围三角形的最小矩形。包围盒夹到 `[0, width-1] × [0, height-1]`，防止访问越界
    3. **逐像素遍历**：双重 for 循环遍历包围盒内的每个像素
    4. **内部测试**：对每个像素中心 `(x + 0.5, y + 0.5)` 计算三条边的边函数值 `e0, e1, e2`。如果三者同号（全 ≥0 或全 ≤0），像素在三角形内部
    5. **写入像素**：内部像素调用 `fb.setPixel(x, y, Color(1,1,1))` 写入白色
  - 关键实现细节：
    - 像素采样点在中心 `(x + 0.5, y + 0.5)`，而非像素的整数角点。这是 GPU 的标准采样位置
    - 边函数比较用 `>= 0` 和 `<= 0`（包含等号），确保恰好落在边上的像素也会被绘制（避免相邻三角形之间出现缝隙）
    - 包围盒用 `std::floor` 和 `std::ceil` 处理，不遗漏边缘上的子像素部分

#### main.cpp 修改

相比阶段 1，main.cpp 增加了：

- 新增 `#include "Pipeline.h"`
- 在渲染循环之前定义三个顶点（NDC 坐标，形成一个位于屏幕中央的三角形）：
  ```cpp
  Vertex v0(glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f));  // 左下
  Vertex v1(glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f));  // 右下
  Vertex v2(glm::vec4( 0.0f,  0.5f, 0.0f, 1.0f));  // 上方中央
  ```
  这三个点构成一个底边水平的等腰三角形，位于屏幕正中央。为什么选这个形状：等腰三角形容易目测是否正确——两边对称，顶部尖角居中，如果代码有 bug（比如 x 方向偏移或 y 轴翻转）一眼就能看出来。
- 创建 `Pipeline pipeline;` 对象
- 渲染循环内变为三个步骤：
  ```cpp
  fb.clear(Color{0.1f, 0.3f, 0.5f});            // 1. 清屏（深蓝灰背景）
  pipeline.drawTriangle(fb, v0, v1, v2);        // 2. 画三角形（白色）
  screen.present(fb);                           // 3. 显示到窗口
  ```

#### CMakeLists.txt 修改

- 根 `CMakeLists.txt`：在 `find_package` 区域新增一行 `find_package(glm CONFIG REQUIRED)`
  - GLM 是纯头文件的数学库（只有 `.hpp`，不需要编译 .cpp），CMake 通过 CONFIG 模式找到它
- `src/CMakeLists.txt`：
  - `add_executable` 新增 `Pipeline.cpp`
  - `target_link_libraries` 新增 `glm::glm`

### 关键注意事项

1. **顶点绕序与边函数符号**：三个顶点的顺序决定了边函数值的符号。如果顶点按逆时针（CCW）排列（在屏幕坐标系，x 向右 y 向上），内部像素的三条边函数值全为正。如果按顺时针排列则全为负。本阶段用"同号判断"（全正或全负都算内部），所以两种绕序都能正确光栅化。但如果你没有检查"同号"而是只检查"全正"，换一种绕序就会什么都画不出来——排查时先检查顶点顺序。
2. **像素中心采样的原因**：边函数测试点选在 `(x + 0.5, y + 0.5)` 而不是 `(x, y)`。因为像素不是一个无限小的点，而是一个单位正方形，它的"代表位置"是中心。如果测试角点 `(x, y)`，三角形在像素角上"擦过"时会出现不自然的锯齿。这是 GPU 硬件的统一做法。
3. **包围盒越界保护**：视口变换产生的屏幕坐标可能落在 [0, width) × [0, height) 之外（比如三角形的某个顶点在屏幕外）。`setPixel` 不做越界检查，所以包围盒必须用 `std::max`/`std::min` 夹到有效范围内。忘记夹会导致 vector 越界访问，程序崩溃。
4. **NDC 坐标系与屏幕坐标系的对应**：
   - NDC：原点在屏幕中心，x 向右，y 向上。可见范围 [-1, 1]
   - 屏幕像素坐标：原点在左下角，(0,0) 是左下角第一个像素，(width-1, height-1) 是右上角最后一个像素
   - 视口变换后，NDC (-1, -1) → 屏幕左下，NDC (1, 1) → 屏幕右上。这和 OpenGL 的约定一致
5. **边函数退化情况**：如果三个顶点共线（在一条直线上），所有边函数值为 0，三角形退化，画不出任何像素。调试时如果三角形莫名消失，检查是不是顶点坐标写错了（比如三个点都在 x 轴上）。
6. **GLM 的坐标系约定**：GLM 默认使用和 OpenGL 一致的坐标系——右手坐标系，NDC 的 z 范围是 [-1, 1]。本阶段 z 分量还不参与计算，只需填 0 即可。关于 GLM 的更多细节（矩阵乘法顺序、透视投影等）会在阶段 3 用到 MVP 矩阵时详细展开。
7. **为什么着色器还不出现**：阶段 2 里顶点着色器的工作（NDC 坐标 → 什么都不做，因为顶点已经手动写在 NDC 了）和片段着色器的工作（返回白色）都硬编码在 Pipeline 里。当你手动改了三角形颜色或顶点位置，改动点在 main.cpp（顶点）和 Pipeline.cpp（颜色）两个不同的文件——这就是"着色器逻辑需要抽离"的信号。阶段 3 会把这个痛点变成引入着色器的动机。
8. **精度与浮点比较**：边函数用 `>= 0` 和 `<= 0` 做判断。由于视口变换和顶点坐标都是有限精度的浮点数，边函数结果也是浮点数。理论上恰好落在边上的像素（边函数 = 0 精确）很少，大部分情况足够精确。不需要引入 epsilon 容差——那会带来更隐蔽的 bug（比如相邻三角形之间出现一条像素缝）。

### 阶段 2 可选的验证方法

完成阶段 2 后，除了用眼睛看屏幕上的三角形，还可以做以下修改来验证光栅化逻辑正确：

- **改三角形颜色**：把 `Color(1,1,1)` 改成 `Color(1,0,0)`（红色），应该看到一个红色三角形
- **改顶点位置**：把 v2 的 y 改成 -0.2，三角形应该变扁甚至翻转
- **画在角落**：把三个顶点都加上 (0.7, 0.7)，三角形应该移到屏幕右上角
- **画超大三角形**：把坐标放大到超出 [-1, 1]，三角形应该被屏幕边缘正确裁剪（不会崩溃）

如果以上修改都得到预期结果，说明光栅化和包围盒裁剪逻辑都正确。

### 本阶段完成后项目的文件结构

```
shader-s_principle/
├── CMakeLists.txt
├── devlog.md
├── plan.md
├── include/
│   ├── Types.h
│   ├── Framebuffer.h
│   ├── Screen.h
│   └── Pipeline.h          ← 新建
└── src/
    ├── CMakeLists.txt
    ├── Framebuffer.cpp
    ├── Screen.cpp
    ├── Pipeline.cpp         ← 新建
    └── main.cpp
```

---

## 阶段 3：引入顶点着色器

**目标**：把顶点变换逻辑从 Pipeline 中抽离为可替换的着色器函数，引入 MVP 矩阵。三角形可以在屏幕上移动和旋转。

**为什么这是第三步**：
阶段 2 的光栅化已经能把三个 NDC 坐标变成屏幕上的像素了。但顶点坐标是硬编码在 NDC 空间的——你想让三角形旋转，就不得不手动修改三个顶点的 position 值。这相当于每次都手工做一遍顶点变换，极其低效。

真实 GPU 的做法是：你把顶点存在"模型空间"（也叫局部空间），然后 GPU 通过顶点着色器把它们变换到裁剪空间。这个变换用三个矩阵完成（MVP），对着色器来说只是做一次矩阵乘法。

阶段 3 引入顶点着色器后，三角形代码分为两部分：
- **不变的**：三个顶点在模型空间中的坐标（只定义一次，不需要动）
- **变化的**：MVP 矩阵（每帧可以修改，驱动三角形移动/旋转）

代码的修改点从"改顶点数据"变成了"改矩阵参数"——这正是 GPU 编程的核心思维。

### 本阶段的数据流

```
[模型空间顶点]  →  [顶点着色器]  →  [裁剪空间坐标]  →  [视口变换]  →  [屏幕坐标]  →  [光栅化]  →  [像素写入]
    Vertex         VertShader          vec4              viewport         vec2          边缘函数      setPixel()
                   (MVP 矩阵)
                      ↑
                 [Uniforms]
```

对比阶段 2 的数据流，唯一的变化是**顶点着色器**这一步——原本直接被视口变换消费的 NDC 坐标，现在先经过一次可替换的着色器函数处理。

### 核心概念：MVP 矩阵

在图形学中，把一个 3D 模型的顶点从"模型空间"变换到"裁剪空间（NDC）"需要经过三个矩阵：

| 矩阵 | 英文 | 变换 | 类比 |
|------|------|------|------|
| Model | 模型矩阵 | 局部空间 → 世界空间 | 把玩具从手里放到桌上（位置、旋转、缩放） |
| View | 视图矩阵 | 世界空间 → 相机空间 | 用相机对准桌上的玩具 |
| Projection | 投影矩阵 | 相机空间 → 裁剪空间 | 按下快门，3D 世界投影到 2D 照片 |

最终变换公式：

```
clip_pos = Projection * View * Model * local_pos
          ←——— 从右往左读 ———
```

`local_pos` 是 vec4，每个矩阵是 4×4，矩阵乘法从右往左依次施加：先 Model，再 View，最后 Projection。三位一体合称 MVP。

**阶段 3 的简化**：本阶段三角形仍然是 2D 的（所有顶点 z=0），Projection 和 View 暂设为单位矩阵（即"什么都不做"），只通过 Model 矩阵做旋转。这让 MVP 概念完整地出现在代码里，但实际只有 M 在干活。阶段 4-5 会逐步启用 V 和 P。

### 需要修改/创建的文件

| 文件 | 操作 | 职责 |
|------|------|------|
| `include/Shader.h` | **新建** | 顶点着色器接口 + Uniforms 定义 |
| `src/Shader.cpp` | **新建** | 内置顶点着色器实现 |
| `include/Pipeline.h` | 修改 | 新增 `setVertexShader()`，`drawTriangle()` 签名加入 `Uniforms` |
| `src/Pipeline.cpp` | 修改 | `drawTriangle()` 内部先调用顶点着色器，再视口变换 |
| `src/main.cpp` | 修改 | 创建 MVP 矩阵和顶点着色器，每帧更新旋转角度 |
| `src/CMakeLists.txt` | 修改 | 新增 `Shader.cpp` 编译 |

### 各模块规格

#### Shader.h（新建）

这是项目中第一个"接口型"模块——它定义着色器"是什么"（签名/契约），而不定义着色器"怎么做"（具体实现留给 .cpp 或 main 里的 lambda）。

需要的 include：
```cpp
#include <glm/glm.hpp>       // glm::mat4, glm::vec4
#include <functional>         // std::function
#include "Types.h"            // Vertex
```

包含一个结构体、一个类、一个命名空间。

---

**`Uniforms` 结构体** — 存放 MVP 三个矩阵

> 为什么叫 Uniforms：在 GPU 术语中，"uniform" 指的是一次 draw call 中所有顶点共享的数据。三个 MVP 矩阵对所有顶点都是一样的，所以放在 Uniforms 里。

成员：

| 成员 | 类型 | 含义 |
|------|------|------|
| `model` | `glm::mat4` | 模型矩阵：局部空间 → 世界空间 |
| `view` | `glm::mat4` | 视图矩阵：世界空间 → 相机空间 |
| `projection` | `glm::mat4` | 投影矩阵：相机空间 → 裁剪空间 |

成员函数：

| 函数 | 签名 | 说明 |
|------|------|------|
| 构造函数 | `Uniforms()` | 三个矩阵初始化为单位矩阵 `glm::mat4(1.0f)` |

单位矩阵的含义：`mat4(1.0f)` 乘以任何向量都等于原向量。所以默认构造的 Uniforms 会让顶点"原封不动"地通过。这作为安全的默认值——如果使用者忘了设置 MVP，顶点不会飞掉。

---

**`VertexShader` 类** — 顶点着色器的包装

> 为什么是类而不是裸 `std::function`：虽然顶点着色器本质上就是一个函数，但包一层类有几个好处：（1）给这个函数一个明确的名字和类型，而不是散落各处的 lambda；（2）阶段 4 引入 `FragmentShader` 时，两个类结构对称，Pipeline 的接口保持一致性；（3）将来如果要加成员（比如着色器名字、调试信息），不需要改调用方代码。

成员：

| 成员 | 类型 | 访问 | 说明 |
|------|------|------|------|
| `m_func` | `std::function<glm::vec4(const Vertex&, const Uniforms&)>` | private | 存储实际的着色器函数 |

类型别名（public，方便调用方书写）：

| 别名 | 定义 | 说明 |
|------|------|------|
| `ShaderFunc` | `std::function<glm::vec4(const Vertex&, const Uniforms&)>` | 顶点着色器函数签名：输入顶点 + Uniforms，输出裁剪空间 vec4 |

成员函数：

| 函数 | 签名 | 说明 |
|------|------|------|
| 默认构造函数 | `VertexShader()` | `m_func` 设为一个"直通"着色器：返回 `v.position`，即认为顶点已经在 NDC 空间。行为等价于阶段 2 的无着色器模式 |
| 带参构造函数 | `explicit VertexShader(ShaderFunc func)` | 用调用方传入的函数对象初始化 `m_func` |
| `process` | `glm::vec4 process(const Vertex& v, const Uniforms& u) const` | 调用 `m_func(v, u)`，执行着色器逻辑 |

---

**`BuiltinVertexShaders` 命名空间** — 内置的顶点着色器函数

| 函数 | 签名 | 说明 |
|------|------|------|
| `mvp` | `glm::vec4 mvp(const Vertex& v, const Uniforms& u)` | 标准 MVP 变换：`return u.projection * u.view * u.model * v.position;` |

这是一个普通函数（不是类），可以直接取地址传给 `VertexShader` 的构造函数：`VertexShader(BuiltinVertexShaders::mvp)`。

---

#### Shader.cpp（新建）

需要的 include：
```cpp
#include "Shader.h"
```

`Uniforms` 构造函数实现：
```cpp
Uniforms::Uniforms()
    : model(1.0f), view(1.0f), projection(1.0f) {}
```
三个矩阵都用 `glm::mat4(1.0f)` 初始化为单位矩阵。

`VertexShader` 默认构造函数实现：
```cpp
VertexShader::VertexShader()
    : m_func([](const Vertex& v, const Uniforms&) { return v.position; }) {}
```
默认行为是"直通"——假设顶点已经是 NDC 坐标，不做任何变换。这个默认值和阶段 2 的行为完全一致，保证向后兼容。

`VertexShader` 带参构造函数：
```cpp
VertexShader::VertexShader(ShaderFunc func)
    : m_func(std::move(func)) {}
```

`VertexShader::process`：
```cpp
glm::vec4 VertexShader::process(const Vertex& v, const Uniforms& u) const {
    return m_func(v, u);
}
```
一行转发，但这是关键的一行——Pipeline 通过 `process()` 获得变换后的顶点位置，而 `process()` 内部具体做了什么（MVP？直通？自定义动画？）Pipeline 完全不需要知道。

`BuiltinVertexShaders::mvp`：
```cpp
glm::vec4 BuiltinVertexShaders::mvp(const Vertex& v, const Uniforms& u) {
    return u.projection * u.view * u.model * v.position;
}
```
矩阵乘法顺序：Projection × View × Model × 顶点。注意矩阵乘法不满足交换律，顺序很重要。

---

#### Pipeline.h 修改

相比阶段 2，Pipeline 多了两个东西：一个成员变量和一个新的 setter 方法，`drawTriangle` 签名多了一个参数。

**新增 include**：
```cpp
#include "Shader.h"    // VertexShader, Uniforms
```

**新增成员**：

| 成员 | 类型 | 访问 | 说明 |
|------|------|------|------|
| `m_vertexShader` | `VertexShader` | private | 当前绑定的顶点着色器 |

**新增/修改的成员函数**：

| 函数 | 签名 | 说明 |
|------|------|------|
| `setVertexShader` | `void setVertexShader(const VertexShader& shader)` | 设置顶点着色器。调用时机：渲染循环开始前设置一次（或在需要切换着色器时调用） |
| `drawTriangle`（修改） | `void drawTriangle(Framebuffer& fb, const Vertex& v0, const Vertex& v1, const Vertex& v2, const Uniforms& uniforms)` | 相比阶段 2 新增最后一个参数 `uniforms`。内部流程变为：着色器变换 → 视口变换 → 光栅化 |

> 关于 `setVertexShader` 的设计：采用"先设置、后使用"的模式，而不是每次 `drawTriangle` 都传着色器函数。这是因为在真实 GPU 编程中，绑定着色器程序（`glUseProgram`）和发出绘制命令（`glDrawArrays`）是两个独立的步骤。这里的 `setVertexShader` 对应前者的概念。

**静态辅助函数**（在 Pipeline.cpp 中，不变）：
- `viewportTransform` — 签名和实现都不变，仍然是接收 NDC vec4 返回屏幕 vec2
- `edgeFunction` — 不变

---

#### Pipeline.cpp 修改

`drawTriangle` 方法的核心变化在步骤 1（视口变换之前），新增了"着色器处理"这一步。方法的执行流程从 5 步变为 6 步：

1. **顶点着色器处理**（新增）：对三个顶点分别调用 `m_vertexShader.process(v0, uniforms)`、`m_vertexShader.process(v1, uniforms)`、`m_vertexShader.process(v2, uniforms)`，得到三个裁剪空间坐标 `glm::vec4`。取每个 vec4 的 `.x` 和 `.y` 作为 NDC 坐标传给下一步
2. **视口变换**（同阶段 2）：将 NDC 坐标转为屏幕像素坐标 `p0, p1, p2`
3. **计算包围盒**（同阶段 2）
4. **逐像素遍历**（同阶段 2）
5. **内部测试**（同阶段 2）
6. **写入像素**（同阶段 2）

步骤 1 的具体代码：

```cpp
glm::vec4 clip0 = m_vertexShader.process(v0, uniforms);
glm::vec4 clip1 = m_vertexShader.process(v1, uniforms);
glm::vec4 clip2 = m_vertexShader.process(v2, uniforms);

glm::vec2 p0 = viewportTransform(clip0, fb.getWidth(), fb.getHeight());
glm::vec2 p1 = viewportTransform(clip1, fb.getWidth(), fb.getHeight());
glm::vec2 p2 = viewportTransform(clip2, fb.getWidth(), fb.getHeight());
```

注意 `viewportTransform` 的第一个参数从 `v.position`（vec4）变成了着色器返回的 `clip` 坐标（也是 vec4）。`viewportTransform` 函数内部只用了 `.x` 和 `.y`，所以着色器返回的 vec4 中只需 x、y 分量有意义即可。

---

#### main.cpp 修改

相比阶段 2，main.cpp 的核心变化是：**三角形的顶点不再写死在 NDC 空间，而是写在模型空间；通过每帧更新 MVP 矩阵来驱动三角形的运动。**

需要的额外 include：
```cpp
#include "Shader.h"                    // Uniforms, VertexShader, BuiltinVertexShaders
#include <glm/gtc/matrix_transform.hpp>  // glm::rotate
```

**创建着色器和 Uniforms**（在渲染循环之前）：

```cpp
// 创建 MVP 矩阵
Uniforms uniforms;
uniforms.model = glm::mat4(1.0f);       // 初始为单位矩阵（之后每帧会被覆盖）
uniforms.view = glm::mat4(1.0f);        // 阶段 3 暂用单位矩阵
uniforms.projection = glm::mat4(1.0f);  // 阶段 3 暂用单位矩阵

// 创建顶点着色器，使用内置的 MVP 变换
VertexShader vertexShader(BuiltinVertexShaders::mvp);

// 把着色器绑定到管线
pipeline.setVertexShader(vertexShader);
```

**渲染循环内的变化**：

```cpp
while (!glfwWindowShouldClose(screen.getWindow())) {
    // 用时间驱动旋转角度
    float angle = (float)glfwGetTime();  // glfwGetTime() 返回程序启动到现在的秒数
    uniforms.model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));

    fb.clear(Color{0.1f, 0.3f, 0.5f});
    pipeline.drawTriangle(fb, v0, v1, v2, uniforms);  // 多传了一个 uniforms 参数
    screen.present(fb);
    glfwPollEvents();
}
```

`glm::rotate` 的三个参数：
- `glm::mat4(1.0f)`：从单位矩阵开始
- `angle`：旋转角度（弧度）。`glfwGetTime()` 返回的秒数作为弧度值，大约每 6.28 秒转完一圈
- `glm::vec3(0, 0, 1)`：绕 Z 轴旋转。在 2D 屏幕上，绕 Z 轴旋转就是"纸面上转圈"

顶点定义不变（和阶段 2 相同）：
```cpp
Vertex v0(glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f));  // 左下
Vertex v1(glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f));  // 右下
Vertex v2(glm::vec4( 0.0f,  0.5f, 0.0f, 1.0f));  // 上方中央
```

这些坐标现在被理解为"模型空间"中的坐标。当 model 矩阵是单位矩阵时，它们恰好等于 NDC 坐标（因为 view 和 projection 也是单位矩阵）。

---

#### src/CMakeLists.txt 修改

`add_executable` 新增 `Shader.cpp`：

```cmake
add_executable(test main.cpp
        Screen.cpp
        Framebuffer.cpp
        Pipeline.cpp
        Shader.cpp)
```

无需新增链接库——Shader 只依赖 GLM（已在链接列表中）。

根 `CMakeLists.txt` 无需修改。

### 关键注意事项

1. **矩阵乘法顺序**：`projection * view * model * vertex`，从右往左读。写反了（比如 `vertex * model * view * projection`）编译会报错或运行时得到错误结果。GLM 使用列主序（column-major）存储，和 OpenGL 的约定一致，矩阵乘法符号 `*` 的行为也和数学定义一致。
2. **`glfwGetTime()` 返回秒数**：值从 0 开始，每秒增加 1。直接用作弧度的话，角速度是 1 rad/s ≈ 57°/s，大约 6.28 秒旋转一圈。如果你想更快，乘以一个系数：`angle = (float)glfwGetTime() * 2.0f` 则每秒转 2 弧度。
3. **默认着色器是"直通"的**：如果创建 Pipeline 后忘了调用 `setVertexShader()`，默认的 `VertexShader` 会直接返回 `v.position`（认为顶点已经是 NDC 坐标）。这不会崩溃，但你的 MVP 旋转不会生效——三角形静止不动。排查"三角形不转"问题时，先检查 `setVertexShader` 是否被调用。
4. **NDC 的 z 分量现阶段是 0**：MVP 变换后，着色器返回的 vec4 包含 (x, y, z, w) 四个分量。阶段 3 的光栅化只看 x 和 y，z 和 w 暂时闲置。本阶段 w 始终为 1（因为没有透视投影），所以不需要做透视除法。阶段 5 引入深度测试时 z 会派上用场。
5. **`std::function` 的开销**：`std::function` 内部使用了类型擦除（type erasure），调用时比裸函数指针多一次间接跳转。对于本项目的规模（每帧只处理 3 个顶点），这个开销完全可以忽略。如果未来每帧要处理上百万个顶点，可以换成模板或函数指针。
6. **include 顺序**：`Shader.h` 需要 `glm/glm.hpp`（mat4、vec4）和 `<functional>`（std::function）。虽然 `Types.h` 已经 include 了 `glm/glm.hpp`，但 Shader.h 最好自己也 include——这遵守"不依赖间接 include"的原则。万一将来 Types.h 不再需要 glm，Shader.h 不会跟着坏掉。
7. **为什么 Pipeline.cpp 里的 viewportTransform 和 edgeFunction 不需要改**：因为着色器抽离只影响"顶点从模型空间到 NDC"这一步。视口变换和光栅化的输入仍然是 NDC 坐标，所以它们完全不变。这是低耦合的体现——改了着色器模块，光栅化模块不受影响。

### 阶段 3 可选的验证方法

完成阶段 3 后，除了看三角形旋转，还可以做以下修改来验证着色器系统正确：

- **改变旋转方向**：把 `glm::vec3(0, 0, 1)` 改成 `glm::vec3(0, 0, -1)`，三角形应该反向旋转
- **改旋转速度**：把 `angle` 乘以 2.0 或 0.5，转速应相应变化
- **绕 X 轴旋转**：改成 `glm::vec3(1, 0, 0)`，三角形应该绕水平轴"翻面"（在 2D 投影下表现为上下压缩-翻转）
- **加位移**：`uniforms.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.3f, 0.0f, 0.0f))`（不旋转），三角形应该整体右移
- **自定义着色器**：不用 `BuiltinVertexShaders::mvp`，改用 lambda 写一个自定义变换：
  ```cpp
  VertexShader customShader([](const Vertex& v, const Uniforms& u) {
      // 把三角形向右上角推
      glm::vec4 pos = v.position;
      pos.x += 0.3f;
      pos.y += 0.3f;
      return u.projection * u.view * u.model * pos;
  });
  pipeline.setVertexShader(customShader);
  ```
  三角形应该偏移到右上角。这验证了着色器的"可替换性"——换一个函数就换一种变换，Pipeline 代码一行不用改。

- **故意写反矩阵乘法**：把 `mvp` 的实现改成 `v.position * u.model * u.view * u.projection`（GLM 编译不过，因为 vec4 × mat4 和 mat4 × vec4 是不同的运算）。这个错误帮你理解矩阵乘法不满足交换律。

### 本阶段完成后项目的文件结构

```
shader-s_principle/
├── CMakeLists.txt
├── devlog.md
├── plan.md
├── include/
│   ├── Types.h
│   ├── Framebuffer.h
│   ├── Screen.h
│   ├── Pipeline.h
│   └── Shader.h              ← 新建
└── src/
    ├── CMakeLists.txt
    ├── Framebuffer.cpp
    ├── Screen.cpp
    ├── Pipeline.cpp
    ├── Shader.cpp             ← 新建
    └── main.cpp
```

---

## 阶段 4：引入片段着色器 + 重心坐标插值

**日期**：2026-05-26（规划）  
**状态**：[ ] 待实现

**目标**：把像素颜色逻辑从 Pipeline 中抽离为可替换的片段着色器，引入重心坐标插值机制，实现彩色三角形。

**为什么这是第四步**：
阶段 3 的三角形虽然在旋转，但始终是纯白色。真实 GPU 在光栅化时会做一件非常重要的事——**插值（interpolation）**。顶点着色器输出的逐顶点数据（颜色、法线、UV），在三角形内部的每个像素位置会被加权平均。这个加权平均用的就是重心坐标（barycentric coordinates）。阶段 4 让学习者理解两个概念：（1）片段着色器是逐像素的可替换逻辑，和顶点着色器对称；（2）重心坐标插值是连接顶点着色器和片段着色器的"胶水"。

### 本阶段的数据流

```
[Vertex(+color)] → [VertexShader] → [VertexOutput(pos+color)] → [viewportTransform] → [screen pos + edge functions] → [barycentric interpolation] → [FragmentInput] → [FragmentShader] → [Color] → [Framebuffer::setPixel]
```

对比阶段 3，增加了三个新环节：
- `VertexOutput`：顶点着色器现在输出一个结构体而不仅是 vec4 位置
- **重心坐标插值**：三个顶点的颜色在像素位置按权重混合
- `FragmentShader`：接收插值后的 `FragmentInput`，输出最终颜色

### 核心概念：重心坐标插值

三个边函数值 `e1, e2, e3` 已经隐含了重心坐标。对于三角形内部的一个像素 P：

```
area = e1 + e2 + e3                // 三角形总面积的 2 倍
w0 = e2 / area                     // 顶点 v0 的权重（对边是 p1→p2，对应 e2）
w1 = e3 / area                     // 顶点 v1 的权重（对边是 p2→p0，对应 e3）
w2 = e1 / area                     // 顶点 v2 的权重（对边是 p0→p1，对应 e1）
```

三个权重满足 `w0 + w1 + w2 = 1`，且都在 [0, 1] 范围内（当 P 在三角形内部时）。

任意逐顶点属性 X 的插值：`X_interpolated = w0 * X0 + w1 * X1 + w2 * X2`

**为什么边函数值直接就是重心坐标？** 因为边函数 `edgeFunction(A, B, P)` 计算的是三角形 ABP 的面积（的 2 倍）。对于三角形 ABC 内部的点 P，P 把大三角形分成三个子三角形：PBC、APC、ABP。这三个子三角形的面积之比，就是 P 到三个顶点的"权重"。而 `e1 = area(P, p1, p2)` 正是子三角形 P-p1-p2 的面积——因此 `e1` 正比于对面顶点 p0 的权重 w0……等一下，对照关系是：`e1 = edgeFunction(p0, p1, P)` → 这是以 p1-p0 为底边的子三角形面积，对应顶点 p2。所以 w2 = e1/area, w0 = e2/area, w1 = e3/area。

**为什么不需要透视校正插值（perspective-correct interpolation）？** 因为阶段 4 的三角形仍是纯 2D 的（所有顶点 z=0，projection 矩阵是单位矩阵），没有透视投影带来的深度差异。在正交投影或纯 2D 场景中，屏幕空间的线性插值就是正确的。阶段 5 引入真正的透视投影后，才需要透视校正。

### 需要修改/创建的文件

| 文件 | 操作 | 职责 |
|------|------|------|
| `include/Types.h` | 修改 | Vertex 新增 `color` 属性；新增 `VertexOutput`、`FragmentInput` 结构体 |
| `include/Shader.h` | 修改 | `VertexShader` 返回类型从 `glm::vec4` 改为 `VertexOutput`；新增 `FragmentShader` 类；新增 `BuiltinFragmentShader` 命名空间 |
| `src/Shader.cpp` | 修改 | 所有顶点着色器函数返回 `VertexOutput`；实现 `FragmentShader` 默认构造 + `passThrough` |
| `include/Pipeline.h` | 修改 | 新增 `FragmentShader m_fragmentShader` 成员 + `setFragmentShader()` |
| `src/Pipeline.cpp` | 修改 | `drawTriangle` 中保存 `VertexOutput`；计算重心坐标；插值颜色；调用片段着色器写入结果 |
| `src/main.cpp` | 修改 | 顶点构造携带颜色；创建并绑定 `FragmentShader` |

无新建文件——所有改动都在现有模块内。

### 各模块规格

#### Types.h 修改

**Vertex 结构体新增 `color` 字段**：

| 成员 | 类型 | 说明 |
|------|------|------|
| `position` | `glm::vec4` | 模型空间位置（已有，不变） |
| `color` | `Color` | **新增**，逐顶点颜色属性，默认值白色 (1,1,1) |

新增构造器 `Vertex(const glm::vec4& pos, Color c)`，允许在构造时直接指定颜色。默认颜色为白色，保证现有不传颜色的代码行为不变。

**VertexOutput 结构体（新增）**：

| 成员 | 类型 | 说明 |
|------|------|------|
| `position` | `glm::vec4` | 裁剪空间位置（等价于 GLSL 的 `gl_Position`） |
| `color` | `Color` | 逐顶点颜色（将由光栅化器插值） |

> 为什么需要 VertexOutput：阶段 3 的顶点着色器只返回 vec4 位置，因为那时没有"需要在三角形表面变化的数据"。从阶段 4 开始，顶点着色器需要同时输出位置和颜色（将来还有 UV 坐标等），VertexOutput 是这些输出的容器。它对应 GLSL 中顶点着色器的所有 `out` 变量 + `gl_Position` 的集合。

**FragmentInput 结构体（新增）**：

| 成员 | 类型 | 说明 |
|------|------|------|
| `color` | `Color` | 重心坐标插值后的颜色 |

> 为什么需要 FragmentInput：它是片段着色器的输入参数。和 VertexOutput 分开的原因是——VertexOutput 含 position（片段着色器不需要），且 VertexOutput 是"逐顶点"的、FragmentInput 是"插值后"的。语义不同，分开命名避免混淆。在真实 GLSL 中，这就是顶点着色器的 `out` 变量对应片段着色器的 `in` 变量——类型相同但值已经是插值后的。

#### Shader.h 修改

**VertexShader 变更**：
- 返回类型从 `glm::vec4` 改为 `VertexOutput`
- `ShaderFunc` 类型别名对应更新：`std::function<VertexOutput(const Vertex&, const Uniforms&)>`
- `process()` 返回 `VertexOutput` 而非 `glm::vec4`

**FragmentShader 类（新增）**：

| 成员 | 类型/签名 | 说明 |
|------|-----------|------|
| `m_func` | `std::function<Color(const FragmentInput&, const Uniforms&)>` | 存储片段着色器函数 |
| `ShaderFunc` (public) | `std::function<Color(const FragmentInput&, const Uniforms&)>` | 类型别名，方便调用方书写 |
| 默认构造函数 | `FragmentShader()` | 返回插值颜色的直通着色器 |
| 带参构造函数 | `explicit FragmentShader(ShaderFunc f)` | 用自定义函数初始化 |
| `process` | `Color process(const FragmentInput&, const Uniforms&) const` | 调用着色器函数 |

> 为什么 FragmentShader 也要接收 Uniforms：真实 GLSL 中 fragment shader 可以读取 uniform 变量（比如光照参数、时间等）。传入 Uniforms 让片段着色器也能访问全局状态——学习者可以在片段着色器中用时间做动态颜色效果（如呼吸灯、色彩循环等）。

**BuiltinFragmentShader 命名空间（新增）**：

| 函数 | 签名 | 说明 |
|------|------|------|
| `passThrough` | `Color passThrough(const FragmentInput& in, const Uniforms& u)` | 原样返回插值后的颜色。这是最基础的片段着色器——"顶点给我什么颜色，我就显示什么颜色" |

#### Shader.cpp 修改

**所有顶点着色器函数需要更新返回类型**：

- 默认构造：`[](const Vertex& v, const Uniforms&) -> VertexOutput { return {v.position, v.color}; }`
- `BuiltinVertexShader::mvp`：返回 `VertexOutput{u.projection * u.view * u.model * v.position, v.color}` —— MVP 变换位置，原样传递顶点颜色
- `BuiltinVertexShader::test`：位置偏移后，颜色原样传递

**FragmentShader 默认构造函数**：
```cpp
FragmentShader::FragmentShader()
    : m_func([](const FragmentInput& in, const Uniforms&) { return in.color; }) {}
```

**FragmentShader::process**：
```cpp
Color FragmentShader::process(const FragmentInput& in, const Uniforms& u) const {
    return m_func(in, u);
}
```

**BuiltinFragmentShader::passThrough**：
```cpp
Color BuiltinFragmentShader::passThrough(const FragmentInput& in, const Uniforms& u) {
    return in.color;
}
```

`passThrough` 接受 `const Uniforms&` 但不使用——保留参数是为了和 `ShaderFunc` 签名匹配。如果学习者想在 passThrough 中加入时间效果，只需修改函数体即可。

#### Pipeline.h 修改

**新增成员**：
| 成员 | 类型 | 说明 |
|------|------|------|
| `m_fragmentShader` | `FragmentShader` | 当前绑定的片段着色器 |

**新增方法**：
| 方法 | 签名 | 说明 |
|------|------|------|
| `setFragmentShader` | `void setFragmentShader(const FragmentShader&)` | 设置片段着色器 |

`drawTriangle` 签名不变。

#### Pipeline.cpp 修改

`drawTriangle` 的核心改动在顶点着色器处理和像素写入两个环节。

**步骤 1（顶点着色器处理）变更**——保存完整的 VertexOutput：
```cpp
VertexOutput out0 = m_vertexShader.process(v0, uniforms);
VertexOutput out1 = m_vertexShader.process(v1, uniforms);
VertexOutput out2 = m_vertexShader.process(v2, uniforms);

glm::vec2 p0 = viewportTransform(out0.position, fb.getWidth(), fb.getHeight());
glm::vec2 p1 = viewportTransform(out1.position, fb.getWidth(), fb.getHeight());
glm::vec2 p2 = viewportTransform(out2.position, fb.getWidth(), fb.getHeight());
```

**新增退化三角形检测**（在包围盒计算之前）：
```cpp
float area = edgeFunction(p0, p1, p2);
if (area == 0.0f) return;  // 三点共线，退化三角形
```

**步骤 6（像素写入）变更**——用重心坐标插值 + 片段着色器替代硬编码颜色：
```cpp
// 计算重心坐标
float w0 = e2 / area;  // 顶点 0 的权重
float w1 = e3 / area;  // 顶点 1 的权重
float w2 = e1 / area;  // 顶点 2 的权重

// 插值逐顶点颜色
Color interpColor(
    w0 * out0.color.r + w1 * out1.color.r + w2 * out2.color.r,
    w0 * out0.color.g + w1 * out1.color.g + w2 * out2.color.g,
    w0 * out0.color.b + w1 * out1.color.b + w2 * out2.color.b
);

// 调用片段着色器
Color finalColor = m_fragmentShader.process(FragmentInput(interpColor), uniforms);

fb.setPixel(x, y, finalColor);
```

**关键细节**：
- 重心坐标权重的对应关系：`e1 = edgeFunction(p0, p1, P)` → w2（顶点 p2 的对边是 p0→p1，所以 e1 对应 p2 的权重）；e2 → w0；e3 → w1
- `area` 仅需计算一次（三角形总面积不变），三个边函数 `e1, e2, e3` 在每个像素不同
- `FragmentInput` 在调用点直接构造
- 片段着色器可能会修改颜色，Pipeline 不假设它会原样返回——这正是可替换性的核心

#### main.cpp 修改

**顶点定义增加颜色**：
```cpp
Vertex v0(glm::vec4(-0.577f, -0.333f, 0.0f, 1.0f), Color(1, 0, 0)); // 左下 红色
Vertex v1(glm::vec4( 0.577f, -0.333f, 0.0f, 1.0f), Color(0, 1, 0)); // 右下 绿色
Vertex v2(glm::vec4( 0.0f,  0.666f, 0.0f, 1.0f), Color(0, 0, 1));   // 上方 蓝色
```

**创建并绑定片段着色器**（在渲染循环之前）：
```cpp
FragmentShader fragShader(BuiltinFragmentShader::passThrough);
pipeline.setFragmentShader(fragShader);
```

主循环不变——三角形继续旋转，但现在以渐变彩色呈现。

### 关键注意事项

1. **重心坐标的权重对应关系容易搞混**：边函数 `e1 = edgeFunction(p0, p1, P)` 对应的是子三角形 P-p0-p1 的面积，该面积正比于 P 到对面顶点 p2 的距离，所以 e1 对应 w2。完整映射：w0 = e2/area, w1 = e3/area, w2 = e1/area。验证方式——如果像素恰好落在 p0 上，P → p0，e1 ≈ 0（P 在边 p0→p1 上），e3 ≈ 0（P 在边 p2→p0 上），只有 e2 ≠ 0（P 不在边 p1→p2 上）。此时 w0=e2/area ≈ 1，w1≈0，w2≈0，权重全部正确落在 p0 上。

2. **退化三角形的 area 可能为负**：如果顶点按顺时针排列，`edgeFunction(p0, p1, p2)` 返回负值。像素内部测试用 `||` 同时接受正负绕序。归一化时 `e/area` 中 e 和 area 同号，结果始终为正。唯一需要注意的是 `area == 0` 判断——三点共线时所有边函数值都为 0（或接近 0），用 `fabs(area) < 1e-8` 判断更安全。

3. **顶点着色器返回的 VertexOutput.color 是"逐顶点"的**：如果顶点着色器不传 `v.color` 而是返回一个固定颜色（如红色），三个顶点输出相同的红色，插值后还是红色，整个三角形会是纯色。这不是 bug——这就是"uniform 颜色"的效果。

4. **Color 的线性插值是在 RGB 空间进行的**：RGB 不是感知均匀的色彩空间，所以渐变看起来不一定"均匀"。这是正确的物理行为——GPU 也是在 RGB 空间做线性插值。

5. **片段着色器在插值之后运行**：顶点颜色先被插值，片段着色器接收到的是一个"已经平滑好"的颜色值。这对应 Gouraud 着色（逐顶点光照 + 插值颜色）。Phong 着色（逐像素光照）需要在片段着色器中拿到插值后的法线向量，然后在片段着色器内部做光照计算——阶段 5 可扩展至此。

### 预期可见输出

运行后将看到一个**旋转的彩色三角形**：三个顶点分别为红、绿、蓝色，三角形内部是平滑的颜色渐变。这是重心坐标插值最直观的展示。

### 阶段 4 可选的验证方法

- **改顶点颜色**：把 v2 的颜色从蓝色改为黄色 `Color(1, 1, 0)`，三角形渐变色应相应变化
- **纯色三角形**：把片段着色器改为 `[](const FragmentInput&, const Uniforms&) { return Color(1, 0, 0); }`，三角形应变为纯红色——验证片段着色器完全控制了颜色
- **亮度减半**：
  ```cpp
  FragmentShader halfBright([](const FragmentInput& in, const Uniforms&) {
      return Color(in.color.r * 0.5f, in.color.g * 0.5f, in.color.b * 0.5f);
  });
  ```
  三角形整体变暗但渐变仍在——验证片段着色器在插值之后还能再加工颜色
- **故意调换权重**：把 `w0, w1, w2` 的计算搞乱（如 w0=e1/area, w1=e2/area, w2=e3/area），渐变方向会错位——验证重心坐标对应关系的正确性
- **逐顶点颜色 vs 固定颜色**：把顶点着色器改为返回固定颜色 `Color(1,0.5,0)`（而非传递 `v.color`），三个顶点输出相同颜色，三角形变为纯橙色——理解"varying 为常量"和"varying 随顶点变化"的区别

### 本阶段完成后的文件结构

（无新增文件）

```
shader-s_principle/
├── CMakeLists.txt
├── devlog.md
├── plan.md
├── include/
│   ├── Types.h          ← 修改（Vertex +color, +VertexOutput, +FragmentInput）
│   ├── Framebuffer.h
│   ├── Screen.h
│   ├── Pipeline.h       ← 修改（+m_fragmentShader, +setFragmentShader）
│   └── Shader.h         ← 修改（VertexShader 返回类型变更, +FragmentShader, +BuiltinFragmentShader）
└── src/
    ├── CMakeLists.txt
    ├── Framebuffer.cpp
    ├── Screen.cpp
    ├── Pipeline.cpp      ← 修改（重心坐标 + 调片段着色器）
    ├── Shader.cpp        ← 修改（顶点着色器返回类型, +FragmentShader 实现）
    └── main.cpp          ← 修改（顶点带颜色, +fragShader）
```
