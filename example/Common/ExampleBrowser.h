#pragma once

#include <Walnut/Application.h>
#include <Walnut/Layer.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Lists examples in an "Examples" window and runs the selected one as a layer of the application.
// Selecting another example pops the running layer and pushes a new one: Walnut detaches and destroys
// the old layer before it attaches the new one, so an example creates its GPU resources in OnAttach.
class ExampleBrowser : public Walnut::Layer
{
public:
	struct Example
	{
		std::string Name;
		std::function<std::shared_ptr<Walnut::Layer>()> Create;
	};

	ExampleBrowser(Walnut::Application& app, std::vector<Example> examples, size_t firstExample = 0);

	virtual void OnAttach() override;
	virtual void OnUIRender() override;

	void Select(size_t index);

	// Docks the example list on the left, "Settings" on the right and "Viewport" in the middle,
	// the windows every example draws. Used while there is no imgui.ini with a saved layout.
	static void SetDefaultDockLayout(Walnut::Application& app);

	// The example the command line asks for: its 1-based number, e.g. "3" in `Examples2D 3`.
	// Returns 0, the first example, when there is no valid number.
	static size_t ExampleFromCommandLine(int argc, char** argv, size_t exampleCount);

private:
	Walnut::Application& m_app;
	std::vector<Example> m_examples;
	size_t m_firstExample = 0;
	size_t m_selected = SIZE_MAX;
	std::shared_ptr<Walnut::Layer> m_running;
};
