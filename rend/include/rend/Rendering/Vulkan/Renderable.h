#pragma once
#include <rend/Rendering/Vulkan/Material.h>
#include <rend/Rendering/Vulkan/Mesh.h>

enum class RenderableType { Geometry, Light };
struct Renderable {
  typedef std::shared_ptr<Renderable> Ptr;
  RenderableType type;
  Mesh::Ptr p_mesh;
  Texture::Ptr p_texture;
  bool reflective = false;
};