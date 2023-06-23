#version 450

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;

layout(location = 0) out vec2 uvOut;
layout(location = 1) out vec4 fragNormalWorld;
layout(location = 2) out vec4 fragPosWorld;

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
  uvOut = vUV;
  fragNormalWorld = normalize(push_constants.model * vec4(vNormal.xyz, 0.0f));
  fragPosWorld = push_constants.model * vec4(vPosition.xyz, 1.0f);
}