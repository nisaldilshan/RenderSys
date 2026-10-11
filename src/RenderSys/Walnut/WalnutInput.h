#pragma once
#include <RenderSys/Camera/EditorCameraController.h>
#include <Walnut/Input/Input.h>

namespace RenderSys
{

// Camera input from Walnut's window: hold Left Alt and drag with a mouse button to navigate.
inline CameraInput GetCameraInputFromWalnut()
{
    const auto [x, y] = Walnut::Input::GetMousePosition();
    CameraInput input;
    input.mousePosition = glm::vec2(x, y);
    input.navigate = Walnut::Input::IsKeyDown(Walnut::Key::LeftAlt);
    input.leftButton = Walnut::Input::IsMouseButtonDown(Walnut::MouseButton::Left);
    input.middleButton = Walnut::Input::IsMouseButtonDown(Walnut::MouseButton::Middle);
    input.rightButton = Walnut::Input::IsMouseButtonDown(Walnut::MouseButton::Right);
    return input;
}

} // namespace RenderSys
