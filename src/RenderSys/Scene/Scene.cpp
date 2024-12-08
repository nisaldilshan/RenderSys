#include "Scene.h"

namespace RenderSys
{

Scene::Scene()
{
}

bool Scene::load(const std::string &modelFilename, const std::string &textureFilename)
{
    return m_scene->load(modelFilename, textureFilename);
}

}
