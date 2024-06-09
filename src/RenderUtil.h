#pragma once

namespace RenderSys
{

    enum class VertexStepMode
    {
        Vertex = 0,
        Instance,
        VertexBufferNotUsed
    };

    struct VertexBufferLayout
    {
        /* data */
    };

    enum class VertexFormat
    {
        Undefined = 0,
        Uint8x2,
        Uint8x4,
        Sint8x2,
        Sint8x4,
        Unorm8x2,
        Unorm8x4,
        Snorm8x2,
        Snorm8x4,
        Uint16x2,
        Uint16x4,
        Sint16x2,
        Sint16x4,
        Unorm16x2,
        Unorm16x4,
        Snorm16x2,
        Snorm16x4,
        Float16x2,
        Float16x4,
        Float32,
        Float32x2,
        Float32x3,
        Float32x4,
        Uint32,
        Uint32x2,
        Uint32x3,
        Uint32x4,
        Sint32,
        Sint32x2,
        Sint32x3,
        Sint32x4,
    };

    struct VertexAttribute
    {
        /* data */
    };

    struct BindGroupLayoutEntry
    {
        /* data */
    };
    
    
} // namespace RenderSys
