#version 450

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 normal;

layout(push_constant) uniform constants {
  mat4 model; // Identity
  mat4 view;  // Identity -10
  mat4 projection;
}
PushConstant;

void main() {
  mat4 model_view = PushConstant.view * PushConstant.model;
  gl_Position = PushConstant.projection * model_view * vec4(vPosition, 1.0f);

  mat3 rotation = mat3(model_view);
  normal = vec4(rotation * vNormal.xyz, 1.0);
  normal = normalize(normal);
}