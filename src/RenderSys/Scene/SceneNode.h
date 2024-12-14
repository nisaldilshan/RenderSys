#pragma once
#include <vector>
#include <memory>
#include <string>
#include <glm/ext.hpp>

class SceneNode {
  public:
    int getNodeNum();
    void setNodeName(std::string name);
    void setScale(glm::vec3 scale);
    void setTranslation(glm::vec3 translation);
    void setRotation(glm::quat rotation);

    void calculateLocalTRSMatrix();
    void calculateNodeMatrix(glm::mat4 parentNodeMatrix);
    glm::mat4 getNodeMatrix();

  private:
    void printNodes(std::shared_ptr<SceneNode> startNode, int indent);

    int mNodeNum = 0;
    std::string mNodeName;
    std::vector<std::shared_ptr<SceneNode>> mChildNodes{};
    glm::vec3 mScale = glm::vec3(1.0f);
    glm::vec3 mTranslation = glm::vec3(0.0f);
    glm::quat mRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::mat4 mLocalTRSMatrix = glm::mat4(1.0f);
    glm::mat4 mNodeMatrix = glm::mat4(1.0f);
};