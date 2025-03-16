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
    m_sceneFilePath = filePath;

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

RenderSys::Primitive GLTFScene::loadPrimitive(const tinygltf::Primitive &primitive, Model::ModelData &modelData, const uint32_t indexCount)
{
    assert(primitive.attributes.find("POSITION") != primitive.attributes.end()); // position is mandatory
    const tinygltf::Accessor &posAccessor = m_model->accessors[primitive.attributes.find("POSITION")->second];
    const auto vertexCount = static_cast<uint32_t>(posAccessor.count);
    assert(vertexCount > 0);
    const uint32_t vertexStart = modelData.vertices.size();
    modelData.vertices.resize(vertexStart + vertexCount);

    const tinygltf::BufferView &positionBufferView = m_model->bufferViews[posAccessor.bufferView];
    const auto* positionBuffer = reinterpret_cast<const float *>(&(m_model->buffers[positionBufferView.buffer].data[posAccessor.byteOffset + positionBufferView.byteOffset]));
    const auto posByteStride = posAccessor.ByteStride(positionBufferView) ? 
                                (posAccessor.ByteStride(positionBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

    for (size_t v = 0; v < vertexCount; v++) 
    {
        Model::Vertex& vert = modelData.vertices[v + vertexStart];
        vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * posByteStride]), 1.0f);
    }

    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
    {
        const tinygltf::Accessor &accessor = m_model->accessors[primitive.attributes.find("TEXCOORD_0")->second];
        const tinygltf::BufferView &bufferView = m_model->bufferViews[accessor.bufferView];
        const auto* bufferPos = reinterpret_cast<const float *>(&(m_model->buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
        const auto byteStride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

        for (size_t v = 0; v < vertexCount; v++) 
        {
            Model::Vertex& vert = modelData.vertices[v + vertexStart];
            vert.uv0 = glm::vec4(glm::make_vec3(&bufferPos[v * byteStride]), 1.0f);
        }
    }

    if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
    {
        const tinygltf::Accessor &accessor = m_model->accessors[primitive.attributes.find("NORMAL")->second];
        const tinygltf::BufferView &bufferView = m_model->bufferViews[accessor.bufferView];
        const auto* bufferPos = reinterpret_cast<const float *>(&(m_model->buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
        const auto byteStride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

        for (size_t v = 0; v < vertexCount; v++) 
        {
            Model::Vertex& vert = modelData.vertices[v + vertexStart];
            vert.normal = glm::vec4(glm::make_vec3(&bufferPos[v * byteStride]), 1.0f);
        }
    }

    if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
    {
        const tinygltf::Accessor &accessor = m_model->accessors[primitive.attributes.find("TANGENT")->second];
        const tinygltf::BufferView &bufferView = m_model->bufferViews[accessor.bufferView];
        const auto* bufferPos = reinterpret_cast<const float *>(&(m_model->buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
        const auto byteStride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

        for (size_t v = 0; v < vertexCount; v++) 
        {
            Model::Vertex& vert = modelData.vertices[v + vertexStart];
            vert.tangent = glm::vec4(glm::make_vec3(&bufferPos[v * byteStride]), 1.0f);
        }
    }

    RenderSys::Primitive prim;
    prim.vertexCount = vertexCount;
    prim.hasIndices = primitive.indices > -1;
    if (prim.hasIndices)
    {
        const uint32_t indexStart = modelData.indices.size();
        prim.firstIndex = indexCount + indexStart;
        
        const tinygltf::Accessor &accessor = m_model->accessors[primitive.indices];
        const tinygltf::BufferView &bufferView = m_model->bufferViews[accessor.bufferView];
        const tinygltf::Buffer &buffer = m_model->buffers[bufferView.buffer];

        const auto currentIndexCount = static_cast<uint32_t>(accessor.count);
        assert(currentIndexCount > 0);
        prim.indexCount = currentIndexCount;
        modelData.indices.resize(indexStart + currentIndexCount);

        const void *dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

        switch (accessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
            const uint32_t *buf = static_cast<const uint32_t*>(dataPtr);
            for (size_t index = 0; index < accessor.count; index++) {
                modelData.indices[index + indexStart] = buf[index] + vertexStart;
            }
            break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
            const uint16_t *buf = static_cast<const uint16_t*>(dataPtr);
            for (size_t index = 0; index < accessor.count; index++) {
                modelData.indices[index + indexStart] = buf[index] + vertexStart;
            }
            break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
            const uint8_t *buf = static_cast<const uint8_t*>(dataPtr);
            for (size_t index = 0; index < accessor.count; index++) {
                modelData.indices[index + indexStart] = buf[index] + vertexStart;
            }
            break;
        }
        default:
            std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
            assert(false);
        }
    }

    prim.materialIndex = primitive.material;
    return prim;
}

RenderSys::Mesh GLTFScene::loadMesh(const tinygltf::Mesh& gltfMesh, Model::ModelData &modelData, const uint32_t indexCount)
{
    RenderSys::Mesh mesh;
    for (const auto &gltfPrimitive : gltfMesh.primitives)
    {
        mesh.primitives.push_back(loadPrimitive(gltfPrimitive, modelData, indexCount));
    }

    return mesh;
}

void GLTFScene::loadJointData(std::vector<glm::tvec4<uint16_t>> &jointVec, std::vector<int>& nodeToJoint, std::vector<glm::vec4> &weightVec)
{
    if(m_model->meshes.at(0).primitives.at(0).attributes.find("JOINTS_0") == m_model->meshes.at(0).primitives.at(0).attributes.end())
    {
        return;
    }
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
    if (m_model->skins.size() == 0)
    {
        return;
    }

    const tinygltf::Skin &skin = m_model->skins.at(0);

    int invBindMatAccessor = skin.inverseBindMatrices;
    const tinygltf::Accessor &accessor = m_model->accessors.at(invBindMatAccessor);
    const tinygltf::BufferView &bufferView = m_model->bufferViews.at(accessor.bufferView);
    const tinygltf::Buffer &buffer = m_model->buffers.at(bufferView.buffer);

    inverseBindMatrices.resize(skin.joints.size());
    std::memcpy(inverseBindMatrices.data(), &buffer.data.at(0) + bufferView.byteOffset, bufferView.byteLength);
}

void GLTFScene::loadTextures(std::vector<Texture>& textures)
{
    auto& gltfTextures = m_model->textures;
    if (gltfTextures.size() == 0)
        return;

    std::cout << "Loading Textures - " << gltfTextures.size() << std::endl;
    textures.reserve(gltfTextures.size());

    for (const auto& gltfTexture : gltfTextures)
    {
        const tinygltf::Image& image = m_model->images[gltfTexture.source];
        const std::string textureFilePath = m_sceneFilePath.parent_path().string() + "/" + image.uri;
        std::cout << "Texture Name: " << image.name 
                    << ", Type=" << image.mimeType 
                    << ", width=" << image.width
                    << ", height=" << image.height
                    << ", URI=" << textureFilePath 
                    << ", pixel_type=" << image.pixel_type 
                    << ", component=" << image.component 
                    << ", sampler=" << gltfTexture.sampler << std::endl;
        textures.emplace_back((unsigned char*)image.image.data(), image.width, image.height, 1);
    }

    std::cout << "Texture loading completed!" << std::endl;
}

RenderSys::SamplerAddressMode getWrapMode(int32_t wrapMode)
{
    switch (wrapMode) {
    case -1:
    case 10497:
        return RenderSys::SamplerAddressMode::REPEAT;
    case 33071:
        return RenderSys::SamplerAddressMode::CLAMP_TO_EDGE;
    case 33648:
        return RenderSys::SamplerAddressMode::MIRRORED_REPEAT;
    }

    std::cerr << "Unknown wrap mode for getVkWrapMode: " << wrapMode << std::endl;
    return RenderSys::SamplerAddressMode::REPEAT;
}

RenderSys::SamplerFilterMode getFilterMode(int32_t filterMode)
{
    switch (filterMode) {
    case -1:
    case 9728:
        return RenderSys::SamplerFilterMode::NEAREST;
    case 9729:
        return RenderSys::SamplerFilterMode::LINEAR;
    case 9984:
        return RenderSys::SamplerFilterMode::NEAREST;
    case 9985:
        return RenderSys::SamplerFilterMode::NEAREST;
    case 9986:
        return RenderSys::SamplerFilterMode::LINEAR;
    case 9987:
        return RenderSys::SamplerFilterMode::LINEAR;
    }

    std::cerr << "Unknown filter mode for getVkFilterMode: " << filterMode << std::endl;
    return RenderSys::SamplerFilterMode::NEAREST;
}

void GLTFScene::loadTextureSamplers(std::vector<TextureSampler>& samplers)
{
    for (const tinygltf::Sampler& smpl : m_model->samplers) {
        RenderSys::TextureSampler sampler{};
        sampler.minFilter = getFilterMode(smpl.minFilter);
        sampler.magFilter = getFilterMode(smpl.magFilter);
        sampler.addressModeU = getWrapMode(smpl.wrapS);
        sampler.addressModeV = getWrapMode(smpl.wrapT);
        sampler.addressModeW = sampler.addressModeV;
        samplers.push_back(sampler);
    }
}

void GLTFScene::loadMaterials(std::vector<Material>& materials)
{
    if (m_model->materials.size() == 0)
        return;

    std::cout << "Loading Materials - " << m_model->materials.size() << std::endl;
    for (const auto& mat : m_model->materials)
    {
        auto& material = materials.emplace_back();
        material.doubleSided = mat.doubleSided;
        material.baseColorFactor = glm::make_vec4(mat.pbrMetallicRoughness.baseColorFactor.data());
        material.metallicFactor = mat.pbrMetallicRoughness.metallicFactor;
        material.roughnessFactor = mat.pbrMetallicRoughness.roughnessFactor;
        // std::cout << "Material Name: " << mat.name 
        //             << ", metallicFactor=" << material.metallicFactor 
        //             << ", roughnessFactor=" << material.roughnessFactor << std::endl;
        material.baseColorTextureIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
        material.metallicRoughnessTextureIndex = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
        material.normalTextureIndex = mat.normalTexture.index;
    }
    std::cout << "Materials loading completed!" << std::endl;
}

void GLTFScene::getNodeGraphs(std::vector<std::shared_ptr<Model::ModelNode>>& rootNodes)
{
    assert(m_model);
    const int nodeCount = m_model->nodes.size();
    assert(nodeCount > 0);
    const int rootNodeCount = m_model->scenes.at(0).nodes.size(); // we are considering only one scene
    assert(rootNodeCount > 0);
    uint32_t indexCount = 0;
    for (const auto &rootNodeNum : m_model->scenes.at(0).nodes)
    {
        const tinygltf::Node &node = m_model->nodes.at(rootNodeNum);
        auto rootNode = traverse(nullptr, node, rootNodeNum, indexCount);
        rootNodes.push_back(rootNode);
    }
    std::cout << "model has " << nodeCount << " total nodes and " << rootNodeCount << " root nodes. [IndexCount=" << indexCount << "]" << std::endl;
}

std::shared_ptr<Model::ModelNode> GLTFScene::traverse(const std::shared_ptr<Model::ModelNode> parent, const tinygltf::Node &node, uint32_t nodeIndex, uint32_t& indexCount)
{
    auto sceneNode = std::make_shared<Model::ModelNode>(nodeIndex);
    sceneNode->setNodeName(node.name);

    if (node.mesh > -1)
    {
        std::cout << node.name << " : Mesh= " << node.mesh << std::endl;
        const auto& gltfMesh = m_model->meshes.at(node.mesh);
        const auto mesh = loadMesh(gltfMesh, sceneNode->m_data, indexCount);
        sceneNode->setMesh(mesh);
        indexCount += sceneNode->m_data.indices.size();
    }

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

    if (parent == nullptr)
    {
        sceneNode->calculateNodeMatrix(glm::mat4(1.0f));
    }
    else
    {
        sceneNode->calculateNodeMatrix(parent->getNodeMatrix());
    }

    if (node.children.size() > 0) 
    {
        for (size_t i = 0; i < node.children.size(); i++) 
        {
            sceneNode->m_childNodes.push_back(traverse(sceneNode, m_model->nodes[node.children[i]], node.children[i], indexCount));
        }
    }

    return sceneNode;
}

}
