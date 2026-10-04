#pragma once
#include <functional>
#include <string>

#include "UI/GuizmoManager.hpp"

#include "rendering/SceneGraph.hpp"

class GUI {
  public:
    GUI();
    ~GUI() = default;

    void DrawEditorUI(
        float& roomWidth, float& roomDepth, float& scale,
        std::function<void(std::string& promptText, char* apiKey,
                           const std::string& model)>
            promptCallback,
                      const std::string& errorMessage, SceneGraph& sceneGraph,
                      GuizmoManager& guizmoManager);

    void DrawAssetPanel(SceneGraph& sceneGraph, float roomWidth,
                        float roomDepth);

  private:
    bool m_AssetPanelOpen{false};
};
