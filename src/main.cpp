//
// Created by neuroil on 2026/5/23.
//
#include "Control.h"
#include "Mesh.h"
#include "ObjLoader.h"
#include "Renderer.h"
#include "Screen.h"
#include "Shader.h"
#include "Types.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <glm/gtc/matrix_transform.hpp>

#ifndef NDEBUG
#include <chrono>
#endif

constexpr int WIDTH = 1000;
constexpr int HEIGHT = 1000;
constexpr const char* TITLE = "Shader's principle";

int main() {
    Screen screen(WIDTH, HEIGHT, TITLE);
    Renderer renderer(WIDTH, HEIGHT, screen);

    float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    Camera camera(glm::radians(60.0f), aspect, 0.1f, 100.0f);
    Control control(camera);

    renderer.setVertexShader(VertexShader(BuiltinVertexShader::mvp));
    renderer.setFragmentShader(FragmentShader(BuiltinFragmentShader::passThrough));

    Mesh mesh = ObjLoader::load("assets/model.obj");
    if (mesh.triangleCount() == 0) {
        std::fprintf(stderr, "Failed to load model, using fallback triangle\n");
        mesh.createFallbackMesh();
    }

    GLFWwindow* window = screen.getWindow();
    glfwSetWindowUserPointer(window, &control);

    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y) {
        auto* ctrl = static_cast<Control*>(glfwGetWindowUserPointer(win));
        static double lastX = x, lastY = y;
        float dx = static_cast<float>(x - lastX);
        float dy = static_cast<float>(y - lastY);
        lastX = x;
        lastY = y;

        int button = -1;
        if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
            button = GLFW_MOUSE_BUTTON_1;
        else if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
            button = GLFW_MOUSE_BUTTON_2;

        if (button != -1) ctrl->onMouseDrag(dx, dy, button);
    });

    glfwSetScrollCallback(window, [](GLFWwindow* win, double /*x_offset*/, double y_offset) {
        auto* ctrl = static_cast<Control*>(glfwGetWindowUserPointer(win));
        ctrl->onMouseScroll(static_cast<float>(y_offset));
    });

    glfwSetKeyCallback(window,
                       [](GLFWwindow* win, int key, int /*scan_code*/, int action, int /*mods*/) {
                           if (action == GLFW_PRESS) {
                               auto* ctrl = static_cast<Control*>(glfwGetWindowUserPointer(win));
                               ctrl->onKeyPress(key);
                           }
                       });

    double lastTime = glfwGetTime();

#ifndef NDEBUG
    int frameCount = 0;
    double accFrameTime = 0.0;
#endif

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float dt = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        control.update(dt);

        Uniforms uni;
        uni.model =
            glm::rotate(glm::mat4(1.0f), control.objectAngle(), glm::vec3(0.0f, 1.0f, 0.0f));
        uni.view = camera.viewMatrix();
        uni.projection = camera.projectionMatrix();

#ifndef NDEBUG
        auto t0 = std::chrono::high_resolution_clock::now();
#endif

        renderer.beginFrame(Color{0.12f, 0.12f, 0.12f});
        renderer.draw(mesh, uni);
        renderer.endFrame();

#ifndef NDEBUG
        auto t1 = std::chrono::high_resolution_clock::now();
        double frameMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        accFrameTime += frameMs;
        ++frameCount;
        if (frameCount % 60 == 0) {
            std::fprintf(stderr, "frame: %5d  avg: %7.3f ms  (%.1f fps)\n", frameCount,
                         accFrameTime / 60.0, 60000.0 / accFrameTime);
            accFrameTime = 0.0;
        }
#endif

        glfwPollEvents();
    }

    return 0;
}
