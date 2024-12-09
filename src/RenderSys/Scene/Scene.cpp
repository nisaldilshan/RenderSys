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

}
