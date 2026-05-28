# 拓展开发日志 — Shader's Principle

## 概述

原型的 5 个阶段已经完成了一个可用的 CPU 软渲染器管线。拓展阶段的目标是把它从一个"技术演示"升级为一个"可交互的渲染工具"——加载模型、操作相机、贴纹理、加光照，最终能跑`oiioi`。

核心原则延续原型阶段：

> **每一步都要有可见的输出，只引入当前需要的抽象。**
> **先重构搭好骨架，再往骨架上挂功能。**

---

## 拓展总规划

| 阶段 | 目标 | 可见结果 |
|------|------|----------|
| 重构. 整理骨架 | 拆分职责，引入 Camera / Renderer / Rasterizer / Control 模块，暴露清晰 API | 行为和现在完全一致，main.cpp 组装逻辑清晰 |
| 6. 索引几何 + 背面剔除 | Mesh 支持索引缓冲区，背面三角形自动跳过 | 三角形背对相机时消失，帧率提升 |
| 7. 模型加载 | .obj 文件解析器，加载外部模型 | 屏幕上的三角形变成小猫模型 |
| 8. 纹理映射 | UV 插值 + 纹理采样 + 图片加载 | 小猫身上有贴图颜色 |
| 9. 光照 | 逐像素漫反射 + 镜面高光 | 小猫有明暗立体感 |

---

## 为什么重构放在第一位

当前代码的问题不是"功能不够"，而是"结构不好改"。举两个例子：

- **没有 Camera 概念**：view 和 projection 矩阵在 main.cpp 里手算。想加鼠标旋转，就得在 main.cpp 里加 GLFW 回调、维护旋转状态、每帧重算矩阵——main 会膨胀到 200 行。
- **Pipeline 是上帝函数**：`drawTriangle` 做了着色器调用、透视除法、视口变换、光栅化、深度测试、片段着色。没有哪一步能独立复用。

重构不添加任何新渲染功能，只做一件事：**把现有功能重新分配到合理的模块里**。外面看行为不变，里面看架构清晰。接下来的纹理、光照、模型加载都直接受益于这次整理。

---

## 阶段 重构：整理骨架 + API 设计

**日期**：2026-05-26（规划），2026-05-27 ~ 2026-05-28（实现）  
**状态**：[x] 已实现

**目标**：拆分 `Pipeline::drawTriangle` 的上帝函数，新增 Camera、Renderer 模块，建立清晰的 API 边界。让 main.cpp 变成薄薄一层胶水代码。

### 需要新建的模块

| 模块 | 文件 | 职责 |
|------|------|------|
| Camera | `include/Camera.h`, `src/Camera.cpp` | 轨道相机：旋转（arcball）、平移、缩放。处理鼠标/键盘输入，生成 view + projection 矩阵 |
| Renderer | `include/Renderer.h`, `src/Renderer.cpp` | 渲染器门面：持有 Pipeline + Framebuffer + DepthBuffer，提供 `beginFrame` / `draw` / `endFrame` 三个高层接口 |
| Rasterizer | `include/Rasterizer.h`, `src/Rasterizer.cpp` | 纯光栅化逻辑：从 `drawTriangle` 中抽离的包围盒计算 + 边函数 + 重心坐标 + 透视校正插值 + 深度测试 + 像素写入 |

### 需要修改的模块

| 模块 | 改动 |
|------|------|
| Pipeline | `drawTriangle` 内部的光栅化循环移到 Rasterizer，Pipeline 变成流程编排层（着色器 → 透视除法 → 视口变换 → 调 Rasterizer） |
| main.cpp | 大幅精简：创建 Renderer + Camera，设置回调，跑主循环。目标 ~30 行 |

### 实现顺序

模块之间的依赖关系决定了实现顺序——Camera 和 Screen 都是被 main.cpp 使用的独立模块，可以先写；Rasterizer 被 Pipeline 依赖；Pipeline 被 Renderer 依赖。

| 步骤 | 创建/修改 | 原因 |
|------|----------|------|
| 1 | 新建 `Camera.h` + `Camera.cpp` | 不依赖项目内任何模块，只依赖 GLM。独立开发、独立验证 |
| 2 | 新建 `Rasterizer.h` + `Rasterizer.cpp` | 依赖 Types / Framebuffer / DepthBuffer / Shader，都是已有模块。可独立测试光栅化逻辑 |
| 3 | 修改 `Pipeline.h` + `Pipeline.cpp` | 移除光栅化循环，改为调用 Rasterizer。`drawTriangle` 从 ~80 行缩到 ~20 行 |
| 4 | 新建 `Renderer.h` + `Renderer.cpp` | 依赖 Pipeline + Framebuffer + DepthBuffer + Screen。组合现有模块为高层门面 |
| 5 | 修改 `main.cpp` | 删除手写的 view/projection 矩阵和所有底层对象的创建，改为使用 Camera + Renderer |
| 6 | 修改 `src/CMakeLists.txt` | `add_executable` 新增 `Camera.cpp`、`Rasterizer.cpp`、`Renderer.cpp` |

每一步完成后都可以编译通过——这是一个"增量重构"策略：每步改动小，方便定位编译错误。

### 重构前后的依赖图对比

**重构前**：

```
Types.h  (Color, Vertex, VertexOutput, FragmentInput)
   ↑
Framebuffer   Shader.h   Mesh.h   DepthBuffer.h
   ↑              ↑        ↑          ↑
Screen         Pipeline
   ↑              ↑
   main.cpp (组装一切，手算 view/projection)
```

main.cpp 承担了过多职责：创建所有对象、手算相机矩阵、设置回调、运行主循环。当需要加鼠标交互时，所有改动都堆在 main.cpp 里，它会膨胀到 200+ 行。

**重构后**：

```
Types.h  (Color, Vertex, VertexOutput, FragmentInput)
   ↑
Framebuffer   Shader.h   Mesh.h   DepthBuffer.h   Camera (只依赖 GLM)
   ↑              ↑        ↑           ↑                    ↑
Screen         Rasterizer              |                    |
   ↑              ↑                    |                    |
   |           Pipeline ←──────────────┘                    |
   |              ↑                                         |
   |       Renderer (Facade -> Pipeline + FB + DB)          |
   |              ↑                                         |
   └──────────────┼─────────────────────────────────────────┘
                  ↑
               main.cpp (薄胶水层，~30 行)
```

关键变化：
- **Camera**：被 main.cpp 直接使用，不依赖任何项目模块。生成 view/projection 矩阵，处理鼠标/键盘输入
- **Rasterizer**：从 Pipeline 中抽离，承担包围盒 + 边函数 + 插值 + 深度测试 + 像素写入
- **Pipeline**：从 ~80 行的"上帝函数"退化为 ~20 行的编排层——着色器 → 透视除法 → 视口变换 → 调 Rasterizer
- **Renderer**：包装 Pipeline + Framebuffer + DepthBuffer，对外暴露 `beginFrame` / `draw` / `endFrame` 三个高层接口
- **main.cpp**：不再手算矩阵、不直接操作 Framebuffer/DepthBuffer/Pipeline。只负责组装顶层对象和运行循环

### CMakeLists.txt 修改

`src/CMakeLists.txt` 需要新增三个源文件：

```cmake
add_executable(test main.cpp
        Screen.cpp
        Framebuffer.cpp
        Pipeline.cpp
        Shader.cpp
        Mesh.cpp
        DepthBuffer.cpp
        Camera.cpp        # ← 新建
        Rasterizer.cpp    # ← 新建
        Renderer.cpp)     # ← 新建
```

链接库无需新增——三个新模块只依赖 GLM 和现有模块头文件，`target_link_libraries` 不额外增加。

