//
// Created by neuroil on 2026/5/23.
//
#include "Camera.h"
#include "Mesh.h"
#include "Renderer.h"
#include "Screen.h"
#include "Shader.h"
#include "Types.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdio>

constexpr int WIDTH = 1000;
constexpr int HEIGHT = 1000;
constexpr const char* TITLE = "Shader's principle";

static Mesh createDemoMesh() {
    Mesh mesh;
    mesh.addTriangle(Vertex(glm::vec4(-0.577f, -0.333f, 0.0f, 1.0f), Color(0.30, 0.65, 0.85)),
                     Vertex(glm::vec4( 0.577f, -0.333f, 0.0f, 1.0f), Color(0.98, 0.75, 0.24)),
                     Vertex(glm::vec4( 0.0f,  0.666f, 0.0f, 1.0f), Color(0.64, 0.54, 0.78)));

    mesh.addTriangle(Vertex(glm::vec4(-0.2f, 0.2f, 0.2f, 1.0f), Color(0.49, 0.80, 0.77)),
                     Vertex(glm::vec4( 0.5f, 0.2f, 0.2f, 1.0f), Color(1.00, 0.60, 0.50)),
                     Vertex(glm::vec4( 0.15f, 0.7f, 0.4f, 1.0f), Color(0.70, 0.65, 0.85)));
    return mesh;
}

int main() {
    Screen screen(WIDTH, HEIGHT, TITLE);
    Renderer renderer(WIDTH, HEIGHT, screen);

    float aspect = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    Camera camera(glm::radians(60.0f), aspect, 0.1f, 100.0f);
    camera.setAutoRotate(true);

    renderer.setVertexShader(VertexShader(BuiltinVertexShader::mvp));
    renderer.setFragmentShader(FragmentShader(BuiltinFragmentShader::passThrough));

    Mesh mesh = createDemoMesh();

    GLFWwindow* window = screen.getWindow();
    glfwSetWindowUserPointer(window, &camera);

    glfwSetCursorPosCallback(window, [](GLFWwindow* win, double x, double y) {
        auto* cam = static_cast<Camera*>(glfwGetWindowUserPointer(win));
        static double lastX = x, lastY = y;
        float dx = static_cast<float>(x - lastX);
        float dy = static_cast<float>(y - lastY);
        lastX = x; lastY = y;

        int button = -1;
        if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
            button = GLFW_MOUSE_BUTTON_1;
        else if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS)
            button = GLFW_MOUSE_BUTTON_2;

        if (button != -1)
            cam->onMouseDrag(dx, dy, button);
    });

    glfwSetScrollCallback(window, [](GLFWwindow* win, double /*x_offset*/, double y_offset) {
        auto* cam = static_cast<Camera*>(glfwGetWindowUserPointer(win));
        cam->onMouseScroll(static_cast<float>(y_offset));
    });

    glfwSetKeyCallback(window, [](GLFWwindow* win, int key, int /*scan_code*/, int action, int /*mods*/) {
        if (action == GLFW_PRESS) {
            auto* cam = static_cast<Camera*>(glfwGetWindowUserPointer(win));
            cam->onKeyPress(key);
        }
    });

    double lastTime = glfwGetTime();
    int frameCount = 0;
    double accFrameTime = 0.0;

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float dt = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        camera.update(dt);

        Uniforms uni;
        uni.model = glm::mat4(1.0f);
        uni.view = camera.viewMatrix();
        uni.projection = camera.projectionMatrix();

        // 性能分析
        auto t0 = std::chrono::high_resolution_clock::now();

        renderer.beginFrame(Color{0.12f, 0.12f, 0.12f});
        renderer.draw(mesh, uni);
        renderer.endFrame();

        // 性能分析
        auto t1 = std::chrono::high_resolution_clock::now();
        double frameMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        accFrameTime += frameMs;
        ++frameCount;
        if (frameCount % 60 == 0) {
            std::fprintf(stderr, "frame: %5d  avg: %7.3f ms  (%.1f fps)\n",
                        frameCount, accFrameTime / 60.0, 60000.0 / accFrameTime);
            accFrameTime = 0.0;
        }

        glfwPollEvents();
    }

    return 0;
}
