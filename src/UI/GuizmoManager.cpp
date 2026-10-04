#include <algorithm>
#include <iostream>

#include "GuizmoManager.hpp"

#include "raylib.h"

bool GuizmoManager::TryHitObject(SceneGraph& sceneGraph,
                                 CameraController& cameraController) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !ImGui::GetIO().WantCaptureMouse) {
        Ray ray = GetMouseRay(GetMousePosition(), cameraController.GetCamera());
        float closestDist = 100000.0f;
        m_CurrentInstanceId.clear();

        // Recorrer instancias para ver a cuál hemos hecho clic
        for (size_t i = 0; i < sceneGraph.GetInstances().size(); ++i) {
            auto& inst = sceneGraph.GetInstances()[i];
            const BoundingBox localBounds =
                sceneGraph.GetModelBounds(inst.propId);

            // Desplazar la caja a la posición de la instancia en el mundo
            BoundingBox worldBounds = {{localBounds.min.x + inst.position.x,
                                        localBounds.min.y + inst.position.y,
                                        localBounds.min.z + inst.position.z},
                                       {localBounds.max.x + inst.position.x,
                                        localBounds.max.y + inst.position.y,
                                        localBounds.max.z + inst.position.z}};

            RayCollision collision = GetRayCollisionBox(ray, worldBounds);
            if (collision.hit && collision.distance < closestDist) {
                closestDist = collision.distance;
                m_CurrentInstanceId = inst.instanceId;
                std::cout << "Colision con rayo" << std::endl;
            }
        }
        return !m_CurrentInstanceId.empty();
    }
    return false;
}

void GuizmoManager::Update(SceneGraph& sceneGraph, CameraController& camera) {
    if (m_CurrentInstanceId.empty() || !CanUse)
        return;

    auto& instances = sceneGraph.GetInstances();
    const auto instanceIt = std::find_if(
        instances.begin(), instances.end(), [&](const SceneInstance& instance) {
            return instance.instanceId == m_CurrentInstanceId;
        });
    if (instanceIt == instances.end()) {
        m_CurrentInstanceId.clear();
        return;
    }
    SceneInstance& currentInstance = *instanceIt;

    // Inicializacion del frame de ImGuizmo
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    ImGuizmo::BeginFrame();

    // Datos de la camara
    Matrix view = GetCameraMatrix(camera.GetCamera());
    float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
    Matrix proj = MatrixPerspective(camera.GetCamera().fovy * DEG2RAD, aspect,
                                    0.01f, 1000.0f);
    // Matrix no está almacenada secuencialmente como ImGuizmo espera.
    // MatrixToFloatV la convierte a la disposición column-major correcta.
    const float16 viewMatrix = MatrixToFloatV(view);
    const float16 projectionMatrix = MatrixToFloatV(proj);

    // 1. Descomponer los datos de tu instancia a arrays (ImGuizmo usa
    // grados por defecto)
    float matrixTranslation[3] = {currentInstance.position.x,
                                  currentInstance.position.y,
                                  currentInstance.position.z};
    float matrixRotation[3] = {0.0f, currentInstance.rotationY,
                               0.0f}; // Solo rotamos en Y
    float matrixScale[3] = {1.0f, 1.0f, 1.0f};
    float transformMatrix[16];

    // 2. Construir la matriz 4x4
    ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation,
                                            matrixScale, transformMatrix);

    // T = mover, R = rotar. W y E se reservan para mover la cámara.
    static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
    if (IsKeyPressed(KEY_W))
        currentGizmoOperation = ImGuizmo::TRANSLATE;
    if (IsKeyPressed(KEY_E))
        currentGizmoOperation = ImGuizmo::ROTATE;
    if (IsKeyPressed(KEY_BACKSPACE))
        std::erase_if(instances, [&](const auto& inst) {
            return inst.instanceId == currentInstance.instanceId;
        });

    // 4. Dibujar y procesar la manipulación
    ImGuizmo::Manipulate(viewMatrix.v, projectionMatrix.v,
                         currentGizmoOperation,
                         ImGuizmo::LOCAL, // Espacio local para que los ejes
                                          // giren con el objeto
                         transformMatrix);

    // 5. Si el usuario está arrastrando la flecha, volcar los datos de
    // vuelta a la escena
    if (ImGuizmo::IsUsing()) {
        ImGuizmo::DecomposeMatrixToComponents(
            transformMatrix, matrixTranslation, matrixRotation, matrixScale);

        currentInstance.position.x = matrixTranslation[0];
        currentInstance.position.y = matrixTranslation[1];
        currentInstance.position.z = matrixTranslation[2];
        currentInstance.rotationY = matrixRotation[1];
    }
}

bool GuizmoManager::IsUsing() const { return ImGuizmo::IsUsing(); }
