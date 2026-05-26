//
// Created by neuroil on 2026/5/24.
//

#ifndef SHADER_S_PRINCIPLE_PIPELINE_H
#define SHADER_S_PRINCIPLE_PIPELINE_H

#include "Framebuffer.h"
#include "Mesh.h"
#include "Shader.h"
#include "Types.h"

class Pipeline {
    VertexShader m_vertexShader;
    FragmentShader m_fragmentShader;

  public:
    void setVertexShader(const VertexShader&);
    void setFragmentShader(const FragmentShader&);
    void drawTriangle(Framebuffer&, const Vertex&, const Vertex&, const Vertex&, const Uniforms&);
    void drawMesh(Framebuffer&, const Mesh&, const Uniforms&);
};

#endif // SHADER_S_PRINCIPLE_PIPELINE_H
