#include "GLTFModel.h"

#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#include <RenderSys/Components/TransformComponent.h>
#include <RenderSys/Components/MeshComponent.h>
#include <RenderSys/Scene/Scene.h>
#include "Skeleton.h"
#include "Animation.h"

namespace RenderSys
{

GLTFModel::GLTFModel(Scene& scene)
    : m_sceneRef(scene)
    , m_gltfModel(std::make_unique<tinygltf::Model>())
{
}

GLTFModel::~GLTFModel()
{
}

bool GLTFModel::load(const std::filesystem::path &filePath)
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
        std::cout << "GLTFModel: warnings while loading glTF model:\n" << loaderWarnings << std::endl;
    }

    if (!loaderErrors.empty()) {
        std::cout << "GLTFModel: errors while loading glTF model:\n" << loaderErrors << std::endl;
    }

    if (!result) {
        std::cout << "GLTFModel: failed to load glTF model from file: " << filePath.string() << std::endl;
    }

    return result;
}

void getNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount)
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

void GLTFModel::computeProps()
{
    static size_t s_vertexCount = 0;
    static size_t s_indexCount = 0;
    const tinygltf::Scene& scene = m_gltfModel->scenes[m_gltfModel->defaultScene > -1 ? m_gltfModel->defaultScene : 0];
    for (size_t i = 0; i < scene.nodes.size(); i++) {
        getNodeProps(m_gltfModel->nodes[scene.nodes[i]], *m_gltfModel, s_vertexCount, s_indexCount);
    }
    std::cout << "GLTFModel: [VertexCount=" << s_vertexCount << "], [IndexCount=" << s_indexCount << "]" << std::endl;
    s_vertexCount = 0;
    s_indexCount = 0;
}

RenderSys::SubMesh GLTFModel::loadPrimitive(const tinygltf::Primitive &primitive, std::shared_ptr<MeshData> modelData, const uint32_t indexCount)
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

void GLTFModel::loadMesh(const tinygltf::Mesh& gltfMesh, entt::entity& nodeEntity, uint32_t& indexCount)
{
    MeshComponent& meshComponent{m_sceneRef.m_Registry.emplace<MeshComponent>(nodeEntity, "", std::make_shared<Mesh>(std::make_shared<MeshData>()))};
    for (const auto &gltfPrimitive : gltfMesh.primitives)
    {
        meshComponent.m_Mesh->subMeshes.push_back(loadPrimitive(gltfPrimitive, meshComponent.m_Mesh->m_meshData, indexCount));
    }    
    indexCount += meshComponent.m_Mesh->m_meshData->indices.size();
}

void GLTFModel::loadJointData()
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
        m_jointVec.resize(accessor.count);
        std::memcpy(m_jointVec.data(), &buffer.data.at(0) + bufferView.byteOffset, bufferView.byteLength);
    }

    m_nodeToJoint.resize(m_gltfModel->nodes.size());
    const tinygltf::Skin &skin = m_gltfModel->skins.at(0);
    for (int i = 0; i < skin.joints.size(); ++i) {
        int destinationNode = skin.joints.at(i);
        m_nodeToJoint.at(destinationNode) = i;
    }

    // Weights
    {
        int weightAccessor = m_gltfModel->meshes.at(0).primitives.at(0).attributes.at("WEIGHTS_0");
        const tinygltf::Accessor &accessor = m_gltfModel->accessors.at(weightAccessor);
        const tinygltf::BufferView &bufferView = m_gltfModel->bufferViews.at(accessor.bufferView);
        const tinygltf::Buffer &buffer = m_gltfModel->buffers.at(bufferView.buffer);

        assert(accessor.count > 0);
        m_weightVec.resize(accessor.count);
        std::memcpy(m_weightVec.data(), &buffer.data.at(0) + bufferView.byteOffset, bufferView.byteLength);
    }
}

void GLTFModel::loadInverseBindMatrices()
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

    m_inverseBindMatrices.resize(skin.joints.size());
    std::memcpy(m_inverseBindMatrices.data(), &buffer.data.at(0) + bufferView.byteOffset, bufferView.byteLength);
}

