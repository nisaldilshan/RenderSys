#include "InstanceBuffer.h"

#include <../example/Resources/Shaders/ShaderResource.h>
#include <iostream>

namespace RenderSys
{

InstanceBuffer::InstanceBuffer()
    : m_buffer(std::make_shared<Buffer>(sizeof(InstanceData) * MAX_INSTANCE, RenderSys::BufferUsage::STORAGE_BUFFER_VISIBLE_TO_CPU))
    , m_Dirty(false)
    , m_DataInstances(MAX_INSTANCE)
{
    m_buffer->MapBuffer();
}

InstanceBuffer::~InstanceBuffer()
{
}

void InstanceBuffer::SetInstanceData(uint32_t index, glm::mat4 const &modelMatrix)
{
    if (index >= MAX_INSTANCE)
    {
        std::cerr << "Instance index out of bounds!" << std::endl;
        assert(false);
        return;
    }

    m_DataInstances[index].m_ModelMatrix = modelMatrix;
    m_Dirty = true;
}

void InstanceBuffer::Update()
{
    if (m_Dirty)
    {
        m_buffer->WriteToBuffer(m_DataInstances.data());
        m_Dirty = false;
    }
}

}