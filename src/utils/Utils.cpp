#include <fstream>
#include <iostream>

#include "Utils.hpp"

bool SaveJsonToFile(const std::string& filepath, const nlohmann::json& data,
                    int indent) {
    std::ofstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "[Error] No se pudo abrir el archivo para escritura: "
                  << filepath << "\n";
        return false;
    }

    // El parámetro indent (por defecto 4) formatea el archivo con sangría
    // legible. Si pasas -1, lo guarda minificado en una sola línea.
    file << std::setw(indent) << data << std::endl;

    return file.good();
}
