#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>

#include "../props/PropModel.hpp"

#include "nlohmann/json_fwd.hpp"

class SceneGraph {
  public:
    SceneGraph() = default;
    ~SceneGraph() = default;

    void Initialize(const char* assetsDir);
    const nlohmann::json BuildAIContext(const std::string& userPrompt,
                                        float roomWidth, float roomDepth);
    void Draw();

  private:
    std::unordered_map<std::string, PropModel> m_Models;
};
