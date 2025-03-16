#include "Scene.h"
#include <iostream>

namespace RenderSys
{

Scene::Scene()
    : m_scene(std::make_unique<GLTFScene>())
{}

bool Scene::load(const std::filesystem::path &filePath, const std::string &textureFilename)
{
    return m_scene->load(filePath, textureFilename);
}

void loadVertexIndexData(std::shared_ptr<RenderSys::Model::ModelNode> node, uint32_t& vertexCount, uint32_t& indexCount, std::vector<Model::Vertex>& m_vertexBuffer, std::vector<uint32_t>& m_indexBuffer)
{
    for (size_t i = vertexCount; i < node->m_data.vertices.size() + vertexCount; i++)
    {
        m_vertexBuffer[i] = node->m_data.vertices[i-vertexCount];
    }

    for (size_t i = indexCount; i < node->m_data.indices.size() + indexCount; i++)
    {
        auto indexVal = node->m_data.indices[i-indexCount] + vertexCount;
        assert(indexVal < m_vertexBuffer.size());
        m_indexBuffer[i] = indexVal;
    }
                
    vertexCount += node->m_data.vertices.size();
    indexCount += node->m_data.indices.size();

    for (auto& childNode : node->m_childNodes)
    {
        loadVertexIndexData(childNode, vertexCount, indexCount, m_vertexBuffer, m_indexBuffer);
    }
}

void Scene::populate()
{
    // traverse through GLTF Scene nodes and prepare graph
    m_scene->getNodeGraphs(m_rootNodes);

    m_scene->computeProps();
    m_vertexBuffer.resize(m_scene->getVertexCount());
    m_indexBuffer.resize(m_scene->getIndexCount());

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    for (auto &rootNode : m_rootNodes)
    {
        loadVertexIndexData(rootNode, vertexCount, indexCount, m_vertexBuffer, m_indexBuffer);
    }
    
    assert(m_vertexBuffer.size() == vertexCount);
    assert(m_indexBuffer.size() == indexCount);

    m_scene->loadTextures(m_textures);
    m_scene->loadTextureSamplers(m_textureSamplers);
    m_scene->loadMaterials(m_materials);

    m_scene->loadJointData(m_jointVec, m_nodeToJoint, m_weightVec);
    m_scene->loadInverseBindMatrices(m_inverseBindMatrices);
    // traverse through the graph again and compute JointMatrices
    if(m_inverseBindMatrices.size() > 0 && m_nodeToJoint.size() > 0)
    {
        m_jointMatrices.resize(m_inverseBindMatrices.size());
        for (auto &rootNode : m_rootNodes)
            rootNode->calculateJointMatrices(m_inverseBindMatrices, m_nodeToJoint, m_jointMatrices);
    }
}

void Scene::printNodeGraph()
{
    std::cout << "---- Scene begin ----\n";
    for (auto &rootNode : m_rootNodes)
    {
        rootNode->printHierarchy(0);
    }
    std::cout << " -- Scene end --" << std::endl;
}

void Scene::applyVertexSkinning()
{
    assert(m_vertexBuffer.size() > 0);
    m_vertexBufferAltered = m_vertexBuffer;

    assert(m_jointVec.size() > 0);
    assert(m_weightVec.size() > 0);

    if (m_jointMatrices.size() == 0)
    {
        return;
    }

    for (int i = 0; i < m_jointVec.size(); ++i) 
    {
        glm::ivec4 jointIndex = glm::make_vec4(m_jointVec.at(i));
        glm::vec4 weightIndex = glm::make_vec4(m_weightVec.at(i));
        glm::mat4 skinMat =
            weightIndex.x * m_jointMatrices.at(jointIndex.x) +
            weightIndex.y * m_jointMatrices.at(jointIndex.y) +
            weightIndex.z * m_jointMatrices.at(jointIndex.z) +
            weightIndex.w * m_jointMatrices.at(jointIndex.w);
        m_vertexBufferAltered.at(i).pos = skinMat * glm::vec4(m_vertexBufferAltered.at(i).pos, 1.0f);
    }
}

}
