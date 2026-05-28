//
// Created by neuroil on 2026/5/24.
//

#include "Pipeline.h"
#include "Rasterizer.h"

static glm::vec2 viewportTransform(const glm::vec4& ndc, int width, int height) {
    float screen_x = (1.0f + ndc.x) * 0.5f * (float)width;
    float screen_y = (1.0f - ndc.y) * 0.5f * (float)height;
    return glm::vec2(screen_x, screen_y);
}

void Pipeline::drawTriangle(Framebuffer& fb, DepthBuffer* depthBuffer,
                            const VertexShader& vertShader, const FragmentShader& fragShader,
                            const Vertex& v0, const Vertex& v1, const Vertex& v2,
                            const Uniforms& uni) {
    VertexOutput out0 = vertShader.process(v0, uni);
    VertexOutput out1 = vertShader.process(v1, uni);
    VertexOutput out2 = vertShader.process(v2, uni);

    glm::vec3 ndc0(out0.position / out0.position.w);
    glm::vec3 ndc1(out1.position / out1.position.w);
    glm::vec3 ndc2(out2.position / out2.position.w);

    glm::vec2 p0 = viewportTransform(glm::vec4(ndc0, 1.0f), fb.getWidth(), fb.getHeight());
    glm::vec2 p1 = viewportTransform(glm::vec4(ndc1, 1.0f), fb.getWidth(), fb.getHeight());
    glm::vec2 p2 = viewportTransform(glm::vec4(ndc2, 1.0f), fb.getWidth(), fb.getHeight());

    Rasterizer::rasterizeTriangle(fb, depthBuffer, fragShader, uni,
                                  p0, p1, p2, ndc0, ndc1, ndc2,
                                  out0, out1, out2);
}

void Pipeline::drawMesh(Framebuffer& fb, DepthBuffer* depthBuffer,
                        const VertexShader& vertShader, const FragmentShader& fragShader,
                        const Mesh& mesh, const Uniforms& uni) {
    for (size_t t = 0; t < mesh.triangleCount(); ++t) {
        glm::uvec3 i = mesh.getTriangleIndices(t);
        drawTriangle(fb, depthBuffer, vertShader, fragShader,
                     mesh.getVertex(i.x), mesh.getVertex(i.y), mesh.getVertex(i.z), uni);
    }
}
