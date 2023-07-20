#pragma once
#include <memory>
#include <rend/macros.h>

#include <Eigen/Dense>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <vk_mem_alloc.h>

#include <rend/Rendering/Vulkan/Shader.h>
#include <rend/Rendering/Vulkan/vk_helper_types.h>
class Mesh {
  // Contiguous chunk of vertices which is passed to the shader as is
  std::vector<float> _vertices;
  int _vertex_count = 0;

public:
  typedef std::shared_ptr<Mesh> Ptr;

  BufferAllocation buffer_allocation;

  Path _mesh_path;

  // Loads a mesh, it's shaders and fills the vertex buffer
  Mesh(Path path);
  int vertex_count() const;
  void generate_allocation_buffer(VmaAllocator &allocator,
                                  Deallocator &deallocator_queue);

  Eigen::Vector3f get_vertex_pos(int idx) const {
    return Eigen::Vector3f::Map(_vertices.data() + 8 * idx);
  }
};

namespace Primitives {
inline Mesh::Ptr &get_default_cube_mesh() {
  static Mesh::Ptr p_cube =
      std::make_shared<Mesh>(Path{ASSET_DIRECTORY} / Path{"models/cube.obj"});
  return p_cube;
}
} // namespace Primitives