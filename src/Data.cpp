//
// Created by Neuroil on 2026/3/9.
//

#include "../include/Data.hpp"

Color Texture::sample(float u, float v) {
    int x = static_cast<int>(u * (width - 1));
    int y = static_cast<int>(v * (height - 1));
    return pixels[y * width + x];
}