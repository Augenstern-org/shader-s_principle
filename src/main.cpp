//
// Created by neuroil on 2026/5/23.
//
#include "Framebuffer.h"
#include "Mesh.h"
#include "Pipeline.h"
#include "Screen.h"
#include "Shader.h"
#include "Types.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#define WIDTH 1000
#define HEIGHT 1000

const char* title = "Shader's principle";

int main() {
    // 创建窗口
    Screen screen(WIDTH, HEIGHT, title);
    // 创建 UBO
    Uniforms uniforms;
    uniforms.model = glm::mat4(1.0f); // 初始为单位矩阵
    uniforms.projection =
        glm::perspective(glm::radians(60.0f), // FOV: 60 度
                         static_cast<float>(WIDTH) / static_cast<float>(HEIGHT), // 宽高比
                         0.1f, // 近裁剪面
                         100.0f // 远裁剪面
        );
    uniforms.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), // 相机位置：z=2，在三角形前方
                                glm::vec3(0.0f, 0.0f, 0.0f), // 看向原点（三角形所在位置）
                                glm::vec3(0.0f, 1.0f, 0.0f) // 上方向：Y 轴
    );

    // 创建着色器
    VertexShader vertexShader(BuiltinVertexShader::mvp);
    FragmentShader fragShader(BuiltinFragmentShader::passThrough);
    // 创建缓冲区
    Framebuffer fb(WIDTH, HEIGHT);
    DepthBuffer depth_buffer(WIDTH, HEIGHT);
    // 创建管线
    Pipeline pipeline;
    // 绑定管线
    pipeline.setVertexShader(vertexShader);
    pipeline.setFragmentShader(fragShader);
    pipeline.setDepthBuffer(&depth_buffer);

    // 创建Mesh
    Mesh mesh;
    mesh.addTriangle(Vertex(glm::vec4(-0.577f, -0.333f, 0.0f, 1.0f), Color(0.30, 0.65, 0.85)),
                     Vertex(glm::vec4(0.577f, -0.333f, 0.0f, 1.0f), Color(0.98, 0.75, 0.24)),
                     Vertex(glm::vec4(0.0f, 0.666f, 0.0f, 1.0f), Color(0.64, 0.54, 0.78)));

    mesh.addTriangle(Vertex(glm::vec4(-0.2f, 0.2f, 0.2f, 1.0f), Color(0.49, 0.80, 0.77)),
                     Vertex(glm::vec4(0.5f, 0.2f, 0.2f, 1.0f), Color(1.00, 0.60, 0.50)),
                     Vertex(glm::vec4(0.15f, 0.7f, 0.4f, 1.0f), Color(0.70, 0.65, 0.85)));

    // 主循环
    while (!glfwWindowShouldClose(screen.getWindow())) {
        float angle = static_cast<float>(glfwGetTime());
        uniforms.model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.1f, 0.0f));

        fb.clear(Color{0.12f, 0.12f, 0.12f}); // 1. 清屏
        depth_buffer.clear(); // 2. 清除深度
        pipeline.drawMesh(fb, mesh, uniforms); // 3. 画三角形
        screen.present(fb); // 4. 显示到窗口
        glfwPollEvents();
    }

    //释放资源
    // screen 析构函数管理窗口生命周期
    return 0;
}
