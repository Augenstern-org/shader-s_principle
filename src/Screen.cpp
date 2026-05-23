//
// Created by neuroil on 2026/5/23.
//

#include <glad/glad.h>
#include "../include/Screen.h"

Screen::Screen(int w, int h, const char* t) : width(w), height(h), title(t) {
    if (!glfwInit()) {
        window = nullptr;
        return;
    }

    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window);
        glfwTerminate();
        window = nullptr;
        return;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

Screen::~Screen() {
    glfwTerminate();
}

void Screen::present(const Framebuffer& fb) {
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height,
                 0, GL_RGB, GL_FLOAT, fb.getPixels().data());

    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-1.0f, -1.0f); // 左下
    glTexCoord2f(1.0f, 1.0f); glVertex2f( 1.0f, -1.0f); // 右下
    glTexCoord2f(1.0f, 0.0f); glVertex2f( 1.0f,  1.0f); // 右上
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-1.0f,  1.0f); // 左上
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glfwSwapBuffers(window);
}

GLFWwindow* Screen::getWindow() {
    return window;
}

