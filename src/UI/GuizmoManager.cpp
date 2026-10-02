#include <iostream>

#include "GuizmoManager.hpp"

bool GuizmoManager::TryHitObject(SceneGraph& sceneGraph,
                                 CameraController& cameraController) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !ImGui::GetIO().WantCaptureMouse) {
        Ray ray = GetMouseRay(GetMousePosition(), cameraController.GetCamera());
        float closestDist = 100000.0f;
        m_CurrentInst = nullptr;

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
                m_CurrentInst = &inst;
                std::cout << "Colision con rayo" << std::endl;
            }
        }
        return m_CurrentInst != nullptr;
    }
    return false;
}

void GuizmoManager::Update(CameraController& camera) {
    if (m_CurrentInst == nullptr)
        return;

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
    float matrixTranslation[3] = {m_CurrentInst->position.x,
                                  m_CurrentInst->position.y,
                                  m_CurrentInst->position.z};
    float matrixRotation[3] = {0.0f, m_CurrentInst->rotationY,
                               0.0f}; // Solo rotamos en Y
    float matrixScale[3] = {1.0f, 1.0f, 1.0f};
    float transformMatrix[16];

    // 2. Construir la matriz 4x4
    ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation,
                                            matrixScale, transformMatrix);

    // T = mover, R = rotar. W y E se reservan para mover la cámara.
    static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
    if (IsKeyPressed(KEY_T))
        currentGizmoOperation = ImGuizmo::TRANSLATE;
    if (IsKeyPressed(KEY_R))
        currentGizmoOperation = ImGuizmo::ROTATE;

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

        m_CurrentInst->position.x = matrixTranslation[0];
        m_CurrentInst->position.y = matrixTranslation[1];
        m_CurrentInst->position.z = matrixTranslation[2];
        m_CurrentInst->rotationY = matrixRotation[1];
    }
}

bool GuizmoManager::IsUsing() const { return ImGuizmo::IsUsing(); }
