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

        float width = m_LocalBounds.max.x - m_LocalBounds.min.x;
        float depth = m_LocalBounds.max.z - m_LocalBounds.min.z;

        // Radio medio proyectado en XZ
        m_BoundingRadius = 0.25f * (width + depth);
    };
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

    // Getters
    const Model& GetModel() const { return m_Model; }
    const float GetRadius() const { return m_BoundingRadius; }
    const BoundingBox GetBounds() const { return m_LocalBounds; }
    const bool IsStatic() const { return m_IsStatic; }

    void Draw() { DrawModel(m_Model, Position, Scale, ModelColor); };

  private:
    Model m_Model{};
    BoundingBox m_LocalBounds{};
    float m_BoundingRadius{0.5f};
    float m_IsStatic{false};
};
