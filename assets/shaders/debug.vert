#version 450

layout(location = 0) in vec3 vPosition;

layout(set = 0, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
  vec4 position;
}
c_data;

layout(push_constant) uniform PushConstants { mat4 model; }
push_constants;

void main() {
  gl_Position = c_data.projection * c_data.view * push_constants.model *
                vec4(vPosition, 1.0f);
}