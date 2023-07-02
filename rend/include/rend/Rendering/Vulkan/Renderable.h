#pragma once
#include <rend/Rendering/Vulkan/Material.h>
#include <rend/Rendering/Vulkan/Mesh.h>

struct Renderable {
  typedef std::shared_ptr<Renderable> Ptr;

  Material::Ptr p_material;
  Mesh::Ptr p_mesh;
  Object object;
};