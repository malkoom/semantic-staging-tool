#pragma once

#include "raylib.h"

class CameraController {
  public:
    CameraController();
    ~CameraController();

    void Update();
    const Camera3D& GetCamera() const { return m_Camera; }

  private:
    Camera3D m_Camera{};
    float m_Speed{5.0f};
    float m_MouseSensitivity{0.003f};
    float m_Yaw{};
    float m_Pitch{};
};
