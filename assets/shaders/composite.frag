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
layout(set = 1, binding = 6) uniform sampler2D occlusion_texture;
layout(set = 1, binding = 7) uniform sampler2D reflection_texture;

layout(push_constant) uniform PushConstants {
  mat4 model;
  int texture_index;
  int light_index;
  int bitmask;
}
push_constants;

#define BLUR_WINDOW 9 // Pixels
int BLUR_RANGE = (BLUR_WINDOW - 1) / 2;

vec3 blur(vec2 uv, sampler2D tex) {
  vec2 pixel_size = 1.0f / textureSize(tex, 0);
  vec3 convolution_sum = vec3(0.0);
  int sample_count = 0;
  for (int i = -BLUR_RANGE; i < BLUR_RANGE; i++) {
    for (int j = -BLUR_RANGE; j < BLUR_RANGE; j++) {
      vec2 neightbour_uv = uv + vec2(i, j) / pixel_size;
      if (neightbour_uv.x < 0 || neightbour_uv.x > 1 || neightbour_uv.y < 0 ||
          neightbour_uv.y > 1) {
        continue;
      }
      convolution_sum += texture(tex, neightbour_uv).xyz;
      sample_count++;
    }
  }

  return (convolution_sum / float(sample_count) + 0.0001) *
         max(sample_count, 1);
}

void main() {
  out_frag_color.xyz =
      texture(shading, screen_uv).xyz * blur(screen_uv, occlusion_texture);
  out_frag_color.xyz =
      mix(out_frag_color.xyz, blur(screen_uv, reflection_texture),
          texture(reflection_texture, screen_uv).a * 0.2f);
  gl_FragDepth = texture(depth, screen_uv).r;
}
