#include <iostream>
#include <GLFW/glfw3.h>

#include "../include/Screen.hpp"
#include "../include/Data.hpp"

#define MAX_IN_FLIGHT = 2;
const int WIDTH = 800;
const int HEIGHT = 600;

void draw(FrameBuffer frame_buffers);

std::vector<Vertex> vertices = {
    { 0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f },
    { -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f },
    { 0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f }
};



int main() {

    auto screen = Screen(WIDTH, HEIGHT);
    screen.frame_buffer = FrameBuffer(WIDTH, HEIGHT);
    if (!screen.get_windows_status()) {
        std::cout << "failed to creat window!" << std::endl;
        return -1;
    }


    // 主循环
    while (!glfwWindowShouldClose(screen.get_window_handle())) {
        draw(screen.frame_buffer);

        screen.present(screen.frame_buffer);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void draw(FrameBuffer frame_buffers) {
    // 每一帧渲染逻辑的最开始执行
    std::fill(frame_buffers.r.begin(), frame_buffers.r.end(), 0.0f);
    std::fill(frame_buffers.g.begin(), frame_buffers.g.end(), 0.0f);
    std::fill(frame_buffers.b.begin(), frame_buffers.b.end(), 0.0f);
}
