#include "WebGPUBuffer.h"

namespace RenderSys
{

WebGPUBuffer::WebGPUBuffer(size_t byteSize, RenderSys::BufferUsage bufferUsage)
{

}

WebGPUBuffer::~WebGPUBuffer()
{
    UnmapBuffer();
}

void WebGPUBuffer::MapBuffer()
{
    assert(m_mapped != nullptr);
}

void WebGPUBuffer::UnmapBuffer()
{
    if (m_mapped == nullptr) {
        return;
    }
    m_mapped = nullptr;
}

void WebGPUBuffer::WriteToBuffer(const void *data)
{
    assert(m_mapped != nullptr);
    std::memcpy(m_mapped, data, 100);
}

bool WebGPUBuffer::Flush()
{
    return false;
}

} // namespace RenderSys