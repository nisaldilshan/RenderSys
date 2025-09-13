#pragma once

#include <glm/ext.hpp>
#include <string>
#include <memory>
#include <functional> 

namespace RenderSys
{
    class InstanceBuffer;
    class TransformComponent
    {
    public:
        TransformComponent();
        ~TransformComponent() = default;
        TransformComponent(const TransformComponent&) = delete;
        TransformComponent &operator=(const TransformComponent&) = delete;
        TransformComponent(TransformComponent&&) = delete;
        TransformComponent &operator=(TransformComponent&&) = delete;

        void SetScale(const glm::vec3 &scale);
        void SetRotation(const glm::vec3& rotation);
        void SetRotation(const glm::quat &quaternion);
        void SetTranslation(const glm::vec3 &translation);

        const glm::vec3 &GetScale() const { return m_Scale; }
        const glm::vec3 &GetRotation() const { return m_Rotation; }
        const glm::vec3 &GetTranslation() const { return m_Translation; }

        void SetMat4Local(const glm::mat4 &mat4);
        void SetMat4Global(const glm::mat4 &parent);
        void SetMat4Global();

        const glm::mat4 &GetMat4Local();
        const glm::mat4 &GetMat4Global() const;
        const glm::mat4 &GetParent() const;
        void SetDirtyFlag();
        bool GetDirtyFlag() const;
        void SetInstance(std::shared_ptr<RenderSys::InstanceBuffer> instanceBuffer, uint32_t instanceIndex);
        void SetChangeNotifyCallback(std::function<void(const glm::vec3&, const glm::vec3&)> callback)
        {
            m_changeNotifyCallback = std::move(callback);
        }

    private:
        void RecalculateMatrices();

    private:
        bool m_Dirty{true};

        // local
        glm::vec3 m_Scale = glm::vec3{1.0f};
        glm::vec3 m_Rotation{0.0f};
        glm::vec3 m_Translation{0.0f};
        glm::mat4 m_Mat4Local = glm::mat4(1.0f);

        // global
        glm::mat4 m_Mat4Global = glm::mat4(1.0f);
        glm::mat4 m_Parent = glm::mat4(1.0f);

        std::shared_ptr<RenderSys::InstanceBuffer> m_InstanceBuffer;
        uint32_t m_InstanceIndex;

        std::function<void(const glm::vec3&, const glm::vec3&)> m_changeNotifyCallback;
    };


   
} // namespace RenderSys
