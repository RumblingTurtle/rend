#pragma once
#include <Material.h>
#include <Mesh.h>

struct Renderable {
  typedef std::shared_ptr<Renderable> Ptr;

  Material::Ptr p_material;
  Mesh::Ptr p_mesh;
  Object object;
};