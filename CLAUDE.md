# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build commands

```bash
# Configure (from repo root)
cmake -B build -G Ninja

# Build
ninja -C build

# Run
./build/src/test
```

There are no tests or linting setup yet.

## Architecture

A CPU software rasterizer that simulates the GPU rendering pipeline. GLFW + OpenGL are used **only for display output** — all rendering happens on the CPU.

**Dependency graph** (bottom-up, no cycles):

```
Types.h  (Color, Vertex, VertexOutput, FragmentInput)
   ↑
Framebuffer   Shader.h (vertex + fragment shaders + Uniforms)   Mesh.h (triangle container)   DepthBuffer.h (depth storage + test)
   ↑              ↑                                              ↑                              ↑
Screen         Rasterizer (edge-function rasterizer, barycentric + perspective-correct interp, depth test, fragment shader invocation)
   ↑              ↑
  main.cpp  ←  Renderer (owns Framebuffer, DepthBuffer, Pipeline, shaders — high-level facade)
                ↑
             Pipeline (vertex shader → perspective divide → viewport → Rasterizer)
                ↑
             Camera (orbit camera, spherical coords → view + projection matrices)
                ↑
             Control (input handling, object auto-rotation toggle)
```

**Data flow through the pipeline:**

```
[Model-space vertices] → [VertexShader (MVP)] → [clip coords] → [perspective divide] → [NDC coords] → [viewportTransform] → [screen coords] → [Rasterizer: edge function → barycentric + perspective-correct interp → depth test → FragmentShader] → [Framebuffer::setPixel]
```

### Key design changes from earlier stages

- **Pipeline is stateless** — shaders and DepthBuffer are passed as parameters to `drawTriangle()`/`drawMesh()`, not stored as member variables. The Renderer class stores them and forwards them.
- **Rasterizer extracted** into its own namespace (`Rasterizer::rasterizeTriangle`) — separates the rasterization algorithm from the pipeline orchestration.
- **Renderer is the high-level facade** — owns Framebuffer, DepthBuffer, Pipeline, and the current shader bindings. `main.cpp` talks to Renderer, not directly to Pipeline.
- **Camera uses spherical coordinates** — orbit camera with `theta/phi/radius` around a target point. Mouse drag (left=orbit, right=pan) and scroll (zoom).
- **Control decouples input from camera** — handles key presses (M toggles auto-rotation, Up/Down adjusts speed) and forwards mouse events to Camera.

## Current state

Post-refactoring (commits `bb242c2` through `95718f5`). All five stages complete plus architectural refactoring.

| Module | File(s) | Role |
|--------|---------|------|
| `Types.h` | `include/Types.h` | `Color` (vec4-based, `alignas(16)`, with `r()/g()/b()/a()` accessors), `Vertex` (pos + color), `VertexOutput`, `FragmentInput`. Depends only on GLM. |
| `Framebuffer` | `include/Framebuffer.h`, `src/Framebuffer.cpp` | CPU pixel buffer. `clear()`, `setPixel()`, `getPixels()`. |
| `Screen` | `include/Screen.h`, `src/Screen.cpp` | GLFW window + OpenGL texture upload for display. `present()` uploads RGBA float data via `glTexImage2D`. VSync disabled (`glfwSwapInterval(0)`). |
| `Pipeline` | `include/Pipeline.h`, `src/Pipeline.cpp` | Stateless: vertex shader → perspective divide → viewport transform → delegates to `Rasterizer::rasterizeTriangle`. `drawTriangle()` + `drawMesh()`. |
| `Rasterizer` | `include/Rasterizer.h`, `src/Rasterizer.cpp` | Namespace with `rasterizeTriangle()`: edge-function rasterizer with barycentric + perspective-correct interpolation, depth test, fragment shader invocation. |
| `Shader` | `include/Shader.h`, `src/Shader.cpp` | `VertexShader` (returns `VertexOutput`), `FragmentShader` (returns `Color`), `Uniforms` (model/view/projection), builtin shaders. |
| `Mesh` | `include/Mesh.h`, `src/Mesh.cpp` | Triangle container. `addTriangle()`, `triangleCount()`, `getVertex()`. |
| `DepthBuffer` | `include/DepthBuffer.h`, `src/DepthBuffer.cpp` | Per-pixel depth storage [0,1]. `clear()`, `testAndSet()` (LESS test). |
| `Camera` | `include/Camera.h`, `src/Camera.cpp` | Orbit camera. Spherical coords (`theta/phi/radius`) around target. `viewMatrix()` via `lookAt`, `projectionMatrix()` via `perspective`. Mouse drag/scroll. |
| `Control` | `include/Control.h`, `src/Control.cpp` | Input dispatch. Forwards mouse to Camera, handles keys (M=toggle auto-rotate, Up/Down=speed). `update(dt)` advances object rotation. |
| `Renderer` | `include/Renderer.h`, `src/Renderer.cpp` | High-level facade. Owns Framebuffer, DepthBuffer, Pipeline. `beginFrame()`/`draw()`/`endFrame()`. Stores current shader bindings. |
| `main` | `src/main.cpp` | Creates Screen, Renderer, Camera, Control. Wires GLFW callbacks. Debug profiling (`#ifndef NDEBUG`) reports avg frame time every 60 frames. |

## Key conventions

- **`#include <glad/glad.h>` must come before any other GL includes** (including GLFW). Currently this include is inside `Screen.cpp`.
- **GLM is column-major**, matching OpenGL conventions. Matrix multiplication reads right-to-left: `P * V * M * vertex`.
- **NDC coordinates**: x right, y up, origin at center. Range [-1, 1].
- **Viewport transform**: `screen_x = (1 + ndc.x) * 0.5 * width`, `screen_y = (1 - ndc.y) * 0.5 * height`. NDC y=+1 maps to screen bottom (y=0), NDC y=-1 maps to screen top (y=height).
- **Color is `alignas(16) glm::vec4`** with `r()/g()/b()/a()` accessors returning `float&`. Implicitly converts to/from `glm::vec4`. Alpha channel exists but is unused in rendering.
- **Screen upload format is `GL_RGBA` + `GL_FLOAT`** — matches Color's 16-byte vec4 layout.
- **Edge function rasterizer** uses pixel-center sampling `(x+0.5, y+0.5)` and accepts both CW and CCW winding (all-positive or all-negative check).
- **`std::function`** for shader function dispatch — no virtual classes, no templates needed at this scale.
- **Pipeline is stateless** — shaders and DepthBuffer are parameters, not members. Renderer owns the state.
- `docs/devlog.md` contains detailed per-stage implementation notes for the 5-stage prototype. **Prototype development is complete** — do not add new stages to this file.
- `docs/devlog_2.md` is the extension development log (refactoring + future stages). **All new planning and implementation notes go here.**
- `docs/plan.md` contains the architectural rationale and design principles guiding module splits.
- `docs/ref.md` contains the rules about teaching the user.

## Planning workflow (from ref.md)

When the user asks to plan the next development stage, follow this process:

1. **Read-only analysis**: Read current source files to understand the codebase state. Do NOT modify any code.
2. **Write the plan to docs/devlog_2.md**: Append the new stage plan at the end of `docs/devlog_2.md`, following the existing format (timestamp header `## YYYY-MM-DD`, then sections for goals, rationale, file changes, module specs, key notes, and verification methods).
3. **Report summary to user**: Output a concise summary of what was planned and written.

**Important**: During planning, only `docs/devlog_2.md` and `CLAUDE.md` may be modified. Source files under `include/` and `src/` must NOT be changed until the user explicitly asks to implement the plan.