根 `CMakeLists.txt` 无需修改（已有 `find_package(glm ...)`）。

### 各模块设计概要

#### Camera

轨道相机（orbit camera）：相机围绕目标点旋转，用球坐标 `(theta, phi, radius)` 描述位置。

```
相机位置：从 target 出发，沿球面走 radius 距离
旋转变换：theta = 水平方位角，phi = 垂直仰角
```

**公开接口**：

```cpp
class Camera {
public:
    Camera(float fov, float aspect, float near, float far);

    // 矩阵（每帧从状态生成）
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;

    // 交互（由 GLFW 回调驱动）
    void onMouseDrag(float dx, float dy, int button);   // 左键旋转 / 右键平移
    void onMouseScroll(float dy);                         // 滚轮缩放
    void onKeyPress(int key);                             // 键盘控制

    // 自动旋转
    void setAutoRotate(bool enable);
    void setAutoRotateSpeed(float speed);
    void update(float dt);                                // 每帧更新（驱动自动旋转）
};
```

**内部状态**：
- 球坐标 `theta, phi, radius` + 目标点 `target`
- 投影参数 `fov, aspect, near, far`
- 自动旋转开关 + 速度
- 上一帧鼠标位置（计算增量）

**设计原因**：把相机逻辑集中在 Camera 里，main.cpp 的鼠标回调只需调用 `camera.onMouseDrag(dx, dy, button)` 一行。view 和 projection 矩阵通过 `camera.viewMatrix()` 和 `camera.projectionMatrix()` 获取，不再在 main 里手算。

#### Renderer

门面（Facade）模式：把 Pipeline + Framebuffer + DepthBuffer + Screen 的协作封装成三个方法。

```cpp
class Renderer {
public:
    Renderer(int width, int height, Screen& screen);

    void beginFrame(Color clearColor);                     // 清除颜色 + 深度
    void draw(const Mesh& mesh, const Uniforms& uniforms); // 渲染一个网格
    void endFrame();                                       // present 到屏幕

    void setVertexShader(const VertexShader&);
    void setFragmentShader(const FragmentShader&);
};
```

**设计原因**：main.cpp 不需要知道 Framebuffer、DepthBuffer、Pipeline 的存在。渲染一个网格就是 `beginFrame → draw → endFrame` 三步。内部持有这些对象，外部只需关心数据（Mesh、Uniforms、Shader）。

#### Rasterizer

从 `Pipeline::drawTriangle` 中把光栅化循环抽出来，放在 `namespace Rasterizer` 中作为一组自由函数。目前只需要一个核心函数 `rasterizeTriangle`。

**为什么用 namespace + 自由函数而不是类**：Rasterizer 每次调用都是纯计算——接收输入、输出像素。它不需要维护任何状态（没有"当前绑定纹理"、"当前混合模式"等）。如果将来引入纹理采样器等有状态的对象，再把 namespace 升级为类。命名空间提供了逻辑分组的好处，而不会引入不必要的构造/析构。

**核心函数签名**：

```cpp
namespace Rasterizer {

void rasterizeTriangle(
    Framebuffer& fb,                     // 像素写入目标
    DepthBuffer* depthBuffer,            // 深度缓冲（nullptr 禁用深度测试）
    const FragmentShader& fragShader,    // 片段着色器
    const Uniforms& uniforms,            // 全局常量

    // 屏幕空间顶点（Pipeline 视口变换后）
    const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2,

    // NDC 坐标（透视除法后，z 用于深度插值）
    const glm::vec3& ndc0, const glm::vec3& ndc1, const glm::vec3& ndc2,

    // 顶点着色器输出（w 用于透视校正，color 用于插值）
    const VertexOutput& v0,
    const VertexOutput& v1,
    const VertexOutput& v2
);

} // namespace Rasterizer
```

**参数分为四组，对应光栅化所需的四类信息**：

| 参数组 | 参数 | 来源 | 用途 |
|--------|------|------|------|
| 写入目标 | `fb`, `depthBuffer` | Renderer 持有 | 像素写入 + 深度比较 |
| 着色 | `fragShader`, `uniforms` | Renderer 持有 | 逐像素确定最终颜色 |
| 屏幕位置 | `p0, p1, p2` | Pipeline 视口变换后 | 包围盒 + 边函数计算 |
| 顶点属性 | `ndc0..2` + `v0..2` | Pipeline（顶点着色器 + 透视除法） | 深度插值 + 透视校正颜色插值 |

**函数执行流程**（和当前 `Pipeline::drawTriangle` 的光栅化部分完全一致）：

1. 计算 `area = edgeFunction(p0, p1, p2)`，`|area| < 1e-8` 时跳过（退化三角形）
2. 计算包围盒，夹到帧缓冲范围 `[0, w-1] × [0, h-1]`
3. 逐顶点预计算：`1/w`、`color/w`（提到循环外，避免每像素重复计算）
4. 逐像素遍历包围盒：
   - 边函数 `e1, e2, e3` 判断 — 同号则像素在三角形内
   - 重心坐标 `w0=e2/area`, `w1=e3/area`, `w2=e1/area`
   - 插值 NDC z → 映射到 `[0,1]` → 深度测试（若启用）
   - 透视校正插值颜色：`(color/w) / (1/w)` 恢复
   - 片段着色器 → `fb.setPixel(x, y, color)`

**抽离动机**：
- `drawTriangle` 目前约 80 行，包含了三种不同层次的逻辑（坐标变换、光栅化、着色）——违反了"一个函数做一件事"的原则
- 拆开后，光栅化算法可以独立测试：给 Rasterizer 喂三个屏幕顶点和着色器参数，检查 Framebuffer 的像素输出——不需要跑整个 Pipeline
- 将来加纹理采样、mipmap、MSAA 等都在 Rasterizer 里改，不影响 Pipeline 的着色器/坐标变换逻辑
- Rasterizer 的输入全部通过参数传入，不依赖任何模块内部状态——这是一个"纯计算"函数，单元测试只需要准备输入值，不需要 mock 对象

#### Pipeline（修改后）

管线变成薄层编排：

```
drawTriangle:
  1. 顶点着色器（获取 VertexOutput）
  2. 透视除法（clip → NDC）
  3. 视口变换（NDC → screen coordinates）
  4. 调 Rasterizer::rasterizeTriangle(...)
```

光栅化循环不再驻留在 Pipeline 中。

### main.cpp 重构目标

```cpp
int main() {
    Screen screen(WIDTH, HEIGHT, "Shader's principle");
    Renderer renderer(WIDTH, HEIGHT, screen);
    Camera camera(60.0f, aspect, 0.1f, 100.0f);
    Mesh mesh = createDemoMesh();   // 提取为函数

    // 绑定着色器
    renderer.setVertexShader(VertexShader(BuiltinVertexShader::mvp));
    renderer.setFragmentShader(FragmentShader(BuiltinFragmentShader::passThrough));

    // GLFW 回调（lambda 转发给 camera）
    glfwSetMouseButtonCallback(...);
    glfwSetCursorPosCallback(...);
    glfwSetScrollCallback(...);
    glfwSetKeyCallback(...);

    while (!screen.shouldClose()) {
        camera.update(deltaTime);
        Uniforms uni;
        uni.model = glm::mat4(1.0f);
        uni.view = camera.viewMatrix();
        uni.projection = camera.projectionMatrix();

        renderer.beginFrame(Color{0.12f, 0.12f, 0.12f});
        renderer.draw(mesh, uni);
        renderer.endFrame();
    }
}
```

约 30 行，每个概念各司其职。

### 关键注意事项

