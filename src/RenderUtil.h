#pragma once

namespace RenderSys
{

    enum class VertexStepMode : uint32_t {
        Vertex = 0x00000000,
        Instance = 0x00000001,
        VertexBufferNotUsed = 0x00000002,
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
        VertexFormat format;
        uint64_t offset;
        uint32_t shaderLocation;
    };

    struct VertexBufferLayout
    {
        uint64_t arrayStride;
        VertexStepMode stepMode = VertexStepMode::Vertex;
        size_t attributeCount;
        VertexAttribute const * attributes;
    };


    enum class ShaderStage : uint32_t {
        None = 0x00000000,
        Vertex = 0x00000001,
        Fragment = 0x00000002,
        Compute = 0x00000004,
    };

    enum class BufferBindingType
    {
        Undefined = 0,
        Uniform,
        Storage,
        ReadOnlyStorage
    };

    struct BufferBindingLayout {
        void const * nextInChain = nullptr; // was "ChainedStruct const *""
        BufferBindingType type = BufferBindingType::Undefined;
        bool hasDynamicOffset = false;
        uint64_t minBindingSize = 0;
    };

    enum class SamplerBindingType
    {
        Undefined = 0,
        Filtering,
        NonFiltering,
        Comparison
    };
    
    struct SamplerBindingLayout {
        void const * nextInChain = nullptr; // was "ChainedStruct const *""
        SamplerBindingType type = SamplerBindingType::Undefined;
    };

    enum class TextureSampleType
    {
        Undefined = 0,
        Float,
        UnfilterableFloat,
        Depth,
        Sint,
        Uint
    };

    enum class TextureViewDimension
    {
        Undefined = 0,
        e1D,
        e2D,
        e2DArray,
        Cube,
        CubeArray,
        e3D
    };

    struct TextureBindingLayout {
        void const * nextInChain = nullptr; // was "ChainedStruct const *"
        TextureSampleType sampleType = TextureSampleType::Undefined;
        TextureViewDimension viewDimension = TextureViewDimension::Undefined;
        bool multisampled = false;
    };

    enum class StorageTextureAccess
    {
        Undefined = 0,
        WriteOnly
    };

