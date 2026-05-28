add_executable(test main.cpp
        Screen.cpp
        Framebuffer.cpp
        Pipeline.cpp
        Shader.cpp
        Mesh.cpp
        DepthBuffer.cpp
        Camera.cpp
        Rasterizer.cpp
        Renderer.cpp
        Control.cpp
        ObjLoader.cpp)

target_link_libraries(test PRIVATE OpenGL::GL glfw glad::glad glm::glm)

add_custom_command(TARGET test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    ${CMAKE_BINARY_DIR}/src/assets
)
