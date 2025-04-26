#include "GLTFScene.h"

#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#include <RenderSys/Components/EntityRegistry.h>
#include <RenderSys/Components/TransformComponent.h>
#include <RenderSys/Components/MeshComponent.h>

namespace RenderSys
{

GLTFModel::GLTFModel()
    : m_gltfModel(std::make_unique<tinygltf::Model>())
{
}

GLTFModel::~GLTFModel()
{
}

bool GLTFModel::load(const std::filesystem::path &filePath, const std::string &textureFilename)
{
    m_sceneFilePath = filePath;

    tinygltf::TinyGLTF gltfLoader;
    std::string loaderErrors;
    std::string loaderWarnings;

    bool result = false;
    if (filePath.extension() == ".glb") 
    {
        result = gltfLoader.LoadBinaryFromFile(m_gltfModel.get(), &loaderErrors, &loaderWarnings, filePath.string());
    }
    else 
    {
        result = gltfLoader.LoadASCIIFromFile(m_gltfModel.get(), &loaderErrors, &loaderWarnings, filePath.string());
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

void GLTFModel::computeProps()
{
    const tinygltf::Scene& scene = m_gltfModel->scenes[m_gltfModel->defaultScene > -1 ? m_gltfModel->defaultScene : 0];
    for (size_t i = 0; i < scene.nodes.size(); i++) {
        getNodeProps(m_gltfModel->nodes[scene.nodes[i]], *m_gltfModel, m_vertexCount, m_indexCount);
    }

    std::cout << "GLTFModel: [VertexCount=" << m_vertexCount << "], [IndexCount=" << m_indexCount << "]" << std::endl;
}

size_t GLTFModel::getVertexCount() const
{
    return m_vertexCount;
}

size_t GLTFModel::getIndexCount() const
{
    return m_indexCount;
}

void GLTFModel::getNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount)
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

RenderSys::SubMesh GLTFModel::loadPrimitive(const tinygltf::Primitive &primitive, std::shared_ptr<ModelData> modelData, const uint32_t indexCount)
{
    assert(primitive.attributes.find("POSITION") != primitive.attributes.end()); // position is mandatory
    const tinygltf::Accessor &posAccessor = m_gltfModel->accessors[primitive.attributes.find("POSITION")->second];
    const auto vertexCount = static_cast<uint32_t>(posAccessor.count);
    assert(vertexCount > 0);
    const uint32_t vertexStart = modelData->vertices.size();
    modelData->vertices.resize(vertexStart + vertexCount);

    const tinygltf::BufferView &positionBufferView = m_gltfModel->bufferViews[posAccessor.bufferView];
    const auto* positionBuffer = reinterpret_cast<const float *>(&(m_gltfModel->buffers[positionBufferView.buffer].data[posAccessor.byteOffset + positionBufferView.byteOffset]));
    const auto posByteStride = posAccessor.ByteStride(positionBufferView) ? 
                                (posAccessor.ByteStride(positionBufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

    for (size_t v = 0; v < vertexCount; v++) 
    {
        ModelVertex& vert = modelData->vertices[v + vertexStart];
        vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * posByteStride]), 1.0f);
    }

    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
    {
        const tinygltf::Accessor &accessor = m_gltfModel->accessors[primitive.attributes.find("TEXCOORD_0")->second];
        const tinygltf::BufferView &bufferView = m_gltfModel->bufferViews[accessor.bufferView];
        const auto* bufferPos = reinterpret_cast<const float *>(&(m_gltfModel->buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
        const auto byteStride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

        for (size_t v = 0; v < vertexCount; v++) 
        {
            ModelVertex& vert = modelData->vertices[v + vertexStart];
            vert.uv0 = glm::vec4(glm::make_vec3(&bufferPos[v * byteStride]), 1.0f);
        }
    }

    if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
    {
        const tinygltf::Accessor &accessor = m_gltfModel->accessors[primitive.attributes.find("NORMAL")->second];
        const tinygltf::BufferView &bufferView = m_gltfModel->bufferViews[accessor.bufferView];
        const auto* bufferPos = reinterpret_cast<const float *>(&(m_gltfModel->buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
        const auto byteStride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

        for (size_t v = 0; v < vertexCount; v++) 
        {
            ModelVertex& vert = modelData->vertices[v + vertexStart];
            vert.normal = glm::vec4(glm::make_vec3(&bufferPos[v * byteStride]), 1.0f);
        }
    }

    if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
    {
        const tinygltf::Accessor &accessor = m_gltfModel->accessors[primitive.attributes.find("TANGENT")->second];
        const tinygltf::BufferView &bufferView = m_gltfModel->bufferViews[accessor.bufferView];
        const auto* bufferPos = reinterpret_cast<const float *>(&(m_gltfModel->buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
        const auto byteStride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

        for (size_t v = 0; v < vertexCount; v++) 
        {
            ModelVertex& vert = modelData->vertices[v + vertexStart];
            vert.tangent = glm::vec4(glm::make_vec3(&bufferPos[v * byteStride]), 1.0f);
        }
    }

    RenderSys::SubMesh prim;
    prim.m_VertexCount = vertexCount;
    if (primitive.indices > -1)
    {
        const uint32_t indexStart = modelData->indices.size();
        prim.m_FirstIndex = indexCount + indexStart;
        
        const tinygltf::Accessor &accessor = m_gltfModel->accessors[primitive.indices];
        const tinygltf::BufferView &bufferView = m_gltfModel->bufferViews[accessor.bufferView];
        const tinygltf::Buffer &buffer = m_gltfModel->buffers[bufferView.buffer];

        const auto currentIndexCount = static_cast<uint32_t>(accessor.count);
        assert(currentIndexCount > 0);
        prim.m_IndexCount = currentIndexCount;
        modelData->indices.resize(indexStart + currentIndexCount);

        const void *dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

        switch (accessor.componentType) {
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
            const uint32_t *buf = static_cast<const uint32_t*>(dataPtr);
            for (size_t index = 0; index < accessor.count; index++) {
                modelData->indices[index + indexStart] = buf[index] + vertexStart;
            }
            break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
            const uint16_t *buf = static_cast<const uint16_t*>(dataPtr);
            for (size_t index = 0; index < accessor.count; index++) {
                modelData->indices[index + indexStart] = buf[index] + vertexStart;
            }
            break;
        }
        case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
            const uint8_t *buf = static_cast<const uint8_t*>(dataPtr);
            for (size_t index = 0; index < accessor.count; index++) {
                modelData->indices[index + indexStart] = buf[index] + vertexStart;
            }
            break;
        }
        default:
            std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
            assert(false);
        }
    }

    prim.m_Material = createMaterial(primitive.material);
    return prim;
}

void GLTFModel::loadMesh(const tinygltf::Mesh& gltfMesh, std::shared_ptr<ModelNode> node, const uint32_t indexCount)
{
    auto& registry = EntityRegistry::Get();
    MeshComponent& meshComponent{registry.emplace<MeshComponent>(node->getEntity(), "", std::make_shared<Mesh>(node->m_data))};
    for (const auto &gltfPrimitive : gltfMesh.primitives)
    {
        meshComponent.m_Mesh->subMeshes.push_back(loadPrimitive(gltfPrimitive, node->m_data, indexCount));
    }    
}

void GLTFModel::loadJointData(std::vector<glm::tvec4<uint16_t>> &jointVec, std::vector<int>& nodeToJoint, std::vector<glm::vec4> &weightVec)
{
    if(m_gltfModel->meshes.at(0).primitives.at(0).attributes.find("JOINTS_0") == m_gltfModel->meshes.at(0).primitives.at(0).attributes.end())
    {
        return;
    }
    // Joints
    {
        int jointsAccessor = m_gltfModel->meshes.at(0).primitives.at(0).attributes.at("JOINTS_0");
        const tinygltf::Accessor &accessor = m_gltfModel->accessors.at(jointsAccessor);
        const tinygltf::BufferView &bufferView = m_gltfModel->bufferViews.at(accessor.bufferView);
        const tinygltf::Buffer &buffer = m_gltfModel->buffers.at(bufferView.buffer);

        assert(accessor.count > 0);
        jointVec.resize(accessor.count);
        std::memcpy(jointVec.data(), &buffer.data.at(0) + bufferView.byteOffset, bufferView.byteLength);
    }

    nodeToJoint.resize(m_gltfModel->nodes.size());
    const tinygltf::Skin &skin = m_gltfModel->skins.at(0);
    for (int i = 0; i < skin.joints.size(); ++i) {
        int destinationNode = skin.joints.at(i);
        nodeToJoint.at(destinationNode) = i;
    }

    // Weights
    {
        int weightAccessor = m_gltfModel->meshes.at(0).primitives.at(0).attributes.at("WEIGHTS_0");
        const tinygltf::Accessor &accessor = m_gltfModel->accessors.at(weightAccessor);
        const tinygltf::BufferView &bufferView = m_gltfModel->bufferViews.at(accessor.bufferView);
        const tinygltf::Buffer &buffer = m_gltfModel->buffers.at(bufferView.buffer);

        assert(accessor.count > 0);
        weightVec.resize(accessor.count);
        std::memcpy(weightVec.data(), &buffer.data.at(0) + bufferView.byteOffset, bufferView.byteLength);
    }
}

void GLTFModel::loadInverseBindMatrices(std::vector<glm::mat4>& inverseBindMatrices)
{
    if (m_gltfModel->skins.size() == 0)
    {
        return;
    }

    const tinygltf::Skin &skin = m_gltfModel->skins.at(0);

    int invBindMatAccessor = skin.inverseBindMatrices;
    const tinygltf::Accessor &accessor = m_gltfModel->accessors.at(invBindMatAccessor);
    const tinygltf::BufferView &bufferView = m_gltfModel->bufferViews.at(accessor.bufferView);
    const tinygltf::Buffer &buffer = m_gltfModel->buffers.at(bufferView.buffer);

    inverseBindMatrices.resize(skin.joints.size());
    std::memcpy(inverseBindMatrices.data(), &buffer.data.at(0) + bufferView.byteOffset, bufferView.byteLength);
}

void GLTFModel::loadTextures()
{
    auto samplers = loadTextureSamplers();

    auto& gltfTextures = m_gltfModel->textures;
    if (gltfTextures.size() == 0)
        return;

    std::cout << "Loading Textures - " << gltfTextures.size() << std::endl;
    m_textures.reserve(gltfTextures.size());

    for (const auto& gltfTexture : gltfTextures)
    {
        const tinygltf::Image& image = m_gltfModel->images[gltfTexture.source];
        // const std::string textureFilePath = m_sceneFilePath.parent_path().string() + "/" + image.uri;
        // std::cout << "Texture Name: " << image.name 
        //             << ", Type=" << image.mimeType 
        //             << ", width=" << image.width
        //             << ", height=" << image.height
        //             << ", URI=" << textureFilePath 
        //             << ", pixel_type=" << image.pixel_type 
        //             << ", component=" << image.component 
        //             << ", sampler=" << gltfTexture.sampler << std::endl;
        m_textures.emplace_back(std::make_shared<Texture>((unsigned char*)image.image.data(), image.width, image.height, 1));
        m_textures.back()->SetSampler(samplers[gltfTexture.sampler]);
    }

    //std::cout << "Texture loading completed!" << std::endl;
}

RenderSys::TextureSampler::AddressMode getWrapMode(int32_t wrapMode)
{
    switch (wrapMode) {
    case -1:
    case 10497:
        return RenderSys::TextureSampler::AddressMode::REPEAT;
    case 33071:
        return RenderSys::TextureSampler::AddressMode::CLAMP_TO_EDGE;
    case 33648:
        return RenderSys::TextureSampler::AddressMode::MIRRORED_REPEAT;
    }

    std::cerr << "Unknown wrap mode for getVkWrapMode: " << wrapMode << std::endl;
    return RenderSys::TextureSampler::AddressMode::REPEAT;
}

RenderSys::TextureSampler::FilterMode getFilterMode(int32_t filterMode)
{
    switch (filterMode) {
    case -1:
    case 9728:
        return RenderSys::TextureSampler::FilterMode::NEAREST;
    case 9729:
        return RenderSys::TextureSampler::FilterMode::LINEAR;
    case 9984:
        return RenderSys::TextureSampler::FilterMode::NEAREST;
    case 9985:
        return RenderSys::TextureSampler::FilterMode::NEAREST;
    case 9986:
        return RenderSys::TextureSampler::FilterMode::LINEAR;
    case 9987:
        return RenderSys::TextureSampler::FilterMode::LINEAR;
    }

    std::cerr << "Unknown filter mode for getVkFilterMode: " << filterMode << std::endl;
    return RenderSys::TextureSampler::FilterMode::NEAREST;
}

void GLTFModel::loadTransform(std::shared_ptr<ModelNode> currentNode, const tinygltf::Node &gltfNode, std::shared_ptr<ModelNode> parentNode)
{
    auto& registry = EntityRegistry::Get();
    TransformComponent& transform{registry.emplace<TransformComponent>(currentNode->getEntity())};
    if (gltfNode.matrix.size() == 16)
    {
        transform.SetMat4Local(glm::make_mat4x4(gltfNode.matrix.data()));
    }
    else
    {
        if (gltfNode.rotation.size() == 4)
        {
            float x = gltfNode.rotation[0];
            float y = gltfNode.rotation[1];
            float z = gltfNode.rotation[2];
            float w = gltfNode.rotation[3];

            transform.SetRotation({w, x, y, z});
        }
        if (gltfNode.scale.size() == 3)
        {
            transform.SetScale({gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]});
        }
        if (gltfNode.translation.size() == 3)
        {
            transform.SetTranslation({gltfNode.translation[0], gltfNode.translation[1], gltfNode.translation[2]});
        }
    }

    if (parentNode == nullptr)
    {
        auto parentNodeMatrix = glm::mat4(1.0f);
        transform.SetMat4Global(parentNodeMatrix);
    }
    else
    {
        auto parentNodeMatrix = parentNode->getNodeMatrix();
        transform.SetMat4Global(parentNodeMatrix);
    }
}

std::vector<TextureSampler> GLTFModel::loadTextureSamplers()
{
    std::vector<TextureSampler> samplers;
    for (const tinygltf::Sampler& smpl : m_gltfModel->samplers) {
        RenderSys::TextureSampler sampler{};
        sampler.minFilter = getFilterMode(smpl.minFilter);
        sampler.magFilter = getFilterMode(smpl.magFilter);
        sampler.addressModeU = getWrapMode(smpl.wrapS);
        sampler.addressModeV = getWrapMode(smpl.wrapT);
        sampler.addressModeW = sampler.addressModeV;
        samplers.push_back(sampler);
    }
    return samplers;
}

void GLTFModel::loadMaterials()
{
    if (m_gltfModel->materials.size() == 0)
        return;

    std::cout << "Loading Materials - " << m_gltfModel->materials.size() << std::endl;
    for (const auto& mat : m_gltfModel->materials)
    {
        MaterialProperties materialProp{};
        // materialProp.doubleSided = mat.doubleSided;
        materialProp.m_baseColor = glm::make_vec4(mat.pbrMetallicRoughness.baseColorFactor.data());
        materialProp.m_metallic = mat.pbrMetallicRoughness.metallicFactor;
        materialProp.m_roughness = mat.pbrMetallicRoughness.roughnessFactor;
        // std::cout << "Material Name: " << mat.name 
        //             << ", metallicFactor=" << material.metallicFactor 
        //             << ", roughnessFactor=" << material.roughnessFactor << std::endl;
        

        auto& material = m_materials.emplace_back(std::make_shared<RenderSys::Material>());
        material->SetMaterialProperties(materialProp);

        if (mat.pbrMetallicRoughness.baseColorTexture.index != -1)
        {
            materialProp.m_features |= RenderSys::MaterialFeatures::HAS_DIFFUSE_MAP;
            auto baseColorTexture = m_textures[mat.pbrMetallicRoughness.baseColorTexture.index];
            material->SetMaterialTexture(RenderSys::TextureIndices::DIFFUSE_MAP_INDEX, baseColorTexture);
        }
        if (mat.normalTexture.index != -1)
        {
            materialProp.m_features |= RenderSys::MaterialFeatures::HAS_NORMAL_MAP;
            auto normalTexture = m_textures[mat.normalTexture.index];
            material->SetMaterialTexture(RenderSys::TextureIndices::NORMAL_MAP_INDEX, normalTexture);
        }
        if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
        {
            materialProp.m_features |= RenderSys::MaterialFeatures::HAS_ROUGHNESS_METALLIC_MAP;
            auto metallicRoughnessTexture = m_textures[mat.pbrMetallicRoughness.metallicRoughnessTexture.index];
            material->SetMaterialTexture(RenderSys::TextureIndices::ROUGHNESS_METALLIC_MAP_INDEX, metallicRoughnessTexture);
        }
        
    }
    //std::cout << "Materials loading completed!" << std::endl;
}

void GLTFModel::getNodeGraphs(std::vector<std::shared_ptr<ModelNode>>& rootNodes)
{
    assert(m_gltfModel);
    const int nodeCount = m_gltfModel->nodes.size();
    assert(nodeCount > 0);
    const int rootNodeCount = m_gltfModel->scenes.at(0).nodes.size(); // we are considering only one scene
    assert(rootNodeCount > 0);
    uint32_t indexCount = 0;
    for (const auto &rootNodeNum : m_gltfModel->scenes.at(0).nodes)
    {
        const tinygltf::Node &node = m_gltfModel->nodes.at(rootNodeNum);
        auto rootNode = traverse(nullptr, node, rootNodeNum, indexCount);
        rootNodes.push_back(rootNode);
    }
    std::cout << "model has " << nodeCount << " total nodes and " << rootNodeCount << " root nodes. [IndexCount=" << indexCount << "]" << std::endl;
}

std::shared_ptr<ModelNode> GLTFModel::traverse(const std::shared_ptr<ModelNode> parent, const tinygltf::Node &node, uint32_t nodeIndex, uint32_t& indexCount)
{
    auto sceneNode = std::make_shared<ModelNode>(nodeIndex);
    sceneNode->setNodeName(node.name);

    if (node.mesh > -1)
    {
        std::cout << node.name << " : Mesh= " << node.mesh << std::endl;
        const auto& gltfMesh = m_gltfModel->meshes.at(node.mesh);
        loadMesh(gltfMesh, sceneNode, indexCount);
        indexCount += sceneNode->m_data->indices.size();
    }

    loadTransform(sceneNode, node, parent);
    
    if (node.children.size() > 0) 
    {
        for (size_t i = 0; i < node.children.size(); i++) 
        {
            sceneNode->m_childNodes.push_back(traverse(sceneNode, m_gltfModel->nodes[node.children[i]], node.children[i], indexCount));
        }
    }

    return sceneNode;
}

std::shared_ptr<RenderSys::Material> GLTFModel::createMaterial(int materialIndex)
{
    if (materialIndex < 0 || materialIndex >= m_materials.size())
    {
        std::cerr << "Invalid material index: " << materialIndex << std::endl;
        return std::shared_ptr<RenderSys::Material>();
    }

    auto mat = m_materials[materialIndex];
    mat->Init();
    return mat;
}
}
