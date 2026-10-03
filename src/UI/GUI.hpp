#pragma once
#include <functional>
#include <string>

#include "rendering/SceneGraph.hpp"

class GUI {
  public:
    GUI();
    ~GUI() = default;

    void DrawEditorUI(float& roomWidth, float& roomDepth, float& scale,
                      std::function<void(std::string& promptText, char* apiKey)>
                          promptCallback,
                      const std::string& errorMessage, SceneGraph& sceneGraph);

    void DrawAssetPanel(SceneGraph& sceneGraph, float roomWidth,
                        float roomDepth);

  private:
    bool m_AssetPanelOpen{false};
};
