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
    glm::vec2 texcoord;

    Vertex() : position(0, 0, 0, 1), texcoord(0.0f) {}
    Vertex(const glm::vec4& pos) : position(pos), texcoord(0.0f) {}
    Vertex(const glm::vec4& pos, const Color& c) : position(pos), color(c), texcoord(0.0f) {}
    Vertex(const glm::vec4& pos, const Color& c, const glm::vec2& uv) : position(pos), color(c), texcoord(uv) {}
};

struct VertexOutput {
    glm::vec4 position;
    Color color;
    glm::vec2 texcoord;

    VertexOutput() : position(0.0f), color() {}
    VertexOutput(const glm::vec4& p, const Color& c) : position(p), color(c) {}
    VertexOutput(const glm::vec4& p, const Color& c, const glm::vec2& uv) : position(p), color(c), texcoord(uv) {}
};

struct FragmentInput {
    Color color;
    glm::vec2 texcoord;

    explicit FragmentInput(Color c, glm::vec2 uv = glm::vec2(0.0f)) : color(c.data), texcoord(uv) {}
};

#endif // SHADER_S_PRINCIPLE_TYPES_H
