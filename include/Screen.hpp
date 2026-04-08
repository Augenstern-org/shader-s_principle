//
// Created by Neuroil on 2026/3/7.
//

#ifndef SHADER_S_PRINCIPLE_SCREEN_HPP
#define SHADER_S_PRINCIPLE_SCREEN_HPP

// #include <vector>
#include <GLFW/glfw3.h>
#include "Data.hpp"

class Screen {
public:
    Screen(int w, int h) : width(w), height(h) {
        if (!glfwInit()) {
            windows_status = 0;
            return;
        }

        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        // 设置纹理参数（保持像素原样，不模糊）
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        window_handle = glfwCreateWindow(w, h, "软渲染器", NULL, NULL);
        glfwMakeContextCurrent(window_handle);
        windows_status = 1;
    }

    void present(FrameBuffer fb);
    bool get_windows_status() const;
    GLFWwindow* get_window_handle() const;

    FrameBuffer frame_buffer;

private:
    int width, height;
    GLFWwindow* window_handle;
    unsigned int textureID; // OpenGL 纹理对象编号
    bool windows_status;

};

#endif //SHADER_S_PRINCIPLE_SCREEN_HPP