#include <iostream>

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
    for (const auto& inst : m_Instances) {
        auto it = m_Models.find(inst.propId);
        if (it != m_Models.end()) {
            DrawModelEx(it->second.GetModel(), inst.position,
                        {0.0f, 1.0f, 0.0f}, // Eje vertical Y
                        inst.rotationY, {1.0f, 1.0f, 1.0f}, WHITE);
        }
    }
}

void SceneGraph::ResolveOverlaps(int maxIterations) {
    if (m_Instances.size() < 2)
        return;

    for (int it = 0; it < maxIterations; ++it) {
        for (size_t i = 0; i < m_Instances.size(); ++i) {
            for (size_t j = i + 1; j < m_Instances.size(); ++j) {
                auto& a = m_Instances[i];
                auto& b = m_Instances[j];

                // Distancia en el plano XZ (suelo)
                float dx = a.position.x - b.position.x;
                float dz = a.position.z - b.position.z;
                float distSq = dx * dx + dz * dz;
                float minDist = a.boundingRadius + b.boundingRadius;

                if (distSq < minDist * minDist && distSq > 0.0001f) {
                    float dist = std::sqrt(distSq);
                    float overlap = minDist - dist;
                    float nx = dx / dist;
                    float nz = dz / dist;

                    // Resolución según masa/fijación estática
                    if (a.isStatic && !b.isStatic) {
                        b.position.x -= nx * overlap;
                        b.position.z -= nz * overlap;
                    } else if (!a.isStatic && b.isStatic) {
                        a.position.x += nx * overlap;
                        a.position.z += nz * overlap;
                    } else if (!a.isStatic && !b.isStatic) {
                        float halfOverlap = overlap * 0.5f;
                        a.position.x += nx * halfOverlap;
                        a.position.z += nz * halfOverlap;
                        b.position.x -= nx * halfOverlap;
                        b.position.z -= nz * halfOverlap;
                    }
                }
            }
        }
    }
}

bool SceneGraph::ApplyLayoutDirectives(const std::string& layoutJsonStr) {
    nlohmann::json root;
    try {
        root = nlohmann::json::parse(layoutJsonStr);
    } catch (const std::exception& e) {
        std::cerr << "[SceneGraph] Error al parsear JSON: " << e.what() << "\n";
        return false;
    }

    if (!root.contains("entities") || !root["entities"].is_array()) {
        std::cerr << "[SceneGraph] JSON no contiene array 'entities'\n";
        return false;
    }

    // 1. Limpiar instancias previas
    m_Instances.clear();

    // Estructura temporal para objetos secundarios que van "on_top_of"
    struct DeferredChild {
        SceneInstance instance;
        std::string parentId;
        Vector2 localOffset;
    };
    std::vector<DeferredChild> deferredChildren;

    // 2. Primera pasada: colocar los objetos en suelo ("ground")
    for (const auto& item : root["entities"]) {
        std::string propId = item.value("prop_id", "");

        // Verificar que el prop existe en el catálogo cargado
        auto itModel = m_Models.find(propId);
        if (itModel == m_Models.end()) {
            std::cerr << "[SceneGraph] Prop desconocido omitido: " << propId
                      << "\n";
            continue;
        }

        const auto& propModel = itModel->second;
        std::string placement = item.value("placement_type", "ground");
        float rotY = item.value("rotation_y", 0.0f);

        Vector2 posHint = {0.0f, 0.0f};
        if (item.contains("position_hint")) {
            posHint.x = item["position_hint"].value("x", 0.0f);
            posHint.y =
                item["position_hint"].value("y", 0.0f); // Mapeado a Z en 3D
        }

        SceneInstance inst;
        inst.instanceId = item.value("instance_id", "");
        inst.propId = propId;
        inst.rotationY = rotY;
        inst.boundingRadius = propModel.GetRadius();

        // Clasificar objetos grandes como anclas estáticas
        inst.isStatic = (inst.boundingRadius > 1.2f);

        if (placement == "ground") {
            // El pivote XZ inicial viene directo de la IA
            // Y inicial se apoya sobre el suelo (asumiendo base en Y = 0)
            BoundingBox b = propModel.GetBounds();
            float pivotOffsetY = -b.min.y; // Si el origen está en el centro,
                                           // sube la mitad de la altura
            inst.position = {posHint.x, pivotOffsetY, posHint.y};

            m_Instances.push_back(std::move(inst));
        } else if (placement == "on_top_of") {
            std::string parentId = item.value("relative_to", "");
            deferredChildren.push_back({std::move(inst), parentId, posHint});
        }
    }

    // 3. Resolver colisiones en el suelo (XZ) antes de montar cosas encima
    ResolveOverlaps(12);

    // 4. Segunda pasada: calcular la posición de los objetos sobre superficies
    for (auto& child : deferredChildren) {
        // Buscar el padre en las instancias ya resueltas
        auto parentIt =
            std::find_if(m_Instances.begin(), m_Instances.end(),
                         [&](const SceneInstance& inst) {
                             return inst.instanceId == child.parentId;
                         });

        if (parentIt != m_Instances.end()) {
            const auto& parentModel = m_Models.at(parentIt->propId);
            BoundingBox parentBox = parentModel.GetBounds();
            float parentTopY = parentIt->position.y + parentBox.max.y;

            // Rotar el offset local según la orientación del padre
            float rad = parentIt->rotationY * DEG2RAD;
            float cosR = std::cos(rad);
            float sinR = std::sin(rad);

            float worldOffsetX =
                child.localOffset.x * cosR - child.localOffset.y * sinR;
            float worldOffsetZ =
                child.localOffset.x * sinR + child.localOffset.y * cosR;

            child.instance.position = {parentIt->position.x + worldOffsetX,
                                       parentTopY,
                                       parentIt->position.z + worldOffsetZ};
            child.instance.rotationY +=
                parentIt->rotationY; // Sumar rotación del padre

            m_Instances.push_back(std::move(child.instance));
        } else {
            // Si el padre no existe, colocarlo en el suelo por defecto
            child.instance.position = {child.localOffset.x, 0.0f,
                                       child.localOffset.y};
            m_Instances.push_back(std::move(child.instance));
        }
    }

    return true;
}
