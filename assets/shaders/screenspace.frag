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

#define MARCHING_MAX_DIST 100.0f
#define REFLECTION_MARCHING_STEP_COUNT 128
#define BIN_SEARCH_STEPS 32
#define SURFACE_THICKNESS 0.0002f
#define MIN_DISTANCE_TO_REFLECTION 20.0f

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
  vec3 view_vector = frag_world_pos.xyz - camera_info.position.xyz;
  vec3 view_dir = normalize(view_vector);
  vec3 world_reflection_dir =
      reflect(view_dir, normalize(frag_world_normal.xyz));

  vec3 ray_start_pos = get_ndc_coords(frag_world_pos.xyz);
  ray_start_pos.xy = ray_start_pos.xy * 0.5f + 0.5f;

  vec3 ray_end_pos = get_ndc_coords(frag_world_pos.xyz +
                                    world_reflection_dir * MARCHING_MAX_DIST);
  ray_end_pos.xy = clamp(ray_end_pos.xy * 0.5f + 0.5f, 0.0f, 1.0f);

  vec3 ray_length = ray_end_pos - ray_start_pos;
  vec3 ray_dir = normalize(ray_length);
  float step_size = max(length(ray_length) / REFLECTION_MARCHING_STEP_COUNT,
                        1.0f / textureSize(depth_texture, 0).x);
  vec3 step_vector = ray_dir * step_size;

  int is_reflective = int(frag_world_normal.a) & 0x1;
  for (int i = 1; i < REFLECTION_MARCHING_STEP_COUNT * is_reflective; i++) {
    vec3 ray_pos = ray_start_pos.xyz + step_vector * float(i);

    if (ray_pos.x < 0.0 || ray_pos.x > 1.0 || ray_pos.y < 0.0 ||
        ray_pos.y > 1.0) {
      break;
    }

    if (z_test(ray_pos.z, texture(depth_texture, ray_pos.xy).r)) {
      // Binary search for the exact point of intersection
      vec3 mid_point;
      vec3 lower_bound = ray_pos - step_vector;
      vec3 upper_bound = ray_pos;
      for (int j = 0; j < BIN_SEARCH_STEPS; j++) {
        mid_point = mix(lower_bound, upper_bound, 0.5f);
        if (z_test(mid_point.z, texture(depth_texture, mid_point.xy).r)) {
          upper_bound = mid_point;
        } else {
          lower_bound = mid_point;
        }
      }
      float attenuation =
          pow(min(length(view_vector), MIN_DISTANCE_TO_REFLECTION) /
                  MIN_DISTANCE_TO_REFLECTION,
              4);
      return vec4(texture(shaded_texture, mid_point.xy).xyz, 1) * attenuation;
    }
  }

  return vec4(0.0);
}

#define SSAO_SAMPLE_COUNT 32
#define SSAO_RADIUS 0.5f
#define SSAO_BIAS 0.000001f

float random(vec2 st) {
  return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

vec4 get_occlusion_ratio() {
  vec3 frag_position = texture(world_positions_texture, screen_uv).xyz;
  vec3 normal = texture(world_normal_texture, screen_uv).xyz;
  vec3 tangent = normalize(cross(normal, vec3(1.0, 0.0, 0.0)));
  vec3 bitangent = normalize(cross(normal, tangent));
  mat3 TBN = mat3(tangent, bitangent, normal);
  float occluded_samples = 0;
  for (int i = 0; i < SSAO_SAMPLE_COUNT; i++) {
    vec3 random_sample =
        vec3(random(bitangent.xy * vec2(12 * (i + 1)) +
                    frag_position.xy * vec2(i + 1)),
             random(tangent.yz * vec2(3 * i) + frag_position.zx * vec2(i + 1)),
             random(normal.zx * vec2(35 * i) + frag_position.xy * vec2(i + 1)));

    vec3 sample_pos = frag_position + TBN * random_sample * SSAO_RADIUS;
    vec3 sample_uv = get_ndc_coords(sample_pos);
    float sample_depth = texture(depth_texture, sample_uv.xy * 0.5f + 0.5f).r;
    if (SSAO_BIAS > (sample_uv.z - sample_depth)) {
      occluded_samples += 1;
    }
  }
  return vec4(occluded_samples / SSAO_SAMPLE_COUNT);
}

void main() {
  out_reflection = get_reflection_albedo();
  out_occlusion = get_occlusion_ratio();
}
