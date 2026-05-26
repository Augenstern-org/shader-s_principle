//
// Created by neuroil on 2026/5/24.
//

#include "../include/Pipeline.h"
#include <algorithm>
#include <cmath>

// 将 NDC 坐标转换为屏幕像素坐标
glm::vec2 viewportTransform(const glm::vec4& ndc, int width, int height) {
    float screen_x = (1.0f + ndc.x) * 0.5f * (float)width;
    float screen_y = (1.0f - ndc.y) * 0.5f * (float)height;
    return glm::vec2(screen_x, screen_y);
}

// 计算点 p 相对于有向边 AB 的"边函数"值
float edgeFunction(const glm::vec2& a, const glm::vec2& b, const glm::vec2& p) {
    return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
}

void Pipeline::setVertexShader(const VertexShader& shader) { m_vertexShader = shader; }
void Pipeline::setFragmentShader(const FragmentShader& shader) { m_fragmentShader = shader; }

void Pipeline::drawTriangle(Framebuffer& fb, const Vertex& v0, const Vertex& v1, const Vertex& v2,
                            const Uniforms& uni) {
    // 着色器
    VertexOutput out_0 = m_vertexShader.process(v0, uni);
    VertexOutput out_1 = m_vertexShader.process(v1, uni);
    VertexOutput out_2 = m_vertexShader.process(v2, uni);
    // 视窗变换
    glm::vec2 p0 = viewportTransform(out_0.position, fb.getWidth(), fb.getHeight());
    glm::vec2 p1 = viewportTransform(out_1.position, fb.getWidth(), fb.getHeight());
    glm::vec2 p2 = viewportTransform(out_2.position, fb.getWidth(), fb.getHeight());

    // 退化三角形检测
    float area = edgeFunction(p0, p1, p2);
    if (area == 0.0f) return;

    // 计算包围盒
    int minX = std::max(0, (int)std::floor(std::min({p0.x, p1.x, p2.x})));
    int maxX = std::min(fb.getWidth() - 1, (int)std::ceil(std::max({p0.x, p1.x, p2.x})));
    int minY = std::max(0, (int)std::floor(std::min({p0.y, p1.y, p2.y})));
    int maxY = std::min(fb.getHeight() - 1, (int)std::ceil(std::max({p0.y, p1.y, p2.y})));

    // 逐像素遍历与内部测试
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

                Color inColor = {w0 * out_0.color.r() + w1 * out_1.color.r() + w2 * out_2.color.r(),
                                 w0 * out_0.color.g() + w1 * out_1.color.g() + w2 * out_2.color.g(),
                                 w0 * out_0.color.b() + w1 * out_1.color.b() + w2 * out_2.color.b(),
                                 0.0f};

                Color finalColor = m_fragmentShader.process(FragmentInput(inColor), uni);
                fb.setPixel(x, y, finalColor);
            }
        }
    }
}
