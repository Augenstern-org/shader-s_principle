//
// Created by neuroil on 2026/5/28.
//
#include "ObjLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION

#include "tiny_obj_loader.h"

#include <algorithm>
#include <cstdio>
#include <glm/glm.hpp>

Mesh ObjLoader::load(const std::string& filePath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;

    bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filePath.c_str());
    if (!err.empty()) {
        std::fprintf(stderr, "[ObjLoader] %s\n", err.c_str());
    }
    if (!ok) return Mesh{};

    // 计算包围盒
    glm::vec3 bboxMin(std::numeric_limits<float>::max());
    glm::vec3 bboxMax(std::numeric_limits<float>::lowest());

    for (const auto& shape : shapes) {
        for (const auto& idx : shape.mesh.indices) {
            if (idx.vertex_index < 0) continue;
            float x = attrib.vertices[3 * idx.vertex_index + 0];
            float y = attrib.vertices[3 * idx.vertex_index + 1];
            float z = attrib.vertices[3 * idx.vertex_index + 2];
            bboxMin = glm::min(bboxMin, glm::vec3(x, y, z));
            bboxMax = glm::max(bboxMax, glm::vec3(x, y, z));
        }
    }

    glm::vec3 center = (bboxMin + bboxMax) * 0.5f;
    glm::vec3 extent = bboxMax - bboxMin;
    float maxExtent = std::max({extent.x, extent.y, extent.z});
    float scale = (maxExtent > 1e-8f) ? 2.0f / maxExtent : 1.0f;

    // 构建 Mesh
    Mesh mesh;
    for (const auto& shape : shapes) {
        const auto& indices = shape.mesh.indices;
        const auto& fvc = shape.mesh.num_face_vertices;

        size_t offset = 0;
        for (size_t f = 0; f < fvc.size(); ++f) {
            int nv = fvc[f];
            if (nv != 3) {
                offset += nv;
                continue; // 剪枝
            }

            const tinyobj::index_t& i0 = indices[offset + 0];
            const tinyobj::index_t& i1 = indices[offset + 1];
            const tinyobj::index_t& i2 = indices[offset + 2];
            offset += 3;

            if (i0.vertex_index < 0 || i1.vertex_index < 0 || i2.vertex_index < 0) continue;

            auto getPos = [&](const tinyobj::index_t& idx) -> glm::vec3 {
                float x = attrib.vertices[3 * idx.vertex_index + 0];
                float y = attrib.vertices[3 * idx.vertex_index + 1];
                float z = attrib.vertices[3 * idx.vertex_index + 2];
                return (glm::vec3(x, y, z) - center) * scale;
            };

            glm::vec3 p0 = getPos(i0);
            glm::vec3 p1 = getPos(i1);
            glm::vec3 p2 = getPos(i2);

            uint32_t vi = static_cast<uint32_t>(mesh.vertexCount());

            auto toColor = [](const glm::vec3& p) -> Color {
                glm::vec3 c = glm::normalize(p) * 0.5f + 0.5f;
                return Color(glm::vec4(c, 0.0f));
            };

            mesh.addVertex(Vertex(glm::vec4(p0, 1.0f), toColor(p0)));
            mesh.addVertex(Vertex(glm::vec4(p1, 1.0f), toColor(p1)));
            mesh.addVertex(Vertex(glm::vec4(p2, 1.0f), toColor(p2)));
            mesh.addIndexedTriangle(vi, vi + 1, vi + 2);
        }
    }

    return mesh;
}