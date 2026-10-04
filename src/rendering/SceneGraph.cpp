#include <cctype>
#include <cmath>

#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <vector>

#include "SceneGraph.hpp"

#include "props/ModelsLoader.hpp"
#include "props/PropModel.hpp"

#include "utils/Utils.hpp"

namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return value;
}

bool HasAnyToken(const std::string& value,
                 std::initializer_list<const char*> tokens) {
    return std::any_of(tokens.begin(), tokens.end(), [&](const char* token) {
        return value.find(token) != std::string::npos;
    });
}

bool IsStructuralAsset(const std::string& propId) {
    const std::string name = ToLower(propId);
    return HasAnyToken(name, {"wall", "floor", "ceiling", "window", "door",
                              "board", "poster"});
}

// Solo se envían hechos medibles. Inferir el significado de un asset desde
// su nombre mezclaba muebles con geometría estructural y daba peores layouts.
nlohmann::json DescribeProp(const std::string& id, const PropModel& model) {
    BoundingBox bounds = model.GetBounds();
    return {{"id", id},
            {"dimensions",
             {{"width", bounds.max.x - bounds.min.x},
              {"height", bounds.max.y - bounds.min.y},
              {"depth", bounds.max.z - bounds.min.z}}}};
}

} // namespace

void SceneGraph::Initialize(const char* assetsDir) {
    ModelsLoader::LoadModelFilesInDirectory(assetsDir, m_Models);
}

bool SceneGraph::IsPlaceableProp(const std::string& propId) const {
    return m_Models.contains(propId) && !IsStructuralAsset(propId);
}

void SceneGraph::ClampToRoom(SceneInstance& instance, float roomWidth,
                             float roomDepth) const {
    const BoundingBox bounds = m_Models.at(instance.propId).GetBounds();
    const float halfWidth = (bounds.max.x - bounds.min.x) * 0.5f;
    const float halfDepth = (bounds.max.z - bounds.min.z) * 0.5f;
    const float radians = instance.rotationY * DEG2RAD;
    const float extentX = std::abs(std::cos(radians)) * halfWidth +
                          std::abs(std::sin(radians)) * halfDepth;
    const float extentZ = std::abs(std::sin(radians)) * halfWidth +
                          std::abs(std::cos(radians)) * halfDepth;
    const float roomHalfWidth = roomWidth * 0.5f;
    const float roomHalfDepth = roomDepth * 0.5f;
    instance.position.x =
        extentX >= roomHalfWidth
            ? 0.0f
            : std::clamp(instance.position.x, -roomHalfWidth + extentX,
                         roomHalfWidth - extentX);
    instance.position.z =
        extentZ >= roomHalfDepth
            ? 0.0f
            : std::clamp(instance.position.z, -roomHalfDepth + extentZ,
                         roomHalfDepth - extentZ);
}

