# shader's principle

纯 CPU 实现的软件光栅化渲染器，模拟 GPU 渲染管线。仅使用 GLFW + OpenGL 做显示输出，所有渲染计算在 CPU 上完成。

## 特性

- **完整的可编程渲染管线** — 顶点着色器 → 透视除法 → 视口变换 → 光栅化 → 片段着色器
- **深度缓冲** — 逐像素深度测试
- **背面剔除** — 屏幕空间 CCW 判定
- **透视校正插值** — 颜色与纹理坐标的透视校正
- **纹理采样** — Nearest / Bilinear 两种过滤模式
- **轨道相机** — 球坐标系，支持旋转/平移/缩放
- **模型加载** — 支持标准 OBJ 和 AOV Bezier 曲面（Utah teapot）

## 构建

依赖由 vcpkg 管理，构建配置使用 CMakePresets.json。

```bash
# 安装 vcpkg（一次性）
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
~/vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=~/vcpkg

# 配置（Debug 模式）
cmake --preset debug

# 编译
ninja -C build/debug

# 运行
./build/debug/src/test
```

Release 模式将 `debug` 替换为 `release` 即可。

## 依赖

| 依赖 | 用途 |
|------|------|
| GLFW 3.4 | 窗口创建与输入 |
| glad | OpenGL 加载 |
| GLM | 数学库（向量/矩阵） |
| stb_image | 图像加载（header-only，已内置） |
| tinyobjloader | OBJ 模型解析（header-only，已内置） |

## 操作

| 按键 | 功能 |
|------|------|
| 鼠标左键拖拽 | 旋转视角 |
| 鼠标右键拖拽 | 平移视角 |
| 滚轮 | 缩放 |
| Space | 切换自动旋转 |
| ↑ / ↓ | 调整旋转速度 |

## 项目结构

```
include/    — 头文件（每模块一个）
src/        — 实现文件 + main.cpp
assets/     — 模型与纹理资源
docs/       — 开发日志
```
