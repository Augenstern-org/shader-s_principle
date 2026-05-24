//
// Created by neuroil on 2026/5/24.
//

#ifndef SHADER_S_PRINCIPLE_PIPELINE_H
#define SHADER_S_PRINCIPLE_PIPELINE_H

#include "Types.h"
#include "Framebuffer.h"


class Pipeline {
public:
    void drawTriangle(Framebuffer& fb, const Vertex& v1, const Vertex& v2, const Vertex& v3);
};


#endif //SHADER_S_PRINCIPLE_PIPELINE_H
