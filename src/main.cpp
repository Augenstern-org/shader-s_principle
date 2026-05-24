//
// Created by neuroil on 2026/5/23.
//
#include <iostream>
#include <GLFW/glfw3.h>
#include "Screen.h"
#include "Framebuffer.h"
#include "Types.h"
#include "Pipeline.h"

#define WIDTH 800
#define HEIGHT 600

const char* title = "Shader's principle";

int main() {

    Screen screen(WIDTH, HEIGHT, title);
    Framebuffer fb(WIDTH, HEIGHT);
    Pipeline pipeline;

    Vertex v0(glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f));  // 左下
    Vertex v1(glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f));  // 右下
    Vertex v2(glm::vec4( 0.0f,  0.5f, 0.0f, 1.0f));  // 上方中央

    while (!glfwWindowShouldClose(screen.getWindow())) {
        fb.clear(Color{0.1f, 0.3f, 0.5f});  // 1. 清屏（深蓝灰背景）
        pipeline.drawTriangle(fb, v0, v1, v2);       // 2. 画三角形（白色）
        screen.present(fb);                          // 3. 显示到窗口
        glfwPollEvents();
    }

    return 0;
}