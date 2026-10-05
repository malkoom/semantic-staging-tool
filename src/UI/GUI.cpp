#include <algorithm>
#include <vector>

#include "GUI.hpp"

#include "imgui.h"
#include "rlImGui.h"

#include "UI/GuizmoManager.hpp"

GUI::GUI() {
    rlImGuiSetup(true);

    // Paleta tomada del logo: fondo tinta, lavanda como color principal y
    // cian/azul para los estados interactivos.
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(0.78f, 0.79f, 0.91f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.48f, 0.49f, 0.62f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.12f, 0.97f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.09f, 0.15f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.14f, 0.98f);
    colors[ImGuiCol_Border] = ImVec4(0.56f, 0.44f, 0.73f, 0.65f);
    colors[ImGuiCol_Separator] = ImVec4(0.30f, 0.27f, 0.42f, 0.75f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.17f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.17f, 0.13f, 0.25f, 1.00f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.23f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.18f, 0.31f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.20f, 0.36f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.45f, 0.32f, 0.64f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.53f, 0.42f, 0.76f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.34f, 0.25f, 0.50f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.39f, 0.57f, 0.75f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.32f, 0.55f, 0.73f, 0.85f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.28f, 0.46f, 0.66f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.62f, 0.90f, 0.62f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.43f, 0.75f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.52f, 0.66f, 0.94f, 1.00f);

    style.WindowRounding = 8.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.WindowPadding = ImVec2(14.0f, 14.0f);
    style.FramePadding = ImVec2(8.0f, 6.0f);
}

void GUI::DrawEditorUI(float& roomWidth, float& roomDepth, float& scale,
                       std::function<void(std::string& promptText, char* apiKey,
                                          const std::string& model)>
                           promptCallback,
                       const std::string& errorMessage, SceneGraph& sceneGraph,
                       GuizmoManager& guizmoManager) {

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340, 530), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Semantic Generator", nullptr,
                     ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextDisabled("STAGING CONTROLS");
        ImGui::Separator();
        ImGui::Spacing();

        static char apiKeyBuffer[255] = "";
        // En tu ventana de ImGui:
        ImGui::InputText("API Key", apiKeyBuffer, sizeof(apiKeyBuffer),
                         ImGuiInputTextFlags_Password);

        static const char* models[] = {
            "openai/gpt-oss-120b", "openai/gpt-oss-20b", "qwen/qwen3.8-27b"};
        static int selectedModel = 0;
        ImGui::Combo("LLM model", &selectedModel, models, IM_ARRAYSIZE(models));

        // Sliders de límites de sala (modifican directamente las variables)
        ImGui::Text("Room Size");
        ImGui::SliderFloat("Width (X)", &roomWidth, 4.0f, 30.0f, "%.1f m");
        ImGui::SliderFloat("Depth (Z)", &roomDepth, 4.0f, 30.0f, "%.1f m");

        ImGui::TextDisabled("Enter a prompt");
        static char textInputBuffer[2048];
        if (ImGui::InputTextMultiline("Prompt", textInputBuffer,
                                      sizeof(textInputBuffer),
                                      ImVec2(-1.0f, 110.0f))) {
            guizmoManager.CanUse = false;
        }

        if (ImGui::Button("Send") && textInputBuffer[0] != '\0') {
            std::string textString = textInputBuffer;
            promptCallback(textString, apiKeyBuffer, models[selectedModel]);
        }

        if (ImGui::Button("Clear")) {
            sceneGraph.Clear();
        }

        if (ImGui::Button("Undo Prompt")) {
            sceneGraph.Undo();
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
            sceneGraph.AddManualInstance(propId, {0.0f, 0.0f, 0.0f}, roomWidth,
                                         roomDepth);
        }
    }

    ImGui::End();
}
