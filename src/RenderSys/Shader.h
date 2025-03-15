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

class Shader
{
public:
    Shader() = delete;
    Shader(const std::string& name, const std::string& shaderSrc)
        : m_name(name)
        , m_shaderSrc(shaderSrc)
    {}
    ~Shader() = default;

    std::string GetName() const { return m_name; }
    const std::string& GetShaderSrc() const { return m_shaderSrc; }
    bool Compile(bool optimize = false);
    const std::vector<uint32_t>& GetCompiledShader() const;

    ShaderType type;
    ShaderStage stage;
private:
    const std::string m_name;
    const std::string m_shaderSrc;
    std::vector<uint32_t> m_compiledShader;
};

} // namespace RenderSys
