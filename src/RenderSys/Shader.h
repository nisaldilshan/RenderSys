#pragma once
#include <vector>
#include <iostream>

#include "RenderUtil.h"

// Code copied from - https://github.com/google/shaderc/blob/main/examples/online-compile/main.cc

namespace RenderSys
{

enum class ShaderType
{
    SPIRV = 0,
    WGSL
};

struct Shader
{
    ShaderType type;
    ShaderStage stage;
    std::string shaderSrc;
    std::vector<uint32_t> compiledShader;
};


namespace ShaderUtils
{
    std::vector<uint32_t> compile_file(const std::string &name,
                                        Shader& shader,
                                        bool optimize = false);

} // namespace ShaderUtils

} // namespace RenderSys
