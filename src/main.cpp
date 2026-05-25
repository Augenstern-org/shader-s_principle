//
// Created by neuroil on 2026/5/23.
//
#include "Framebuffer.h"
#include "Pipeline.h"
#include "Screen.h"
#include "Shader.h"
#include "Types.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#define WIDTH 800
#define HEIGHT 600

const char* title = "Shader's principle";

int main() {

    Screen screen(WIDTH, HEIGHT, title);
    Framebuffer fb(WIDTH, HEIGHT);
    Pipeline pipeline;
    // 创建 MVP 矩阵
    Uniforms uniforms;
    uniforms.model = glm::mat4(1.0f);      // 初始为单位矩阵（之后每帧会被覆盖）
    uniforms.view = glm::mat4(1.0f);       // 阶段 3 暂用单位矩阵
    uniforms.projection = glm::mat4(1.0f); // 阶段 3 暂用单位矩阵

    // 创建顶点着色器，使用内置的 MVP 变换
    VertexShader vertexShader(BuiltinVertexShader::test);

    // 绑定管线
    pipeline.setVertexShader(vertexShader);

    // 硬编码三个顶点
    Vertex v0(glm::vec4(-0.3f, -0.5f, 0.0f, 1.0f)); // 左下
    Vertex v1(glm::vec4(0.3f, -0.5f, 0.0f, 1.0f));  // 右下
    Vertex v2(glm::vec4(0.0f, 0.5f, 0.0f, 1.0f));   // 上方中央

    while (!glfwWindowShouldClose(screen.getWindow())) {
        float angle = static_cast<float>(glfwGetTime());
        uniforms.model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 0.1f));

        fb.clear(Color{0.1f, 0.3f, 0.5f});               // 1. 清屏（深蓝灰背景）
        pipeline.drawTriangle(fb, v0, v1, v2, uniforms); // 2. 画三角形（白色）
        screen.present(fb);                              // 3. 显示到窗口
        glfwPollEvents();
    }

    return 0;
}