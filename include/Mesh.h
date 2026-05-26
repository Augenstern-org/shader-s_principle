//
// Created by neuroil on 2026/5/26.
//
#ifndef SHADER_S_PRINCIPLE_MESH_H
#define SHADER_S_PRINCIPLE_MESH_H

#include "Types.h"
#include <vector>

class Mesh {
    std::vector<Vertex> m_vertices;

  public:
    Mesh() = default;
    void addTriangle(const Vertex&, const Vertex&, const Vertex&);
    size_t triangleCount() const { return m_vertices.size() / 3; }
    const Vertex& getVertex(size_t i) const { return m_vertices[i]; }
    void clear() { m_vertices.clear(); }
};

#endif // SHADER_S_PRINCIPLE_MESH_H