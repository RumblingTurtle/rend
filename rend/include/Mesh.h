#pragma once
#include <Shader.h>
#include <macros.h>
#include <types.h>

#include <Eigen/Dense>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <memory>
#include <vk_mem_alloc.h>

class Mesh {
  // Contiguous chunk of vertices which is passed to the shader as is
  std::vector<float> _vertices;
  int _vertex_count = 0;
  bool _buffer_generated = false;

public:
  typedef std::shared_ptr<Mesh> Ptr;

  BufferAllocation buffer_allocation;

  Path _mesh_path;

  // Loads a mesh, it's shaders and fills the vertex buffer
  bool load(Path path);
  int vertex_count();
  bool generate_allocation_buffer(VmaAllocator &allocator,
                                  Deallocator &deallocator_queue);
};
