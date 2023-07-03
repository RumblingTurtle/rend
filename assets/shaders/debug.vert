#version 450

layout(location = 0) in vec3 vPosition;

layout(set = 0, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
  vec4 position;
}
c_data;

void main() {
  gl_Position = c_data.projection * c_data.view * vec4(vPosition, 1.0f);
}