1. **Rasterizer 的函数 vs 类选择**：先用一组自由函数（`rasterizeTriangle(...)`），放在 `namespace Rasterizer` 下。目前不需要 Rasterizer 维护状态——它每次调用都是纯计算。命名空间提供了"逻辑分组"的好处，而不会引入类的构造/析构开销。如果将来加纹理采样器等有状态对象，再把 namespace 升级为类。

2. **Camera 的球坐标与万向节锁**：用 `(theta, phi)` 球坐标描述旋转时，`phi` 接近 ±90° 会出现万向节锁（水平和垂直旋转轴重合）。限制 `phi ∈ [-89°, 89°]` 即可避免。`phi` 的 clamp 在 `onMouseDrag` 更新 phi 之后执行——不要等 `viewMatrix()` 被调用时才 clamp，否则用户会看到视角突然"卡住"。

3. **Camera 的矩阵更新时机**：每帧从球坐标重新计算 view 矩阵，不需要缓存上一次的结果。计算量很小（一次 `glm::lookAt` + 几个三角函数），也不需要处理矩阵累积误差。如果担心性能（完全没有必要，每帧只算一次），可以在 `update()` 中预计算 `viewMatrix()` 和 `projectionMatrix()` 的结果并缓存，但当前阶段不需要这个复杂度。

4. **Renderer 持有 Framebuffer 和 DepthBuffer**：用值语义，而非指针或引用。这两个对象和 Renderer 的生命周期完全一致——Renderer 存在期间它们始终需要，Renderer 销毁时它们也销毁。`beginFrame` 同时清除颜色和深度缓冲，`endFrame` 调用 `Screen::present`。外部（main.cpp）不需要感知深度缓冲的存在。

5. **GLFW 回调与 Camera 的桥接**：GLFW 的回调是 C 函数指针（`GLFWmousebuttonfun` = `void(*)(GLFWwindow*, int, int, int)`），不能直接传成员函数指针或带捕获的 lambda。推荐的桥接方式：

   ```cpp
   // 用 glfwSetWindowUserPointer 存储 Camera 指针，回调里取回
   glfwSetWindowUserPointer(window, &camera);

   glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y) {
       auto* cam = static_cast<Camera*>(glfwGetWindowUserPointer(win));
       static double lastX = x, lastY = y;
       float dx = static_cast<float>(x - lastX);
       float dy = static_cast<float>(y - lastY);
       lastX = x; lastY = y;
       cam->onMouseDrag(dx, dy);
   });
   ```

   注意：上面的 lambda 不带捕获（`[]`），可以隐式转换为 C 函数指针。`static` 变量绕过了"需要记住上一帧鼠标位置"的问题——但这在多窗口场景下有风险（所有窗口共享同一个 `lastX/lastY`）。更干净的做法（但代码更多）是写普通自由函数 + 在 `glfwSetWindowUserPointer` 中存一个包含 Camera 指针 + 鼠标状态的包装结构体。

6. **Pipeline 的着色器状态迁移到 Renderer**：重构后，`m_vertexShader` 和 `m_fragmentShader` 从 Pipeline 移到 Renderer——因为着色器绑定是"渲染器级别的状态"。同一个渲染器在一次 `draw()` 调用中对所有三角形使用同一对着色器。Pipeline 的方法变为接收着色器作为参数（而非持有它们）。这简化了 Pipeline：它不再有状态，只是过程编排——"给我什么着色器我就调用什么"。

7. **重构不改渲染结果**：这是最关键的验证原则。Camera 的默认状态应产生和当前 main.cpp 中手写的 `glm::lookAt` + `glm::perspective` 完全相同的 view/projection 矩阵。Rasterizer 的光栅化逻辑和当前 `drawTriangle` 中的循环逐像素一致。重构后屏幕上的两个三角形和之前一模一样——外观不变，内部结构改变。在接入 Camera 交互之前，先确认"使用 Camera 矩阵"和"手写矩阵"的输出完全一致。

8. **为什么 `drawTriangle` 保留 public**：Pipeline 重构后同时保留 `drawTriangle`（单个三角形）和 `drawMesh`（网格批处理）两个公开接口。`drawMesh` 内部调用 `drawTriangle`，`drawTriangle` 内部调用 `Rasterizer::rasterizeTriangle`。保留单三角形接口是为了方便测试——"代码可测试性"是重构的目标之一，拆开后确实能做到独立测试。

### 验证方法

- 编译运行，行为和重构前完全一致（两个三角形绕 Y 轴旋转）
- 鼠标左键拖动：三角形应跟随旋转
- 鼠标滚轮：三角形放大/缩小
- 按空格键：自动旋转暂停/恢复
- 上/下箭头：调整旋转速度

### 实际实现与原计划的差异

实现过程中（4 次提交：`bb242c2`、`0cfae2f`、`5225f9c`、`95718f5`），在计划基础上做了以下调整：

1. **Pipeline 变为无状态**：原计划 Pipeline 保留着色器绑定状态（`setVertexShader`/`setFragmentShader`）。实现中 Pipeline 完全无状态——着色器和 DepthBuffer 全部通过参数传入 `drawTriangle`/`drawMesh`。着色器绑定状态移到 Renderer 中。这让 Pipeline 成为纯函数，更容易理解和测试。

2. **新增 Control 类**（原计划外）：最初 Camera 同时承担了 view/projection 矩阵生成、鼠标交互、键盘交互和对象自动旋转四个职责。在 `95718f5` 提交中，将对象动画逻辑（自动旋转开关、速度控制、按 M 键切换）从 Camera 移到新建的 Control 类。Camera 回归纯粹的"相机是什么"职责，Control 承担"用户操作 → 场景变化"的翻译职责。详情见 devlog.md 阶段 6e。

3. **`Shader::process()` 标记 const**：着色器的 process 方法不应修改内部状态，const 标记让编译器帮助检查。原计划未提及，系实现中自然发生的改进。

4. **`#ifndef NDEBUG` 调试宏**：在原计划之外添加了帧计时和性能输出（每 60 帧输出平均帧时间），通过 `#ifndef NDEBUG` 确保 Release 构建零开销。

5. **VSync 关闭**：`glfwSwapInterval(0)` 禁用垂直同步，让帧率能真实反映软渲染器性能。原计划未提及。

6. **Camera 初始化细节**：原计划相机默认自动旋转开启；实现中改为默认关闭，通过 Control 的 M 键手动开启。`radius` 初始值设为 1.5（而非原计划的默认值），使三角形在屏幕上比例合适。

---

## 阶段 6：索引几何 + 背面剔除

**日期**：2026-05-28（规划）  
**状态**：[ ] 待实现

**目标**：Mesh 内部改用索引缓冲区（`vector<Vertex>` + `vector<glm::uvec3>`），支持顶点复用；Rasterizer 在光栅化前剔除背向相机的三角形，提升帧率。

---

### 一、设计决策

#### 1.1 索引缓冲区：向后兼容的内部重构

Mesh 当前是扁平的顶点数组（`vector<Vertex>`，每 3 个 = 1 个三角形）。重构为**索引几何**：顶点数组 + 索引数组。

关键设计：**保持 `addTriangle(v0, v1, v2)` 的 API 不变**。它内部自动将 3 个顶点追加到 `m_vertices`，然后添加 3 个索引 `{n, n+1, n+2}`。外部调用者（main.cpp 的 `createDemoMesh()`）无需修改。

同时新增 `addVertex()` 和 `addIndexedTriangle()`，为阶段 7 的 OBJ 加载器提供索引式接口。OBJ 文件天生就是"先声明所有顶点，再用 `f` 行组合索引"——这两个新方法直接对应此模型。

#### 1.2 背面剔除：屏幕空间判定，CCW = 正面

背面剔除在屏幕空间进行：用 `edgeFunction(p0, p1, p2)` 计算屏幕空间三角形"有向面积"。在屏幕空间（x 右, y 下），CCW 绕序对应 `area > 0`。

