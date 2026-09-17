#pragma once

#include <string>
#include <unordered_map>

#include "PropModel.hpp"

namespace ModelsLoader {

void LoadModelFilesInDirectory(
    const char* dirPath, std::unordered_map<std::string, PropModel>& modelMap);
} // namespace ModelsLoader
