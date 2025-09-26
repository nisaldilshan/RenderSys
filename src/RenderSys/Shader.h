#pragma once
#include <vector>
#include <iostream>
#include <filesystem>
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
    Shader(const std::string& name, const std::string& shaderSrc);
    Shader(const std::filesystem::path &filePath);
    Shader(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader& operator=(Shader&&) = delete;
    ~Shader() = default;

    std::string GetName() const { return m_name; }
    const std::string& GetShaderSrc() const { return m_shaderSrc; }
    bool Compile(bool optimize = false);
    const std::vector<uint32_t>& GetCompiledShader() const;
    void SetIncludeDirectory(const std::string& dir);
    std::string GetIncludeDirectory() const;

    ShaderType type;
    ShaderStage stage;
private:
    std::string m_name;
    std::string m_shaderSrc;
    std::vector<uint32_t> m_compiledShader;
    std::string m_includeDir;
};

} // namespace RenderSys
