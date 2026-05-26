//
// Created by neuroil on 2026/5/26.
//

#include "DepthBuffer.h"

DepthBuffer::DepthBuffer(int w, int h) : m_width(w), m_height(h), m_depths(w * h, 1.0f) {}
void DepthBuffer::clear(float value) { m_depths.assign(m_depths.size(), value); }
bool DepthBuffer::testAndSet(int x, int y, float depth) {
    size_t i = y * m_width + x;
    if (depth < m_depths[i]) {
        m_depths[i] = depth;
        return true;
    }
    return false;
}
float DepthBuffer::getDepth(int x, int y) const { return m_depths[y * m_width + x]; }
int DepthBuffer::getWidth() const { return m_width; }
int DepthBuffer::getHeight() const { return m_height; }

