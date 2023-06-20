#include <Mesh.h>

Mesh::Mesh() {
  _vertex_info_description = VertexInfo::get_vertex_info_description();

  _push_constant_range.offset = 0;
  _push_constant_range.size = sizeof(ShaderConstants);
  // Accessible only in the vertex shader
  _push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
}

bool Mesh::load(Path path, Path vert_shader, Path frag_shader) {
  if (path.native().size() == 0) {
    std::cerr << "Mesh path is empty" << std::endl;
    return false;
  }
  _mesh_path = path;

  if (vert_shader.native().size() != 0 && frag_shader.native().size() != 0) {
    _shader = Shader(vert_shader, frag_shader);
  }

  Assimp::Importer importer;
  scene = importer.ReadFile(path.native(), aiProcess_Triangulate);

  uint32_t mNumMeshes = scene->mNumMeshes;

  // If the import failed, report it
  if (scene || mNumMeshes == 0) {
    mesh = scene->mMeshes[0];
    _vertices.resize(mesh->mNumFaces * 3);

    for (int f = 0; f < mesh->mNumFaces; f++) {
      for (int v = 0; v < 3; v++) {
        int v_idx = mesh->mFaces[f].mIndices[v];
        VertexInfo vertex;
        vertex.position[0] = mesh->mVertices[v_idx].x;
        vertex.position[1] = mesh->mVertices[v_idx].y;
        vertex.position[2] = mesh->mVertices[v_idx].z;

        vertex.normal[0] = mesh->mNormals[v_idx].x;
        vertex.normal[1] = mesh->mNormals[v_idx].y;
        vertex.normal[2] = mesh->mNormals[v_idx].z;

        vertex.uv[0] = mesh->mTextureCoords[0][v_idx].x;
        vertex.uv[1] = mesh->mTextureCoords[0][v_idx].y;

        _vertices.push_back(vertex);
      }
    }
    return true;
  }

  std::cerr << "Error loading mesh: " << path << std::endl;
  return false;
}

VertexInfoDescription Mesh::VertexInfo::get_vertex_info_description() {
  VertexInfoDescription description;
  VkVertexInputBindingDescription mainBinding = {};
  mainBinding.binding = 0;
  mainBinding.stride = sizeof(VertexInfo);
  mainBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  description.bindings.push_back(mainBinding);

  // Position will be stored at Location 0
  VkVertexInputAttributeDescription position_attr = {};
  position_attr.binding = 0;
  position_attr.location = 0;
  position_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
  position_attr.offset = offsetof(VertexInfo, position);

  // Normal will be stored at Location 1
  VkVertexInputAttributeDescription normal_attr = {};
  normal_attr.binding = 0;
  normal_attr.location = 1;
  normal_attr.format = VK_FORMAT_R32G32B32_SFLOAT;
  normal_attr.offset = offsetof(VertexInfo, normal);

  // UV coords will be stored at Location 2
  VkVertexInputAttributeDescription uv_attr = {};
  uv_attr.binding = 0;
  uv_attr.location = 2;
  uv_attr.format = VK_FORMAT_R32G32_SFLOAT;
  uv_attr.offset = offsetof(VertexInfo, uv);

  description.attributes.push_back(position_attr);
  description.attributes.push_back(normal_attr);
  description.attributes.push_back(uv_attr);
  return description;
}