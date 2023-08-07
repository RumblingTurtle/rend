#pragma once
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>
#include <lodepng.h>
#include <rend/Rendering/Vulkan/Mesh.h>
#include <rend/Rendering/Vulkan/Texture.h>

namespace rend {
struct AssetImporter {
  Path scene_path;
  Assimp::Importer importer;

  AssetImporter(const Path &path) : scene_path(path) {
    if (!std::filesystem::exists(path)) {
      throw std::runtime_error("AssetImporter: File does not exist");
    }
    const aiScene *scene =
        importer.ReadFile(scene_path.native(), aiProcess_EmbedTextures);
    if (scene == nullptr) {
      throw std::runtime_error("AssetImporter: Failed to load scene");
    }
  }

  int get_mesh_count() {
    const aiScene *p_scene = importer.GetScene();
    return p_scene->mNumMeshes;
  }

  int get_material_count() {
    const aiScene *p_scene = importer.GetScene();
    return p_scene->mNumMaterials;
  }

  int get_texture_count() {
    const aiScene *p_scene = importer.GetScene();
    return p_scene->mNumTextures;
  }

  std::vector<float> get_packed_mesh(int mesh_index) {
    const aiScene *p_scene = importer.GetScene();
    if (p_scene->mNumMeshes == 0) {
      std::cerr << "No meshes in the scene" << scene_path << std::endl;
    }

    const aiMesh *mesh = p_scene->mMeshes[mesh_index];
    std::vector<float> vertices(mesh->mNumFaces * 24);
    for (int f = 0; f < mesh->mNumFaces; f++) {
      for (int v = 0; v < 3; v++) {
        int v_idx = mesh->mFaces[f].mIndices[v];
        vertices[f * 24 + v * 8 + 0] = mesh->mVertices[v_idx].x;
        vertices[f * 24 + v * 8 + 1] = mesh->mVertices[v_idx].y;
        vertices[f * 24 + v * 8 + 2] = mesh->mVertices[v_idx].z;
        vertices[f * 24 + v * 8 + 3] = mesh->mNormals[v_idx].x;
        vertices[f * 24 + v * 8 + 4] = mesh->mNormals[v_idx].y;
        vertices[f * 24 + v * 8 + 5] = mesh->mNormals[v_idx].z;
        vertices[f * 24 + v * 8 + 6] = mesh->mTextureCoords[0][v_idx].x;
        vertices[f * 24 + v * 8 + 7] = 1 - mesh->mTextureCoords[0][v_idx].y;
      }
    }
    return vertices;
  }

  int get_mesh_index(std::string mesh_name) {
    const aiScene *p_scene = importer.GetScene();
    if (p_scene->mNumMeshes == 0) {
      throw std::runtime_error("No meshes in the scene");
    }
    for (int i = 0; i < p_scene->mNumMeshes; i++) {
      const aiMesh *mesh = p_scene->mMeshes[i];
      if (mesh->mName.C_Str() == mesh_name) {
        return i;
      }
    }
    return -1;
  }

  Texture get_mesh_texture(std::string mesh_name, int texture_index) {
    return get_mesh_texture(get_mesh_index(mesh_name), texture_index);
  }

  Texture get_mesh_texture(int mesh_index, int texture_index) {
    const aiScene *p_scene = importer.GetScene();
    if (p_scene->mNumMeshes < mesh_index) {
      throw std::runtime_error("No meshes in the scene");
    }
    const aiMesh *mesh = p_scene->mMeshes[mesh_index];
    const aiMaterial *material = p_scene->mMaterials[mesh->mMaterialIndex];

    if (material->GetTextureCount(aiTextureType_DIFFUSE) < texture_index) {
      throw std::runtime_error("No diffuse textures in a material");
    }

    aiString path;
    if (material->GetTexture(aiTextureType_DIFFUSE, texture_index, &path) !=
        aiReturn_SUCCESS) {
      throw std::runtime_error("Failed to get texture");
    }

    const aiTexture *texture = p_scene->GetEmbeddedTexture(path.C_Str());

    if (texture->mHeight == 0 && texture->CheckFormat("png")) {
      int buffer_size = 0;
      buffer_size = texture->mWidth;
      std::vector<unsigned char> data(buffer_size);
      memcpy(data.data(), texture->pcData, buffer_size);
      unsigned int width, height;
      std::vector<unsigned char> decoded_data = decode_png(data, width, height);
      PixelBuffer pixel_buffer{decoded_data, static_cast<int>(width),
                               static_cast<int>(height)};

      return pixel_buffer;
    }
    throw std::runtime_error("Unsupported texture format");
  }

  std::vector<unsigned char> decode_png(std::vector<unsigned char> &in_buffer,
                                        unsigned int &width,
                                        unsigned int &height) {
    std::vector<unsigned char> out_buffer;
    unsigned int error = lodepng::decode(out_buffer, width, height, in_buffer);
    if (error) {
      throw std::runtime_error("Failed to decode PNG");
    }
    return out_buffer;
  }
};
} // namespace rend