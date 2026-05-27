add_executable(test main.cpp
        Screen.cpp
        Framebuffer.cpp
        Pipeline.cpp
        Shader.cpp
        Mesh.cpp
        DepthBuffer.cpp
        Camera.cpp
        Rasterizer.cpp
        Renderer.cpp)

target_link_libraries(test PRIVATE OpenGL::GL glfw glad::glad glm::glm)
