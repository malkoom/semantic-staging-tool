#include <cmath>

#include "CameraController.hpp"

#include "raylib.h"
#include "raymath.h"

CameraController::CameraController() {
    m_Camera.position = {10.0f, 10.0f, 10.0f};
    m_Camera.target = {0.0f, 0.0f, 0.0f};
    m_Camera.up = {0.0f, 1.0f, 0.0f};
    m_Camera.fovy = 45.0f;
    m_Camera.projection = CAMERA_PERSPECTIVE;

    const Vector3 forward =
        Vector3Normalize(Vector3Subtract(m_Camera.target, m_Camera.position));
    m_Yaw = std::atan2(forward.z, forward.x);
    m_Pitch = std::asin(forward.y);

    DisableCursor();
}

CameraController::~CameraController() { EnableCursor(); }

void CameraController::Update() {
    const Vector2 mouseDelta = GetMouseDelta();
    m_Yaw += mouseDelta.x * m_MouseSensitivity;
    m_Pitch = Clamp(m_Pitch - mouseDelta.y * m_MouseSensitivity, -1.55f, 1.55f);

    const Vector3 forward = Vector3Normalize({
        std::cos(m_Yaw) * std::cos(m_Pitch),
        std::sin(m_Pitch),
        std::sin(m_Yaw) * std::cos(m_Pitch),
    });
    const Vector3 right =
        Vector3Normalize(Vector3CrossProduct(forward, m_Camera.up));
    const Vector3 horizontalForward =
        Vector3Normalize({forward.x, forward.y, forward.z});

    Vector3 movement{};
    if (IsKeyDown(KEY_W))
        movement = Vector3Add(movement, horizontalForward);
    if (IsKeyDown(KEY_S))
        movement = Vector3Subtract(movement, horizontalForward);
    if (IsKeyDown(KEY_D))
        movement = Vector3Add(movement, right);
    if (IsKeyDown(KEY_A))
        movement = Vector3Subtract(movement, right);
    if (IsKeyDown(KEY_E))
        movement.y += 1.0f;
    if (IsKeyDown(KEY_Q))
        movement.y -= 1.0f;

    if (Vector3LengthSqr(movement) > 0.0f) {
        const float speed = m_Speed * (IsKeyDown(KEY_LEFT_SHIFT) ? 3.0f : 1.0f);
        movement =
            Vector3Scale(Vector3Normalize(movement), speed * GetFrameTime());
        m_Camera.position = Vector3Add(m_Camera.position, movement);
    }
    m_Camera.target = Vector3Add(m_Camera.position, forward);
}
