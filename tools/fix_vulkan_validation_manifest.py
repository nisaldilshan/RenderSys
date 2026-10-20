#!/usr/bin/env python3
"""Rewrites the Conan-packaged VK_LAYER_KHRONOS_validation manifest so the Vulkan
loader can actually find and load it.

Conan's vulkan-validationlayers package ships a manifest whose "api_version" has
4 dot-separated components (e.g. "1.3.239.0"); the loader's manifest parser only
expects 3 ("major.minor.patch") and silently skips the layer otherwise. This
script also makes "library_path" absolute, so VK_LAYER_PATH can point at the
corrected copy without caring where the real .so lives.

Usage: fix_vulkan_validation_manifest.py <conan_package_folder> <output_dir>
"""
import json
import pathlib
import sys


def main(argv):
    if len(argv) != 3:
        print(f"usage: {argv[0]} <conan_package_folder> <output_dir>", file=sys.stderr)
        return 1

    package_folder = pathlib.Path(argv[1])
    output_dir = pathlib.Path(argv[2])
    manifest_path = package_folder / "res" / "vulkan" / "explicit_layer.d" / "VkLayer_khronos_validation.json"

    manifest = json.loads(manifest_path.read_text())
    layer = manifest["layer"]

    api_version_parts = layer["api_version"].split(".")
    layer["api_version"] = ".".join(api_version_parts[:3])

    layer["library_path"] = str((package_folder / "lib" / layer["library_path"]).resolve())

    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / manifest_path.name).write_text(json.dumps(manifest, indent=4))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
