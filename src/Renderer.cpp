//
// Created by neuroil on 2026/5/27.
//

#include "Renderer.h"

Renderer::Renderer(int width, int height, Screen& screen)
    : m_framebuffer(width, height)
    , m_depthBuffer(width, height)
    , m_screen(screen)
{}

void Renderer::beginFrame(Color clearColor) {
    m_framebuffer.clear(clearColor);
    m_depthBuffer.clear();
}

void Renderer::draw(const Mesh& mesh, const Uniforms& uniforms) {
    m_pipeline.drawMesh(m_framebuffer, &m_depthBuffer,
                        m_vertexShader, m_fragmentShader,
                        mesh, uniforms);
}

void Renderer::endFrame() {
    m_screen.present(m_framebuffer);
}

void Renderer::setVertexShader(const VertexShader& shader) { m_vertexShader = shader; }
void Renderer::setFragmentShader(const FragmentShader& shader) { m_fragmentShader = shader; }
