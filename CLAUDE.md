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
Screen         Pipeline (vertex shader → perspective divide → viewport → rasterization → depth test → fragment shader)
   ↑              ↑
   main.cpp  (wires everything, owns the game loop)
```

**Data flow through the pipeline:**

```
[Model-space vertices] → [VertexShader (MVP)] → [clip coords] → [perspective divide] → [NDC coords] → [viewportTransform] → [screen coords] → [edge function rasterizer] → [barycentric + perspective-correct interp] → [depth test] → [FragmentShader] → [Framebuffer::setPixel]
```

## Current state

**Stage 5 complete** — all five stages implemented. Depth buffer with perspective-correct interpolation is working.

| Module | File(s) | Role |
|--------|---------|------|
| `Types.h` | `include/Types.h` | `Color` (vec4-based with accessors), `Vertex` (pos + color), `VertexOutput`, `FragmentInput`. Depends only on GLM. |
| `Framebuffer` | `include/Framebuffer.h`, `src/Framebuffer.cpp` | CPU pixel buffer. `clear()`, `setPixel()`, `getPixels()`. |
| `Screen` | `include/Screen.h`, `src/Screen.cpp` | GLFW window + OpenGL texture upload for display. |
| `Pipeline` | `include/Pipeline.h`, `src/Pipeline.cpp` | Vertex shader → perspective divide → viewport → edge-function rasterizer (barycentric + perspective-correct interp) → depth test → fragment shader. `drawTriangle()` + `drawMesh()`. |
| `Shader` | `include/Shader.h`, `src/Shader.cpp` | `VertexShader` (returns `VertexOutput`), `FragmentShader` (returns `Color`), `Uniforms` (model/view/projection), builtin shaders. |
| `Mesh` | `include/Mesh.h`, `src/Mesh.cpp` | Triangle container. `addTriangle()`, `triangleCount()`, `getVertex()`. |
| `DepthBuffer` | `include/DepthBuffer.h`, `src/DepthBuffer.cpp` | Per-pixel depth storage [0,1]. `clear()`, `testAndSet()` (LESS test). |
| `main` | `src/main.cpp` | Creates mesh with 2 triangles at different z-depths, perspective projection, depth buffer, rotating model matrix, main loop. |

## Key conventions

- **`#include <glad/glad.h>` must come before any other GL includes** (including GLFW). Currently this include is inside `Screen.cpp`.
- **GLM is column-major**, matching OpenGL conventions. Matrix multiplication reads right-to-left: `P * V * M * vertex`.
- **NDC coordinates**: x right, y up, origin at center. Range [-1, 1]. The viewport transform maps NDC (-1,-1) to screen bottom-left.
- **Edge function rasterizer** uses pixel-center sampling `(x+0.5, y+0.5)` and accepts both CW and CCW winding (all-positive or all-negative check).
- **`std::function`** for shader function dispatch — no virtual classes, no templates needed at this scale.
- `plan.md` contains the architectural rationale and design principles guiding module splits.
- `devlog.md` contains detailed per-stage implementation notes.
- `ref.md` contains the rules about teaching the user.

## Planning workflow (from ref.md)

When the user asks to plan the next development stage, follow this process:

1. **Read-only analysis**: Read current source files to understand the codebase state. Do NOT modify any code.
2. **Write the plan to devlog.md**: Append the new stage plan at the end of `devlog.md`, following the existing format (timestamp header `## YYYY-MM-DD`, then sections for goals, rationale, file changes, module specs, key notes, and verification methods).
3. **Report summary to user**: Output a concise summary of what was planned and written.

**Important**: During planning, only `devlog.md` and `CLAUDE.md` may be modified. Source files under `include/` and `src/` must NOT be changed until the user explicitly asks to implement the plan.
