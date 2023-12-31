#version 450

layout(location = 0) in vec3 vert_pos;

layout(set = 0, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
}
camera_info;

layout(set = 0, binding = 1) uniform LightSource {
  vec4 position;
  vec4 color;
  vec4 direction;
  mat4 view_matrix;
  mat4 projection_matrix;
}
light_sources[64];

layout(push_constant) uniform PushConstants {
  mat4 model;
  int texture_index;
  int light_index;
  int bitmask;
}
push_constants;
void main() {
  gl_Position = light_sources[push_constants.light_index].projection_matrix *
                light_sources[push_constants.light_index].view_matrix *
                push_constants.model * vec4(vert_pos, 1.0f);
}