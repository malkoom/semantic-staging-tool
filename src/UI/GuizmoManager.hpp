#pragma once
// noincludeformat
#include "raylib.h"
#include "rendering/CameraController.hpp"
#include "rendering/SceneGraph.hpp"
#include <imgui.h>
#include <ImGuizmo.h>
#include <raymath.h>

class GuizmoManager {
  public:
    bool TryHitObject(SceneGraph& sceneGraph,
                      CameraController& cameraController);
    void Update(CameraController& camera);

    bool IsUsing() const;

  private:
    SceneInstance* m_CurrentInst = nullptr;
};
