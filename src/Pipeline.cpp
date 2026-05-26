//
// Created by neuroil on 2026/5/24.
//

#include "Pipeline.h"
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
    // 透视除法
    glm::vec3 ndc_0(out_0.position / out_0.position.w);
    glm::vec3 ndc_1(out_1.position / out_1.position.w);
    glm::vec3 ndc_2(out_2.position / out_2.position.w);
    // 视窗变换
    glm::vec2 p0 = viewportTransform(glm::vec4(ndc_0, 1.0f), fb.getWidth(), fb.getHeight());
    glm::vec2 p1 = viewportTransform(glm::vec4(ndc_1, 1.0f), fb.getWidth(), fb.getHeight());
    glm::vec2 p2 = viewportTransform(glm::vec4(ndc_2, 1.0f), fb.getWidth(), fb.getHeight());

    // 退化三角形检测
    float area = edgeFunction(p0, p1, p2);
    if (std::abs(area) < 1e-8f) return;

    // 计算包围盒
    int minX = std::max(0, (int)std::floor(std::min({p0.x, p1.x, p2.x})));
    int maxX = std::min(fb.getWidth() - 1, (int)std::ceil(std::max({p0.x, p1.x, p2.x})));
    int minY = std::max(0, (int)std::floor(std::min({p0.y, p1.y, p2.y})));
    int maxY = std::min(fb.getHeight() - 1, (int)std::ceil(std::max({p0.y, p1.y, p2.y})));

    // 循环体之前计算
    float inv_w0 = 1.0f / out_0.position.w;
    float inv_w1 = 1.0f / out_1.position.w;
    float inv_w2 = 1.0f / out_2.position.w;
    glm::vec4 c0_div_w = glm::vec4(out_0.color) * inv_w0;
    glm::vec4 c1_div_w = glm::vec4(out_1.color) * inv_w1;
    glm::vec4 c2_div_w = glm::vec4(out_2.color) * inv_w2;

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

                float depth_ndc = w0 * ndc_0.z + w1 * ndc_1.z + w2 * ndc_2.z;
                float depth_01 = depth_ndc * 0.5f + 0.5f;

                if (m_depthBuffer && !m_depthBuffer->testAndSet(x, y, depth_01)) continue;

                // 插值
                float inv_w = w0 * inv_w0 + w1 * inv_w1 + w2 * inv_w2;
                glm::vec4 color_over_w = c0_div_w * w0 + c1_div_w * w1 + c2_div_w * w2;

                // 恢复
                Color inColor(color_over_w / inv_w);
                Color finalColor = m_fragmentShader.process(FragmentInput(inColor), uni);
                fb.setPixel(x, y, finalColor);
            }
        }
    }
}

void Pipeline::drawMesh(Framebuffer& fb, const Mesh& mesh, const Uniforms& uni) {
    for (size_t t = 0; t < mesh.triangleCount(); ++t) {
        size_t i = t * 3;
        drawTriangle(fb, mesh.getVertex(i), mesh.getVertex(i + 1), mesh.getVertex(i + 2), uni);
    }
}

void Pipeline::setDepthBuffer(DepthBuffer* db) { m_depthBuffer = db; }
