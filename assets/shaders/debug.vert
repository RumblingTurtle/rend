#version 450

layout(location = 0) in vec3 vert_pos;
layout(location = 1) in vec3 vert_color;
layout(location = 0) out vec3 out_vert_color;

layout(set = 0, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
  vec4 position;
}
c_data;

void main() {
  gl_Position = c_data.projection * c_data.view * vec4(vert_pos, 1.0f);
  out_vert_color = vert_color;
}