    enum class TextureFormat : uint32_t {
        Undefined = 0x00000000,
        R8Unorm = 0x00000001,
        R8Snorm = 0x00000002,
        R8Uint = 0x00000003,
        R8Sint = 0x00000004,
        R16Uint = 0x00000005,
        R16Sint = 0x00000006,
        R16Float = 0x00000007,
        RG8Unorm = 0x00000008,
        RG8Snorm = 0x00000009,
        RG8Uint = 0x0000000A,
        RG8Sint = 0x0000000B,
        R32Float = 0x0000000C,
        R32Uint = 0x0000000D,
        R32Sint = 0x0000000E,
        RG16Uint = 0x0000000F,
        RG16Sint = 0x00000010,
        RG16Float = 0x00000011,
        RGBA8Unorm = 0x00000012,
        RGBA8UnormSrgb = 0x00000013,
        RGBA8Snorm = 0x00000014,
        RGBA8Uint = 0x00000015,
        RGBA8Sint = 0x00000016,
        BGRA8Unorm = 0x00000017,
        BGRA8UnormSrgb = 0x00000018,
        RGB10A2Unorm = 0x00000019,
        RG11B10Ufloat = 0x0000001A,
        RGB9E5Ufloat = 0x0000001B,
        RG32Float = 0x0000001C,
        RG32Uint = 0x0000001D,
        RG32Sint = 0x0000001E,
        RGBA16Uint = 0x0000001F,
        RGBA16Sint = 0x00000020,
        RGBA16Float = 0x00000021,
        RGBA32Float = 0x00000022,
        RGBA32Uint = 0x00000023,
        RGBA32Sint = 0x00000024,
        Stencil8 = 0x00000025,
        Depth16Unorm = 0x00000026,
        Depth24Plus = 0x00000027,
        Depth24PlusStencil8 = 0x00000028,
        Depth32Float = 0x00000029,
        Depth32FloatStencil8 = 0x0000002A,
        BC1RGBAUnorm = 0x0000002B,
        BC1RGBAUnormSrgb = 0x0000002C,
        BC2RGBAUnorm = 0x0000002D,
        BC2RGBAUnormSrgb = 0x0000002E,
        BC3RGBAUnorm = 0x0000002F,
        BC3RGBAUnormSrgb = 0x00000030,
        BC4RUnorm = 0x00000031,
        BC4RSnorm = 0x00000032,
        BC5RGUnorm = 0x00000033,
        BC5RGSnorm = 0x00000034,
        BC6HRGBUfloat = 0x00000035,
        BC6HRGBFloat = 0x00000036,
        BC7RGBAUnorm = 0x00000037,
        BC7RGBAUnormSrgb = 0x00000038,
        ETC2RGB8Unorm = 0x00000039,
        ETC2RGB8UnormSrgb = 0x0000003A,
        ETC2RGB8A1Unorm = 0x0000003B,
        ETC2RGB8A1UnormSrgb = 0x0000003C,
        ETC2RGBA8Unorm = 0x0000003D,
        ETC2RGBA8UnormSrgb = 0x0000003E,
        EACR11Unorm = 0x0000003F,
        EACR11Snorm = 0x00000040,
        EACRG11Unorm = 0x00000041,
        EACRG11Snorm = 0x00000042,
        ASTC4x4Unorm = 0x00000043,
        ASTC4x4UnormSrgb = 0x00000044,
        ASTC5x4Unorm = 0x00000045,
        ASTC5x4UnormSrgb = 0x00000046,
        ASTC5x5Unorm = 0x00000047,
        ASTC5x5UnormSrgb = 0x00000048,
        ASTC6x5Unorm = 0x00000049,
        ASTC6x5UnormSrgb = 0x0000004A,
        ASTC6x6Unorm = 0x0000004B,
        ASTC6x6UnormSrgb = 0x0000004C,
        ASTC8x5Unorm = 0x0000004D,
        ASTC8x5UnormSrgb = 0x0000004E,
        ASTC8x6Unorm = 0x0000004F,
        ASTC8x6UnormSrgb = 0x00000050,
        ASTC8x8Unorm = 0x00000051,
        ASTC8x8UnormSrgb = 0x00000052,
        ASTC10x5Unorm = 0x00000053,
        ASTC10x5UnormSrgb = 0x00000054,
        ASTC10x6Unorm = 0x00000055,
        ASTC10x6UnormSrgb = 0x00000056,
        ASTC10x8Unorm = 0x00000057,
        ASTC10x8UnormSrgb = 0x00000058,
        ASTC10x10Unorm = 0x00000059,
        ASTC10x10UnormSrgb = 0x0000005A,
        ASTC12x10Unorm = 0x0000005B,
        ASTC12x10UnormSrgb = 0x0000005C,
        ASTC12x12Unorm = 0x0000005D,
        ASTC12x12UnormSrgb = 0x0000005E,
        R8BG8Biplanar420Unorm = 0x0000005F,
    };


    struct StorageTextureBindingLayout {
        void const * nextInChain = nullptr; // was "ChainedStruct const *"
        StorageTextureAccess access = StorageTextureAccess::Undefined;
        TextureFormat format = TextureFormat::Undefined;
        TextureViewDimension viewDimension = TextureViewDimension::Undefined;
    };

    struct BindGroupLayoutEntry
    {
        void const * nextInChain = nullptr; // was "ChainedStruct const *""
        uint32_t binding;
        ShaderStage visibility;
        BufferBindingLayout buffer;
        SamplerBindingLayout sampler;
        TextureBindingLayout texture;
        StorageTextureBindingLayout storageTexture;
    };

    struct TextureDescriptor
    {
        /* data */
    };
    
    
} // namespace RenderSys
