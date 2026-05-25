//
// Created by neuroil on 2026/5/23.
//

#ifndef SHADER_S_PRINCIPLE_TYPES_H
#define SHADER_S_PRINCIPLE_TYPES_H

#include <glm/glm.hpp>

struct Color {
    float r, g, b;
    Color() : r(0.0), g(0.0), b(0.0) {}
    Color(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
};

struct Vertex {
    glm::vec4 position;
    Vertex() : position(0, 0, 0, 1) {}
    Vertex(const glm::vec4& pos) : position(pos) {}
};

#endif // SHADER_S_PRINCIPLE_TYPES_H
