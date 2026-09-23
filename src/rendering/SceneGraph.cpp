#include "SceneGraph.hpp"

#include "props/ModelsLoader.hpp"
#include "props/PropModel.hpp"

void SceneGraph::Initialize(const char* assetsDir) {
    ModelsLoader::LoadModelFilesInDirectory(assetsDir, m_Models);
}

const nlohmann::json SceneGraph::BuildAIContext(const std::string& userPrompt,
                                                float roomWidth,
                                                float roomDepth) {
    nlohmann::json context;

    // 1. Límites del plano horizontal XZ
    context["room_bounds"] = {{"width_x", roomWidth}, {"depth_z", roomDepth}};

    // 2. Inventario de assets cargados dinámicamente desde m_Models
    context["available_props"] = nlohmann::json::array();

    for (const auto& [name, model] : m_Models) {
        nlohmann::json propEntry;
        propEntry["id"] = name;

        // Radio del modelo
        propEntry["bounding_radius"] = model.GetRadius();

        // Opcional: pasar las dimensiones de la caja para que el LLM entienda
        // proporciones
        BoundingBox bounds = model.GetBounds();
        propEntry["dimensions"] = {{"width", bounds.max.x - bounds.min.x},
                                   {"height", bounds.max.y - bounds.min.y},
                                   {"depth", bounds.max.z - bounds.min.z}};

        context["available_props"].push_back(std::move(propEntry));
    }

    // 3. La intención semántica introducida en ImGui
    context["user_intent"] = userPrompt;

    return context;
}

void SceneGraph::Draw() {
    for (auto& model : m_Models) {
        model.second.Draw();
    }
}

void SceneGraph::ResolveOverlaps(int maxIterations) {

    std::vector<PropModel*> propVector;
    propVector.reserve(m_Models.size()); // Evita realocaciones innecesarias

    for (auto& [clave, valor] : m_Models) {
        propVector.push_back(&valor);

        for (int it = 0; it < maxIterations; ++it) {
            for (size_t i = 0; propVector.size(); ++i) {
                for (size_t j = i + 1; j < propVector.size(); ++j) {
                    auto& a = propVector[i];
                    auto& b = propVector[j];

                    Vector2 delta = {a->Position.x - b->Position.x,
                                     a->Position.y - b->Position.y};
                    float distSq = delta.x * delta.x + delta.y * delta.y;
                    float minDist = a->GetRadius() + b->GetRadius();

                    if (distSq < minDist * minDist && distSq > 0.0001f) {
                        float dist = std::sqrt(distSq);
                        float overlap = minDist - dist;
                        Vector2 normal = {delta.x / dist, delta.y / dist};

                        // Si uno es estático, solo se desplaza el móvil
                        if (a->IsStatic() && !b->IsStatic()) {
                            b->Position.x -= normal.x * overlap;
                            b->Position.y -= normal.y * overlap;
                        } else if (!a->IsStatic() && b->IsStatic()) {
                            a->Position.x += normal.x * overlap;
                            a->Position.y += normal.y * overlap;
                        } else if (!a->IsStatic() && !b->IsStatic()) {
                            // Si ambos son móviles, se reparten la separación
                            float halfOverlap = overlap * 0.5f;

                            a->Position.x += normal.x * halfOverlap;
                            a->Position.y += normal.y * halfOverlap;
                            b->Position.x -= normal.x * halfOverlap;
                            b->Position.y -= normal.y * halfOverlap;
                        }
                    }
                }
            }
        }
    }
}
