#version 450

layout(location = 0) in vec3 vert_pos;
layout(location = 1) in vec3 vert_normal;
layout(location = 2) in vec2 vert_uv;

layout(location = 0) out vec3 frag_normal_world;
layout(location = 1) out vec3 frag_pos_world;
layout(location = 2) out vec2 frag_uv;

layout(set = 1, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
  vec4 position;
}
camera_info;

layout(push_constant) uniform PushConstants {
  mat4 model;
  int texture_index;
  int light_index;
}
push_constants;

void main() {
  gl_Position = camera_info.projection * camera_info.view *
                push_constants.model * vec4(vert_pos, 1.0f);
  frag_normal_world =
      normalize(push_constants.model * vec4(vert_normal.xyz, 0.0f)).xyz;
  frag_pos_world = (push_constants.model * vec4(vert_pos.xyz, 1.0f)).xyz;
  frag_uv = vert_uv;
}