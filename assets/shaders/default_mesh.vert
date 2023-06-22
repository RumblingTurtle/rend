#version 450

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 normal;
layout(location = 1) out vec2 uvOut;

layout(set = 0, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
}
c_data;

layout(push_constant) uniform PushConstants { mat4 model; }
push_constants;

void main() {
  mat4 model_view = c_data.view * push_constants.model;
  gl_Position = c_data.projection * model_view * vec4(vPosition, 1.0f);

  mat3 rotation = mat3(model_view);
  normal = vec4(rotation * vNormal.xyz, 1.0);
  normal = normalize(normal);
  uvOut = uv;
}