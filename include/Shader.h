//
// Created by neuroil on 2026/5/24.
//
#ifndef SHADER_S_PRINCIPLE_SHADER_H
#define SHADER_S_PRINCIPLE_SHADER_H

#include "Types.h"
#include <functional>
#include <glm/glm.hpp>

// ########### Uniforms ###########

struct Uniforms {
    glm::mat4 model, view, projection;

    Uniforms();
};

// ########### Vertex-Shader ###########

class VertexShader {
    std::function<VertexOutput(const Vertex&, const Uniforms&)> m_func;

  public:
    using ShaderFunc = std::function<VertexOutput(const Vertex&, const Uniforms&)>;

    VertexShader();
    explicit VertexShader(ShaderFunc f) : m_func(f) {}
    VertexOutput process(const Vertex&, const Uniforms&) const;
};

namespace BuiltinVertexShader {

VertexOutput mvp(const Vertex&, const Uniforms&);
VertexOutput test(const Vertex&, const Uniforms&);

} // namespace BuiltinVertexShader

// ########### Fragment-Shader ###########

class FragmentShader {
    std::function<Color(const FragmentInput&, const Uniforms&)> m_func;

  public:
    using ShaderFunc = std::function<Color(const FragmentInput&, const Uniforms&)>;

    FragmentShader();
    explicit FragmentShader(ShaderFunc f) : m_func(f) {}
    Color process(const FragmentInput&, const Uniforms&) const;
};

namespace BuiltinFragmentShader{

Color passThrough(const FragmentInput&, const Uniforms&);

}

#endif // SHADER_S_PRINCIPLE_SHADER_H