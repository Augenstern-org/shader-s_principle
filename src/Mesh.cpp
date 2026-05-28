//
// Created by neuroil on 2026/5/26.
//
#include "Mesh.h"

uint32_t Mesh::addVertex(const Vertex& v) {
    uint32_t i = static_cast<uint32_t>(m_vertices.size());
    m_vertices.push_back(v);
    return i;
}
void Mesh::addIndexedTriangle(uint32_t i0, uint32_t i1, uint32_t i2) {
    m_indices.push_back({i0, i1, i2});
}

void Mesh::addTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    uint32_t base = static_cast<uint32_t>(m_vertices.size());
    m_vertices.push_back(v0);
    m_vertices.push_back(v1);
    m_vertices.push_back(v2);
    m_indices.push_back({base, base + 1, base + 2});
}

void Mesh::createFallbackMesh() {
    addTriangle(Vertex(glm::vec4(-0.577f, -0.333f, 0.0f, 1.0f), Color(0.30, 0.65, 0.85)),
                Vertex(glm::vec4(0.577f, -0.333f, 0.0f, 1.0f), Color(0.98, 0.75, 0.24)),
                Vertex(glm::vec4(0.0f, 0.666f, 0.0f, 1.0f), Color(0.64, 0.54, 0.78)));
}