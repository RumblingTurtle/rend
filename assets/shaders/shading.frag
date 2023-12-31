#version 450

layout(location = 0) in vec2 screen_uv;
layout(location = 0) out vec4 out_frag_color;

layout(set = 0, binding = 0) uniform sampler2D textures[10];

layout(set = 1, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
  vec4 position;
}
camera_info;

layout(set = 1, binding = 1) uniform LightSource {
  vec4 position;
  vec4 color;
  vec4 direction;
  mat4 view_matrix;
  mat4 projection_matrix;
}
light_sources[64];

layout(set = 1, binding = 2) uniform sampler2D shadow_texture;
layout(set = 1, binding = 3) uniform sampler2D world_normal;
layout(set = 1, binding = 4) uniform sampler2D world_positions;
layout(set = 1, binding = 5) uniform sampler2D albedo_texture;
layout(set = 1, binding = 6) uniform sampler2D depth;

layout(push_constant) uniform PushConstants {
  mat4 model;
  int texture_index;
  int light_index;
  int bitmask;
}
push_constants;

vec3 ambient = vec3(0.001f);
#define DEPTH_BIAS 0.00001f
#define SHADOW_MAP_RESOLUTION 1024
#define SHADOW_ATLAS_SIZE 8192
#define SHADOW_MAP_SCALE SHADOW_MAP_RESOLUTION / SHADOW_ATLAS_SIZE
#define MAX_LIGHTS 64

float shadow_test(int light_idx, vec4 frag_pos_world) {
  float shadow = 1.0f;
  vec4 light_mvp_projection = light_sources[light_idx].projection_matrix *
                              light_sources[light_idx].view_matrix *
                              vec4(frag_pos_world.xyz, 1.0);

  vec3 shadow_coords = light_mvp_projection.xyz / light_mvp_projection.w;
  vec2 atlas_offset = vec2(light_idx % 8, light_idx / 8) * SHADOW_MAP_SCALE;
  vec2 shadow_coords_renormalized =
      (shadow_coords.xy * 0.5f + 0.5f) * SHADOW_MAP_SCALE;

  if (shadow_coords.z > -1.0f && shadow_coords.z < 1.0f &&
      shadow_coords_renormalized.x > 0.0f &&
      shadow_coords_renormalized.x < 1.0f &&
      shadow_coords_renormalized.y > 0.0f &&
      shadow_coords_renormalized.y < 1.0f) {
    float closest_depth =
        texture(shadow_texture, shadow_coords_renormalized + atlas_offset).r;
    float current_depth = shadow_coords.z;
    shadow = (current_depth - DEPTH_BIAS) > closest_depth ? 0.0f : 1.0f;
  }
  return shadow;
}

vec3 calculate_light_contrib(int light_idx, vec3 view_dir,
                             vec4 frag_normal_world, vec4 frag_pos_world) {
  float light_intensity = light_sources[light_idx].color.w;
  vec3 light_color = light_sources[light_idx].color.xyz * light_intensity;
  float light_fov = light_sources[light_idx].position.w;
  vec3 light_dir = light_sources[light_idx].direction.xyz;
  mat4 light_view = light_sources[light_idx].view_matrix;

  vec3 light_to_frag =
      frag_pos_world.xyz - light_sources[light_idx].position.xyz;
  vec3 light_to_frag_normalized = normalize(light_to_frag);

  float diffuse_intensity = max(dot(frag_normal_world.xyz, -light_dir), 0.0);
  vec3 diffuse = diffuse_intensity * light_color;

  vec3 reflected_dir =
      reflect(-light_to_frag_normalized, frag_normal_world.xyz);
  vec3 specular =
      20 * pow(max(dot(-view_dir, reflected_dir), 0.0), 32) * light_color;

  float distance = length(light_to_frag);

  float theta_cosine = dot(light_to_frag_normalized, light_dir);
  float half_fov = cos(light_fov / 2);

  if (theta_cosine < half_fov) {
    return vec3(0);
  }

  float soft_cutoff =
      pow(clamp((theta_cosine - half_fov) / (0.95 - half_fov), 0.0, 1.0), 4);

  float attenuation =
      soft_cutoff * 1.0f /
      (1.0f + 0.001 * distance + 0.0001 * (distance * distance));

  return diffuse * attenuation + specular * attenuation;
}

void main() {
  vec3 view_dir = normalize(camera_info.view[2].xyz);

  vec4 frag_normal_world = texture(world_normal, screen_uv);
  vec4 frag_pos_world = texture(world_positions, screen_uv);
  float frag_depth = texture(depth, screen_uv).r;
  vec4 frag_color = texture(albedo_texture, screen_uv);

  vec4 light_contrib = vec4(0);
  for (int i = 0; i < 64; i++) {
    if (light_sources[i].color.w < 0) {
      continue;
    }
    float is_lit = shadow_test(i, frag_pos_world);
    light_contrib.xyz += calculate_light_contrib(i, view_dir, frag_normal_world,
                                                 frag_pos_world) *
                         is_lit;
    light_contrib.a += is_lit;
  }
  light_contrib.a = clamp(light_contrib.a, 0, 1);

  out_frag_color =
      vec4(ambient + (light_contrib.xyz * frag_color.xyz) * light_contrib.a, 1);
}
