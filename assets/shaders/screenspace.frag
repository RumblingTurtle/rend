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

layout(set = 1, binding = 1) uniform sampler2D world_normal_texture;
layout(set = 1, binding = 2) uniform sampler2D world_positions_texture;
layout(set = 1, binding = 3) uniform sampler2D albedo_texture;
layout(set = 1, binding = 4) uniform sampler2D depth_texture;
layout(set = 1, binding = 5) uniform sampler2D shaded_texture;

layout(push_constant) uniform PushConstants {
  mat4 model;
  int texture_index;
  int light_index;
  int bitmask;
}
push_constants;

#define REFLECTION_MARCHING_STEPS 256
#define REFLECTION_MARCHING_STEP_SIZE 0.1f
#define BIN_SEARCH_STEPS 16
#define SURFACE_THICKNESS 0.00001f

mat4 VP_matrix = camera_info.projection * camera_info.view;
vec3 get_ndc_coords(vec3 world_coords) {
  vec4 projected_point = VP_matrix * vec4(world_coords, 1.0);
  projected_point.xyz /= projected_point.w;
  return projected_point.xyz;
}

bool z_test(float z1, float z2) {
  float z_dist = z1 - z2;
  return SURFACE_THICKNESS > z_dist && z_dist > 0;
}

vec4 get_reflection_albedo() {
  vec4 frag_world_normal = texture(world_normal_texture, screen_uv);
  vec4 frag_world_pos = texture(world_positions_texture, screen_uv);
  vec3 frag_pos_screen_space = get_ndc_coords(frag_world_pos.xyz);
  vec3 view_dir = normalize(frag_world_pos.xyz - camera_info.position.xyz);
  vec3 world_reflection_dir =
      reflect(view_dir, normalize(frag_world_normal.xyz));

  int is_reflective = int(frag_world_normal.a) & 0x1;
  for (int i = 1; i < REFLECTION_MARCHING_STEPS * is_reflective; i++) {
    vec3 ray_pos = frag_world_pos.xyz + world_reflection_dir * float(i) *
                                            REFLECTION_MARCHING_STEP_SIZE;
    vec3 projected_point = get_ndc_coords(ray_pos);

    vec2 texture_pos = projected_point.xy * 0.5f + 0.5f;

    if (texture_pos.x < 0.0 || texture_pos.x > 1.0 || texture_pos.y < 0.0 ||
        texture_pos.y > 1.0 || projected_point.z > 1.0) {
      break;
    }

    float hit_depth = texture(depth_texture, texture_pos).r;

    if (z_test(projected_point.z, hit_depth)) {
      // Binary search for the exact point of intersection
      vec3 projected_mid_point;

      for (int j = 0; j < BIN_SEARCH_STEPS; j++) {
        vec3 lower_bound =
            ray_pos - world_reflection_dir * REFLECTION_MARCHING_STEP_SIZE;
        vec3 upper_bound = ray_pos;
        vec3 mid_point = mix(lower_bound, upper_bound, 0.5f);
        projected_mid_point = get_ndc_coords(mid_point);
        projected_mid_point.xy = projected_mid_point.xy * 0.5f + 0.5f;
        if (z_test(projected_mid_point.z,
                   texture(depth_texture, projected_mid_point.xy).r)) {
          upper_bound = mid_point;
        } else {
          lower_bound = mid_point;
        }
      }

      return texture(shaded_texture, projected_mid_point.xy);
    }
  }

  return vec4(0.0);
}

void main() {
  out_reflection = get_reflection_albedo();
  out_occlusion = vec4(1.0);
}
