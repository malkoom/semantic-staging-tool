#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>

#include "../props/PropModel.hpp"

#include "nlohmann/json_fwd.hpp"

struct SceneInstance {
    std::string instanceId;
    std::string propId; // Referencia a la clave en m_Models
    Vector3 position{0.0f, 0.0f, 0.0f};
    float rotationY{0.0f}; // Grados
    float boundingRadius{0.5f};
    bool isStatic{false}; // Muebles ancla/pesados
};

class SceneGraph {
  public:
    SceneGraph() = default;
    ~SceneGraph() = default;

    void Initialize(const char* assetsDir);
    const nlohmann::json BuildAIContext(const std::string& userPrompt,
                                        float roomWidth, float roomDepth,
                                        const nlohmann::json& currentScene,
                                        bool isExtension);
    bool ApplyLayoutDirectives(const std::string& layoutJsonStr,
                               bool append, float roomWidth, float roomDepth);
    void ResolveOverlaps(float roomWidth, float roomDepth,
                         int maxIterations = 8);
    bool ExportSceneToFile(const std::string& filepath) const;
    nlohmann::json GenerateJSON() const;
    bool IsEmpty() const;
    void Clear();
    void Draw();

  private:
    bool IsPlaceableProp(const std::string& propId) const;
    void ClampToRoom(SceneInstance& instance, float roomWidth,
                     float roomDepth) const;
    std::unordered_map<std::string, PropModel> m_Models;
    std::vector<SceneInstance> m_Instances;
};
