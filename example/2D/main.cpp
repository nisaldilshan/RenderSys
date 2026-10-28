#include <Walnut/Application.h>
#include <RenderSys/Walnut/WalnutAdapter.h>
#include <Walnut/EntryPoint.h>

#include "Common/ExampleBrowser.h"
#include "Examples2D.h"

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	std::vector<ExampleBrowser::Example> examples = {
		{ "1. First Triangle", CreateFirstTriangleLayer },
		{ "2. Vertex Buffers", CreateVertexBuffersLayer },
		{ "3. Vertex Attributes", CreateVertexAttributesLayer },
		{ "4. Index Buffers", CreateIndexBuffersLayer },
		{ "5. Uniform Buffers", CreateUniformBuffersLayer },
		{ "6. Dynamic Uniforms", CreateDynamicUniformsLayer },
	};

	Walnut::ApplicationSpecification spec;
	spec.Name = "Renderer2D Examples";
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
