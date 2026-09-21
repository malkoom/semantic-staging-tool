#include "SceneGraph.hpp"

#include "props/ModelsLoader.hpp"

void SceneGraph::Initialize(const char* assetsDir) {
    ModelsLoader::LoadModelFilesInDirectory(assetsDir, m_Models);
}

const nlohmann::json SceneGraph::BuildAIContext(const std::string& userPrompt,
                                                float roomWidth,
                                                float roomDepth) {
    nlohmann::json payload;

    // 1. Límites del espacio
    payload["room_bounds"] = {{"size_x", roomWidth}, {"size_z", roomDepth}};

    // 2. Extracción automática de nombres de modelos cargados
    payload["available_props"] = nlohmann::json::array();
    for (const auto& [propName, propModel] : m_Models) {
        payload["available_props"].push_back({{"id", propName}});
    }

    // 3. La intención que el usuario escribió en ImGui
    payload["user_intent"] = userPrompt;

    return payload;
}

void SceneGraph::Draw() {
    for (auto& model : m_Models) {
        model.second.Draw();
    }
}
