#version 450

// output write
layout(location = 0) out vec4 outFragColor;
layout(location = 0) in vec3 inFragColor;

void main() { outFragColor = vec4(inFragColor, 1.0f); }