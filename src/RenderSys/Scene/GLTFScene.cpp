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

void GLTFScene::loadVertexAttributes(std::vector<Model::Vertex>& vertexBuffer)
{
    assert(m_model->meshes.size() == 1);
    const tinygltf::Mesh mesh = m_model->meshes[0];

    for (const auto &primitive : mesh.primitives)
    {
        assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

        const tinygltf::Accessor &posAccessor = m_model->accessors[primitive.attributes.find("POSITION")->second];
        const tinygltf::BufferView &posView = m_model->bufferViews[posAccessor.bufferView];
        const auto* bufferPos = reinterpret_cast<const float *>(&(m_model->buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
        const auto vertexCount = static_cast<uint32_t>(posAccessor.count);
        const auto posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
        assert(vertexBuffer.size() == vertexCount);

        for (size_t v = 0; v < vertexCount; v++) 
        {
            Model::Vertex& vert = vertexBuffer[v];
			vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
        }

        if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
        {
            std::cout << "GLTFScene::loadVertexAttributes found TEXCOORD_0" << std::endl;
            const tinygltf::Accessor &posAccessor = m_model->accessors[primitive.attributes.find("TEXCOORD_0")->second];
            const tinygltf::BufferView &posView = m_model->bufferViews[posAccessor.bufferView];
            const auto* bufferPos = reinterpret_cast<const float *>(&(m_model->buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
            const auto vertexCount = static_cast<uint32_t>(posAccessor.count);
            const auto byteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
            assert(vertexBuffer.size() == vertexCount);

            for (size_t v = 0; v < vertexCount; v++) 
            {
                Model::Vertex& vert = vertexBuffer[v];
                vert.uv0 = glm::vec4(glm::make_vec3(&bufferPos[v * byteStride]), 1.0f);
            }
        }
    }
    
}

void GLTFScene::loadIndices(std::vector<uint32_t>& indexBuffer)
{
    assert(m_model->meshes.size() == 1);
    const tinygltf::Mesh mesh = m_model->meshes[0];

    assert(mesh.primitives.size() == 1);
    for (const auto &primitive : mesh.primitives)
    {
        const bool hasIndices = primitive.indices > -1;
        if (hasIndices)
        {
            const tinygltf::Accessor &accessor = m_model->accessors[primitive.indices];
            const tinygltf::BufferView &bufferView = m_model->bufferViews[accessor.bufferView];
            const tinygltf::Buffer &buffer = m_model->buffers[bufferView.buffer];

            const auto indexCount = static_cast<uint32_t>(accessor.count);
            assert(indexBuffer.size() == indexCount);

            const void *dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

            switch (accessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                const uint32_t *buf = static_cast<const uint32_t*>(dataPtr);
                for (size_t index = 0; index < accessor.count; index++) {
                    indexBuffer[index] = buf[index];
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                const uint16_t *buf = static_cast<const uint16_t*>(dataPtr);
                for (size_t index = 0; index < accessor.count; index++) {
                    indexBuffer[index] = buf[index];
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                const uint8_t *buf = static_cast<const uint8_t*>(dataPtr);
                for (size_t index = 0; index < accessor.count; index++) {
                    indexBuffer[index] = buf[index];
                }
                break;
            }
            default:
                std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                return;
            }
        }
    }
}

void GLTFScene::loadJointData(std::vector<glm::tvec4<uint16_t>> &jointVec, std::vector<int>& nodeToJoint, std::vector<glm::vec4> &weightVec)
{
    // Joints
    {
        int jointsAccessor = m_model->meshes.at(0).primitives.at(0).attributes.at("JOINTS_0");
        const tinygltf::Accessor &accessor = m_model->accessors.at(jointsAccessor);
        const tinygltf::BufferView &bufferView = m_model->bufferViews.at(accessor.bufferView);
        const tinygltf::Buffer &buffer = m_model->buffers.at(bufferView.buffer);

        assert(accessor.count > 0);
        jointVec.resize(accessor.count);
        std::memcpy(jointVec.data(), &buffer.data.at(0) + bufferView.byteOffset, bufferView.byteLength);
    }

    nodeToJoint.resize(m_model->nodes.size());
    const tinygltf::Skin &skin = m_model->skins.at(0);
    for (int i = 0; i < skin.joints.size(); ++i) {
        int destinationNode = skin.joints.at(i);
        nodeToJoint.at(destinationNode) = i;
    }

    // Weights
    {
        int weightAccessor = m_model->meshes.at(0).primitives.at(0).attributes.at("WEIGHTS_0");
        const tinygltf::Accessor &accessor = m_model->accessors.at(weightAccessor);
        const tinygltf::BufferView &bufferView = m_model->bufferViews.at(accessor.bufferView);
        const tinygltf::Buffer &buffer = m_model->buffers.at(bufferView.buffer);

        assert(accessor.count > 0);
        weightVec.resize(accessor.count);
        std::memcpy(weightVec.data(), &buffer.data.at(0) + bufferView.byteOffset, bufferView.byteLength);
    }
}

void GLTFScene::loadInverseBindMatrices(std::vector<glm::mat4>& inverseBindMatrices)
{

}

std::shared_ptr<SceneNode> GLTFScene::getNodeGraph()
{
    assert(m_model);
    const int nodeCount = m_model->nodes.size();
    const int rootNodeNum = m_model->scenes.at(0).nodes.at(0);
    assert(nodeCount > 0);
    std::cout << "model has " << nodeCount << " nodes, root node is " << rootNodeNum << std::endl;
    const tinygltf::Node &node = m_model->nodes.at(rootNodeNum);
    auto rootNode = traverse(node, rootNodeNum);
    return rootNode;
}

std::shared_ptr<SceneNode> GLTFScene::traverse(const tinygltf::Node &node, uint32_t nodeIndex)
{
    auto sceneNode = std::make_shared<SceneNode>(nodeIndex);
    sceneNode->setNodeName(node.name);
    if (node.translation.size()) {
        sceneNode->setTranslation(glm::make_vec3(node.translation.data()));
    }
    if (node.rotation.size()) {
        sceneNode->setRotation(glm::make_quat(node.rotation.data()));
    }
    if (node.scale.size()) {
        sceneNode->setScale(glm::make_vec3(node.scale.data()));
    }

    sceneNode->calculateLocalTRSMatrix();

    if (node.children.size() > 0) 
    {
        for (size_t i = 0; i < node.children.size(); i++) 
        {
            sceneNode->m_childNodes.push_back(traverse(m_model->nodes[node.children[i]], node.children[i]));
        }
    }

    return sceneNode;
}

}
