#include "GLTFScene.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

namespace RenderSys
{

GLTFScene::GLTFScene()
    : m_model(std::make_shared<tinygltf::Model>())
{
}

GLTFScene::~GLTFScene()
{
}

bool GLTFScene::load(const std::filesystem::path &filePath, const std::string &textureFilename)
{
    tinygltf::TinyGLTF gltfLoader;
    std::string loaderErrors;
    std::string loaderWarnings;

    bool result = false;
    if (filePath.extension() == ".glb") 
    {
        result = gltfLoader.LoadBinaryFromFile(m_model.get(), &loaderErrors, &loaderWarnings, filePath.string());
    }
    else 
    {
        result = gltfLoader.LoadASCIIFromFile(m_model.get(), &loaderErrors, &loaderWarnings, filePath.string());
    }

    if (!loaderWarnings.empty()) {
        //Logger::log(1, "%s: warnings while loading glTF model:\n%s\n", __FUNCTION__, loaderWarnings.c_str());
    }

    if (!loaderErrors.empty()) {
        //Logger::log(1, "%s: errors while loading glTF model:\n%s\n", __FUNCTION__, loaderErrors.c_str());
    }

    if (!result) {
        //Logger::log(1, "%s error: could not load file '%s'\n", __FUNCTION__, modelFilename.c_str());
        return false;
    }

    return true;
}

}
