#include "Scene.h"

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
}

void Scene::prepareNodeGraph()
{
    m_nodeGraph = m_scene->getNodeGraph();
}

}
