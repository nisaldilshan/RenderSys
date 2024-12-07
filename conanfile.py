from conans import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain
from conan.errors import ConanInvalidConfiguration

class RenderSysConan(ConanFile):
    name = "RenderSys"
    version = "0.0.1"
    license = "MIT license"
    url = "https://github.com/nisaldilshan/RenderSys.git"
    description = "A simple rendering system written in C++"
    author = "Nisal Dilshan"
    settings = "os", "compiler", "build_type", "arch"
    generators = 'VirtualBuildEnv'
    options = {
        'rendering_backend': ["OpenGL", "Vulkan", "WebGPU"],
        'fPIC': [True, False]
    }
    default_options = {
        'rendering_backend': "WebGPU",
        'fPIC': True
    }
    
    def requirements(self):
        self.requires('walnut/latest')
        self.requires('tinyobjloader/2.0.0-rc10')
        self.requires('tinygltf/2.9.0')
        if self.options.rendering_backend == "Vulkan":
            self.requires('shaderc/2023.6')
        elif self.options.rendering_backend == "WebGPU":
            self.requires("WebGPU/latest")
        else:
            raise ConanInvalidConfiguration("Unsupported Renderer Type")

    def build_requirements(self):
        self.tool_requires('cmake/3.25.3')

    def config_options(self):
        self.options['walnut'].rendering_backend = self.options.rendering_backend

    def generate(self):
        tc = CMakeToolchain(self)
        rendererName = self.options.rendering_backend
        print("Using Renderer - " + str(rendererName))
        tc.variables["RENDERER"] = rendererName
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.verbose = True
        cmake.configure()
        cmake.build()

    def package_id(self):
        pass

    def package_info(self):
        self.cpp_info.components['Renderer2D'].set_property('cmake_file_name', 'Renderer2D')
        self.cpp_info.components['Renderer2D'].set_property('pkg_config_name', 'Renderer2D')
        self.cpp_info.components['Renderer2D'].set_property('cmake_target_name', 'RenderSys::Renderer2D')
        self.cpp_info.components['Renderer2D'].set_property('cmake_target_aliases', ['Renderer2D'])
        self.cpp_info.components['Renderer2D'].set_property('pkg_config_name', 'Renderer2D')
        self.cpp_info.components['Renderer2D'].libs = ['RenderSys2D']
        self.cpp_info.components['Renderer2D'].defines = ["RENDERER_BACKEND=3"]

        self.cpp_info.components['Renderer3D'].set_property('cmake_file_name', 'Renderer3D')
        self.cpp_info.components['Renderer3D'].set_property('pkg_config_name', 'Renderer3D')
        self.cpp_info.components['Renderer3D'].set_property('cmake_target_name', 'RenderSys::Renderer3D')
        self.cpp_info.components['Renderer3D'].set_property('cmake_target_aliases', ['Renderer3D'])
        self.cpp_info.components['Renderer3D'].set_property('pkg_config_name', 'Renderer3D')
        self.cpp_info.components['Renderer3D'].libs = ['RenderSys3D']
        self.cpp_info.components['Renderer3D'].defines = ["RENDERER_BACKEND=3"]

        self.cpp_info.components['Compute'].set_property('cmake_file_name', 'Compute')
        self.cpp_info.components['Compute'].set_property('pkg_config_name', 'Compute')
        self.cpp_info.components['Compute'].set_property('cmake_target_name', 'RenderSys::Compute')
        self.cpp_info.components['Compute'].set_property('cmake_target_aliases', ['Compute'])
        self.cpp_info.components['Compute'].set_property('pkg_config_name', 'Compute')
        self.cpp_info.components['Compute'].libs = ['ComputeSys']
        self.cpp_info.components['Compute'].defines = ["RENDERER_BACKEND=3"]

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def layout(self):
        self.folders.source = "."
        self.folders.build = "build"
        self.folders.generators = "build"
        self.folders.imports = self.folders.build