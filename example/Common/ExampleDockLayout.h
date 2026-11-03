#pragma once

#include <Walnut/Application.h>

#include <string>
#include <vector>

// Where an example's windows go while there is no imgui.ini with a saved layout. Walnut saves the layout
// to imgui.ini in the working directory when the app closes and restores it on the next start, so delete
// that file to get this layout back.
struct ExampleDockLayout
{
	// Windows stacked top to bottom in a column on the left, sharing its height. No column when empty.
	std::vector<std::string> Left;
	// The window that fills the middle.
	std::string Center = "Viewport";
	// Windows stacked top to bottom in a column on the right, sharing its height. No column when empty.
	std::vector<std::string> Right = { "Settings" };
	// Column widths, as fractions of the main window's width.
	float LeftWidth = 0.22f;
	float RightWidth = 0.22f;

	// Makes this the default layout of the application's main dockspace.
	void SetAsDefault(Walnut::Application& app) const;
};
