//
// Created by neuroil on 2026/5/28.
//

#ifndef SHADER_S_PRINCIPLE_CONTROL_H
#define SHADER_S_PRINCIPLE_CONTROL_H

#include "Camera.h"

class Control {
public:
    Control(Camera& camera);

    void onMouseDrag(float dx, float dy, int button);
    void onMouseScroll(float dy);
    void onKeyPress(int key);

    void update(float dt);

    float objectAngle() const { return m_objAngle; }

private:
    Camera& m_camera;

    bool m_objAutoRotate = false;
    float m_objAngle = 0.0f;
    float m_objSpeed = 1.0f;
};

#endif // SHADER_S_PRINCIPLE_CONTROL_H
