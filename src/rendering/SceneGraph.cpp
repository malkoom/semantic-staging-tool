#include "SceneGraph.hpp"

#include "props/ModelsLoader.hpp"

void SceneGraph::Initialize(const char* assetsDir) {
    ModelsLoader::LoadModelFilesInDirectory(assetsDir, m_Models);
}

void SceneGraph::Draw() {
    for (auto& model : m_Models) {
        model.second.Draw();
    }
}
