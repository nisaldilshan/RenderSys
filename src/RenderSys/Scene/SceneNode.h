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
    glm::mat4 getNodeMatrix();
    void printHierarchy(int indent);

    std::vector<std::shared_ptr<SceneNode>> m_childNodes{};
  private:

    int m_nodeNum = 0;
    std::string m_nodeName;
    glm::vec3 mScale = glm::vec3(1.0f);
    glm::vec3 mTranslation = glm::vec3(0.0f);
    glm::quat mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::mat4 mLocalTRSMatrix = glm::mat4(1.0f);
    glm::mat4 mNodeMatrix = glm::mat4(1.0f);
};