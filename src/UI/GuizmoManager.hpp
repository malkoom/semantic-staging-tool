#pragma once
// noincludeformat
#include <string>

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
    void Update(SceneGraph& sceneGraph, CameraController& camera);

    bool IsUsing() const;

  private:
    std::string m_CurrentInstanceId;
};
