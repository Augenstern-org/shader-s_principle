//
// Created by neuroil on 2026/5/24.
//

#ifndef SHADER_S_PRINCIPLE_PIPELINE_H
#define SHADER_S_PRINCIPLE_PIPELINE_H

#include "Framebuffer.h"
#include "Mesh.h"
#include "Shader.h"
#include "Types.h"
#include "DepthBuffer.h"

class Pipeline {
public:
    void drawTriangle(Framebuffer& fb, DepthBuffer* depthBuffer,
                      const VertexShader& vertShader, const FragmentShader& fragShader,
                      const Vertex& v0, const Vertex& v1, const Vertex& v2,
                      const Uniforms& uni);
    void drawMesh(Framebuffer& fb, DepthBuffer* depthBuffer,
                  const VertexShader& vertShader, const FragmentShader& fragShader,
                  const Mesh& mesh, const Uniforms& uni);
};

#endif // SHADER_S_PRINCIPLE_PIPELINE_H