**绕序追踪**（模型空间 → 屏幕空间）：

```
模型空间 CCW (右手系) → NDC CCW (x右y上, edgeFunc < 0) → 视口Y翻转 → 屏幕空间 CCW (x右y下, edgeFunc > 0)
```

视口变换 `screen_y = (1 - ndc.y) * 0.5 * H` 翻转了 Y 轴，导致 edge function 的符号反转。模型空间 CCW 三角形 → 屏幕空间 `area > 0`。

剔除条件是 `area < 1e-8f`——同时处理了退化三角形（`|area| ≈ 0`）和背向三角形（`area < 0`）。

内部循环也随之简化：不再需要 `all >= 0 || all <= 0` 双分支判断，只需 `e1 >= 0 && e2 >= 0 && e3 >= 0`（因为背向三角形已经在外部被剔除）。

**为什么不在 NDC 中剔除**：视口变换后屏幕坐标是整数像素，包围盒和边函数计算都基于屏幕坐标。在屏幕空间做剔除更自然——不需要额外的坐标系统转换。

#### 1.3 为什么剔除不可配置（当前阶段）

背面剔除是硬件渲染器的默认行为，没有场景需要画背向三角形。如果将来需要（如调试法线方向），可以在 Rasterizer 加一个 `bool cullBackfaces` 参数。当前阶段始终保持剔除。

---

### 二、文件变更清单

| 操作 | 文件 | 说明 |
|------|------|------|
| 修改 | `include/Mesh.h` | 新增 `m_indices`、`addVertex()`、`addIndexedTriangle()`、`getTriangleIndices()`、`vertexCount()` |
| 修改 | `src/Mesh.cpp` | 实现上述新方法，`addTriangle()` 内部改为先加顶点再加索引 |
| 修改 | `include/Rasterizer.h` | 无签名变更（背面剔除是内部实现变化） |
| 修改 | `src/Rasterizer.cpp` | 剔除逻辑 + 内循环简化 |
| 修改 | `src/Pipeline.cpp` | `drawMesh` 改用索引遍历 |
| 修改 | `src/main.cpp` | 可能需要调整 `createDemoMesh()` 顶点绕序以确保正面可见 |

无需新建文件，无需修改 CMakeLists.txt。

---

### 三、模块设计

#### 3.1 Mesh（修改后）

```cpp
class Mesh {
    std::vector<Vertex> m_vertices;
    std::vector<glm::uvec3> m_indices;  // 新增：每个三角形 = 3 个索引

public:
    // 新增：单独添加顶点，返回索引
    uint32_t addVertex(const Vertex& v);

    // 新增：用索引组装三角形（OBJ 加载器用）
    void addIndexedTriangle(uint32_t i0, uint32_t i1, uint32_t i2);

    // 保持兼容：同时添加顶点和索引
    void addTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2);

    // 基于索引的遍历接口
    size_t triangleCount() const { return m_indices.size(); }
    glm::uvec3 getTriangleIndices(size_t t) const { return m_indices[t]; }

    // 顶点访问（不变）
    size_t vertexCount() const { return m_vertices.size(); }
    const Vertex& getVertex(size_t i) const { return m_vertices[i]; }

    void clear() { m_vertices.clear(); m_indices.clear(); }
};
```

**`addTriangle` 的新实现**：
```cpp
void Mesh::addTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    uint32_t base = static_cast<uint32_t>(m_vertices.size());
    m_vertices.push_back(v0);
    m_vertices.push_back(v1);
    m_vertices.push_back(v2);
    m_indices.push_back({base, base + 1, base + 2});
}
```

每个三角形仍然占用 3 个顶点槽位——在没有共享顶点的场景下，索引缓冲区不节省内存，但 API 统一。当 OBJ 加载器通过 `addVertex` + `addIndexedTriangle` 使用时，共享顶点才真正被复用。

#### 3.2 Pipeline::drawMesh（修改后）

遍历方式从"3 个一组取顶点"变为"取索引 → 查顶点"：

```cpp
void Pipeline::drawMesh(..., const Mesh& mesh, ...) {
    for (size_t t = 0; t < mesh.triangleCount(); ++t) {
        glm::uvec3 idx = mesh.getTriangleIndices(t);
        drawTriangle(fb, depthBuffer, vertShader, fragShader,
                     mesh.getVertex(idx.x), mesh.getVertex(idx.y), mesh.getVertex(idx.z), uni);
    }
}
```

`drawTriangle` 签名不变，它只关心"拿到 3 个 Vertex 引用"，不管调用方是从扁平数组还是索引数组取出的。

#### 3.3 Rasterizer::rasterizeTriangle（修改后）

两处改动：

**(1) 背面剔除 + 退化三角形合并**（函数开头）：

```cpp
// 旧：
float area = edgeFunction(p0, p1, p2);
if (std::abs(area) < 1e-8f) return;

// 新：
float area = edgeFunction(p0, p1, p2);
// 背面剔除：屏幕空间 CCW = front face (area > 0)。
// 剔除面积 ≤ 0 的三角形（退化 + 背面 CW）。
if (area < 1e-8f) return;
```

**(2) 内部循环简化**（像素点内外判断）：

```cpp
// 旧：
if ((e1 >= 0 && e2 >= 0 && e3 >= 0) || (e1 <= 0 && e2 <= 0 && e3 <= 0)) {

// 新：
if (e1 >= 0 && e2 >= 0 && e3 >= 0) {
```

背向三角形（area < 0，各边函数 ≤0）已被剔除，不再需要双分支判断。

---

### 四、实现顺序

| 步骤 | 操作 | 原因 |
|------|------|------|
| 1 | 修改 `Mesh.h` + `Mesh.cpp` | 数据结构层，不依赖其他模块改动。`addTriangle` 的旧 API 保持不变，现有调用者无需修改即可编译 |
| 2 | 修改 `Pipeline::drawMesh` | 适配新索引遍历方式。编译验证阶段 1-2 无报错 |
| 3 | 修改 `Rasterizer::rasterizeTriangle` | 加背面剔除 + 简化内循环。独立于步骤 1-2，可最后改 |
| 4 | 编译运行 + 验证绕序 | 确认正面三角形可见、背面三角形消失 |
| 5 | （必要时）调整 `createDemoMesh()` 绕序 | 如果某个三角形默认是背面朝向，调整顶点顺序 |

每一步完成后编译验证（`ninja -C build`），确保增量正确。

---

### 五、关键注意事项

#### 5.1 `createDemoMesh()` 的绕序验证

当前两个三角形在模型空间都是 CCW（从 +Z 方向看）。默认相机位置 `(0, 0, 1.5)` 看向原点 `(0, 0, 0)`，即从 +Z 方向观察 XY 平面上的三角形。

- **三角形 1**：`v0(-0.577,-0.333,0)→v1(0.577,-0.333,0)→v2(0,0.666,0)`。`(v1-v0)×(v2-v0) = (1.154,0,0)×(0.577,0.999,0)`，z 分量 > 0 → CCW → 正面 ✓
- **三角形 2**：`v0(-0.2,0.2,0.2)→v1(0.5,0.2,0.2)→v2(0.15,0.7,0.4)`。`(v1-v0)×(v2-v0) = (0.7,0,0)×(0.35,0.5,0.2)`，z 分量 = 0.7×0.5 - 0×0.35 = 0.35 > 0 → CCW → 正面 ✓

两个三角形在默认视角下都应该是正面。如果运行后发现某个三角形消失，检查其绕序并调换两个顶点即可。

#### 5.2 旋转后绕序变化

