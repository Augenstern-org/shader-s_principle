//
// Created by neuroil on 2026/5/28.
//
#include "ObjLoader.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <glm/glm.hpp>

// ---------------------------------------------------------------------------
// 双三次 Bézier 曲面的伯恩斯坦多项式
// ---------------------------------------------------------------------------
static float B0(float t) { float u = 1.0f - t; return u * u * u; }
static float B1(float t) { float u = 1.0f - t; return 3.0f * t * u * u; }
static float B2(float t) { return 3.0f * t * t * (1.0f - t); }
static float B3(float t) { return t * t * t; }

static float dB0(float t) { float u = 1.0f - t; return -3.0f * u * u; }
static float dB1(float t) { float u = 1.0f - t; return 3.0f * u * u - 6.0f * t * u; }
static float dB2(float t) { return 6.0f * t * (1.0f - t) - 3.0f * t * t; }
static float dB3(float t) { return 3.0f * t * t; }

// 计算 Bézier 曲面上一点的位置、u向偏导、v向偏导
static void evalBezier(const glm::vec3 cp[4][4], float u, float v,
                       glm::vec3& pos, glm::vec3& du, glm::vec3& dv) {
    float bu[4]  = {B0(u), B1(u), B2(u), B3(u)};
    float bv[4]  = {B0(v), B1(v), B2(v), B3(v)};
    float dbu[4] = {dB0(u), dB1(u), dB2(u), dB3(u)};
    float dbv[4] = {dB0(v), dB1(v), dB2(v), dB3(v)};

    pos = du = dv = glm::vec3(0.0f);
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            pos += bv[i] * bu[j]  * cp[i][j];
            du  += bv[i] * dbu[j] * cp[i][j];
            dv  += dbv[i] * bu[j]  * cp[i][j];
        }
    }
}

// ---------------------------------------------------------------------------
// 解析 AOV 格式的 Bézier 曲面文件（如 Utah 茶壶）
// ---------------------------------------------------------------------------
static bool isAOVFormat(const std::string& content) {
    return content.find("TeaSrfs:") != std::string::npos ||
           content.find("surface(") != std::string::npos;
}

static std::vector<std::vector<glm::vec3>> parseAOV(const std::string& content) {
    std::vector<std::vector<glm::vec3>> patches;
    size_t pos = 0;

    while (true) {
        pos = content.find("surface(", pos);
        if (pos == std::string::npos) break;

        // 用括号深度计数找到与之匹配的右括号
        int depth = 0;
        size_t end = pos;
        for (; end < content.size(); ++end) {
            if (content[end] == '(') ++depth;
            else if (content[end] == ')') {
                if (--depth == 0) break;
            }
        }
        if (end >= content.size()) break;

        std::string block = content.substr(pos, end - pos + 1);

        // 提取所有 pt(x, y, z) 控制点
        std::vector<glm::vec3> pts;
        size_t ptPos = 0;
        while ((ptPos = block.find("pt(", ptPos)) != std::string::npos) {
            size_t open  = ptPos + 3;
            size_t close = block.find(')', open);
            if (close == std::string::npos) break;

            std::string coords = block.substr(open, close - open);
            std::stringstream ss(coords);
            float x, y, z;
            char sep1, sep2;
            ss >> x >> sep1 >> y >> sep2 >> z;
            pts.emplace_back(x, y, z);

            ptPos = close + 1;
        }

        if (pts.size() == 16) {             // 4×4 双三次曲面片
            patches.push_back(std::move(pts));
        } else if (!pts.empty()) {
            std::fprintf(stderr, "[ObjLoader] 警告: 控制点数量为 %zu 的曲面片被忽略\n",
                         pts.size());
        }

        pos = end + 1;
    }

    return patches;
}

