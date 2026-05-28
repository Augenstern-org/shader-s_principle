//
// Created by neuroil on 2026/5/26.
//
#include "Mesh.h"

uint32_t Mesh::addVertex(const Vertex& v){}
void Mesh::addIndexedTriangle(uint32_t i0, uint32_t i1, uint32_t i2){}

void Mesh::addTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2) {
    uint32_t base = static_cast<uint32_t>(m_vertices.size());
    m_vertices.push_back(v0);
    m_vertices.push_back(v1);
    m_vertices.push_back(v2);
    m_indices.push_back({base, base+1, base+2});
}