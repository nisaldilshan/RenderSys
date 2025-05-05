#pragma once

#include <string>
#include <RenderSys/Scene/UUID.h>

namespace RenderSys
{

class IDComponent
{
public:
    UUID ID;

    IDComponent() = delete;
    IDComponent(UUID& uuid)
        : ID(uuid) 
    {}
    IDComponent(const IDComponent&) = default;
};

class TagComponent
{
public:
    std::string Tag;

    TagComponent() = default;
    TagComponent(const TagComponent&) = default;
    TagComponent(const std::string& tag)
        : Tag(tag) {}
};
   
} // namespace RenderSys