const nlohmann::json
SceneGraph::BuildAIContext(const std::string& userPrompt, float roomWidth,
                           float roomDepth, const nlohmann::json& currentScene,
                           bool isExtension) {
    nlohmann::json context;

    // 1. Límites del plano horizontal XZ
    context["room_bounds"] = {{"width_x", roomWidth}, {"depth_z", roomDepth}};

    // 2. Inventario de assets cargados dinámicamente desde m_Models
    context["available_props"] = nlohmann::json::array();

    std::vector<std::string> propNames;
    propNames.reserve(m_Models.size());
    for (const auto& [name, model] : m_Models) {
        (void)model;
        propNames.push_back(name);
    }
    std::sort(propNames.begin(), propNames.end());
    for (const auto& name : propNames) {
        if (IsPlaceableProp(name))
            context["available_props"].push_back(
                DescribeProp(name, m_Models.at(name)));
    }

    // 3. La intención semántica introducida en ImGui
    context["user_intent"] = userPrompt;
    // La escena existente conserva sus coordenadas y añade únicamente medidas
    // reales; no se deduce semántica de los nombres de archivo.
    nlohmann::json enrichedScene = currentScene;
    enrichedScene["entities"] = nlohmann::json::array();
    for (const auto& inst : m_Instances) {
        nlohmann::json entity =
            DescribeProp(inst.propId, m_Models.at(inst.propId));
        entity["instance_id"] = inst.instanceId;
        entity["position"] = {{"x", inst.position.x},
                              {"y", inst.position.y},
                              {"z", inst.position.z}};
        entity["rotation_y"] = inst.rotationY;
        enrichedScene["entities"].push_back(std::move(entity));
    }
    context["current_scene"] = std::move(enrichedScene);
    context["request_mode"] =
        isExtension ? "add_to_existing_scene" : "create_new_scene";

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

void SceneGraph::SetShader(Shader shader) {
    for (auto& [_, model] : m_Models) {
        model.SetShader(shader);
    }
}

void SceneGraph::ResolveOverlaps(float roomWidth, float roomDepth,
                                 int maxIterations) {
    for (auto& instance : m_Instances)
        ClampToRoom(instance, roomWidth, roomDepth);
    if (m_Instances.size() < 2)
        return;

    for (int it = 0; it < maxIterations; ++it) {
        for (size_t i = 0; i < m_Instances.size(); ++i) {
            for (size_t j = i + 1; j < m_Instances.size(); ++j) {
                auto& a = m_Instances[i];
                auto& b = m_Instances[j];

                // Rectángulos alineados a los ejes que contienen al prop
                // rotado. Es mucho más fiel que el círculo medio anterior.
                const BoundingBox aBounds = m_Models.at(a.propId).GetBounds();
                const BoundingBox bBounds = m_Models.at(b.propId).GetBounds();
                const float aRadians = a.rotationY * DEG2RAD;
                const float bRadians = b.rotationY * DEG2RAD;
                const float aHalfWidth = (aBounds.max.x - aBounds.min.x) * 0.5f;
                const float aHalfDepth = (aBounds.max.z - aBounds.min.z) * 0.5f;
                const float bHalfWidth = (bBounds.max.x - bBounds.min.x) * 0.5f;
                const float bHalfDepth = (bBounds.max.z - bBounds.min.z) * 0.5f;
                const float aExtentX =
                    std::abs(std::cos(aRadians)) * aHalfWidth +
                    std::abs(std::sin(aRadians)) * aHalfDepth;
                const float aExtentZ =
                    std::abs(std::sin(aRadians)) * aHalfWidth +
                    std::abs(std::cos(aRadians)) * aHalfDepth;
                const float bExtentX =
                    std::abs(std::cos(bRadians)) * bHalfWidth +
                    std::abs(std::sin(bRadians)) * bHalfDepth;
                const float bExtentZ =
                    std::abs(std::sin(bRadians)) * bHalfWidth +
                    std::abs(std::cos(bRadians)) * bHalfDepth;
                float dx = a.position.x - b.position.x;
                float dz = a.position.z - b.position.z;
                const float overlapX = aExtentX + bExtentX - std::abs(dx);
                const float overlapZ = aExtentZ + bExtentZ - std::abs(dz);

                if (overlapX > 0.0f && overlapZ > 0.0f) {
                    const bool resolveX = overlapX <= overlapZ;
                    const float overlap = resolveX ? overlapX : overlapZ;
                    const float delta =
                        (resolveX ? dx : dz) < 0.0f ? -overlap : overlap;

                    // Resolución según masa/fijación estática
                    if (a.isStatic && !b.isStatic) {
                        if (resolveX)
                            b.position.x -= delta;
                        else
                            b.position.z -= delta;
                    } else if (!a.isStatic && b.isStatic) {
                        if (resolveX)
                            a.position.x += delta;
                        else
                            a.position.z += delta;
                    } else if (!a.isStatic && !b.isStatic) {
                        float halfOverlap = overlap * 0.5f;
                        const float halfDelta =
                            delta < 0.0f ? -halfOverlap : halfOverlap;
                        if (resolveX) {
                            a.position.x += halfDelta;
                            b.position.x -= halfDelta;
                        } else {
                            a.position.z += halfDelta;
                            b.position.z -= halfDelta;
                        }
                    }
                    ClampToRoom(a, roomWidth, roomDepth);
                    ClampToRoom(b, roomWidth, roomDepth);
                }
            }
        }
    }
}

bool SceneGraph::ApplyLayoutDirectives(const std::string& layoutJsonStr,
                                       bool append, float roomWidth,
                                       float roomDepth) {
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

    // Una ampliación conserva las instancias existentes; una creación las
    // reemplaza completamente.
    if (!append) {
        m_Instances.clear();
    }

    // Estructura temporal para objetos secundarios que van "on_top_of"
    struct DeferredChild {
        SceneInstance instance;
        std::string parentId;
        Vector2 localOffset;
        bool updatesExisting;
    };
    std::vector<DeferredChild> deferredChildren;

    // 2. Primera pasada: colocar los objetos en suelo ("ground")
    for (const auto& item : root["entities"]) {
        std::string propId = item.value("prop_id", "");

        // Verificar que el prop existe en el catálogo cargado
        auto itModel = m_Models.find(propId);
        if (!IsPlaceableProp(propId)) {
            std::cerr
                << "[SceneGraph] Prop desconocido o no colocable omitido: "
                << propId << "\n";
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
        const BoundingBox bounds = propModel.GetBounds();
        const float width = bounds.max.x - bounds.min.x;
        const float depth = bounds.max.z - bounds.min.z;

        // Los props con una huella grande sirven de anclas al resolver solapes.
        inst.isStatic = (width * depth > 1.5f);

        auto existingIt =
            append ? std::find_if(m_Instances.begin(), m_Instances.end(),
                                  [&](const SceneInstance& existing) {
                                      return existing.instanceId == inst.instanceId;
                                  })
                   : m_Instances.end();
        const bool updatesExisting = existingIt != m_Instances.end();

        if (placement == "ground") {
            // El pivote XZ inicial viene directo de la IA
            // Y inicial se apoya sobre el suelo (asumiendo base en Y = 0)
            BoundingBox b = propModel.GetBounds();
            float pivotOffsetY = -b.min.y; // Si el origen está en el centro,
                                           // sube la mitad de la altura
            inst.position = {posHint.x, pivotOffsetY, posHint.y};

            // En una solicitud incremental, el mismo instance_id representa
            // una edición de la instancia, no un duplicado que deba omitirse.
            if (updatesExisting)
                *existingIt = std::move(inst);
            else
                m_Instances.push_back(std::move(inst));
        } else if (placement == "on_top_of") {
            std::string parentId = item.value("relative_to", "");
            deferredChildren.push_back(
                {std::move(inst), parentId, posHint, updatesExisting});
        }
    }

    // 3. Resolver colisiones en el suelo (XZ) antes de montar cosas encima
    ResolveOverlaps(roomWidth, roomDepth, 12);

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
            const BoundingBox childBox =
                m_Models.at(child.instance.propId).GetBounds();
            float parentTopY = parentIt->position.y + parentBox.max.y;

            // position_hint es un offset local; no puede sacar al hijo fuera
            // de la superficie del padre.
            const float parentHalfWidth =
                (parentBox.max.x - parentBox.min.x) * 0.5f;
            const float parentHalfDepth =
                (parentBox.max.z - parentBox.min.z) * 0.5f;
            const float childHalfWidth =
                (childBox.max.x - childBox.min.x) * 0.5f;
            const float childHalfDepth =
                (childBox.max.z - childBox.min.z) * 0.5f;
            child.localOffset.x =
                std::clamp(child.localOffset.x,
                           -std::max(0.0f, parentHalfWidth - childHalfWidth),
                           std::max(0.0f, parentHalfWidth - childHalfWidth));
            child.localOffset.y =
                std::clamp(child.localOffset.y,
                           -std::max(0.0f, parentHalfDepth - childHalfDepth),
                           std::max(0.0f, parentHalfDepth - childHalfDepth));

            // Rotar el offset local según la orientación del padre
            float rad = parentIt->rotationY * DEG2RAD;
            float cosR = std::cos(rad);
            float sinR = std::sin(rad);

            float worldOffsetX =
                child.localOffset.x * cosR - child.localOffset.y * sinR;
            float worldOffsetZ =
                child.localOffset.x * sinR + child.localOffset.y * cosR;

            child.instance.position = {parentIt->position.x + worldOffsetX,
                                       parentTopY - childBox.min.y,
                                       parentIt->position.z + worldOffsetZ};
            child.instance.rotationY +=
                parentIt->rotationY; // Sumar rotación del padre

            if (child.updatesExisting) {
                auto existingIt =
                    std::find_if(m_Instances.begin(), m_Instances.end(),
                                 [&](const SceneInstance& inst) {
                                     return inst.instanceId ==
                                            child.instance.instanceId;
                                 });
                if (existingIt != m_Instances.end())
                    *existingIt = std::move(child.instance);
                else
                    m_Instances.push_back(std::move(child.instance));
            } else {
                m_Instances.push_back(std::move(child.instance));
            }
        } else {
            // Si el padre no existe, colocarlo en el suelo por defecto
            const BoundingBox childBox =
                m_Models.at(child.instance.propId).GetBounds();
            child.instance.position = {child.localOffset.x, -childBox.min.y,
                                       child.localOffset.y};
            if (child.updatesExisting) {
                auto existingIt =
                    std::find_if(m_Instances.begin(), m_Instances.end(),
                                 [&](const SceneInstance& inst) {
                                     return inst.instanceId ==
                                            child.instance.instanceId;
                                 });
                if (existingIt != m_Instances.end())
                    *existingIt = std::move(child.instance);
                else
                    m_Instances.push_back(std::move(child.instance));
            } else {
                m_Instances.push_back(std::move(child.instance));
            }
        }
    }

    return true;
}

