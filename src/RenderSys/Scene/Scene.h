#pragma once
#include "GLTFScene.h"

namespace RenderSys
{
    
class Scene
{
public:
    Scene();
    ~Scene() = default;
    bool load(const std::string& modelFilename, const std::string& textureFilename);
private:
    std::unique_ptr<GLTFScene> m_scene;
};

}

