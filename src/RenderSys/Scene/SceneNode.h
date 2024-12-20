#pragma once
#include <vector>
#include <memory>
#include <string>
#include <glm/ext.hpp>

class SceneNode {
  public:
    SceneNode(int nodeNum);
    ~SceneNode() = default;

    int getNodeNum();
    void setNodeName(std::string name);
    void setScale(glm::vec3 scale);
    void setTranslation(glm::vec3 translation);
    void setRotation(glm::quat rotation);

    void calculateLocalTRSMatrix();
    void calculateNodeMatrix(glm::mat4 parentNodeMatrix);
    void calculateJointMatrices(const std::vector<glm::mat4>& inverseBindMatrices, const std::vector<int>& nodeToJoint, std::vector<glm::mat4>& jointMatrices);
    glm::mat4 getNodeMatrix();
    void printHierarchy(int indent);

    std::vector<std::shared_ptr<SceneNode>> m_childNodes{};
  private:

    int m_nodeNum = 0;
    std::string m_nodeName;
    glm::vec3 m_scale = glm::vec3(1.0f);
    glm::vec3 m_translation = glm::vec3(0.0f);
    glm::quat m_rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::mat4 mLocalTRSMatrix = glm::mat4(1.0f);
    glm::mat4 m_nodeMatrix = glm::mat4(1.0f);
};