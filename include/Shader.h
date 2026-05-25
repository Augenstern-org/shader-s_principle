//
// Created by neuroil on 2026/5/24.
//
#ifndef SHADER_S_PRINCIPLE_SHADER_H
#define SHADER_S_PRINCIPLE_SHADER_H

#include "Types.h"
#include <functional>
#include <glm/glm.hpp>

struct Uniforms {
    glm::mat4 model, view, projection;

    Uniforms()
        : model(glm::mat4(1.0f)), view(glm::mat4(1.0f)),
          projection(glm::mat4(1.0f)) {}
};

class VertexShader {
    std::function<glm::vec4(const Vertex&, const Uniforms&)> m_func;

  public:
    using ShaderFunc = std::function<glm::vec4(const Vertex&, const Uniforms&)>;

    VertexShader()
        : m_func([](const Vertex& v, const Uniforms& u) -> glm::vec4 {
              return v.position;
          }) {}
    explicit VertexShader(ShaderFunc f) : m_func(f) {}
    glm::vec4 process(const Vertex&, const Uniforms&);
};

#endif // SHADER_S_PRINCIPLE_SHADER_H