auto g_Skeleton = std::make_shared<Skeleton>();
std::vector<std::shared_ptr<SkeletalAnimation>> g_animations;
void GLTFModel::loadjointMatrices()
{
    // traverse through the graph again and compute JointMatrices
    // if(m_inverseBindMatrices.size() > 0 && m_nodeToJoint.size() > 0)
    // {
    //     m_jointMatrices.resize(m_inverseBindMatrices.size());
    //     for (auto &rootNode : rootNodes)
    //         rootNode->calculateJointMatrices(m_inverseBindMatrices, m_nodeToJoint, m_jointMatrices, m_registryRef);
    // }

    g_Skeleton->Update();

}

// recursive function via global gltf nodes (which have children)
// tree structure links (local) skeleton joints
void LoadJoint(int globalGltfNodeIndex, int parentJoint, std::unique_ptr<tinygltf::Model>& gltfModel)
{
    int currentJoint = g_Skeleton->m_GlobalNodeToJointIndex[globalGltfNodeIndex];
    g_Skeleton->m_Joints[currentJoint].m_ParentJoint = parentJoint;

    // process children (if any)
    size_t numberOfChildren = gltfModel->nodes[globalGltfNodeIndex].children.size();
    if (numberOfChildren > 0)
    {
        g_Skeleton->m_Joints[currentJoint].m_Children.resize(numberOfChildren);
        for (size_t childIndex = 0; childIndex < numberOfChildren; ++childIndex)
        {
            uint32_t globalGltfNodeIndexForChild = gltfModel->nodes[globalGltfNodeIndex].children[childIndex];
            g_Skeleton->m_Joints[currentJoint].m_Children[childIndex] = g_Skeleton->m_GlobalNodeToJointIndex[globalGltfNodeIndexForChild];
            LoadJoint(globalGltfNodeIndexForChild, currentJoint, gltfModel);
        }
    }
}

void GLTFModel::loadSkeletons()
{
    size_t numberOfSkeletons = m_gltfModel->skins.size();
    if (!numberOfSkeletons)
    {
        return;
    }
    assert(numberOfSkeletons == 1);

    const tinygltf::Skin& glTFSkin = m_gltfModel->skins[0];
    assert(glTFSkin.inverseBindMatrices != -1);

    // set up number of joints
    size_t numberOfJoints = glTFSkin.joints.size();
    // resize the joints vector of the skeleton object (to be filled)
    g_Skeleton->m_Joints.resize(numberOfJoints);
    g_Skeleton->m_ShaderData.m_FinalJointsMatrices.resize(numberOfJoints);

    // set up name of skeleton
    g_Skeleton->m_Name = glTFSkin.name;

    // loop over all joints from gltf model and fill the skeleton with joints
    for (size_t jointIndex = 0; jointIndex < numberOfJoints; ++jointIndex)
    {
        int globalGltfNodeIndex = glTFSkin.joints[jointIndex];
        auto& joint = g_Skeleton->m_Joints[jointIndex]; // just a reference for easier code
        joint.m_InverseBindMatrix = m_inverseBindMatrices[jointIndex];
        joint.m_Name = m_gltfModel->nodes[globalGltfNodeIndex].name;

        // set up map "global node" to "joint index"
        g_Skeleton->m_GlobalNodeToJointIndex[globalGltfNodeIndex] = jointIndex;
    }

    int rootJoint = glTFSkin.joints[0]; // the here always works but the gltf field skins.skeleton can be ignored

    LoadJoint(rootJoint, NO_PARENT, m_gltfModel); // recursive function to fill the skeleton with joints and their children
}

