#include <Mesh.h>

bool Mesh::load(Path path) {
  if (path.native().size() == 0) {
    std::cerr << "Mesh path is empty" << std::endl;
    return false;
  }
  _mesh_path = path;

  Assimp::Importer importer;
  const aiScene *scene =
      importer.ReadFile(path.native(), aiProcess_Triangulate);

  if (scene || scene->mNumMeshes == 0) {
    const aiMesh *mesh = scene->mMeshes[0];
    _vertices.resize(mesh->mNumFaces * 24);
    _vertex_count = mesh->mNumFaces * 3;
    for (int f = 0; f < mesh->mNumFaces; f++) {
      for (int v = 0; v < 3; v++) {
        int v_idx = mesh->mFaces[f].mIndices[v];
        _vertices[f * 24 + v * 8 + 0] = mesh->mVertices[v_idx].x;
        _vertices[f * 24 + v * 8 + 1] = mesh->mVertices[v_idx].y;
        _vertices[f * 24 + v * 8 + 2] = mesh->mVertices[v_idx].z;
        _vertices[f * 24 + v * 8 + 3] = mesh->mNormals[v_idx].x;
        _vertices[f * 24 + v * 8 + 4] = mesh->mNormals[v_idx].y;
        _vertices[f * 24 + v * 8 + 5] = mesh->mNormals[v_idx].z;
        _vertices[f * 24 + v * 8 + 6] = mesh->mTextureCoords[0][v_idx].x;
        _vertices[f * 24 + v * 8 + 7] = mesh->mTextureCoords[0][v_idx].y;
      }
    }
    return true;
  }

  std::cerr << "Error loading mesh: " << path << std::endl;
  return false;
}

int Mesh::vertex_count() { return _vertex_count; }

bool Mesh::generate_allocation_buffer(VmaAllocator &allocator,
                                      Deallocator &deallocator_queue) {
  if (_buffer_generated) {
    return true;
  }
  buffer_allocation = BufferAllocation::create(
      _vertices.size() * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU, allocator);

  // Destroy allocated buffer
  deallocator_queue.push([=]() { buffer_allocation.destroy(); });

  void *data;
  vmaMapMemory(allocator, buffer_allocation.allocation, &data);
  memcpy(data, _vertices.data(), _vertices.size() * sizeof(float));
  vmaUnmapMemory(allocator, buffer_allocation.allocation);
  _buffer_generated = true;
  return true;
}
