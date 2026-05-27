//
// Created by neuroil on 2026/5/27.
//

#ifndef SHADER_S_PRINCIPLE_CAMERA_H
#define SHADER_S_PRINCIPLE_CAMERA_H

#include <glm/glm.hpp>

class Camera {
public:
    Camera(float fov, float aspect, float near, float far);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;

    void onMouseDrag(float dx, float dy, int button);
    void onMouseScroll(float dy);
    void onKeyPress(int key);

    void setAutoRotate(bool enable);
    void setAutoRotateSpeed(float speed);
    void update(float dt);

private:
    float m_theta, m_phi, m_radius;
    glm::vec3 m_target;

    float m_fov, m_aspect, m_near, m_far;

    bool m_autoRotate;
    float m_autoRotateSpeed;

    glm::vec3 sphericalToCartesian() const;
};

#endif // SHADER_S_PRINCIPLE_CAMERA_H
