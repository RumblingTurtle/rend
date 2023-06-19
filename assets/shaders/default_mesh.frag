#version 450

// output write
layout(location = 0) in vec4 vNormal;
layout(location = 0) out vec4 outFragColor;

void main() {
  float color = clamp(vNormal.z, 0.0f, 1.0f);
  outFragColor = vec4(vNormal.z, vNormal.z, vNormal.z, 1.0f);
}
