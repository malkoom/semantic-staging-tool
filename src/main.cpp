#include "imgui.h"
#include "raylib.h"
#include "rlImGui.h"

#include "props/PropModel.hpp"

#include "rendering/CameraController.hpp"
#include "rendering/SceneGraph.hpp"

int main() {
    // Inicialización de la ventana (Raylib)
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Semantic Staging Tool");
    SetTargetFPS(60);

    // Inicialización del contexto de ImGui (rlImGui)
    rlImGuiSetup(true);

    // Camara
    CameraController camera{};
    DisableCursor();

    // Objects
    PropModel model1{"assets/models/StylizedFood_Toyamon/Assets/obj/"
                     "Food_Vegetables_Broccoli.obj"}; // Bucle principal
    PropModel model2{"assets/models/StylizedFood_Toyamon/Assets/obj/"
                     "Food_Vegetables_Potato.obj"}; // Bucle principal

    model2.Position = {3, 0, 0};

    model1.Scale = 10;
    model2.Scale = 10;

    model2.ModelColor = MAGENTA;

    SceneGraph sceneGraph;
    sceneGraph.Initialize("assets/models");

    while (!WindowShouldClose()) {

        // --- UPDATE ---
        camera.Update();

        // --- DRAW (Renderizado) ---
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // Renderizado 3D
        BeginMode3D(camera.GetCamera());
        // DrawCube({0, 0, 0}, 2.f, 2.f, 2.f, RAYWHITE);
        // model1.Draw();
        // model2.Draw();
        sceneGraph.Draw();
        EndMode3D();

        // Iniciar bloque de UI
        rlImGuiBegin();
        // Ventana de prueba para verificar que ImGui compila y funciona
        bool open = true;
        ImGui::ShowDemoWindow(&open);

        // Finalizar bloque de UI
        rlImGuiEnd();

        EndDrawing();
    }

    // 4. Limpieza de memoria y cierre de contextos
    rlImGuiShutdown();
    CloseWindow();

    return 0;
}
