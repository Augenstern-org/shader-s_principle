//
// Created by neuroil on 2026/5/23.
//
#include <iostream>
#include <GLFW/glfw3.h>
#include "Screen.h"
#include "Framebuffer.h"
#include "Types.h"

#define WIDTH 800
#define HEIGHT 600

const char* title = "Shader's principle";

int main() {

    Screen screen(WIDTH, HEIGHT, title);
    Framebuffer fb(WIDTH, HEIGHT);

    while (!glfwWindowShouldClose(screen.getWindow())) {
        fb.clear(Color{0.1f, 0.3f, 0.5f});
        screen.present(fb);
        glfwPollEvents();
    }

    return 0;
}