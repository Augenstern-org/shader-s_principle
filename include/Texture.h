//
// Created by neuroil on 2026/5/28.
//

#ifndef SHADER_S_PRINCIPLE_TEXTURE_H
#define SHADER_S_PRINCIPLE_TEXTURE_H

#include "Types.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Texture {
    std::vector<Color> m_data;
    int m_width = 0;
    int m_height = 0;

public:
    enum class Fliter { Nearest, Bilinear };

    Texture() = default;
    ~Texture() = default;

    bool loadFromFile(const std::string& filePath);

    Color sample(const glm::vec2& uv, Fliter fliter = Fliter::Bilinear) const;
    bool valid() const;

    int width() const { return m_width; }
    int height() const { return m_height; }
};


#endif //SHADER_S_PRINCIPLE_TEXTURE_H
