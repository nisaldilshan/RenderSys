#pragma once

#include <string>
#include <memory>
#include <filesystem>

#include "SceneNode.h"

namespace tinygltf
{
class Model;
class Node;
}

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
    };
}

class GLTFScene
{
public:
    GLTFScene();
    ~GLTFScene();
    bool load(const std::filesystem::path &filePath, const std::string& textureFilename);
    void computeProps();
    size_t getVertexCount() const;
    size_t getIndexCount() const;
    void loadVertexAttributes(std::vector<Model::Vertex>& vertexBuffer);
    void loadIndices(std::vector<uint32_t>& indexBuffer);
    void loadJointData(std::vector<glm::tvec4<uint16_t>>& jointVec, std::vector<int>& nodeToJoint, std::vector<glm::vec4>& weightVec);
    void loadInverseBindMatrices(std::vector<glm::mat4>& inverseBindMatrices);
    std::shared_ptr<SceneNode> getNodeGraph();
private:
    static void getNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount);
    std::shared_ptr<SceneNode> traverse(const tinygltf::Node &node, uint32_t nodeIndex);

    std::shared_ptr<tinygltf::Model> m_model;
    size_t m_vertexCount = 0;
    size_t m_indexCount = 0;
};

}

