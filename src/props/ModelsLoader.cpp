#include <filesystem>
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>

#include "ModelsLoader.hpp"

#include "props/PropModel.hpp"

namespace fs = std::filesystem;
namespace ModelsLoader {

void LoadModelFilesInDirectory(
    const char* dirPath,
    std::unordered_map<std::string, PropModel>& modelsMap) {
    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (entry.is_directory()) {
            LoadModelFilesInDirectory(entry.path().c_str(), modelsMap);
        }
        if (entry.path().extension() == ".obj" ||
            entry.path().extension() == ".glb") {
            modelsMap.try_emplace(
                entry.path().stem().string(), // Clave (std::string)
                entry.path()
                    .string()
                    .c_str() // Argumento para el constructor de PropModel
            );
            std::cout << "Añadido el elemento " << entry.path().stem()
                      << " en el mapa de modelos" << std::endl;
        }
    }
}

} // namespace ModelsLoader
