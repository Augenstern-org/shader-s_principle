//
// Created by neuroil on 2026/5/26.
//

#ifndef SHADER_S_PRINCIPLE_DEPTHBUFFER_H
#define SHADER_S_PRINCIPLE_DEPTHBUFFER_H
#include <vector>


class DepthBuffer {
    int m_width, m_height;
    // [0, 1] -> 0 = 近裁剪面, 1 = 远裁剪面
    std::vector<float> m_depths;

public:
    DepthBuffer(int w, int h);
    void clear(float value = 1.0f);
    // 只画最前方的像素
    bool testAndSet(int x, int y, float depth);
    float getDepth(int x, int y) const;
    int getWidth() const;
    int getHeight() const;
};


#endif //SHADER_S_PRINCIPLE_DEPTHBUFFER_H
