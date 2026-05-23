//
// Created by neuroil on 2026/5/23.
//

#ifndef SHADER_S_PRINCIPLE_FRAMEBUFFER_H
#define SHADER_S_PRINCIPLE_FRAMEBUFFER_H

#include <vector>
#include "Types.h"

class Framebuffer {
    int width, height;
    std::vector<Color> pixels;

public:
    Framebuffer(int w, int h):width(w), height(h), pixels(w*h, Color()){}

    void clear(Color c);
    void setPixel(int x, int y, Color c);
    const std::vector<Color>& getPixels() const{return pixels;};
};


#endif //SHADER_S_PRINCIPLE_FRAMEBUFFER_H
