//
// Created by neuroil on 2026/5/26.
//
#ifndef SHADER_S_PRINCIPLE_MESH_H
#define SHADER_S_PRINCIPLE_MESH_H

#include "Types.h"
#include <vector>

class Mesh {
    std::vector<Vertex> m_vertices;
    std::vector<glm::uvec3> m_indices;

  public:
    Mesh() = default;

    uint32_t addVertex(const Vertex& v);
    void addIndexedTriangle(uint32_t i0, uint32_t i1, uint32_t i2);
    void addTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2);
    size_t triangleCount() const { return m_indices.size(); }
    glm::uvec3 getTriangleIndices(size_t t) const { return m_indices[t]; }
    const Vertex& getVertex(size_t i) const { return m_vertices[i]; }
    size_t vertexCount() const { return m_vertices.size(); }

    void clear() {
        m_vertices.clear();
        m_indices.clear();
    }
    void createFallbackMesh();
};

#endif // SHADER_S_PRINCIPLE_MESH_H