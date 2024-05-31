# merian-hdr-viewer

A simple HDR viewer built with [merian](https://github.com/LDAP/merian) providing various exposure and tone-mapping controls. 

## Requirements

- A fairly recent C++ compiler
- A GPU with Vulkan support

## Usage


```bash
# Clone this repo including submodules
git clone --recursive <url>

# Setup the build directory
meson setup build
# Or: Use a debug build that enables debug logging and extra checks
meson setup build --buildtype=debugoptimized

# Compile
meson compile -C build

# Run
# An example shader is provided in res/shaders/shader.comp.glsl
./build/merian-hdr-viewer <path/to/shader>

```
