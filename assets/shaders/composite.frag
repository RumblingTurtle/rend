#version 450

layout(location = 0) in vec2 screen_uv;
layout(location = 0) out vec4 out_frag_color;

layout(set = 0, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
  vec4 position;
}
camera_info;

layout(set = 1, binding = 0) uniform sampler2D shadow_texture;
layout(set = 1, binding = 1) uniform sampler2D world_normal;
layout(set = 1, binding = 2) uniform sampler2D world_positions;
layout(set = 1, binding = 3) uniform sampler2D albedo_texture;
layout(set = 1, binding = 4) uniform sampler2D depth;
layout(set = 1, binding = 5) uniform sampler2D shading;
layout(set = 1, binding = 6) uniform sampler2D occlusion;
layout(set = 1, binding = 7) uniform sampler2D reflection;

layout(push_constant) uniform PushConstants {
  mat4 model;
  int texture_index;
  int light_index;
  int bitmask;
}
push_constants;

mat3 SMOOTHING_KERNEL = {{1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0},
                         {2.0 / 16.0, 4.0 / 16.0, 2.0 / 16.0},
                         {1.0 / 16.0, 2.0 / 16.0, 1.0 / 16.0}};

float convolve(vec2 uv, mat3 convolution_mat, int channel, sampler2D tex) {

  vec2 pixel_size = 1.0 / textureSize(tex, 0);
  float convolution_sum = 0.0;
  for (int i = -1; i < 2; i++) {
    for (int j = -1; j < 2; j++) {
      vec2 neightbour_uv = uv + vec2(i, j) / pixel_size;
      if (neightbour_uv.x < 0 || neightbour_uv.x > 1 || neightbour_uv.y < 0 ||
          neightbour_uv.y > 1) {
        continue;
      }
      convolution_sum += SMOOTHING_KERNEL[i + 1][j + 1] *
                         texture(tex, uv + vec2(i, j) / pixel_size).r;
    }
  }
  return convolution_sum;
}

void main() {
  vec4 reflection =
      vec4(convolve(screen_uv, SMOOTHING_KERNEL, 0, reflection),
           convolve(screen_uv, SMOOTHING_KERNEL, 1, reflection),
           convolve(screen_uv, SMOOTHING_KERNEL, 2, reflection), 1.0f);
  out_frag_color = vec4(texture(shading, screen_uv).xyz, 1);
  if (texture(occlusion, screen_uv).r > 0) {
    out_frag_color = mix(out_frag_color, reflection, 0.5f);
  }
  gl_FragDepth = texture(depth, screen_uv).r;
}
