#pragma once

#include <Walnut/Layer.h>

#include <memory>

// One layer per example, listed in main.cpp
std::shared_ptr<Walnut::Layer> CreateFirst3DShapeLayer();
std::shared_ptr<Walnut::Layer> CreateTransformationMatricesLayer();
std::shared_ptr<Walnut::Layer> CreateBasicShadingLayer();
std::shared_ptr<Walnut::Layer> CreateFirstTextureLayer();
std::shared_ptr<Walnut::Layer> CreateTextureSamplerLayer();
std::shared_ptr<Walnut::Layer> CreateTextureLoaderLayer();
std::shared_ptr<Walnut::Layer> CreateCameraControlsLayer();
std::shared_ptr<Walnut::Layer> CreateLightingLayer();
std::shared_ptr<Walnut::Layer> CreateNormalMappingLayer();
