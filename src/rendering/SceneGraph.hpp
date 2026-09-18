#pragma once

#include <string>
#include <unordered_map>

#include "../props/PropModel.hpp"

class SceneGraph {
  public:
    SceneGraph() = default;
    ~SceneGraph() = default;

    void Initialize(const char* assetsDir);
    void Draw();

  private:
    std::unordered_map<std::string, PropModel> m_Models;
};
