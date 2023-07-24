#version 450

layout(location = 0) out vec2 frag_uv;
// https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
void main() {
  frag_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  gl_Position = vec4(frag_uv * 2.0f - 1.0f, 0.0f, 1.0f);
}