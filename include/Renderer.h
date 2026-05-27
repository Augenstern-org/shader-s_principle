//
// Created by neuroil on 2026/5/27.
//

#ifndef SHADER_S_PRINCIPLE_RENDERER_H
#define SHADER_S_PRINCIPLE_RENDERER_H

#include "Types.h"
#include "Framebuffer.h"
#include "DepthBuffer.h"
#include "Pipeline.h"
#include "Screen.h"
#include "Mesh.h"
#include "Shader.h"

class Renderer {
public:
    Renderer(int width, int height, Screen& screen);

    void beginFrame(Color clearColor);
    void draw(const Mesh& mesh, const Uniforms& uniforms);
    void endFrame();

    void setVertexShader(const VertexShader& shader);
    void setFragmentShader(const FragmentShader& shader);

private:
    Framebuffer m_framebuffer;
    DepthBuffer m_depthBuffer;
    Pipeline m_pipeline;
    Screen& m_screen;

    VertexShader m_vertexShader;
    FragmentShader m_fragmentShader;
};

#endif // SHADER_S_PRINCIPLE_RENDERER_H
