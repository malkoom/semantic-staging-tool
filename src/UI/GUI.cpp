#include <algorithm>
#include <vector>

#include "GUI.hpp"

#include "imgui.h"
#include "rlImGui.h"

GUI::GUI() { rlImGuiSetup(true); }

void GUI::DrawEditorUI(
    float& roomWidth, float& roomDepth, float& scale,
    std::function<void(std::string& promptText, char* apiKey)> promptCallback,
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
        ImGui::InputText("Prompt", textInputBuffer, sizeof(textInputBuffer));

        if (ImGui::Button("Send") && textInputBuffer[0] != '\0') {
            std::string textString = textInputBuffer;
            promptCallback(textString, apiKeyBuffer);
        }

        if (ImGui::Button("Clear")) {
            sceneGraph.Clear();
        }

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Export to JSON")) {
            if (sceneGraph.ExportSceneToFile("saved_scene.json")) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "JSON saved");
            }
        }

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Asset Panel")) {
            m_AssetPanelOpen = true;
        }

        if (!errorMessage.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
                               "Request error");
            ImGui::TextWrapped("%s", errorMessage.c_str());
        }

        ImGui::End();
    }

    if (m_AssetPanelOpen)
        DrawAssetPanel(sceneGraph, roomWidth, roomDepth);
}

void GUI::DrawAssetPanel(SceneGraph& sceneGraph, float roomWidth,
                         float roomDepth) {

    ImGui::SetNextWindowPos(ImVec2(360, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(260, 420), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Assets", &m_AssetPanelOpen)) {
        ImGui::End();
        return;
    }

    ImGui::TextDisabled("Add an asset at the centre of the room");
    ImGui::Separator();

    std::vector<std::string> propIds;
    for (const auto& [propId, model] : sceneGraph.GetModels()) {
        (void)model;
        if (sceneGraph.IsPlaceableProp(propId))
            propIds.push_back(propId);
    }
    std::sort(propIds.begin(), propIds.end());

    for (const auto& propId : propIds) {
        if (ImGui::Button(propId.c_str(), ImVec2(-1.0f, 0.0f))) {
            sceneGraph.AddManualInstance(propId, {0.0f, 0.0f, 0.0f},
                                         roomWidth, roomDepth);
        }
    }

    ImGui::End();
}
