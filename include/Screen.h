//
// Created by neuroil on 2026/5/23.
//

#ifndef SHADER_S_PRINCIPLE_SCREEN_H
#define SHADER_S_PRINCIPLE_SCREEN_H

#include <GLFW/glfw3.h>
#include "Framebuffer.h"

class Screen {
    int width, height;
    GLFWwindow* window;
    const char* title;
    GLuint textureID;

public:
    Screen(int w, int h, const char* t);
    ~Screen();

    void present(const Framebuffer& fb);
    bool isOpen();
    GLFWwindow* getWindow();

};


#endif //SHADER_S_PRINCIPLE_SCREEN_H


