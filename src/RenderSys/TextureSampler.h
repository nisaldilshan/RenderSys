#pragma once

namespace RenderSys
{

struct TextureSampler
{
    enum class AddressMode
    {
        REPEAT = 0,
        CLAMP_TO_EDGE,
        MIRRORED_REPEAT,
        CLAMP_TO_BORDER, // reads outside [0, 1] return borderColor
    };

    enum class FilterMode
    {
        NEAREST = 0,
        LINEAR,
    };

    enum class BorderColor
    {
        TRANSPARENT_BLACK = 0,
        OPAQUE_BLACK,
        OPAQUE_WHITE, // depth 1, the far plane: a shadow map's border then reads as lit
    };

    // How a depth sample compares the reference value (the receiver's depth) against the stored depth.
    // A sampler2DShadow read returns 1 where "reference op stored" holds and 0 where it does not,
    // and LINEAR filtering averages the four nearest results (2x2 percentage-closer filtering).
    enum class CompareOp
    {
        NONE = 0, // a plain depth read, for sampler2D
        LESS_OR_EQUAL,
        GREATER_OR_EQUAL,
    };

    FilterMode magFilter;
    FilterMode minFilter;
    AddressMode addressModeU;
    AddressMode addressModeV;
    AddressMode addressModeW;
    BorderColor borderColor = BorderColor::OPAQUE_BLACK;
    CompareOp compareOp = CompareOp::NONE;
};

} // namespace RenderSys
