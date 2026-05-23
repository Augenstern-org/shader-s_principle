add_executable(test main.cpp
        Screen.cpp
        Framebuffer.cpp
        ../include/Framebuffer.h)

target_link_libraries(test PRIVATE OpenGL::GL glfw glad::glad)
