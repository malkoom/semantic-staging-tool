#pragma once
#include <functional>
#include <string>

#include "imgui.h"
#include "rlImGui.h"

#include "rendering/SceneGraph.hpp"

class GUI {
  public:
    GUI() { rlImGuiSetup(true); };
    ~GUI() = default;

    void DrawEditorUI(float& roomWidth, float& roomDepth, float& scale,
                      std::function<void(std::string& promptText, char* apiKey)>
                          promptCallback,
                      const std::string& errorMessage, SceneGraph& sceneGraph) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 420), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Semantic Generator", nullptr,
                         ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextDisabled("STAGING CONTROLS");
            ImGui::Separator();
            ImGui::Spacing();

            static char apiKeyBuffer[255] = "";
            // En tu ventana de ImGui:
            ImGui::InputText("API Key", apiKeyBuffer, sizeof(apiKeyBuffer),
                             ImGuiInputTextFlags_Password);

            // Sliders de límites de sala (modifican directamente las variables)
            ImGui::Text("Room Size");
            ImGui::SliderFloat("Width (X)", &roomWidth, 4.0f, 30.0f, "%.1f m");
            ImGui::SliderFloat("Depth (Z)", &roomDepth, 4.0f, 30.0f, "%.1f m");

            ImGui::TextDisabled("Enter a prompt");
            static char textInputBuffer[255];
            ImGui::InputText("Prompt", textInputBuffer,
                             sizeof(textInputBuffer));

            if (ImGui::Button("Send") && textInputBuffer[0] != '\0') {
                std::string textString = textInputBuffer;
                promptCallback(textString, apiKeyBuffer);
            }

            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Exportar Escena (JSON)")) {
                if (sceneGraph.ExportSceneToFile("saved_scene.json")) {
                    // Notificación de éxito
                }
            }

            if (!errorMessage.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
                                   "Request error");
                ImGui::TextWrapped("%s", errorMessage.c_str());
            }

            ImGui::End();
        }
    }

  private:
};
