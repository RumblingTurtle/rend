#version 450

layout(location = 0) in vec3 vert_normal_world;
layout(location = 1) in vec3 vert_pos_world;
layout(location = 2) in vec2 vert_uv;

layout(location = 0) out vec4 frag_normal_world;
layout(location = 1) out vec4 frag_pos_world;
layout(location = 2) out vec4 frag_albedo;

layout(push_constant) uniform PushConstants {
  mat4 model;
  int texture_index;
  int light_index;
}
push_constants;

layout(set = 0, binding = 0) uniform sampler2D textures[10];

void main() {
  if (push_constants.texture_index <= 0) {
    discard;
  }

  frag_normal_world = vec4(vert_normal_world, 1.0);
  frag_pos_world = vec4(vert_pos_world, 1.0);
  frag_albedo = texture(textures[push_constants.texture_index - 1], vert_uv);
}
