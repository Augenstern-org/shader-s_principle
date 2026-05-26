//
// Created by neuroil on 2026/5/26.
//
#include "Mesh.h"

void Mesh::addTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    m_vertices.push_back(v0);
    m_vertices.push_back(v1);
    m_vertices.push_back(v2);
}