#pragma once

#include <string>
#include <memory>
#include <RenderSys/Buffer.h>

namespace RenderSys
{
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
        const glm::mat4 &GetMat4Global();
        void SetDirtyFlag();
        bool GetDirtyFlag() const;
        void SetInstance(std::shared_ptr<RenderSys::Buffer> &instanceBuffer, uint32_t instanceIndex);

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

        std::shared_ptr<RenderSys::Buffer> m_InstanceBuffer;
        uint32_t m_InstanceIndex;
    };


   
} // namespace RenderSys
