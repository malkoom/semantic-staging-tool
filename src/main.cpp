
#include <string>

#include "raylib.h"
#include "rlImGui.h"

#include "UI/GUI.hpp"

#include "network/AIClient.hpp"

#include "rendering/CameraController.hpp"
#include "rendering/SceneGraph.hpp"

const std::string SYSTEM_PROMPT =
    R"(You are an expert 3D level designer and set dresser.
Your job is to arrange 3D props in an environment based on the user's intent.

### INPUT RULES:
1. You will receive a list of "available_props" (their IDs and bounds) and a "user_intent".
2. You MUST ONLY use prop IDs present in "available_props". Do NOT invent new IDs.
3. Coordinates X and Z must remain within the room bounds: [-width/2, width/2] and [-depth/2, depth/2].

### OUTPUT FORMAT:
Respond ONLY with a valid JSON object. No markdown fences, no conversational prose, no comments.
The JSON must adhere strictly to this schema:

{
  "layout_name": "<short_semantic_name>",
  "entities": [
    {
      "instance_id": "<unique_string_id>",
      "prop_id": "<must_match_an_available_prop_id>",
      "placement_type": "ground" | "on_top_of",
      "relative_to": "<instance_id_of_parent_or_empty>",
      "position_hint": { "x": <float>, "y": <float_representing_z_plane> },
      "rotation_y": <float_degrees_0_to_360>,
      "state": "upright" | "knocked_over"
    }
  ]
}

### PLACEMENT LOGIC:
- "ground": Objects placed on the floor. 'relative_to' must be empty (""). 'position_hint' is world (X, Z).
- "on_top_of": Small props placed on surfaces (desks, tables). 'relative_to' must be the 'instance_id' of the surface prop. 'position_hint' is a local offset from that surface's center.)";

int main() {
    // Inicialización de la ventana (Raylib)
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Semantic Staging Tool");
    SetTargetFPS(60);

    // Inicialización del contexto de ImGui (rlImGui)
    GUI uiManager{};
    float roomWidth = 10;
    float roomDepth = 10;
    float roomScale = 1;
    std::string userPrompt = "";
    // Camara
    CameraController camera{};
    DisableCursor();

    // Objects
    SceneGraph sceneGraph;
    sceneGraph.Initialize("assets/models");

    // AI Client
    AIClient aiManager{};
    std::string lastAIError;
    auto promptCallback = [&](std::string& promptText, char* apiKey) {
        lastAIError.clear();
        // Estructura del request
        nlohmann::json fullPayload = {
            {"model", "llama-3.1-8b-instant"},
            {"temperature", 0.2},
            {"messages",
             nlohmann::json::array(
                 {{{"role", "system"}, {"content", SYSTEM_PROMPT}},
                  {{"role", "user"},
                   {"content",
                    sceneGraph.BuildAIContext(promptText, roomWidth, roomDepth)
                        .dump()}}})},
            {"response_format", {{"type", "json_object"}}}};

        // Mandar el request a la API
        aiManager.RequestLayoutAsync(
            "https://api.groq.com/openai/v1/chat/completions", apiKey,
            fullPayload);
    };
    while (!WindowShouldClose()) {

        // --- UPDATE ---
        camera.Update();

        if (auto error = aiManager.PollError()) {
            lastAIError = std::move(*error);
        }

        // --- DRAW ---
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // Renderizado 3D
        BeginMode3D(camera.GetCamera());
        DrawGrid(20, 1.0f);
        sceneGraph.Draw();
        EndMode3D();

        // Iniciar bloque de UI
        rlImGuiBegin();

        // Pintar mi objeto de la clase UI
        uiManager.DrawEditorUI(roomWidth, roomDepth, roomScale, promptCallback,
                               lastAIError);

        // Finalizar bloque de UI
        rlImGuiEnd();

        EndDrawing();
    }

    // 4. Limpieza de memoria y cierre de contextos
    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
