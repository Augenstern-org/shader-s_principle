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

    Uniforms();
};

class VertexShader {
    std::function<glm::vec4(const Vertex&, const Uniforms&)> m_func;

  public:
    using ShaderFunc = std::function<glm::vec4(const Vertex&, const Uniforms&)>;

    VertexShader();
    explicit VertexShader(ShaderFunc f) : m_func(f) {}
    glm::vec4 process(const Vertex&, const Uniforms&);
};

namespace BuiltinVertexShader {

glm::vec4 mvp(const Vertex&, const Uniforms&);
glm::vec4 test(const Vertex&, const Uniforms&);

} // namespace BuiltinVertexShader

#endif // SHADER_S_PRINCIPLE_SHADER_H