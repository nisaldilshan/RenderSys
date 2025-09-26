#pragma once

#include <memory>
#include <array>
#include <RenderSys/Buffer.h>

namespace RenderSys
{

#if (RENDERER_BACKEND == 1)
class OpenGLResourceDescriptor;
typedef OpenGLResourceDescriptor ResourceDescriptorType;
#elif (RENDERER_BACKEND == 2)
class VulkanResourceDescriptor;
typedef VulkanResourceDescriptor ResourceDescriptorType;
#elif (RENDERER_BACKEND == 3)
class WebGPUResourceDescriptor;
typedef WebGPUResourceDescriptor ResourceDescriptorType;
#else
static_assert(false);
#endif

#include <resources/Shaders/ShaderResource.h>

class ResourceDescriptor
{
public:
    ResourceDescriptor();
    ResourceDescriptor(const ResourceDescriptor&) = delete;
    ResourceDescriptor& operator=(const ResourceDescriptor&) = delete;
    ResourceDescriptor(ResourceDescriptor&&) = delete;
    ResourceDescriptor& operator=(ResourceDescriptor&&) = delete;
    ~ResourceDescriptor();

    void AttachBuffer(uint32_t binding, const std::shared_ptr<Buffer>& buffer);
    void Init();
    ResourceDescriptorType* GetPlatformDescriptor() const { return m_platformDescriptor.get(); }

private:
    std::unique_ptr<ResourceDescriptorType> m_platformDescriptor{nullptr};
};

class Resource
{
public:
    enum BufferIndices
    {
        INSTANCE_BUFFER_INDEX = 0,
        SKELETAL_ANIMATION_BUFFER_INDEX,
        HEIGHTMAP,
        MULTI_PURPOSE_BUFFER,
        NUM_BUFFERS
    };

    enum ResourceFeatures // bitset
    {
        HAS_INSTANCING = GLSL_HAS_INSTANCING,
        HAS_SKELETAL_ANIMATION = GLSL_HAS_SKELETAL_ANIMATION,
        HAS_HEIGHTMAP = GLSL_HAS_HEIGHTMAP
    };

    Resource();
    ~Resource();

    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;
    Resource(Resource&&) = delete;
    Resource& operator=(Resource&&) = delete;

    void SetBuffer(BufferIndices index, const std::shared_ptr<Buffer>& buffer);
    void Init();
    std::shared_ptr<ResourceDescriptor> GetDescriptor() const { return m_ResourceDescriptor; }
private:
    std::shared_ptr<ResourceDescriptor> m_ResourceDescriptor;
    std::array<std::shared_ptr<Buffer>, Resource::NUM_BUFFERS> m_ResourceBuffers;
};

} // namespace RenderSys
   