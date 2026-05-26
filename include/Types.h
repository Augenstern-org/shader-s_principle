//
// Created by neuroil on 2026/5/23.
//

#ifndef SHADER_S_PRINCIPLE_TYPES_H
#define SHADER_S_PRINCIPLE_TYPES_H

#include <glm/glm.hpp>

// struct Color {
//     float r, g, b;
//     Color() : r(0.0), g(0.0), b(0.0) {}
//     Color(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
// };

// Color -> {r, g, b, a}
struct alignas(16) Color {
    glm::vec4 data;

    Color() : data(0.0f, 0.0f, 0.0f, 0.0f) {}
    Color(float r, float g, float b, float a = 0.0f) : data(r, g, b, a) {}

    // 从 glm::vec4 隐式构造
    Color(const glm::vec4& v) : data(v) {}

    float& r() { return data.x; }
    float& g() { return data.y; }
    float& b() { return data.z; }
    float& a() { return data.w; }

    operator glm::vec4&() { return data; }
    operator const glm::vec4&() const { return data; }
};

struct Vertex {
    glm::vec4 position;
    Color color;
    Vertex() : position(0, 0, 0, 1) {}
    Vertex(const glm::vec4& pos) : position(pos) {}
    Vertex(const glm::vec4& pos, const Color& c) : position(pos), color(c) {}
};

struct VertexOutput {
    glm::vec4 position;
    Color color;

    VertexOutput() : position(0.0f), color() {}
    VertexOutput(const glm::vec4& p, const Color& c) : position(p), color(c) {}
};

struct FragmentInput {
    Color color;
    FragmentInput(Color c):color(c.data){}
};

#endif // SHADER_S_PRINCIPLE_TYPES_H