当 Control 开启自动旋转（按 M）后，模型绕 Y 轴旋转。旋转 180° 时，三角形的绕序反转（背面朝向相机），三角形会消失——这是正确的背面剔除行为。用户看到的应该是"三角形转到背面时自动隐藏，转回正面时重新出现"。

#### 5.3 索引缓冲区与 OBJ 的对齐

OBJ 文件的 `f` 行格式为 `f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3`，其中 `vN` 是**基于 1** 的顶点索引。`addIndexedTriangle` 接收基于 0 的索引，加载器需要减 1。这个约定在阶段 7 实现 OBJ 加载时直接匹配。

#### 5.4 性能提升

背面剔除在大模型上效果显著——约一半三角形被跳过（封闭网格的背面）。当前只有 2 个三角形的 demo 看不出帧率变化，但代码路径已验证，到阶段 7 加载模型时自然受益。

#### 5.5 `glm::uvec3` 的类型

GLM 的 `glm::uvec3` 是 `glm::vec<3, unsigned int>`，三个分量 `.x`, `.y`, `.z`。在 `getVertex(idx.x)` 中用作 `vector::operator[]` 的下标时，`unsigned int` → `size_t` 是安全的隐式转换。不需要 `static_cast`。

---

### 六、验证方法

1. **编译通过**：`ninja -C build` 无错误
2. **默认视角两个三角形均可见**：行为和阶段重构完全一致（三角形颜色、位置不变）
3. **按 M 开启自动旋转**：三角形旋转到背面时消失，转回正面时出现
4. **鼠标左键拖拽旋转视角**：从不同角度观察，背面三角形始终被剔除
5. **帧率无明显退化**：背面剔除的额外开销极小（每三角形一次比较），不应影响性能
6. **Release 构建**（可选）：`cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`，确认无编译警告

---

## 阶段 7：模型加载

**日期**：2026-05-28（规划）  
**状态**：[x] 已实现

**目标**：引入 `tinyobjloader` 解析 OBJ 模型文件，生成 Mesh，替代手写三角形。

---

### 一、设计决策

#### 1.1 使用 tinyobjloader

选择 **tinyobjloader**（单头文件库），不手写解析器。

原因：
- 渲染管线才是这个项目的核心，OBJ 解析不是。手写解析器就是花时间解决已解决的问题
- tinyobjloader 是一个成熟的单头文件库，处理了三角形面、四边形三角化、Bézier/NURBS 曲面 tessellation、负索引、相对索引、行续行等等所有边界情况
- ObjLoader 的角色简化为"薄转换层"：tinyobjloader 数据 → Mesh，约 40 行代码
- tinyobjloader 返回的顶点已包含 position + normal + texcoord，天然对接阶段 8/9 的 Vertex 扩展

#### 1.2 ObjLoader 的角色

ObjLoader 不再是"解析器"，而是"转换适配器"——把 tinyobjloader 的输出结构转换为项目的 Mesh 格式：

```
tinyobj::LoadObj() → tinyobj::attrib_t + tinyobj::shape_t → 循环 → Mesh::addVertex/addIndexedTriangle
```

#### 1.3 顶点颜色合成（不变）

OBJ 不含顶点颜色。tinyobjloader 返回的每个顶点有 position，但没有 color。仍然用 `normalize(position) * 0.5 + 0.5` 合成，阶段 9 光照替代。

#### 1.4 模型归一化（不变）

和之前方案一致：加载后计算包围盒，缩放使最长轴 = 2.0，居中于原点。

---

### 二、文件变更清单

| 操作 | 文件 | 说明 |
|------|------|------|
| 修改 | `src/Mesh.cpp` | 补全 `addVertex` 和 `addIndexedTriangle`（阶段六遗留 stub） |
| **新建** | `include/ObjLoader.h` | `namespace ObjLoader { Mesh load(const std::string&); }` |
| **新建** | `src/ObjLoader.cpp` | tinyobjloader 调用 + 数据 → Mesh 转换（~40 行） |
| **新建** | `include/tiny_obj_loader.h` | tinyobjloader 单头文件（从 GitHub 下载） |
| 修改 | `src/CMakeLists.txt` | `add_executable` 新增 `ObjLoader.cpp` |
| 修改 | `src/main.cpp` | 删除 `createDemoMesh()`，改用 `ObjLoader::load()` |
| **新建** | `assets/` | 存放 .obj 模型文件 |

---

### 三、模块设计

#### 3.1 ObjLoader

```cpp
// include/ObjLoader.h
#ifndef SHADER_S_PRINCIPLE_OBJLOADER_H
#define SHADER_S_PRINCIPLE_OBJLOADER_H

#include "Mesh.h"
#include <string>

namespace ObjLoader {

Mesh load(const std::string& filepath);

} // namespace ObjLoader

#endif
```

**`load()` 的执行流程**：

```
1. tinyobj::ObjReader reader;
2. reader.ParseFromFile(filepath);
   失败 → 打印 reader.Error() + reader.Warning()，返回空 Mesh
3. const auto& attrib = reader.GetAttrib();   // positions, normals, texcoords
   const auto& shapes = reader.GetShapes();   // 每个 shape 含 mesh.indices
4. 第一遍：遍历所有 shape 的所有索引，收集顶点位置 → 计算包围盒
5. 计算 scale 和 offset（归一化到 2.0 单位）
6. 第二遍：遍历所有 shape 的所有面（tinyobjloader 已将四边形三角化）：
   - 提取 3 个顶点的 position（从 attrib.vertices 按索引取，应用 scale + offset）
   - 合成颜色 = normalize(pos) * 0.5 + 0.5
   - mesh.addVertex(Vertex(vec4(pos, 1.0), color))
   - mesh.addIndexedTriangle(vi, vi+1, vi+2)
7. 返回 Mesh
```

**tinyobjloader 数据访问**：

```cpp
// tinyobjloader 的索引结构
tinyobj::index_t idx = shape.mesh.indices[i];
float px = attrib.vertices[3 * idx.vertex_index + 0];  // x
float py = attrib.vertices[3 * idx.vertex_index + 1];  // y
float pz = attrib.vertices[3 * idx.vertex_index + 2];  // z
// idx.normal_index, idx.texcoord_index — 阶段 8/9 用
```

tinyobjloader 已经处理了：三角形/四边形、负索引、Bézier/NURBS tessellation、顶点去重/不复用。我们只需按 `mesh.indices` 的顺序取出顶点位置，构建 Mesh。

#### 3.2 Mesh 补全

同之前方案：
```cpp
uint32_t Mesh::addVertex(const Vertex& v) {
    uint32_t idx = static_cast<uint32_t>(m_vertices.size());
    m_vertices.push_back(v);
    return idx;
}

void Mesh::addIndexedTriangle(uint32_t i0, uint32_t i1, uint32_t i2) {
    m_indices.push_back({i0, i1, i2});
}
```

#### 3.3 main.cpp 变化

删除 `createDemoMesh()`，改为：

```cpp
Mesh mesh = ObjLoader::load("../assets/model.obj");
if (mesh.triangleCount() == 0) {
    std::fprintf(stderr, "Failed to load model\n");
    return 1;
}
```

---

### 四、获取 tinyobjloader

```bash
# 从仓库根目录执行
wget https://raw.githubusercontent.com/tinyobjloader/tinyobjloader/release/tiny_obj_loader.h \
     -O include/tiny_obj_loader.h
```

或在 CMake 中用 `FetchContent` 自动下载。简单起见，手动下载放到 `include/` 下即可。

`tiny_obj_loader.h` 是 MIT 许可的单头文件，`#define TINYOBJLOADER_IMPLEMENTATION` 在一个 .cpp 中（ObjLoader.cpp）启用实现。

---

### 五、实现顺序

