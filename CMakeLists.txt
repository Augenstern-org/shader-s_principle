cmake_minimum_required(VERSION 3.12)
project(shader-s_principle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)

find_package(glfw3 3.4 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_subdirectory(src)