bool SceneGraph::ExportSceneToFile(const std::string& filepath) const {

    return SaveJsonToFile(filepath, GenerateJSON());
}

nlohmann::json SceneGraph::GenerateJSON() const {
    nlohmann::json root;
    root["entities"] = nlohmann::json::array();

    for (const auto& inst : m_Instances) {
        nlohmann::json entity;
        entity["instance_id"] = inst.instanceId;
        entity["prop_id"] = inst.propId;
        entity["position"] = {{"x", inst.position.x},
                              {"y", inst.position.y},
                              {"z", inst.position.z}};
        entity["rotation_y"] = inst.rotationY;
        entity["is_static"] = inst.isStatic;

        root["entities"].push_back(std::move(entity));
    }

    return root;
}

bool SceneGraph::IsEmpty() const { return m_Instances.empty(); }

void SceneGraph::Clear() { m_Instances.clear(); }

std::vector<SceneInstance>& SceneGraph::GetInstances() { return m_Instances; }

const std::vector<SceneInstance>& SceneGraph::GetInstances() const {
    return m_Instances;
}

BoundingBox SceneGraph::GetModelBounds(const std::string& propId) const {
    return m_Models.at(propId).GetBounds();
}

const std::unordered_map<std::string, PropModel>&
SceneGraph::GetModels() const {
    return m_Models;
}

