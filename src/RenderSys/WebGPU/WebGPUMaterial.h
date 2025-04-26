#pragma once

#include <stdint.h>
#include <RenderSys/Material.h>

namespace RenderSys
{

class WebGPUMaterialDescriptor
{
public:
    WebGPUMaterialDescriptor(MaterialTextures& textures);
    ~WebGPUMaterialDescriptor();

};


} // namespace RenderSys