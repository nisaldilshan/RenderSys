#pragma once
#include <vector>
#include <memory>
#include <string>
#include <glm/ext.hpp>

#include "../Buffer.h"

namespace RenderSys
{

namespace Model
{
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv0;
        glm::vec2 uv1;
        glm::uvec4 joint0;
        glm::vec4 weight0;
        glm::vec4 color;
        glm::vec3 tangent;
    };
}

class SceneNode {
  public:
    SceneNode(int nodeNum);
    ~SceneNode() = default;

	SceneNode(const SceneNode&) = delete;
	SceneNode &operator=(const SceneNode&) = delete;
	SceneNode(SceneNode&&) = delete;
	SceneNode &operator=(SceneNode&&) = delete;

    int getNodeNum();
    void setNodeName(std::string name);
    void setScale(glm::vec3 scale);
    void setTranslation(glm::vec3 translation);
    void setRotation(glm::quat rotation);
	void setMesh(RenderSys::Mesh mesh);

    void calculateLocalTRSMatrix();
    void calculateNodeMatrix(glm::mat4 parentNodeMatrix);
    void calculateJointMatrices(const std::vector<glm::mat4>& inverseBindMatrices, const std::vector<int>& nodeToJoint, std::vector<glm::mat4>& jointMatrices);
    glm::mat4 getNodeMatrix();
    Mesh getMesh();
    void printHierarchy(int indent);

    std::vector<std::shared_ptr<SceneNode>> m_childNodes{};
	std::vector<Model::Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
  private:

    int m_nodeNum = 0;
    std::string m_nodeName;
    Mesh m_mesh;
    int m_materialIndex = 0;

    glm::vec3 m_scale = glm::vec3(1.0f);
    glm::vec3 m_translation = glm::vec3(0.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::mat4 mLocalTRSMatrix = glm::mat4(1.0f);
    glm::mat4 m_nodeMatrix = glm::mat4(1.0f);
};

} // namespace RenderSys