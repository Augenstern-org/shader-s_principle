//
// Created by neuroil on 2026/5/23.
//

#include "../include/Framebuffer.h"

void Framebuffer::clear(Color c) { pixels.assign(pixels.size(), c); }

void Framebuffer::setPixel(int x, int y, Color c) { pixels[y * width + x] = c; }
