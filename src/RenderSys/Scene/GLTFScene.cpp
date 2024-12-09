#include "GLTFScene.h"

#include <iostream>

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

void GLTFScene::computeProps()
{
    const tinygltf::Scene& scene = m_model->scenes[m_model->defaultScene > -1 ? m_model->defaultScene : 0];
    for (size_t i = 0; i < scene.nodes.size(); i++) {
        getNodeProps(m_model->nodes[scene.nodes[i]], *m_model, m_vertexCount, m_indexCount);
    }

    std::cout << "GLTFModel: [VertexCount=" << m_vertexCount << "], [IndexCount=" << m_indexCount << "]" << std::endl;
}

size_t GLTFScene::getVertexCount() const
{
    return m_vertexCount;
}

size_t GLTFScene::getIndexCount() const
{
    return m_indexCount;
}

void GLTFScene::getNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount)
{
    if (node.children.size() > 0) {
        for (size_t i = 0; i < node.children.size(); i++) {
            getNodeProps(model.nodes[node.children[i]], model, vertexCount, indexCount);
        }
    }
    if (node.mesh > -1) {
        const tinygltf::Mesh mesh = model.meshes[node.mesh];
        for (size_t i = 0; i < mesh.primitives.size(); i++) {
            auto primitive = mesh.primitives[i];
            vertexCount += model.accessors[primitive.attributes.find("POSITION")->second].count;
            if (primitive.indices > -1) {
                indexCount += model.accessors[primitive.indices].count;
            }
        }
    }
}

}
