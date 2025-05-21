#pragma once
#include <RenderSys/Buffer.h>

namespace RenderSys
{

class WebGPUBuffer
{
public:
    WebGPUBuffer(size_t byteSize, RenderSys::BufferUsage bufferUsage);
    ~WebGPUBuffer();

    void MapBuffer();
    void UnmapBuffer();
    void WriteToBuffer(const void *data);
    bool Flush();

private:
    void* m_mapped = nullptr;
};

} // namespace RenderSys