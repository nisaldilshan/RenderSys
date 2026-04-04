import os
from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.scm import Git
from conan.tools.files import copy
from conan.errors import ConanInvalidConfiguration, ConanException

class RenderSysConan(ConanFile):
    name = "RenderSys"
    version = "0.0.1"
    license = "MIT license"
    url = "https://github.com/nisaldilshan/RenderSys"
    description = "A simple rendering system written in C++"
    author = "Nisal Dilshan"
    settings = "os", "compiler", "build_type", "arch"
    
    options = {
        "shared": [True, False],
        "rendering_backend": ["OpenGL", "Vulkan", "WebGPU"],
        "build_examples": [True, False],
        "fPIC": [True, False],
        "branch": ["ANY"],
    }
    default_options = {
        "shared": False,
        "rendering_backend": "Vulkan",
        "build_examples": True,
        "fPIC": True,
        "branch": "main",
    }

    def requirements(self):
        self.requires("walnut/2.0.0")
        self.requires("tinyobjloader/2.0.0-rc10")
        self.requires("tinygltf/2.9.0")
        self.requires("shaderc/2023.6")
        self.requires("entt/3.14.0")
        self.requires("glm/1.0.1")
        if self.options.rendering_backend == "Vulkan":
            self.requires("vulkan-memory-allocator/3.3.0")
        elif self.options.rendering_backend == "WebGPU":
            pass # self.requires("webgpu/latest")
        else:
            raise ConanInvalidConfiguration(f"Unsupported Renderer Type: {self.options.rendering_backend}")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.25.3]")
    
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.get_safe("shared"):
            self.options.rm_safe("fPIC")
        self.options["walnut"].rendering_backend = self.options.rendering_backend

    def source(self):
        git = Git(self)
        target_ref = "main" if self.version == "latest" else f"v{self.version}"
        self.output.info(f"Attempting to checkout git ref: {target_ref}")

        try:
            self.run(f"git clone --depth 1 --branch {target_ref} {self.url}.git .")
        except Exception as e:
            raise ConanException(
                f"\n\nERROR: Could not checkout version '{self.version}'.\n"
                f"Attempted to fetch git ref '{target_ref}' from {self.url}\n"
                f"Please verify that the tag 'v{self.version}' exists in the remote repository.\n"
                f"Original Git Error: {str(e)}\n"
            )

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["RENDERER"] = self.options.rendering_backend
        tc.variables["BUILD_EXAMPLES"] = self.options.build_examples
        self.output.info(f"Renderer: {self.options.rendering_backend}")
        tc.generate()
        
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, pattern="*", 
             src=os.path.join(self.source_folder, "src", "resources"), 
             dst=os.path.join(self.package_folder, "res"))

    def package_info(self):
        # Component: Renderer2D
        self.cpp_info.components['Renderer2D'].set_property('cmake_target_name', 'RenderSys::Renderer2D')
        self.cpp_info.components['Renderer2D'].libs = ['RenderSys2D']

        # Component: Renderer3D
        self.cpp_info.components['Renderer3D'].set_property('cmake_target_name', 'RenderSys::Renderer3D')
        self.cpp_info.components['Renderer3D'].libs = ['RenderSys3D']
        self.cpp_info.components['Renderer3D'].requires = ['Common']

        # Component: Compute
        self.cpp_info.components['Compute'].set_property('cmake_target_name', 'RenderSys::Compute')
        self.cpp_info.components['Compute'].libs = ['ComputeSys']
        self.cpp_info.components['Compute'].requires = ['Common']

        # Component: Common
        self.cpp_info.components['Common'].set_property('cmake_target_name', 'RenderSys::Common')
        self.cpp_info.components['Common'].libs = ['RenderSysCommon']
        self.cpp_info.components['Common'].resdirs = ['res']

    def layout(self):
        # Standardizes build and source folder structures for Conan 2.x
        cmake_layout(self)