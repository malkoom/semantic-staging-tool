
#include <string>

#include "raylib.h"
#include "rlImGui.h"

#include "UI/GUI.hpp"

#include "network/AIClient.hpp"

#include "rendering/CameraController.hpp"
#include "rendering/SceneGraph.hpp"

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
    std::string apiKey = "Mi API key";
    std::string endPoint = "Endpoint del modelo de lenguaje";
    auto promptCallback = [&]() {
        auto payload =
            sceneGraph.BuildAIContext(userPrompt, roomWidth, roomDepth);
        aiManager.RequestLayoutAsync(endPoint, apiKey, payload);
    };
    while (!WindowShouldClose()) {

        // --- UPDATE ---
        camera.Update();

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
        uiManager.DrawEditorUI(userPrompt, roomWidth, roomDepth, roomScale,
                               promptCallback);

        // Finalizar bloque de UI
        rlImGuiEnd();

        EndDrawing();
    }

    // 4. Limpieza de memoria y cierre de contextos
    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
