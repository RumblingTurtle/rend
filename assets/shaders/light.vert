#version 450

layout(location = 0) in vec3 vert_pos;
layout(location = 1) in vec3 vert_normal;
layout(location = 2) in vec2 vert_uv;

layout(set = 1, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
}
c_data;

layout(push_constant) uniform PushConstants {
  mat4 model;
  int texture_index;
  int light_index;
}
push_constants;

void main() {
  gl_Position = c_data.projection * c_data.view * push_constants.model *
                vec4(vert_pos, 1.0f);
}