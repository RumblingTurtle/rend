#pragma once
#include <Shader.h>
#include <macros.h>
#include <types.h>

#include <assimp/Importer.hpp>  // C++ importer interface
#include <assimp/postprocess.h> // Post processing flags
#include <assimp/scene.h>       // Output data structure

class Mesh {
  const aiScene *scene;
  const aiMesh *mesh;

public:
  Shader _shader;
  VertexInfoDescription _vertex_info_description;
  std::vector<VertexInfo> _vertices;
  AllocatedBuffer _vertex_buffer;
  Path _mesh_path;

  Mesh() {
    _vertex_info_description = VertexInfo::get_vertex_info_description();
  }

  bool load(Path path, Path vert_shader = {}, Path frag_shader = {}) {
    if (path.native().size() == 0) {
      std::cerr << "Mesh path is empty" << std::endl;
      return false;
    }
    _mesh_path = path;

    if (vert_shader.native().size() != 0 && frag_shader.native().size() != 0) {
      _shader = Shader(vert_shader, frag_shader);
    }

    Assimp::Importer importer;
    scene = importer.ReadFile(path.native(), aiProcess_SortByPType);

    uint32_t mNumMeshes = scene->mNumMeshes;

    // If the import failed, report it
    if (scene || mNumMeshes == 0) {
      mesh = scene->mMeshes[0];
      fill_vertex_info();
      return true;
    }

    std::cerr << "Error loading mesh: " << path << std::endl;
    return false;
  }

  bool fill_vertex_info() {
    for (int v = 0; v < mesh->mNumVertices; v++) {
      VertexInfo vertex;
      vertex.position[0] = mesh->mVertices[v].x;
      vertex.position[1] = mesh->mVertices[v].y;
      vertex.position[2] = mesh->mVertices[v].z;

      vertex.normal[0] = mesh->mNormals[v].x;
      vertex.normal[1] = mesh->mNormals[v].y;
      vertex.normal[2] = mesh->mNormals[v].z;

      vertex.uv[0] = mesh->mTextureCoords[0][v].x;
      vertex.uv[1] = mesh->mTextureCoords[0][v].y;

      _vertices.push_back(vertex);
    }
    return true;
  }
};