// 将一组 Bézier 曲面片细分为三角形 Mesh
static Mesh buildMeshFromPatches(const std::vector<std::vector<glm::vec3>>& patches) {
    const int TESSELLATION = 8; // 每个曲面片每条边的细分网格数

    // 计算所有曲面片的包围盒
    glm::vec3 bboxMin(std::numeric_limits<float>::max());
    glm::vec3 bboxMax(std::numeric_limits<float>::lowest());
    for (const auto& patch : patches) {
        for (const auto& p : patch) {
            bboxMin = glm::min(bboxMin, p);
            bboxMax = glm::max(bboxMax, p);
        }
    }

    glm::vec3 center = (bboxMin + bboxMax) * 0.5f;
    glm::vec3 extent = bboxMax - bboxMin;
    float maxExtent = std::max({extent.x, extent.y, extent.z});
    float scale = (maxExtent > 1e-8f) ? 2.0f / maxExtent : 1.0f;

    Mesh mesh;
    const int n = TESSELLATION;

    for (const auto& ctrlPts : patches) {
        // 控制点数组按行存储: 行0第0~3列, 行1第0~3列, …
        glm::vec3 cp[4][4];
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                cp[i][j] = ctrlPts[i * 4 + j];

        // (n+1)×(n+1) 顶点网格
        std::vector<std::vector<uint32_t>> vi(n + 1, std::vector<uint32_t>(n + 1));

        for (int iv = 0; iv <= n; ++iv) {
            float v = static_cast<float>(iv) / n;
            for (int iu = 0; iu <= n; ++iu) {
                float u = static_cast<float>(iu) / n;

                glm::vec3 pos, du, dv;
                evalBezier(cp, u, v, pos, du, dv);
                glm::vec3 normal = glm::normalize(glm::cross(du, dv));

                glm::vec3 pNorm = (pos - center) * scale;
                Color color(glm::vec4(normal * 0.5f + 0.5f, 0.0f));
                vi[iv][iu] = static_cast<uint32_t>(mesh.vertexCount());
                mesh.addVertex(Vertex(glm::vec4(pNorm, 1.0f), color));
            }
        }

        // 每个网格单元输出 2 个三角形
        for (int iv = 0; iv < n; ++iv) {
            for (int iu = 0; iu < n; ++iu) {
                uint32_t a = vi[iv][iu];
                uint32_t b = vi[iv][iu + 1];
                uint32_t c = vi[iv + 1][iu + 1];
                uint32_t d = vi[iv + 1][iu];
                mesh.addIndexedTriangle(a, b, c);
                mesh.addIndexedTriangle(a, c, d);
            }
        }
    }

    return mesh;
}

// ---------------------------------------------------------------------------
// 通过 tinyobjloader 加载标准 OBJ 文件
// ---------------------------------------------------------------------------
static Mesh loadStandardObj(const std::string& filePath) {
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

    Mesh mesh;
    for (const auto& shape : shapes) {
        const auto& indices = shape.mesh.indices;
        const auto& fvc = shape.mesh.num_face_vertices;

        size_t offset = 0;
        for (size_t f = 0; f < fvc.size(); ++f) {
            int nv = fvc[f];
            if (nv != 3) {
                offset += nv;
                continue;
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

// ---------------------------------------------------------------------------
// 公开入口 — 自动检测格式并分发
// ---------------------------------------------------------------------------
Mesh ObjLoader::load(const std::string& filePath) {
    // 读取整个文件以判断格式
    std::ifstream in(filePath);
    if (!in.is_open()) {
        std::fprintf(stderr, "[ObjLoader] 无法打开文件: %s\n", filePath.c_str());
        return Mesh{};
    }

    std::stringstream buf;
    buf << in.rdbuf();
    std::string content = buf.str();

    if (isAOVFormat(content)) {
        std::fprintf(stderr, "[ObjLoader] 检测到 AOV Bézier 曲面格式\n");
        auto patches = parseAOV(content);
        if (patches.empty()) {
            std::fprintf(stderr, "[ObjLoader] 解析 AOV 曲面失败\n");
            return Mesh{};
        }
        std::fprintf(stderr, "[ObjLoader] 已解析 %zu 个 Bézier 曲面片\n", patches.size());
        return buildMeshFromPatches(patches);
    }

    // 标准 OBJ — 直接传文件路径给 tinyobj（需要路径用于加载 mtl）
    return loadStandardObj(filePath);
}
