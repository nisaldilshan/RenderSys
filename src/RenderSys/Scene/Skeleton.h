#pragma once
#include <vector>
#include <string>
#include <map>
#include <glm/ext.hpp>

namespace RenderSys
{
static constexpr int NO_PARENT = -1;
static constexpr int ROOT_JOINT = 0;

struct ShaderData
{
    std::vector<glm::mat4> m_FinalJointsMatrices;
};

struct Joint
{
    std::string m_Name;
    glm::mat4 m_InverseBindMatrix; // a.k.a undeformed inverse node matrix

    // deformed / animated
    // to be applied to the node matrix a.k.a bind matrix in the world coordinate system,
    // controlled by an animation or a single pose (they come out of gltf animation samplers)
    glm::vec3 m_DeformedNodeTranslation{0.0f};                            // T
    glm::quat m_DeformedNodeRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // R
    glm::vec3 m_DeformedNodeScale{1.0f};   

    glm::mat4 GetDeformedBindMatrix()
    {
        // apply scale, rotation, and translation IN THAT ORDER (read from right to the left)
        // to the original undefomed bind matrix
        // dynamically called once per frame
        return glm::translate(glm::mat4(1.0f), m_DeformedNodeTranslation) * // T
                glm::mat4(m_DeformedNodeRotation) *                          // R
                glm::scale(glm::mat4(1.0f), m_DeformedNodeScale);            // S
    }

    // parents and children for the tree hierachy
    int m_ParentJoint;
    std::vector<int> m_Children;
};

struct Skeleton
{
    void Traverse();
    void Traverse(Joint const &joint, uint32_t indent = 0);
    void Update();
    void UpdateJoint(int16_t jointIndex); // signed because -1 maybe used for invalid joint

    bool m_IsAnimated = true;
    std::string m_Name;
    std::vector<Joint> m_Joints;
    std::map<int, int> m_GlobalNodeToJointIndex;
    ShaderData m_ShaderData;
};

} // namespace RenderSys
