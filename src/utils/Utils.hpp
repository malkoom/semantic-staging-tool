#pragma once

#include <nlohmann/json.hpp>

#include <string>

bool SaveJsonToFile(const std::string& filepath, const nlohmann::json& data,
                    int indent = 4);
