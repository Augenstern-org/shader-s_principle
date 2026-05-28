//
// Created by neuroil on 2026/5/28.
//

#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

bool Texture::loadFromFile(const std::string& filePath) {
    stbi_set_flip_vertically_on_load(true);

    int channels;
    unsigned char* pixels = stbi_load(
        filePath.c_str(),
        &m_width, &m_height,
        &channels, 4
    );

    if (!pixels) {
        std::fprintf(stderr, "[Texture] 加载失败: %s — %s\n",
                     filePath.c_str(), stbi_failure_reason());
        return false;
    }

    m_data.resize(m_width * m_height);
    for (int i = 0; i < m_width * m_height; ++i) {
        float r = pixels[i * 4 + 0] / 255.0f;
        float g = pixels[i * 4 + 1] / 255.0f;
        float b = pixels[i * 4 + 2] / 255.0f;
        float a = pixels[i * 4 + 3] / 255.0f;
        m_data[i] = Color(r, g, b, a);
    }

    stbi_image_free(pixels);
    std::fprintf(stderr, "[Texture] 已加载: %s (%dx%d)\n",
                 filePath.c_str(), m_width, m_height);
    return true;
}

Color Texture::sample(const glm::vec2& uv, Fliter filter) const {
    if (m_data.empty()) return Color(0.0f, 0.0f, 0.0f, 0.0f);

    float px = uv.x * (m_width - 1);
    float py = uv.y * (m_height - 1);

    if (filter == Fliter::Nearest) {
        int x = std::clamp((int)std::round(px), 0, m_width - 1);
        int y = std::clamp((int)std::round(py), 0, m_height - 1);
        return m_data[y * m_width + x];
    }

    // Bilinear
    int x0 = (int)std::floor(px);
    int y0 = (int)std::floor(py);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    x0 = std::clamp(x0, 0, m_width - 1);
    x1 = std::clamp(x1, 0, m_width - 1);
    y0 = std::clamp(y0, 0, m_height - 1);
    y1 = std::clamp(y1, 0, m_height - 1);

    float fx = px - std::floor(px);
    float fy = py - std::floor(py);

    Color top    = glm::mix(m_data[y0 * m_width + x0].data,
                            m_data[y0 * m_width + x1].data, fx);
    Color bottom = glm::mix(m_data[y1 * m_width + x0].data,
                            m_data[y1 * m_width + x1].data, fx);
    return Color(glm::mix(top.data, bottom.data, fy));
}

bool Texture::valid() const {
    return !m_data.empty();
}
