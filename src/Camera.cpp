//
// Created by neuroil on 2026/5/27.
//

#include "Camera.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

Camera::Camera(float fov, float aspect, float near, float far)
    : m_theta(0.0f)
    , m_phi(0.0f)
    , m_radius(1.5f)
    , m_target(0.0f, 0.0f, 0.0f)
    , m_fov(fov)
    , m_aspect(aspect)
    , m_near(near)
    , m_far(far)
{}

glm::vec3 Camera::sphericalToCartesian() const {
    float x = m_radius * std::cos(m_phi) * std::sin(m_theta);
    float y = m_radius * std::sin(m_phi);
    float z = m_radius * std::cos(m_phi) * std::cos(m_theta);
    return glm::vec3(x, y, z);
}

glm::mat4 Camera::viewMatrix() const {
    glm::vec3 eye = m_target + sphericalToCartesian();
    return glm::lookAt(eye, m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projectionMatrix() const {
    return glm::perspective(m_fov, m_aspect, m_near, m_far);
}

void Camera::onMouseDrag(float dx, float dy, int button) {
    if (button == GLFW_MOUSE_BUTTON_1) {
        float sensitivity = 0.005f;
        m_theta -= dx * sensitivity;
        m_phi += dy * sensitivity;

        const float limit = glm::radians(89.0f);
        m_phi = std::clamp(m_phi, -limit, limit);
    } else if (button == GLFW_MOUSE_BUTTON_2) {
        float sensitivity = 0.005f;
        glm::vec3 eye = m_target + sphericalToCartesian();
        glm::vec3 forward = glm::normalize(m_target - eye);
        glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
        glm::vec3 up = glm::cross(right, forward);

        m_target -= right * (dx * sensitivity) + up * (-dy * sensitivity);
    }
}

void Camera::onMouseScroll(float dy) {
    m_radius -= dy * 0.1f;
    m_radius = std::max(m_radius, 0.1f);
}
