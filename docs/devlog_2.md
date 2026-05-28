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

**日期**：（待规划）  
**状态**：[ ] 待实现

**目标**：解析 `.obj` 文件（Wavefront OBJ 格式），生成 Mesh，替代手写三角形。

### 概要

- 新建 `ObjLoader` 模块：逐行解析 `.obj` 的 `v`（顶点位置）、`vt`（纹理坐标）、`vn`（法线）、`f`（面）
- 仅支持三角形面（`f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3`）
- 或使用 tinyobjloader 等单头文件库
- 加载 oiioi 小猫模型，替换 demo 三角形

---

## 阶段 8：纹理映射

**日期**：（待规划）  
**状态**：[ ] 待实现

**目标**：支持纹理采样，三角形可贴图片。

### 概要

- 新建 `Texture` 模块：CPU 端图片存储 + 采样接口
- 图片加载：`stb_image.h`（单头文件，支持 PNG/JPEG/BMP）
- 采样模式：最近邻（nearest）+ 双线性（bilinear）
- FragmentInput 新增 `uv` 字段，Rasterizer 透视校正插值 UV
- FragmentShader 可通过 `Uniforms` 访问绑定的纹理

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
│   └── ObjLoader.h         ← 阶段 7 新建
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
