#version 450

// output write
layout(location = 0) in vec4 vNormal;
layout(location = 1) in vec2 uvIn;

layout(location = 0) out vec4 outFragColor;

void main() {
  float color = clamp(vNormal.z, 0.0f, 1.0f);
  outFragColor = vec4(0, color, 0, 1.0f);
}

/*
layout(set = 1, binding = 0) uniform sampler2D tex;

void main() {
  vec3 color = texture(tex, uvIn).xyz;
  outFragColor = vec4(color, 1);
}
*/
