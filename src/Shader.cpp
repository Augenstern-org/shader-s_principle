#include "../include/Shader.h"

// ########### Uniforms ###########

Uniforms::Uniforms() : model(1.0f), view(1.0f), projection(1.0f) {}

// ########### Vertex-Shader ###########

VertexShader::VertexShader()
    : m_func([](const Vertex& v, const Uniforms& u) -> VertexOutput {
          return {v.position, v.color};
      }) {}

VertexOutput VertexShader::process(const Vertex& v, const Uniforms& u) { return m_func(v, u); }

VertexOutput BuiltinVertexShader::mvp(const Vertex& v, const Uniforms& u) {
    return {u.projection * u.view * u.model * v.position, v.color};
}

VertexOutput BuiltinVertexShader::test(const Vertex& v, const Uniforms& u) {
    glm::vec4 pos = v.position;
    pos.x -= 0.3f;
    pos.y -= 0.3f;
    return {u.projection * u.view * u.model * pos, v.color};
}

// ########### Fragment-Shader ###########

FragmentShader::FragmentShader()
    : m_func([](const FragmentInput& in, const Uniforms&) -> Color { return in.color; }) {}

Color FragmentShader::process(const FragmentInput& in, const Uniforms& u) { return m_func(in, u); }

Color BuiltinFragmentShader::passThrough(const FragmentInput& in, const Uniforms& u){ return in.color;}