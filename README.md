# Basic Vulkan renderer

This is a repo for a simple Vulkan based rederer. It may be used as a boilerplate base for your projects. You can expand it by implementing:

- Asset management system
- ECS 
(https://github.com/skypjack/entt)
- Scripting system (https://github.com/jordanvrtanoski/luacpp)
- Physics
(https://github.com/bulletphysics/bullet3)
- UI rendering
(https://github.com/ocornut/imgui)


## Requirements
- SDL2
- Vulkan
- glslangValidator
- assimp
```
sudo apt install assimp-utils
```
- VMA
```
sudo apt-get install libvma-utils
```

Bootstrapping and buffer allocation is handled by submodules. Namely vk-bootstrap (https://github.com/charles-lunarg/vk-bootstrap) and VMA (https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git)
## Building

```
git clone 
git submodule update --init --recursive

mkdir build && cd build
cmake .. && make
```