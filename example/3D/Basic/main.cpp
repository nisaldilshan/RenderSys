#include <Walnut/Application.h>
#include <RenderSys/Walnut/WalnutAdapter.h>
#include <Walnut/EntryPoint.h>

#include "Common/ExampleBrowser.h"
#include "Examples3DBasic.h"

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	std::vector<ExampleBrowser::Example> examples = {
		{ "1. First 3D Shape", CreateFirst3DShapeLayer },
		{ "2. Transformation Matrices", CreateTransformationMatricesLayer },
		{ "3. Basic Shading", CreateBasicShadingLayer },
		{ "4. First Texture", CreateFirstTextureLayer },
		{ "5. Texture Sampler", CreateTextureSamplerLayer },
		{ "6. Texture Loader", CreateTextureLoaderLayer },
		{ "7. Camera Controls", CreateCameraControlsLayer },
		{ "8. Lighting", CreateLightingLayer },
		{ "9. Normal Mapping", CreateNormalMappingLayer },
	};

	Walnut::ApplicationSpecification spec;
	spec.Name = "Renderer3D Basic Examples";
#ifndef NDEBUG
	spec.EnableGraphicsValidation = true; // used only when the loader finds the layer, see VK_LAYER_PATH in launch.json
#endif

	Walnut::Application* app = new Walnut::Application(spec);
	RenderSys::AttachToWalnut();
	ExampleBrowser::SetDefaultDockLayout(*app);
	const size_t firstExample = ExampleBrowser::ExampleFromCommandLine(argc, argv, examples.size());
	app->PushLayer<ExampleBrowser>(*app, std::move(examples), firstExample);
	return app;
}
