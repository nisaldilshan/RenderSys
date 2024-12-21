#pragma once

#include "GLTFScene.h"
#include "SceneNode.h"

namespace RenderSys
{
    
class Scene
{
public:
    Scene();
    ~Scene() = default;
    bool load(const std::filesystem::path &filePath, const std::string& textureFilename);
    void allocateMemory();
    void populate();
    void prepareNodeGraph();
    void printNodeGraph();
    void applyVertexSkinning();
    const std::vector<Model::Vertex>& getVertexBuffer() { return m_vertexBufferAltered.size() > 0 ? m_vertexBufferAltered : m_vertexBuffer; }
    const std::vector<uint32_t>& getIndexBuffer() { return m_indexBuffer; }
    
    std::vector<glm::tvec4<uint16_t>> m_jointVec;
    std::vector<int> m_nodeToJoint;
    std::vector<glm::vec4> m_weightVec;
    std::vector<glm::mat4> m_inverseBindMatrices;
    std::vector<glm::mat4> m_jointMatrices;
private:
    std::vector<Model::Vertex> m_vertexBuffer;
    std::vector<Model::Vertex> m_vertexBufferAltered;
    std::vector<uint32_t> m_indexBuffer;
    std::unique_ptr<GLTFScene> m_scene;
    std::shared_ptr<SceneNode> m_nodeGraph;
};

}

