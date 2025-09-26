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
        'build_examples': [True, False],
        'fPIC': [True, False],
        'branch': 'ANY',
    }
    default_options = {
        'rendering_backend': "WebGPU",
        'build_examples': True,
        'fPIC': True,
        'branch': 'main',
    }
    
    def requirements(self):
        self.requires('walnut/latest')
        self.requires('tinyobjloader/2.0.0-rc10')
        self.requires('tinygltf/2.9.0')
        self.requires('shaderc/2023.6')
        if self.options.rendering_backend == "Vulkan":
            self.requires('vulkan-memory-allocator/3.1.0')
        elif self.options.rendering_backend == "WebGPU":
            self.requires("WebGPU/latest")
        else:
            raise ConanInvalidConfiguration("Unsupported Renderer Type")
        self.requires('entt/3.14.0')

    def build_requirements(self):
        self.tool_requires('cmake/3.25.3')

    def config_options(self):
        self.options['walnut'].rendering_backend = self.options.rendering_backend

    def source(self):
        self.run('git clone --depth=1 '
                 'git@github.com:nisaldilshan/RenderSys.git '
                 f'--branch {self.options.branch} . ')

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["RENDERER"] = self.options.rendering_backend
        tc.variables["BUILD_EXAMPLES"] = self.options.build_examples
        print("Renderer: " + str(tc.variables["RENDERER"]) )
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.verbose = True
        cmake.parallel = True
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

        self.cpp_info.components['Renderer3D'].set_property('cmake_file_name', 'Renderer3D')
        self.cpp_info.components['Renderer3D'].set_property('pkg_config_name', 'Renderer3D')
        self.cpp_info.components['Renderer3D'].set_property('cmake_target_name', 'RenderSys::Renderer3D')
        self.cpp_info.components['Renderer3D'].set_property('cmake_target_aliases', ['Renderer3D'])
        self.cpp_info.components['Renderer3D'].set_property('pkg_config_name', 'Renderer3D')
        self.cpp_info.components['Renderer3D'].libs = ['RenderSys3D']
        self.cpp_info.components['Renderer3D'].requires = ['Common']

        self.cpp_info.components['Compute'].set_property('cmake_file_name', 'Compute')
        self.cpp_info.components['Compute'].set_property('pkg_config_name', 'Compute')
        self.cpp_info.components['Compute'].set_property('cmake_target_name', 'RenderSys::Compute')
        self.cpp_info.components['Compute'].set_property('cmake_target_aliases', ['Compute'])
        self.cpp_info.components['Compute'].set_property('pkg_config_name', 'Compute')
        self.cpp_info.components['Compute'].libs = ['ComputeSys']
        self.cpp_info.components['Renderer3D'].requires = ['Common']

        self.cpp_info.components['Common'].set_property('cmake_file_name', 'Common')
        self.cpp_info.components['Common'].set_property('pkg_config_name', 'Common')
        self.cpp_info.components['Common'].set_property('cmake_target_name', 'RenderSys::Common')
        self.cpp_info.components['Common'].set_property('cmake_target_aliases', ['Common'])
        self.cpp_info.components['Common'].set_property('pkg_config_name', 'Common')
        self.cpp_info.components['Common'].libs = ['RenderSysCommon']
        self.cpp_info.components['Common'].resdirs = ['res']

    def package(self):
        cmake = CMake(self)
        cmake.install()

        self.copy("*", dst="res", src="src/resources", keep_path=True)

    def layout(self):
        self.folders.source = "."
        self.folders.build = "build"
        self.folders.generators = "build"
        self.folders.imports = self.folders.build