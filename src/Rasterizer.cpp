//
// Created by neuroil on 2026/5/27.
//

#include "Rasterizer.h"
#include <algorithm>
#include <cmath>

namespace Rasterizer {

static float edgeFunction(const glm::vec2& a, const glm::vec2& b, const glm::vec2& p) {
    return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
}

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
) {
    float area = edgeFunction(p0, p1, p2);
    if (std::abs(area) < 1e-8f) return;

    int minX = std::max(0, (int)std::floor(std::min({p0.x, p1.x, p2.x})));
    int maxX = std::min(fb.getWidth() - 1, (int)std::ceil(std::max({p0.x, p1.x, p2.x})));
    int minY = std::max(0, (int)std::floor(std::min({p0.y, p1.y, p2.y})));
    int maxY = std::min(fb.getHeight() - 1, (int)std::ceil(std::max({p0.y, p1.y, p2.y})));

    float inv_w0 = 1.0f / out0.position.w;
    float inv_w1 = 1.0f / out1.position.w;
    float inv_w2 = 1.0f / out2.position.w;
    glm::vec4 c0_div_w = glm::vec4(out0.color) * inv_w0;
    glm::vec4 c1_div_w = glm::vec4(out1.color) * inv_w1;
    glm::vec4 c2_div_w = glm::vec4(out2.color) * inv_w2;

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            glm::vec2 pixelCenter(x + 0.5f, y + 0.5f);

            float e1 = edgeFunction(p0, p1, pixelCenter);
            float e2 = edgeFunction(p1, p2, pixelCenter);
            float e3 = edgeFunction(p2, p0, pixelCenter);

            if ((e1 >= 0 && e2 >= 0 && e3 >= 0) || (e1 <= 0 && e2 <= 0 && e3 <= 0)) {
                float w0 = e2 / area;
                float w1 = e3 / area;
                float w2 = e1 / area;

                float depth_ndc = w0 * ndc0.z + w1 * ndc1.z + w2 * ndc2.z;
                float depth_01 = depth_ndc * 0.5f + 0.5f;

                if (depthBuffer && !depthBuffer->testAndSet(x, y, depth_01)) continue;

                float inv_w = w0 * inv_w0 + w1 * inv_w1 + w2 * inv_w2;
                glm::vec4 color_over_w = c0_div_w * w0 + c1_div_w * w1 + c2_div_w * w2;

                Color inColor(color_over_w / inv_w);
                Color finalColor = fragShader.process(FragmentInput(inColor), uniforms);
                fb.setPixel(x, y, finalColor);
            }
        }
    }
}

} // namespace Rasterizer