| 步骤 | 操作 | 原因 |
|------|------|------|
| 1 | 补全 `Mesh::addVertex` + `addIndexedTriangle` | 阶段六遗留 stub |
| 2 | 下载 `tiny_obj_loader.h` → `include/` | 依赖就位 |
| 3 | 新建 `ObjLoader.h` + `ObjLoader.cpp` | 转换层实现 |
| 4 | 修改 `src/CMakeLists.txt` | 新增 ObjLoader.cpp |
| 5 | 创建 `assets/` + 放入模型 | 测试资产就位 |
| 6 | 修改 `main.cpp` | 替换 demo mesh |
| 7 | 编译运行 + 验证 | 端到端测试 |

---

### 六、关键注意事项

#### 6.1 三角形数可能很大

tinyobjloader 会将 Bézier 曲面 tessellate 为三角形（使用 `--tesstype` 或默认配置）。含曲面片的 .obj 文件加载后可能有数千三角形。如果 FPS 过低，减小 tessellation 密度或换用更简单的模型。

#### 6.2 OBJ 文件路径

和之前方案一致：从 build 目录运行 `./src/test`，路径为 `../assets/model.obj`。

#### 6.3 顶点不复用的原因

tinyobjloader 默认按面遍历，每个面返回独立的顶点（即使不同面共享同一位置）。这和之前方案的行为一致——每个面创建独立顶点，不在面之间复用。原因同样是 hard edges 的法线不连续问题（阶段 9）。

#### 6.4 模型归一化时机

归一化发生在顶点数据从 tinyobjloader 取出之后、`addVertex` 之前。即 scale/offset 只影响 Mesh 中的数据，不影响 tinyobjloader 的原始数据。

#### 6.5 只支持单个 shape

当前版本遍历所有 shapes 并合并到一个 Mesh 中。如果模型有多个 shapes（如多个物体组合），它们会被合并渲染。未来可按需扩展为多 Mesh 或多 draw call。

#### 6.6 错误处理

| 错误类型 | 策略 |
|----------|------|
| 文件不存在 | tinyobjloader 返回错误，打印 `reader.Error()`，返回空 Mesh |
| OBJ 格式错误 | tinyobjloader 返回 warning，打印 `reader.Warning()` |
| 加载后 triangleCount == 0 | main.cpp 判断并退出，stderr 报错 |

---

### 七、验证方法

1. **编译通过**：`ninja -C build` 无错误
2. **load 成功**：`triangleCount() > 0`，无 stderr 报错
3. **模型可见**：屏幕上显示彩色模型（position-derived 颜色）
4. **背面剔除正常**：旋转时背面三角形消失
5. **Bézier 曲面**：用户提供的 Bezier patch .obj 文件可直接加载（tinyobjloader 自动 tessellate）
6. **交互正常**：鼠标旋转缩放、键盘 M/Up/Down 均正常工作

## 阶段 8：纹理映射

**日期**：2026-05-28（规划）  
**状态**：[ ] 待实现

**目标**：CPU 端加载 PNG/JPG 图片，三角形可贴纹理。管线支持 UV 坐标的透视校正插值，片段着色器可采样纹理。

---

### 一、设计决策

#### 1.1 UV 作为顶点属性，流经整个管线

UV 坐标和颜色（color）一样是**逐顶点属性**。它沿着相同的路径流动：

```
Vertex.texcoord → VertexShader → VertexOutput.texcoord → Rasterizer 透视校正插值 → FragmentInput.texcoord → FragmentShader.sample()
```

这意味着 texcoord 需添加到三个结构体中：`Vertex`、`VertexOutput`、`FragmentInput`。

**为什么不把 UV 单独传**：color 已经走通了逐顶点属性 → 光栅化插值 → 片段着色器的完整路径。UV 沿相同路径走，新增代码集中在数据字段上，不改变控制流。Rasterizer 的插值逻辑也只是"多插值一个 vec2"——对称于已有的 color 插值。

#### 1.2 Texture 类：独立的 CPU 图片存储 + 采样

Texture 只做两件事：**存像素**、**按 UV 采样**。它不知道渲染管线、着色器、Mesh 的存在。

为什么独立成类：
- 图片加载（stb_image）和纹理采样是独立的关注点，和渲染管线逻辑无关（plan.md 原则 2.1 高内聚）
- 如果将来换图片加载库（如 libpng），只改 Texture 内部
- 可以独立测试：给一个 UV 坐标，检查返回的颜色对不对

#### 1.3 纹理通过 Uniforms 指针传入片段着色器

`Uniforms` 中新增 `const Texture* texture`（默认为 `nullptr`）。片段着色器通过 `uniforms.texture->sample(uv)` 访问。

**为什么用指针而不是引用或值**：
- 纹理可能不存在（无纹理时 `nullptr`），着色器回退到顶点颜色——天然支持"有纹理"和"无纹理"两种模式
- 纹理生命周期由 main/Renderer 管理，Uniforms 只是"借用"——不持有所有权
- 前向声明 `class Texture;` 即可，Shader.h 不需要 include Texture.h——保持头文件轻量（plan.md 原则 2.2 低耦合）

#### 1.4 透视校正 UV 插值

UV 和颜色一样需要**透视校正插值**——直接线性插值会在斜面上产生纹理扭曲。

公式和颜色插值完全相同：

```
UV/w 在屏幕空间线性插值，除以 1/w 恢复：
texcoord = (w0·uv0/w0' + w1·uv1/w1' + w2·uv2/w2') / (w0/w0' + w1/w1' + w2/w2')
```

Rasterizer 中已有的 `inv_w` 和重心坐标可直接复用，只需额外计算 `uv/w`。

#### 1.5 采样模式：最近邻 + 双线性

| 模式 | 做法 | 效果 |
|------|------|------|
| Nearest | `round(uv)` 取最近像素 | 像素块状，调试 UV 映射时更直观 |
| Bilinear | 4 邻域像素按小数部分 lerp | 平滑过渡 |

默认用双线性（视觉效果正确），最近邻用 `Filter::Nearest` 显式切换。阶段 9 做光照时切换回最近邻可能有助于调试法线。

包裹模式（Wrap/Clamp/Repeat）当前阶段固定为 Clamp-to-edge——UV 超出 `[0,1]` 时夹到边缘。Repeat 模式待需要时再加。

#### 1.6 为什么不在 ObjLoader 中做纹理加载

根据 plan.md 原则 2.1（一个模块只做一件事）：ObjLoader 只负责"把 OBJ 文件转成 Mesh"。纹理图片的加载和采样是 Texture 的职责。两者通过 main.cpp 组装：

```cpp
Texture tex("model.png");
Mesh mesh = ObjLoader::load("model.obj");
Uniforms uni;
uni.texture = &tex;
```

这遵循了"单向依赖"原则：ObjLoader 不依赖 Texture，Texture 不依赖 ObjLoader。

---

### 二、文件变更清单

| 操作 | 文件 | 说明 |
|------|------|------|
| 修改 | `include/Types.h` | Vertex / VertexOutput / FragmentInput 新增 `glm::vec2 texcoord` |
| **新建** | `include/Texture.h` | Texture 类声明（width/height、loadFromFile、sample） |
| **新建** | `src/Texture.cpp` | Texture 实现 + `#define STB_IMAGE_IMPLEMENTATION` |
| 修改 | `include/Shader.h` | 前向声明 Texture；Uniforms 新增 `const Texture* texture`；BuiltinFragmentShader 新增 `texture` 函数 |
| 修改 | `src/Shader.cpp` | 实现 `BuiltinFragmentShader::texture`；`BuiltinVertexShader::mvp` 透传 texcoord |
| 修改 | `src/Rasterizer.cpp` | 插值循环新增 texcoord 的透视校正插值 |
| 修改 | `src/ObjLoader.cpp` | `loadStandardObj` 提取 texcoord，Vertex 构造包含 UV |
| 修改 | `src/CMakeLists.txt` | `add_executable` 新增 `Texture.cpp` |
| 修改 | `src/main.cpp` | 加载纹理图片，绑定到 Uniforms，切换片段着色器 |

