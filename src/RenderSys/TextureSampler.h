#pragma once

namespace RenderSys
{
    
struct TextureSampler
{
    enum class AddressMode
    {
        REPEAT = 0,
        CLAMP_TO_EDGE,
        MIRRORED_REPEAT
    };

    enum class FilterMode
    {
        NEAREST = 0,
        LINEAR,
    };

    FilterMode magFilter;
    FilterMode minFilter;
    AddressMode addressModeU;
    AddressMode addressModeV;
    AddressMode addressModeW;
};

} // namespace RenderSys