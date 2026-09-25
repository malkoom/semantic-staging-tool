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
                                        float roomWidth, float roomDepth);
    bool ApplyLayoutDirectives(const std::string& layoutJsonStr);
    void ResolveOverlaps(int maxIterations = 8);
    void Draw();

  private:
    std::unordered_map<std::string, PropModel> m_Models;
    std::vector<SceneInstance> m_Instances;
};
