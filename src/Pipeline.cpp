//
// Created by neuroil on 2026/5/24.
//

#include "../include/Pipeline.h"
#include <cmath>
#include <algorithm>

// 将 NDC 坐标转换为屏幕像素坐标
glm::vec2 viewportTransform(const glm::vec4& ndc, int width, int height) {
    float screen_x = (ndc.x + 1.0f) * 0.5f * (float)width;
    float screen_y = (ndc.y + 1.0f) * 0.5f * (float)height;
    return glm::vec2(screen_x, screen_y);
}

// 计算点 p 相对于有向边 AB 的"边函数"值
float edgeFunction(const glm::vec2& a, const glm::vec2& b, const glm::vec2& p) {
    return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
}

void Pipeline::drawTriangle(Framebuffer& fb, const Vertex& v1, const Vertex& v2, const Vertex& v3) {
    // 1. 视窗变换
    glm::vec2 p1 = viewportTransform(v1.position, fb.getWidth(), fb.getHeight());
    glm::vec2 p2 = viewportTransform(v2.position, fb.getWidth(), fb.getHeight());
    glm::vec2 p3 = viewportTransform(v3.position, fb.getWidth(), fb.getHeight());

    // 2. 计算包围盒
    int minX = std::max(0, (int)std::floor(std::min({p1.x, p2.x, p3.x})));
    int maxX = std::min(fb.getWidth() - 1, (int)std::ceil(std::max({p1.x, p2.x, p3.x})));
    int minY = std::max(0, (int)std::floor(std::min({p1.y, p2.y, p3.y})));
    int maxY = std::min(fb.getHeight() - 1, (int)std::ceil(std::max({p1.y, p2.y, p3.y})));

    // 3 ~ 4. 逐像素遍历与内部测试
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            glm::vec2 pixelCenter(x + 0.5f, y + 0.5f);

            float e1 = edgeFunction(p1, p2, pixelCenter);
            float e2 = edgeFunction(p2, p3, pixelCenter);
            float e3 = edgeFunction(p3, p1, pixelCenter);

            if ((e1 >= 0 && e2 >= 0 && e3 >= 0) || (e1 <= 0 && e2 <= 0 && e3 <= 0)) {
                fb.setPixel(x, y, {1, 1, 1});
            }
        }
    }
}
