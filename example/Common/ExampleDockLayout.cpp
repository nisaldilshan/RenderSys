#include "ExampleDockLayout.h"

#include <imgui.h>
#include <imgui_internal.h> // DockBuilder

namespace
{
	// Docks the windows into the node from top to bottom, each with an equal share of its height
	void DockColumn(ImGuiID node, const std::vector<std::string>& windows)
	{
		for (size_t i = 0; i < windows.size(); i++)
		{
			const size_t remaining = windows.size() - i;
			ImGuiID windowNode = node;
			if (remaining > 1)
				windowNode = ImGui::DockBuilderSplitNode(node, ImGuiDir_Up, 1.0f / remaining, nullptr, &node);
			ImGui::DockBuilderDockWindow(windows[i].c_str(), windowNode);
		}
	}
}

void ExampleDockLayout::SetAsDefault(Walnut::Application& app) const
{
	app.SetDefaultDockLayoutCallback([layout = *this](uint32_t dockspaceId)
	{
		// Each split keeps the dockspace's central node on the side away from the new column
		ImGuiID centerId = dockspaceId;
		float centerWidth = 1.0f;
		if (!layout.Left.empty())
		{
			DockColumn(ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Left, layout.LeftWidth, nullptr, &centerId), layout.Left);
			centerWidth -= layout.LeftWidth;
		}
		if (!layout.Right.empty())
		{
			// The ratio is relative to the node being split, which is what the left column left over
			DockColumn(ImGui::DockBuilderSplitNode(centerId, ImGuiDir_Right, layout.RightWidth / centerWidth, nullptr, &centerId), layout.Right);
		}
		ImGui::DockBuilderDockWindow(layout.Center.c_str(), centerId);
	});
}
