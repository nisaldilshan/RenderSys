#include "Scene.h"
#include <iostream>

namespace RenderSys
{

Scene::Scene()
    : m_scene(std::make_unique<GLTFScene>())
{
}

bool Scene::load(const std::filesystem::path &filePath, const std::string &textureFilename)
{
    return m_scene->load(filePath, textureFilename);
}

void Scene::allocateMemory()
{
    m_scene->computeProps();
    m_vertexBuffer.resize(m_scene->getVertexCount());
    m_indexBuffer.resize(m_scene->getIndexCount());
}

void Scene::populate()
{
    m_scene->loadVertexAttributes(m_vertexBuffer);
    m_scene->loadIndices(m_indexBuffer);
    m_scene->loadJointData(m_jointVec, m_nodeToJoint, m_weightVec);
    m_scene->loadInverseBindMatrices(m_inverseBindMatrices);
}

void Scene::prepareNodeGraph()
{
    // traverse through GLTF Scene nodes and prepare graph
    m_nodeGraph = m_scene->getNodeGraph(); 

    // traverse through the prepared graph and update node matrices
    m_nodeGraph->calculateNodeMatrix(glm::mat4(1.0f)); 

    // traverse through the graph again and compute JointMatrices
    assert(m_inverseBindMatrices.size() > 0);
    assert(m_nodeToJoint.size() > 0);
    m_jointMatrices.resize(m_inverseBindMatrices.size());
    m_nodeGraph->calculateJointMatrices(m_inverseBindMatrices, m_nodeToJoint, m_jointMatrices);
}

void Scene::printNodeGraph()
{
    std::cout << "---- tree ----\n";
    m_nodeGraph->printHierarchy(1);
    std::cout << " -- end tree --" << std::endl;
}

void Scene::applyVertexSkinning()
{
    assert(m_vertexBuffer.size() > 0);
    m_vertexBufferAltered = m_vertexBuffer;

    assert(m_jointVec.size() > 0);
    assert(m_weightVec.size() > 0);

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
