# remind.md — 踩坑记录

记录本项目中犯过的错误及注意事项，避免重复踩坑。

---

## 1. 数据结构 layout 变了，必须同步更新所有"传输点"

**场景**：`Color` 从 `struct { float r,g,b; }`（12 字节）改为 `alignas(16) { glm::vec4 data; }`（16 字节）。

**现象**：屏幕画面颜色完全错乱，像素之间颜色互相串扰。

**根因**：`Screen::present()` 用 `glTexImage2D(..., GL_RGB, GL_FLOAT, ...)` 上传像素数据。`GL_RGB` + `GL_FLOAT` 告诉 OpenGL"每像素 3 个 float = 12 字节"。但实际 `Color` 已经是 16 字节（4 个 float），OpenGL 按 12 字节步长读内存，从第 2 个像素开始全都错位。

**教训**：改了数据结构的 `sizeof` 或 `alignof` 后，全局搜索以下模式，确保它们同步更新：
- `sizeof(T)` 被用于计算 buffer 大小的地方
- `reinterpret_cast<const char*>` 或 `.data()` 传给外部 API 的地方（OpenGL、fwrite、socket send 等）
- 任何假设"一个元素占 N 字节"的硬编码常量

**修复**：`GL_RGB` → `GL_RGBA`，让 OpenGL 知道源数据是 4 通道。

---

## 2. 访问器函数要同时提供 const 和非 const 版本

**场景**：`Color` 提供了 `float& r()` 等修改器，但 `Framebuffer::getPixels()` 返回 `const std::vector<Color>&`。

**现象**：用 `const auto& c = pixels[i]` 遍历时，`c.r()` 编译失败——非 const 方法不能用在 const 对象上。

**教训**：如果一个类有 getter/setter 风格的方法，要么同时写 `const` 重载：
```cpp
float& r() { return data.x; }
const float& r() const { return data.x; }
```
要么把读操作用不同方式暴露（比如公开成员，或者用 `operator[]`）。

---

## 3. 默认值要考虑"不可见"场景

**场景**：`Color()` 默认初始化为 `(0,0,0,0)`（透明黑色），`Vertex` 的 `color` 字段用默认值。

**现象**：三角形在深色背景上渲染为黑色，几乎看不到，难以判断是渲染逻辑错了还是颜色设错了。

**教训**：数据类型的默认值应该选一个"出错时显眼"的值。对于颜色，白色 `(1,1,1)` 或品红色 `(1,0,1)` 比黑色更好——前者是"正常默认"，后者是"你忘了设颜色"的信号。

---

## 4. 改完 bug 要能"证明"修好了

**场景**：只改 `GL_RGB` → `GL_RGBA` 但没验证，因为三角形仍是黑色（顶点颜色默认黑），看不到效果。

**教训**：修一个 bug 之后，确认测试用例能覆盖这个 bug 的场景。如果当前数据让 bug 修没修好看不出来，先修正测试数据（比如给顶点设置显眼的颜色），再验证修复效果。

---

## 5. 先跑起来，再重构

**已内化进项目原则**：阶段 1 就是一个能清屏的窗口，阶段 2 画三角形，每步都有可见输出。

**反例（来自上一版）**：一次性写了几百行 DescriptorSet / DescriptorPool 但没有能跑的入口，最后全废了。

---

## 6. include 路径要统一，别混用相对路径

**现状**：项目中 `.cpp` 文件混用了 `"../include/xxx.h"` 和 `"xxx.h"` 两种 include 风格，都能编译因为 CMake 配了 `include_directories(include)`。

**风险**：如果将来改 CMake 配置或移动文件，`"../include/"` 路径会全坏。统一用 `"xxx.h"` 更稳健。

---

## 7. Git commit 前要编译

*（预留，暂无相关踩坑）*

---

## 更新记录

- **2026-05-26**：记录 Color layout / GL_RGB 对齐问题、const 访问器缺失、默认值选择、验证方法、include 路径风格。