无需修改：`Mesh.h/cpp`、`Pipeline.h/cpp`、`Renderer.h/cpp`、`Camera`、`Control`、`Screen`、`Framebuffer`、`DepthBuffer`。

---

### 三、模块设计

#### 3.1 Types.h（修改）

```cpp
struct Vertex {
    glm::vec4 position;
    Color color;
    glm::vec2 texcoord;  // 新增：纹理坐标，默认 (0,0)

    Vertex() : position(0, 0, 0, 1), texcoord(0.0f) {}
    Vertex(const glm::vec4& pos) : position(pos), texcoord(0.0f) {}
    Vertex(const glm::vec4& pos, const Color& c) : position(pos), color(c), texcoord(0.0f) {}
    Vertex(const glm::vec4& pos, const Color& c, const glm::vec2& uv)
        : position(pos), color(c), texcoord(uv) {}
};

struct VertexOutput {
    glm::vec4 position;
    Color color;
    glm::vec2 texcoord;  // 新增

    VertexOutput() : position(0.0f), color() {}
    VertexOutput(const glm::vec4& p, const Color& c) : position(p), color(c) {}
    VertexOutput(const glm::vec4& p, const Color& c, const glm::vec2& uv)
        : position(p), color(c), texcoord(uv) {}
};

struct FragmentInput {
    Color color;
    glm::vec2 texcoord;  // 新增：透视校正插值后的 UV

    FragmentInput(Color c, glm::vec2 uv = glm::vec2(0.0f)) : color(c.data), texcoord(uv) {}
};
```

**设计原因**：
- `texcoord` 默认值 `(0,0)` 确保旧代码（如 `createFallbackMesh()`、AOV Bézier 曲面加载）无需修改即可编译
- 三参数构造函数供 ObjLoader 使用：`Vertex(pos, color, uv)`
- `VertexOutput` 的双参数构造函数保持不变（旧着色器兼容），三参数供 `mvp` 着色器使用

#### 3.2 Texture 类

```cpp
// include/Texture.h
#ifndef SHADER_S_PRINCIPLE_TEXTURE_H
#define SHADER_S_PRINCIPLE_TEXTURE_H

#include "Types.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Texture {
public:
    enum class Filter { Nearest, Bilinear };

    Texture();
    ~Texture();

    bool loadFromFile(const std::string& path);

    Color sample(const glm::vec2& uv, Filter filter = Filter::Bilinear) const;

    int width() const { return m_width; }
    int height() const { return m_height; }
    bool valid() const { return !m_data.empty(); }

private:
    std::vector<Color> m_data;
    int m_width = 0;
    int m_height = 0;
};

#endif
```

**`loadFromFile(path)` 执行流程**：
```
1. stbi_set_flip_vertically_on_load(true)  — 翻转 Y 轴，使 UV (0,0) = 左下角（OpenGL 约定）
2. stbi_load(path, &w, &h, &channels, 4)  — 强制 4 通道（RGBA）
3. 失败 → 打印 stbi_failure_reason()，返回 false
4. 将 unsigned char* 转换为 vector<Color>（逐像素归一化到 [0,1]）
5. stbi_image_free() 释放原始数据
```

**`sample(uv, filter)` 执行流程**：
```
1. Clamp UV 到 [0,1]（Clamp-to-edge 包裹模式）
2. 转换到像素坐标：px = u * (width - 1), py = v * (height - 1)
   （注意：这里不使用 -0.5 偏移，因为我们要的是像素索引而非像素中心）
3. Nearest：取 (round(px), round(py)) 的像素
4. Bilinear：取 4 邻域像素，按 px/py 的小数部分做双线性插值
5. 返回 Color
```

**为什么用 `vector<Color>` 而不是 `unsigned char*`**：
- Color 是本项目的标准像素类型（float vec4），采样返回 Color 无需每次转换
- 内存开销不是问题（512×512 纹理 = 4MB float vs 1MB char，学习项目可接受）
- 双线性插值在 float 空间进行，用 Color 存储避免了每次采样的 uint8→float 转换
- 如果将来做大场景，再优化为 `uint8_t` 存储 + 采样时转换

#### 3.3 Shader.h / Shader.cpp（修改）

**Uniforms 新增纹理指针**：

```cpp
class Texture;  // 前向声明

struct Uniforms {
    glm::mat4 model, view, projection;
    const Texture* texture = nullptr;  // 新增

    Uniforms();
};
```

**新增内置片段着色器**：

```cpp
namespace BuiltinFragmentShader {
    Color passThrough(const FragmentInput&, const Uniforms&);
    Color texture(const FragmentInput&, const Uniforms&);   // 新增：采样纹理
}
```

`texture` 实现：

```cpp
Color BuiltinFragmentShader::texture(const FragmentInput& in, const Uniforms& u) {
    if (u.texture && u.texture->valid()) {
        return u.texture->sample(in.texcoord);
    }
    return in.color;  // 无纹理时回退到顶点颜色
}
```

**顶点着色器 `mvp` 透传 texcoord**：

```cpp
VertexOutput BuiltinVertexShader::mvp(const Vertex& v, const Uniforms& u) {
    return {u.projection * u.view * u.model * v.position, v.color, v.texcoord};
}
```

#### 3.4 Rasterizer.cpp（修改）

在透视校正插值循环中新增 UV 插值。改动集中在 `rasterizeTriangle` 函数：

**预计算部分（循环外）**——新增 uv/w：

```cpp
// 已有：
glm::vec4 c0_div_w = glm::vec4(out0.color) * inv_w0;  // color/w
// 新增：
glm::vec2 t0_div_w = out0.texcoord * inv_w0;           // uv/w
glm::vec2 t1_div_w = out1.texcoord * inv_w1;
glm::vec2 t2_div_w = out2.texcoord * inv_w2;
```

**像素循环内**——新增 UV 恢复：

```cpp
// 已有：
glm::vec4 color_over_w = c0_div_w * w0 + c1_div_w * w1 + c2_div_w * w2;
Color inColor(color_over_w / inv_w);
// 新增：
glm::vec2 texcoord_over_w = t0_div_w * w0 + t1_div_w * w1 + t2_div_w * w2;
glm::vec2 texcoord = texcoord_over_w / inv_w;

Color finalColor = fragShader.process(FragmentInput(inColor, texcoord), uniforms);
```

**FragmentInput 构造变化**：原来是 `FragmentInput(inColor)`，现在变为 `FragmentInput(inColor, texcoord)`——把插值后的 UV 传给片段着色器。

#### 3.5 ObjLoader.cpp（修改）

`loadStandardObj` 中创建 Vertex 时提取 texcoord：

```cpp
auto getTexcoord = [&](const tinyobj::index_t& idx) -> glm::vec2 {
    if (idx.texcoord_index < 0) return glm::vec2(0.0f);
    float u = attrib.texcoords[2 * idx.texcoord_index + 0];
    float v = attrib.texcoords[2 * idx.texcoord_index + 1];
    return glm::vec2(u, v);
};

// Vertex 构造改为三参数：
mesh.addVertex(Vertex(glm::vec4(p0, 1.0f), toColor(p0), getTexcoord(i0)));
```

AOV Bézier 曲面路径（`buildMeshFromPatches`）无需修改——它继续使用双参数构造函数 `Vertex(pos, color)`，texcoord 自动为 `(0,0)`。