void GLTFModel::loadAnimations()
{
    size_t numberOfAnimations = m_gltfModel->animations.size();
    for (size_t animationIndex = 0; animationIndex < numberOfAnimations; ++animationIndex)
    {
        auto &gltfAnimation = m_gltfModel->animations[animationIndex];
        std::string name = gltfAnimation.name;
        //LOG_CORE_INFO("name of animation: {0}", name);
        std::shared_ptr<SkeletalAnimation> animation = std::make_shared<SkeletalAnimation>(name);

        // Samplers
        size_t numberOfSamplers = gltfAnimation.samplers.size();
        animation->m_Samplers.resize(numberOfSamplers);
        for (size_t samplerIndex = 0; samplerIndex < numberOfSamplers; ++samplerIndex)
        {
            tinygltf::AnimationSampler glTFSampler = gltfAnimation.samplers[samplerIndex];
            auto &sampler = animation->m_Samplers[samplerIndex];

            sampler.m_Interpolation = SkeletalAnimation::InterpolationMethod::LINEAR;
            if (glTFSampler.interpolation == "STEP")
            {
                sampler.m_Interpolation = SkeletalAnimation::InterpolationMethod::STEP;
            }
            else if (glTFSampler.interpolation == "CUBICSPLINE")
            {
                sampler.m_Interpolation = SkeletalAnimation::InterpolationMethod::CUBICSPLINE;
            }

            // get timestamp
            {
                uint32_t count = 0;
                const float *timestampBuffer;
                auto componentType =
                    LoadAccessor<float>(m_gltfModel->accessors[glTFSampler.input], timestampBuffer, &count);

#define GL_FLOAT 0x1406          // 5126
                if (componentType == GL_FLOAT)
                {
                    sampler.m_Timestamps.resize(count);
                    for (size_t index = 0; index < count; ++index)
                    {
                        sampler.m_Timestamps[index] = timestampBuffer[index];
                    }
                }
                else
                {
                    //CORE_ASSERT(false, "GltfBuilder::LoadSkeletonsGltf: cannot handle timestamp format");
                }
            }

            // Read sampler keyframe output translate/rotate/scale values
            {
                uint32_t count = 0;
                int type;
                const uint32_t *buffer;
                LoadAccessor<uint32_t>(m_gltfModel->accessors[glTFSampler.output], buffer, &count, &type);

                switch (type)
                {
                case TINYGLTF_TYPE_VEC3:
                {
                    const glm::vec3 *outputBuffer = reinterpret_cast<const glm::vec3 *>(buffer);
                    sampler.m_TRSoutputValuesToBeInterpolated.resize(count);
                    for (size_t index = 0; index < count; index++)
                    {
                        sampler.m_TRSoutputValuesToBeInterpolated[index] = glm::vec4(outputBuffer[index], 0.0f);
                    }
                    break;
                }
                case TINYGLTF_TYPE_VEC4:
                {
                    const glm::vec4 *outputBuffer = reinterpret_cast<const glm::vec4 *>(buffer);
                    sampler.m_TRSoutputValuesToBeInterpolated.resize(count);
                    for (size_t index = 0; index < count; index++)
                    {
                        sampler.m_TRSoutputValuesToBeInterpolated[index] = glm::vec4(outputBuffer[index]);
                    }
                    break;
                }
                default:
                {
                    assert(false);
                    //CORE_ASSERT(false, "void GltfBuilder::LoadSkeletonsGltf(...): accessor type not found");
                    break;
                }
                }
            }
        }
        if (animation->m_Samplers.size()) // at least one sampler found
        {
            auto &sampler = animation->m_Samplers[0];
            if (sampler.m_Timestamps.size() >= 2) // samplers have at least 2 keyframes to interpolate in between
            {
                animation->SetFirstKeyFrameTime(sampler.m_Timestamps[0]);
                animation->SetLastKeyFrameTime(sampler.m_Timestamps.back());
            }
        }
        // Each node of the skeleton has channels that point to samplers
        size_t numberOfChannels = gltfAnimation.channels.size();
        animation->m_Channels.resize(numberOfChannels);
        for (size_t channelIndex = 0; channelIndex < numberOfChannels; ++channelIndex)
        {
            tinygltf::AnimationChannel glTFChannel = gltfAnimation.channels[channelIndex];
            SkeletalAnimation::Channel &channel = animation->m_Channels[channelIndex];
            channel.m_SamplerIndex = glTFChannel.sampler;
            channel.m_Node = glTFChannel.target_node;
            if (glTFChannel.target_path == "translation")
            {
                channel.m_Path = SkeletalAnimation::Path::TRANSLATION;
            }
            else if (glTFChannel.target_path == "rotation")
            {
                channel.m_Path = SkeletalAnimation::Path::ROTATION;
            }
            else if (glTFChannel.target_path == "scale")
            {
                channel.m_Path = SkeletalAnimation::Path::SCALE;
            }
            else
            {
                assert(false);
                //LOG_CORE_CRITICAL("path not supported");
            }
        }
        g_animations.push_back(animation);
    }

    // testing begin
    if (g_animations.size() > 0)
    {
        g_animations[0]->Start();
        g_animations[0]->Update(0.1f, *g_Skeleton);
    }
    // testing end
}

