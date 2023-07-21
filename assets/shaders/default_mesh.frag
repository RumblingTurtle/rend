#version 450

layout(location = 0) in vec2 vert_uv;
layout(location = 1) in vec4 frag_normal_world;
layout(location = 2) in vec4 frag_pos_world;

layout(location = 0) out vec4 out_frag_color;

layout(set = 0, binding = 0) uniform sampler2D textures[10];

layout(set = 1, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
  vec4 position;
}
c_data;

layout(set = 1, binding = 1) uniform LightSource {
  vec4 position;
  vec4 color;
  vec4 direction;
  mat4 view_matrix;
  mat4 projection_matrix;
}
light_sources[64];

layout(push_constant) uniform PushConstants {
  mat4 model;
  int texture_index;
  int light_index;
}
push_constants;

layout(set = 1, binding = 2) uniform sampler2D shadow_texture;

vec3 ambient = vec3(0.001f);
#define DEPTH_BIAS 0.0001f
#define SHADOW_MAP_RESOLUTION 1024
#define SHADOW_ATLAS_SIZE 8192
#define SHADOW_MAP_SCALE SHADOW_MAP_RESOLUTION / SHADOW_ATLAS_SIZE
#define MAX_LIGHTS 64

float shadow_test(int light_idx) {
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
    shadow = current_depth - DEPTH_BIAS > closest_depth ? 0.0f : 1.0f;
  }
  return shadow;
}

vec3 calculate_light_contrib(int light_idx, vec3 view_dir) {
  vec3 light_color = light_sources[light_idx].color.xyz;
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
      soft_cutoff * shadow_test(light_idx) * 1.0f /
      (1.0f + 0.001 * distance + 0.0001 * (distance * distance));

  return ambient * attenuation + diffuse * attenuation + specular * attenuation;
}

void main() {
  vec4 frag_color = texture(textures[push_constants.texture_index], vert_uv);
  vec3 view_dir = normalize(c_data.view[2].xyz);
  vec3 out_color = vec3(0);
  for (int i = 0; i < 6; i++) {
    if (light_sources[i].color.w < 0) {
      continue;
    }
    out_color += calculate_light_contrib(i, view_dir);
  }

  out_frag_color = vec4(out_color * frag_color.xyz, 1);
}
