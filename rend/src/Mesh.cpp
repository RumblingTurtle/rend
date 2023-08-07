#include <rend/Rendering/Vulkan/Mesh.h>

Mesh::Mesh(Path path) {
  if (path.native().size() == 0) {
    throw std::runtime_error("Mesh path is empty");
  }
  if (!std::filesystem::exists(path.native())) {
    throw std::runtime_error("Mesh path does not exist");
  }

  _mesh_path = path;

  Assimp::Importer importer;
  const aiScene *scene =
      importer.ReadFile(path.native(), aiProcess_Triangulate);

  if (scene || scene->mNumMeshes == 0) {
    const int num_meshes = scene->mNumMeshes;
    _vertex_count = 0;
    for (int i = 0; i < num_meshes; i++) {
      const aiMesh *mesh = scene->mMeshes[i];
      int prev_size = _vertices.size();
      _vertices.resize(_vertices.size() + mesh->mNumFaces * 24);
      _vertex_count += mesh->mNumFaces * 3;
      for (int f = 0; f < mesh->mNumFaces; f++) {
        for (int v = 0; v < 3; v++) {
          int v_idx = mesh->mFaces[f].mIndices[v];
          _vertices[prev_size + f * 24 + v * 8 + 0] = mesh->mVertices[v_idx].x;
          _vertices[prev_size + f * 24 + v * 8 + 1] = mesh->mVertices[v_idx].y;
          _vertices[prev_size + f * 24 + v * 8 + 2] = mesh->mVertices[v_idx].z;
          _vertices[prev_size + f * 24 + v * 8 + 3] = mesh->mNormals[v_idx].x;
          _vertices[prev_size + f * 24 + v * 8 + 4] = mesh->mNormals[v_idx].y;
          _vertices[prev_size + f * 24 + v * 8 + 5] = mesh->mNormals[v_idx].z;
          _vertices[prev_size + f * 24 + v * 8 + 6] =
              mesh->mTextureCoords[0][v_idx].x;
          _vertices[prev_size + f * 24 + v * 8 + 7] =
              1 - mesh->mTextureCoords[0][v_idx].y;
        }
      }
    }

  } else {
    throw std::runtime_error("Error loading mesh: " + path.native());
  }
}

int Mesh::vertex_count() const { return _vertex_count; }

void Mesh::generate_allocation_buffer(VmaAllocator &allocator,
                                      Deallocator &deallocator_queue) {
  if (buffer_allocation.buffer_allocated) {
    throw std::runtime_error("Mesh allocation buffer already generated");
  }

  buffer_allocation = BufferAllocation::create(
      _vertices.size() * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, allocator);

  // Destroy allocated buffer
  deallocator_queue.push([=]() { buffer_allocation.destroy(); });

  buffer_allocation.copy_from(_vertices.data(),
                              _vertices.size() * sizeof(float));
}
