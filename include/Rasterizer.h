//
// Created by neuroil on 2026/5/27.
//

#ifndef SHADER_S_PRINCIPLE_RASTERIZER_H
#define SHADER_S_PRINCIPLE_RASTERIZER_H

#include "Types.h"
#include "Framebuffer.h"
#include "DepthBuffer.h"
#include "Shader.h"
#include <glm/glm.hpp>

namespace Rasterizer {

void rasterizeTriangle(
    Framebuffer& fb,
    DepthBuffer* depthBuffer,
    const FragmentShader& fragShader,
    const Uniforms& uniforms,
    const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2,
    const glm::vec3& ndc0, const glm::vec3& ndc1, const glm::vec3& ndc2,
    const VertexOutput& out0,
    const VertexOutput& out1,
    const VertexOutput& out2
);

} // namespace Rasterizer

#endif // SHADER_S_PRINCIPLE_RASTERIZER_H
