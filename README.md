# RenderSys
A simple rendering system written in C++.

## Features
- Lightweight and modular rendering system.
- Supports basic 3D, 2D and Compute pipelines.
- Supported platforms : Linux, Windows and MacOS.
- Rendering backends : Vulkan, WebGPU and OpenGL(not implemented yet).

## Getting Started
1. Clone the repository:
    ```bash
    git clone https://github.com/nisaldilshan/RenderSys.git
    ```
2. Install dependencies using Conan:
    ```bash
    mkdir build
    cd build
    conan install ..
    ```
3. Build the project using CMake:
    ```bash
    cmake ..
    make
    ```

## Dependencies
- Based on a fork of Cherno's Walnut.
- ImGui for UI rendering.
- GLFW/SDL2 for window and input handling.
- GLM for mathematics operations.

## Examples
The 2D and the basic 3D examples are each built into one application. Pick an example from the
"Examples" list; the application destroys the running example and starts the selected one.
Pass an example's number to start with it:
```bash
build/Debug/example/2D/Examples2D 3
build/Debug/example/3D/Basic/Examples3DBasic 9
```

## Contributing
Contributions are welcome!

## License
This project is licensed under the MIT License. See the `LICENSE` file for details.