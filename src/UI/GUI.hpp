#pragma once
#include <functional>
#include <string>

#include "imgui.h"
#include "rlImGui.h"

class GUI {
  public:
    GUI() { rlImGuiSetup(true); };
    ~GUI() = default;

    void DrawEditorUI(std::string& userPrompt, float& roomWidth,
                      float& roomDepth, float& scale,
                      std::function<void()> promptCallback) {
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 420), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Semantic Generator", nullptr,
                         ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextDisabled("STAGING CONTROLS");
            ImGui::Separator();
            ImGui::Spacing();

            // Sliders de límites de sala (modifican directamente las variables)
            ImGui::Text("Room Size");
            ImGui::SliderFloat("Width (X)", &roomWidth, 4.0f, 30.0f, "%.1f m");
            ImGui::SliderFloat("Depth (Z)", &roomDepth, 4.0f, 30.0f, "%.1f m");

            ImGui::TextDisabled("Enter a prompt");
            static char textInput[255];
            ImGui::InputText("Prompt", textInput, 255);

            ImGui::End();
        }
    }

  private:
};
