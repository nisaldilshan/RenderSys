#pragma once

#include "GLTFScene.h"
#include "SceneNode.h"
#include <RenderSys/Buffer.h>

namespace RenderSys
{
    
class Scene
{
public:
    Scene();
    ~Scene() = default;
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;
    
    bool load(const std::filesystem::path &filePath, const std::string& textureFilename);
    void populate();
    void printNodeGraph();
    void applyVertexSkinning();
    const std::vector<Model::Vertex>& getVertexBuffer() const { return m_vertexBufferAltered.size() > 0 ? m_vertexBufferAltered : m_vertexBuffer; }
    const RenderSys::VertexBuffer getVertexBufferForRenderer() const;
    const std::vector<uint32_t>& getIndexBuffer() const { return m_indexBuffer; }
    const std::vector<std::shared_ptr<Texture>>& getTextures() const { return m_scene->GetTextures(); }
    const std::vector<std::shared_ptr<RenderSys::Material>>& getMaterials() const { return m_scene->GetMaterials(); }
    const std::vector<std::shared_ptr<Model::ModelNode>>& getRootNodes() const { return m_rootNodes; }
    
    std::vector<glm::tvec4<uint16_t>> m_jointVec;
    std::vector<int> m_nodeToJoint;
    std::vector<glm::vec4> m_weightVec;
    std::vector<glm::mat4> m_inverseBindMatrices;
    std::vector<glm::mat4> m_jointMatrices;
private:
    std::vector<Model::Vertex> m_vertexBuffer;
    std::vector<Model::Vertex> m_vertexBufferAltered;
    std::vector<uint32_t> m_indexBuffer;
    
    std::vector<std::shared_ptr<Model::ModelNode>> m_rootNodes;
    std::unique_ptr<GLTFScene> m_scene;
};

}