void GLTFModel::applyVertexSkinning(RenderSys::VertexBuffer& vertexBuffer)
{
    if (m_jointVec.size() == 0)
    {
        return;
    }
    if (m_weightVec.size() == 0)
    {
        return;
    }
    // if (m_jointMatrices.size() == 0)
    // {
    //     return;
    // }

    for (int jointIndex = 0; jointIndex < m_jointVec.size(); ++jointIndex) 
    {
        auto jointMatrix = g_Skeleton->m_ShaderData.m_FinalJointsMatrices[m_jointVec[jointIndex].x];
        //glm::ivec4 jointIndex = glm::make_vec4(m_jointVec.at(i));
        glm::vec4 weightIndex = glm::make_vec4(m_weightVec.at(jointIndex));
        glm::mat4 skinMat =
            weightIndex.x * jointMatrix +
            weightIndex.y * jointMatrix +
            weightIndex.z * jointMatrix +
            weightIndex.w * jointMatrix;
        vertexBuffer.vertices.at(jointIndex).position = jointMatrix * glm::vec4(vertexBuffer.vertices.at(jointIndex).position, 1.0f);
    }
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

void GLTFModel::loadTransform(entt::entity& nodeEntity, const tinygltf::Node &gltfNode, const uint32_t parent)
{
    TransformComponent& transform{m_sceneRef.m_Registry.emplace<TransformComponent>(nodeEntity)};
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

    auto parentEntity = m_sceneGraph.GetNode(parent).GetGameObject();
    if (!m_sceneRef.m_Registry.all_of<TransformComponent>(parentEntity))
    {
        auto parentNodeMatrix = glm::mat4(1.0f);
        transform.SetMat4Global(parentNodeMatrix);
    }
    else
    {
        auto& parentTransform = m_sceneRef.m_Registry.get<RenderSys::TransformComponent>(parentEntity);
        transform.SetMat4Global(parentTransform.GetMat4Global());
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

void GLTFModel::getNodeGraphs()
{
    assert(m_gltfModel);
    const int nodeCount = m_gltfModel->nodes.size();
    assert(nodeCount > 0);
    const int rootNodeCount = m_gltfModel->scenes.at(0).nodes.size(); // we are considering only one scene
    assert(rootNodeCount > 0);
    uint32_t indexCount = 0;
    m_modelRootNodeIndex = m_sceneGraph.CreateRootNode(m_sceneRef.CreateEntity("RootNode"), "RootNode");
    for (const auto &rootNodeNum : m_gltfModel->scenes.at(0).nodes)
    {
        traverse(m_modelRootNodeIndex, rootNodeNum, indexCount);
    }
    std::cout << "model has " << nodeCount << " total nodes and " << rootNodeCount << " root nodes. [IndexCount=" << indexCount << "]" << std::endl;
}

void GLTFModel::printNodeGraph() const
{
    m_sceneGraph.TraverseLog(m_modelRootNodeIndex);
}

void GLTFModel::traverse(const uint32_t parent, uint32_t nodeIndex, uint32_t& indexCount)
{
    const tinygltf::Node &node = m_gltfModel->nodes.at(nodeIndex);
    auto nodeEntity = m_sceneRef.CreateEntity(node.name);
    auto sceneNode = m_sceneGraph.CreateNode(parent, nodeEntity, node.name + std::to_string(nodeIndex) + "_Node"); 

    if (node.mesh > -1)
    {
        std::cout << node.name << " : Mesh= " << node.mesh << std::endl;
        const auto& gltfMesh = m_gltfModel->meshes.at(node.mesh);
        loadMesh(gltfMesh, nodeEntity, indexCount);
        
    }

    loadTransform(nodeEntity, node, parent);

    size_t childNodeCount = node.children.size();
    for (size_t childNodeIndex = 0; childNodeIndex < childNodeCount; ++childNodeIndex)
    {
        int gltfChildNodeIndex = node.children[childNodeIndex];
        traverse(sceneNode, gltfChildNodeIndex, indexCount);
    }
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