---

### 四、获取 stb_image

```bash
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h \
     -O include/stb_image.h
```

在 `src/Texture.cpp` 中：

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
```

stb_image 是 MIT/公共领域授权的单头文件库。支持 PNG、JPEG、BMP、GIF、PSD、HDR 等格式。

---

### 五、实现顺序

| 步骤 | 操作 | 原因 |
|------|------|------|
| 1 | 修改 `Types.h`：texcoord 加入三个结构体 | 数据层改动。旧代码使用双参数构造函数自动得到 `(0,0)`，编译不受影响 |
| 2 | 下载 `stb_image.h` → `include/` | 依赖就位 |
| 3 | 新建 `Texture.h` + `Texture.cpp` | 独立模块，可单独测试（load + sample） |
| 4 | 修改 `Rasterizer.cpp`：新增 UV 插值 | 插值逻辑改动。此时 passThrough 着色器仍忽略 texcoord，渲染结果不变 |
| 5 | 修改 `Shader.h` + `Shader.cpp`：Uniforms 加纹理指针 + 新增内置着色器 | 着色器改动。此时仍可编译运行（texture 指针为 nullptr，回退到 vertex color） |
| 6 | 修改 `ObjLoader.cpp`：提取 texcoord | 模型加载改动 |
| 7 | 修改 `src/CMakeLists.txt` | 新增 Texture.cpp |
| 8 | 修改 `main.cpp`：加载纹理 + 绑定 + 切换着色器 | 端到端串联 |
| 9 | 编译运行 + 验证 | 看到小猫身上出现贴图颜色 |

步骤 4 和 5 之间可任意调换，它们互不依赖。步骤 3 完成后即可在步骤 8 中使用。

---

### 六、关键注意事项

#### 6.1 stb_image Y 轴翻转

**OpenGL UV 约定**：`(0,0)` 在纹理左下角。**stb_image 默认**：`(0,0)` 在图片左上角（第一行像素）。

必须在 `loadFromFile` 中调用 `stbi_set_flip_vertically_on_load(true)` 翻转 Y 轴。否则纹理上下颠倒——小猫的鼻子会在头顶上。

#### 6.2 纹理路径

oiiai 模型的 MTL 引用 `Muchkin2_BaseColor.png`，实际文件在 `assets/oiiai_fbx/textures/Muchkin2_BaseColor.png`。在 main.cpp 中加载时需使用正确路径。或复制纹理到 `assets/` 下统一管理。

当前 `CMakeLists.txt` 的 `POST_BUILD` 只复制了 `assets/` 目录到 build。需确认纹理文件在复制路径中。

#### 6.3 无纹理模型的兼容性

AOV Bézier 曲面（茶壶）和标准 OBJ 都使用相同的 Vertex / VertexOutput 类型。AOV 路径的 Vertex 构造不提供 texcoord → 自动 `(0,0)`。片段着色器 `texture` 在 `texture == nullptr` 时回退到 `in.color`。所有现有模型（茶壶、cube）的行为保持不变。

#### 6.4 UV 超出 [0,1] 的处理

当前阶段固定 Clamp-to-edge：`sample()` 内将 UV 夹到 `[0,1]`。不使用 Repeat 模式——现阶段没有需要 tiling 纹理的场景。当 UV 在 `[0,1]` 之外时，Clamp 产生"边缘拉伸"效果，这比 Repeat 更容易发现 UV 映射问题。

#### 6.5 双线性插值的边界条件

当 UV 落在像素 `(x, y)` 和 `(x+1, y+1)` 之间时，取 4 个邻域像素。边界像素（如 `x+1 >= width`）的邻域 clamp 到边缘。具体实现中——先 clamp 像素坐标，再做 lerp——避免了越界访问。

#### 6.6 Rasterizer 参数不变

UV 在 `VertexOutput` 中而非 Rasterizer 的独立参数——`rasterizeTriangle` 的函数签名不变。插值逻辑的改动是对称的：color 怎么插值，texcoord 就怎么插值。这避免了 Rasterizer 接口膨胀。

#### 6.7 顶点颜色 × 纹理颜色

当前阶段片段着色器直接返回纹理采样结果。阶段 9 光照时，纹理颜色会作为漫反射的 `baseColor` 输入。可以在 `texture` 着色器中加入 `in.color * sample(...)` 的选项，用顶点颜色调制纹理——但当前阶段保持简单：纹理采样颜色直出。

#### 6.8 OBJ 的 texcoord 索引可能为 -1

tinyobjloader 中 `idx.texcoord_index == -1` 表示该面没有纹理坐标（如纯几何模型）。ObjLoader 检测到 `-1` 时返回 `(0,0)`——三角形将被贴纹理的 `(0,0)` 处像素（通常是纹理左下角颜色），表现上是一块纯色区域。

---

### 七、验证方法

1. **编译通过**：`ninja -C build` 无错误，无新增警告
2. **无纹理回退**：不加载纹理时，`passThrough` 着色器的渲染效果和之前完全一致（vertex color 显示正确的模型形状）
3. **纹理加载成功**：`Texture::loadFromFile` 返回 true，`valid()` 返回 true，`width()/height()` 匹配实际图片
4. **小猫贴纹理**：加载 `oiiai.obj` + `Muchkin2_BaseColor.png`，屏幕上小猫显示正确纹理颜色
5. **无纹理模型兼容**：茶壶（AOV Bézier）继续使用 vertex color 渲染，行为不变
6. **最近邻 vs 双线性**：切换 Filter 模式后能观察到块状 vs 平滑的差异
7. **透视校正**：旋转模型到倾斜角度时，纹理无扭曲/滑动感（和非透视校正插值对比，差异明显）
8. **交互正常**：鼠标旋转缩放、键盘 M/Up/Down 均正常工作

---

## 阶段 9：光照

**日期**：（待规划）  
**状态**：[ ] 待实现

**目标**：逐像素漫反射 + 镜面高光，模型呈现立体感。

### 概要

- Vertex 新增 `normal` 字段
- FragmentInput 新增插值后的法线
- Uniforms 新增光源位置/颜色/强度
- BuiltinFragmentShader 新增 `phong` 或 `blinnPhong`
- 法线贴图暂不涉及

---

## 完成后的文件结构（预期）

```
shader-s_principle/
├── CMakeLists.txt
├── CLAUDE.md
├── docs/
│   ├── devlog.md
│   ├── devlog_2.md
│   ├── plan.md
│   └── ref.md
├── include/
│   ├── Types.h
│   ├── Framebuffer.h
│   ├── DepthBuffer.h
│   ├── Screen.h
│   ├── Pipeline.h
│   ├── Rasterizer.h        ← 重构新建
│   ├── Shader.h
│   ├── Mesh.h
│   ├── Camera.h            ← 重构新建
│   ├── Renderer.h          ← 重构新建
│   ├── Control.h           ← 重构新建（原计划外）
│   ├── Texture.h           ← 阶段 8 新建
│   ├── ObjLoader.h         ← 阶段 7 新建
│   └── stb_image.h         ← 阶段 8 新建（第三方单头文件）
└── src/
    ├── CMakeLists.txt
    ├── main.cpp
    ├── Framebuffer.cpp
    ├── DepthBuffer.cpp
    ├── Screen.cpp
    ├── Pipeline.cpp
    ├── Rasterizer.cpp      ← 重构新建
    ├── Shader.cpp
    ├── Mesh.cpp
    ├── Camera.cpp          ← 重构新建
    ├── Renderer.cpp        ← 重构新建
    ├── Control.cpp         ← 重构新建（原计划外）
    ├── Texture.cpp         ← 阶段 8 新建
    └── ObjLoader.cpp       ← 阶段 7 新建
```
