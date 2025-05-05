#include "Model.h"
#include <iostream>

namespace RenderSys
{

Model::Model(entt::registry& registry)
    : m_scene(std::make_unique<GLTFModel>(registry))
{}

bool Model::load(const std::filesystem::path &filePath, const std::string &textureFilename)
{
    return m_scene->load(filePath, textureFilename);
}

void Model::populate()
{
    m_scene->loadTextures();
    m_scene->loadMaterials();

    // traverse through GLTF Scene nodes and prepare graph
    m_scene->getNodeGraphs(m_rootNodes);
    m_scene->computeProps();

    m_scene->loadJointData(m_jointVec, m_weightVec);
    m_scene->loadInverseBindMatrices();
    m_scene->loadjointMatrices(m_rootNodes);
}

void Model::printNodeGraph()
{
    std::cout << "---- Scene begin ----\n";
    for (auto &rootNode : m_rootNodes)
    {
        rootNode->printHierarchy(0);
    }
    std::cout << " -- Scene end --" << std::endl;
}

void Model::applyVertexSkinning(RenderSys::VertexBuffer& vertexBuffer)
{
    if (m_jointVec.size() == 0)
    {
        return;
    }
    if (m_weightVec.size() == 0)
    {
        return;
    }
    auto jointMatrices = m_scene->GetJointMatrices();
    if (jointMatrices.size() == 0)
    {
        return;
    }

    for (int i = 0; i < m_jointVec.size(); ++i) 
    {
        glm::ivec4 jointIndex = glm::make_vec4(m_jointVec.at(i));
        glm::vec4 weightIndex = glm::make_vec4(m_weightVec.at(i));
        glm::mat4 skinMat =
            weightIndex.x * jointMatrices.at(jointIndex.x) +
            weightIndex.y * jointMatrices.at(jointIndex.y) +
            weightIndex.z * jointMatrices.at(jointIndex.z) +
            weightIndex.w * jointMatrices.at(jointIndex.w);
        vertexBuffer.vertices.at(i).position = skinMat * glm::vec4(vertexBuffer.vertices.at(i).position, 1.0f);
    }
}

} // namespace RenderSys
