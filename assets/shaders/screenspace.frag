#version 450

layout(location = 0) in vec2 screen_uv;
layout(location = 0) out vec4 out_occlusion;
layout(location = 1) out vec4 out_reflection;

layout(set = 1, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
  vec4 position;
}
camera_info;

layout(set = 1, binding = 3) uniform sampler2D world_normal_texture;
layout(set = 1, binding = 4) uniform sampler2D world_positions_texture;
layout(set = 1, binding = 5) uniform sampler2D albedo_texture;
layout(set = 1, binding = 6) uniform sampler2D depth_texture;

layout(push_constant) uniform PushConstants {
  mat4 model;
  int texture_index;
  int light_index;
  int bitmask;
}
push_constants;

#define REFLECTION_MARCHING_STEPS 128
#define REFLECTION_MARCHING_STEP_SIZE 0.05f
#define REFLECTION_BIAS 0.0001f
#define BIN_SEARCH_STEPS 16
#define MAX_DIST_TO_REFLECTION 6.0f

vec4 get_reflection_albedo() {
  vec4 frag_world_normal = texture(world_normal_texture, screen_uv);
  vec4 frag_world_pos = texture(world_positions_texture, screen_uv);

  vec3 view_dir = normalize(frag_world_pos.xyz - camera_info.position.xyz);
  vec3 world_reflection_dir =
      reflect(view_dir, normalize(frag_world_normal.xyz));
  mat4 VP_matrix = camera_info.projection * camera_info.view;

  int is_reflective = int(frag_world_normal.a) & 0x1;
  float start_depth = texture(depth_texture, screen_uv).r;
  for (int i = 1; i < REFLECTION_MARCHING_STEPS * is_reflective; i++) {
    vec3 ray_pos = frag_world_pos.xyz + world_reflection_dir * float(i) *
                                            REFLECTION_MARCHING_STEP_SIZE;
    vec4 projected_point = VP_matrix * vec4(ray_pos, 1.0);
    projected_point.xyz /= projected_point.w;

    vec2 texture_pos = projected_point.xy * 0.5f + 0.5f;

    if (texture_pos.x < 0.0 || texture_pos.x > 1.0 || texture_pos.y < 0.0 ||
        texture_pos.y > 1.0) {
      break;
    }

    if (projected_point.z > texture(depth_texture, texture_pos).r) {
      if (start_depth >
          texture(depth_texture, texture_pos).r - REFLECTION_BIAS) {
        return vec4(0.0);
      }
      // Binary search for the exact point of intersection
      vec4 projected_mid_point;
      float dist_to_frag = length(ray_pos - frag_world_pos.xyz);
      for (int j = 0; j < BIN_SEARCH_STEPS; j++) {
        vec3 lower_bound =
            ray_pos - world_reflection_dir * REFLECTION_MARCHING_STEP_SIZE;
        vec3 upper_bound = ray_pos;
        vec3 mid_point = mix(lower_bound, upper_bound, 0.5f);
        dist_to_frag = length(mid_point - frag_world_pos.xyz);
        projected_mid_point = VP_matrix * vec4(mid_point, 1.0);
        projected_mid_point.xyz /= projected_mid_point.w;
        projected_mid_point.xy = projected_mid_point.xy * 0.5f + 0.5f;
        if (projected_mid_point.z >
            texture(depth_texture, projected_mid_point.xy).r) {
          upper_bound = mid_point;
        } else {
          lower_bound = mid_point;
        }
      }
      float attenuation =
          pow(1.0f - clamp(dist_to_frag, 0.0f, MAX_DIST_TO_REFLECTION) /
                         MAX_DIST_TO_REFLECTION,
              2);

      return texture(albedo_texture, projected_mid_point.xy) * attenuation;
    }
  }

  return vec4(0.0);
}

void main() {
  out_reflection = get_reflection_albedo();
  out_occlusion = vec4(1.0);
}
