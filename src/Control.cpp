//
// Created by neuroil on 2026/5/28.
//

#include "Control.h"
#include <GLFW/glfw3.h>

Control::Control(Camera& camera) : m_camera(camera) {}

void Control::onMouseDrag(float dx, float dy, int button) {
    m_camera.onMouseDrag(dx, dy, button);
}

void Control::onMouseScroll(float dy) {
    m_camera.onMouseScroll(dy);
}

void Control::onKeyPress(int key) {
    if (key == GLFW_KEY_M) {
        m_objAutoRotate = !m_objAutoRotate;
    } else if (key == GLFW_KEY_UP) {
        m_objSpeed *= 1.5f;
    } else if (key == GLFW_KEY_DOWN) {
        m_objSpeed /= 1.5f;
    }
}

void Control::update(float dt) {
    if (m_objAutoRotate) {
        m_objAngle += m_objSpeed * dt;
    }
}