bool SceneGraph::AddManualInstance(const std::string& propId, Vector3 position,
                                   float roomWidth, float roomDepth) {

    // FIXME: Al añadir instancias manuales no se tienen en cuenta para el
    // prompt del llm

    if (!IsPlaceableProp(propId))
        return false;

    SceneInstance inst;
    inst.propId = propId;
    inst.rotationY = 0.0f;

    const BoundingBox bounds = m_Models.at(propId).GetBounds();
    const float width = bounds.max.x - bounds.min.x;
    const float depth = bounds.max.z - bounds.min.z;

    // Los props con una huella grande sirven de anclas al resolver solapes.
    inst.isStatic = (width * depth > 1.5f);

    // El asset puede reutilizarse; cada colocación necesita un ID propio.
    for (size_t suffix = 1;; ++suffix) {
        inst.instanceId = "manual_" + propId + "_" + std::to_string(suffix);
        const bool exists = std::any_of(
            m_Instances.begin(), m_Instances.end(), [&](const auto& existing) {
                return existing.instanceId == inst.instanceId;
            });
        if (!exists)
            break;
    }

    // La posición de entrada representa XZ; apoyamos el modelo en el suelo.
    inst.position = {position.x, -bounds.min.y, position.z};
    m_Instances.push_back(std::move(inst));
    ResolveOverlaps(roomWidth, roomDepth);

    return true;
}
