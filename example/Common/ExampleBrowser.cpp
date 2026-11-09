#include "ExampleBrowser.h"
#include "ExampleDockLayout.h"

#include <imgui.h>

#include <cstdlib>

ExampleBrowser::ExampleBrowser(Walnut::Application& app, std::vector<Example> examples, size_t firstExample)
	: m_app(app)
	, m_examples(std::move(examples))
	, m_firstExample(firstExample)
{}

void ExampleBrowser::OnAttach()
{
	Select(m_firstExample);
}

void ExampleBrowser::OnUIRender()
{
	ImGui::Begin("Examples");
	for (size_t i = 0; i < m_examples.size(); i++)
	{
		if (ImGui::Selectable(m_examples[i].Name.c_str(), i == m_selected))
			Select(i);
	}
	ImGui::End();
}

void ExampleBrowser::Select(size_t index)
{
	if (index == m_selected || index >= m_examples.size())
		return;

	// Not held here any more, so the old example is destroyed as soon as Walnut detaches it
	if (m_running)
		m_app.PopLayer(m_running);
	m_running = m_examples[index].Create();
	m_app.PushLayer(m_running);
	m_selected = index;
}

void ExampleBrowser::SetDefaultDockLayout(Walnut::Application& app)
{
	ExampleDockLayout layout;
	layout.Left = { "Examples" };
	layout.LeftWidth = 0.15f;
	layout.SetAsDefault(app);
}

size_t ExampleBrowser::ExampleFromCommandLine(int argc, char** argv, size_t exampleCount)
{
	if (argc < 2)
		return 0;

	const long number = std::strtol(argv[1], nullptr, 10);
	if (number < 1 || static_cast<size_t>(number) > exampleCount)
		return 0;
	return static_cast<size_t>(number) - 1;
}
