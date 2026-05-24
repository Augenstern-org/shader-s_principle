add_executable(test main.cpp
        Screen.cpp
        Framebuffer.cpp
        Pipeline.cpp)

target_link_libraries(test PRIVATE OpenGL::GL glfw glad::glad glm::glm)
