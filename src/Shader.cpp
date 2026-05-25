#include "../include/Shader.h"

Uniforms::Uniforms() : model(1.0f), view(1.0f), projection(1.0f) {}

VertexShader::VertexShader()
    : m_func([](const Vertex& v, const Uniforms& u) -> glm::vec4 { return v.position; }) {}

glm::vec4 VertexShader::process(const Vertex& v, const Uniforms& u) { return m_func(v, u); }

glm::vec4 BuiltinVertexShader::mvp(const Vertex& v, const Uniforms& u) {
    return u.projection * u.view * u.model * v.position;
}