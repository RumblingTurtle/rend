#pragma once
#include <Shader.h>
#include <macros.h>
#include <types.h>

#include <Eigen/Dense>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

class Mesh {
  const aiScene *scene;
  const aiMesh *mesh;

public:
  struct ShaderConstants { // Push constants passed to the vertex shader
    float model[16];
    float view[16];
    float projection[16];
  };

  struct VertexInfo {
    float position[3];
    float normal[3];
    float uv[2];

    static VertexInfoDescription get_vertex_info_description();
  };

  Path _mesh_path;
  Shader _shader;
  std::vector<VertexInfo> _vertices;

  VertexInfoDescription _vertex_info_description;
  AllocatedBuffer _vertex_buffer;
  VkPushConstantRange _push_constant_range;

  Mesh();

  // Loads a mesh, it's shaders and fills the vertex info
  bool load(Path path, Path vert_shader = {}, Path frag_shader = {});
};