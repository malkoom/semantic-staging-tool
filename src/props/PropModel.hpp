#pragma once

#include "raylib.h"

class PropModel {
  public:
    Vector3 Position{0, 0, 0};
    float Scale{1.f};
    Color ModelColor{RAYWHITE};

  public:
    PropModel(const char* filePath) {
        m_Model = LoadModel(filePath);
        m_LocalBounds = GetModelBoundingBox(m_Model);

    };
    ~PropModel() { UnloadModel(m_Model); };

    // Constructores copia no permitidos
    PropModel(const PropModel& model) = delete;
    PropModel& operator=(const PropModel& model) = delete;

    PropModel(PropModel&& other) noexcept {
        m_Model = other.m_Model;
        m_LocalBounds = other.m_LocalBounds;
        m_IsStatic = other.m_IsStatic;
        other.m_Model = {0};
    }

    PropModel& operator=(PropModel&& other) noexcept {
        if (this != &other) {
            if (m_Model.meshes != nullptr) {
                UnloadModel(m_Model);
            }
            m_Model = other.m_Model;
            m_LocalBounds = other.m_LocalBounds;
            m_IsStatic = other.m_IsStatic;
            other.m_Model = {0};
        }
        return *this;
    }

    // Getters
    const Model& GetModel() const { return m_Model; }
    const BoundingBox GetBounds() const { return m_LocalBounds; }
    const bool IsStatic() const { return m_IsStatic; }

    void SetShader(Shader shader) {
        for (int i = 0; i < m_Model.materialCount; ++i) {
            m_Model.materials[i].shader = shader;
        }
    }

    void Draw() { DrawModel(m_Model, Position, Scale, ModelColor); };

  private:
    Model m_Model{};
    BoundingBox m_LocalBounds{};
    float m_IsStatic{false};
};
