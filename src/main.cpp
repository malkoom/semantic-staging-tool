#include <iostream>

#include "UI/GuizmoManager.hpp"

#if defined(_WIN32)
#define NOGDI  // Evita conflictos de estructuras GDI
#define NOUSER // Opcional si no necesitas User32 directamente
#define WIN32_LEAN_AND_MEAN
#endif

#include <raylib.h>

// Si necesitas incluir CPR o windows.h después:
#if defined(_WIN32)
// Raylib ya definió CloseWindow y ShowCursor.
// Desactivamos o renombramos las macros de Windows si causan colisión:
#undef CloseWindow
#undef ShowCursor
#endif

#include <string>

#include <nlohmann/json.hpp>

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
- "on_top_of": Small props placed on surfaces (desks, tables). 'relative_to' must be the 'instance_id' of the surface prop. 'position_hint' is a local offset from that surface's center.
- Use dimensions to estimate the scale of each prop and avoid obviously overlapping ground objects.
- current_scene contains already-placed entities with their actual positions and dimensions. In an incremental request, use it to avoid collisions, as the parent of an explicitly requested on_top_of placement, or to edit an existing entity.

### INCREMENTAL REQUESTS:
- The input includes "request_mode" and "current_scene".
- When request_mode is "create_new_scene", return all requested entities.
- When request_mode is "add_to_existing_scene", return only new entities or entities that must change. To edit an entity, return it with its existing instance_id and its desired placement_type, position_hint, rotation_y, and relative_to. Do not return unchanged entities. Do not delete entities.)";

nlohmann::json BuildLayoutResponseFormat() {
    const nlohmann::json positionHintSchema = {
        {"type", "object"},
        {"properties", {{"x", {{"type", "number"}}},
                        {"y", {{"type", "number"}}}}},
        {"required", {"x", "y"}},
        {"additionalProperties", false}};

    const nlohmann::json entitySchema = {
        {"type", "object"},
        {"properties",
         {{"instance_id", {{"type", "string"}}},
          {"prop_id", {{"type", "string"}}},
          {"placement_type",
           {{"type", "string"}, {"enum", {"ground", "on_top_of"}}}},
          {"relative_to", {{"type", "string"}}},
          {"position_hint", positionHintSchema},
          {"rotation_y", {{"type", "number"}}},
          {"state",
           {{"type", "string"}, {"enum", {"upright", "knocked_over"}}}}}},
        {"required", {"instance_id", "prop_id", "placement_type",
                      "relative_to", "position_hint", "rotation_y",
                      "state"}},
        {"additionalProperties", false}};

    return {{"type", "json_schema"},
            {"json_schema",
             {{"name", "scene_layout"},
              {"strict", true},
              {"schema",
               {{"type", "object"},
                {"properties",
                 {{"layout_name", {{"type", "string"}}},
                  {"entities", {{"type", "array"}, {"items", entitySchema}}}}},
                {"required", {"layout_name", "entities"}},
                {"additionalProperties", false}}}}}};
}

int main() {
    // Inicialización de la ventana (Raylib)
    const int screenWidth = 1280;
    const int screenHeight = 720;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MINIMIZED |
                   FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Semantic Staging Tool");
    SetTargetFPS(60);

    // Inicialización del contexto de ImGui (rlImGui)
    GUI uiManager{};
    float roomWidth = 10;
    float roomDepth = 10;
    float roomScale = 1;
    std::string userPrompt = "";
    GuizmoManager guizmoManager;

    // Camara
    CameraController camera{};

    // Objects
    SceneGraph sceneGraph;
    sceneGraph.Initialize("assets/models");

    // Ambos shaders mantienen posición, normal y cámara en espacio mundo.
    Shader phongShader =
        LoadShader("shaders/basic_light.vs", "shaders/basic_light.glsl");
    int viewPosLoc = -1;
    if (phongShader.id != 0) {
        std::cout << "Bindeando el shader al sceneGraph" << std::endl;
        sceneGraph.SetShader(phongShader);
        viewPosLoc = GetShaderLocation(phongShader, "viewPos");
    }

    // JSON save
    nlohmann::json currentLayout{};

    // AI Client
    AIClient aiManager{};
    std::string lastAIError;
    auto promptCallback = [&](std::string& promptText, char* apiKey,
                              const std::string& model) {
        lastAIError.clear();
        const bool extendScene = !sceneGraph.IsEmpty();
        const nlohmann::json currentScene = sceneGraph.GenerateJSON();

        nlohmann::json fullPayload = {
            {"model", model},
            {"temperature", 0.2},
            {"messages",
             nlohmann::json::array(
                 {{{"role", "system"}, {"content", SYSTEM_PROMPT}},
                  {{"role", "user"},
                   {"content",
                    sceneGraph
                        .BuildAIContext(promptText, roomWidth, roomDepth,
                                        currentScene, extendScene)
                        .dump()}}})},
            {"response_format", BuildLayoutResponseFormat()}};

        // Mandar el request a la API
        aiManager.RequestLayoutAsync(
            "https://api.groq.com/openai/v1/chat/completions", apiKey,
            fullPayload);
    };

    while (!WindowShouldClose()) {
        // --- UPDATE ---
        // No mover la cámara mientras se arrastra un manipulador.
        if (!guizmoManager.IsUsing() && IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            guizmoManager.CanUse = false;
            HideCursor();
            camera.Update();
        } else {
            ShowCursor();
            guizmoManager.CanUse = true;
        }

        if (auto error = aiManager.PollError()) {
            lastAIError = std::move(*error);
        }

        if (auto layoutJson = aiManager.PollResult()) {
            currentLayout = nlohmann::json::parse(layoutJson.value());
            sceneGraph.ApplyLayoutDirectives(*layoutJson, !sceneGraph.IsEmpty(),
                                             roomWidth, roomDepth);
        }

        guizmoManager.TryHitObject(sceneGraph, camera);

        // --- DRAW ---
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // Renderizado 3D
        BeginMode3D(camera.GetCamera());
        DrawGrid(20, 1.0f);

        if (viewPosLoc >= 0) {
            SetShaderValue(phongShader, viewPosLoc,
                           &camera.GetCamera().position, SHADER_UNIFORM_VEC3);
        }
        sceneGraph.Draw();
        EndMode3D();

        // Iniciar bloque de UI
        rlImGuiBegin();

        // Pintar mi objeto de la clase UI
        uiManager.DrawEditorUI(roomWidth, roomDepth, roomScale, promptCallback,
                               lastAIError, sceneGraph, guizmoManager);
        // Control de Guizmos
        guizmoManager.Update(sceneGraph, camera);

        // Finalizar bloque de UI
        rlImGuiEnd();

        EndDrawing();
    }

    // 4. Limpieza de memoria y cierre de contextos
    rlImGuiShutdown();
    if (phongShader.id != 0) {
        UnloadShader(phongShader);
    }
    CloseWindow();

    return 0;
}
