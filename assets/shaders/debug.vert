#version 450

layout(location = 0) in vec3 vert_pos;
layout(location = 1) in vec3 vert_color;
layout(location = 0) out vec3 out_vert_color;

layout(set = 0, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
  vec4 position;
}
camera_info;

void main() {
  gl_Position =
      camera_info.projection * camera_info.view * vec4(vert_pos, 1.0f);
  out_vert_color = vert_color;
}