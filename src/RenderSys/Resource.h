#pragma once

#include <memory>

namespace RenderSys
{

class Resources
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

    // fixed-size array for resource buffers
    typedef std::array<std::shared_ptr<Buffer>, Resources::NUM_BUFFERS> ResourceBuffers;

    enum ResourceFeatures // bitset
    {
        HAS_INSTANCING = GLSL_HAS_INSTANCING,
        HAS_SKELETAL_ANIMATION = GLSL_HAS_SKELETAL_ANIMATION,
        HAS_HEIGHTMAP = GLSL_HAS_HEIGHTMAP
    };

public:
    std::shared_ptr<ResourceDescriptor> m_ResourceDescriptor;
    ResourceBuffers m_ResourceBuffers;
};

} // namespace RenderSys
   