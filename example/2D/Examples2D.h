#pragma once

#include <Walnut/Layer.h>

#include <memory>

// One layer per example, listed in main.cpp
std::shared_ptr<Walnut::Layer> CreateFirstTriangleLayer();
std::shared_ptr<Walnut::Layer> CreateVertexBuffersLayer();
std::shared_ptr<Walnut::Layer> CreateVertexAttributesLayer();
std::shared_ptr<Walnut::Layer> CreateIndexBuffersLayer();
std::shared_ptr<Walnut::Layer> CreateUniformBuffersLayer();
std::shared_ptr<Walnut::Layer> CreateDynamicUniformsLayer();
