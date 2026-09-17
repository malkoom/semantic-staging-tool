#pragma once

#include "raylib.h"

class PropModel {
  public:
    Vector3 Position{0, 0, 0};
    float Scale{1.f};
    Color ModelColor{RAYWHITE};

  public:
    PropModel(const char* filePath) { m_Model = LoadModel(filePath); };
    ~PropModel() { UnloadModel(m_Model); };

    // Constructores copia no permitidos
    PropModel(const PropModel& model) = delete;
    PropModel& operator=(const PropModel& model) = delete;

    PropModel(PropModel&& other) noexcept {
        m_Model = other.m_Model;
        other.m_Model = {0};
    }

    PropModel& operator=(PropModel&& other) noexcept {
        if (this != &other) {
            if (m_Model.meshes != nullptr) {
                UnloadModel(m_Model);
            }
            m_Model = other.m_Model;
            other.m_Model = {0};
        }
        return *this;
    }

    // Getter
    const Model& GetModel() const { return m_Model; }

    void Draw() { DrawModel(m_Model, Position, Scale, ModelColor); };

  private:
    Model m_Model{};
};
