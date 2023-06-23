#version 450

// INS
layout(location = 0) in vec2 uvIn;
layout(location = 1) in vec4 fragNormalWorld;
layout(location = 2) in vec4 fragPosWorld;

// OUTS
layout(location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform CameraData {
  mat4 view;
  mat4 projection;
  vec4 position;
}
c_data;

layout(set = 0, binding = 1) uniform LightSource {
  vec3 position;
  vec4 color;
}
light_sources[6];

layout(set = 1, binding = 0) uniform sampler2D tex;

vec3 calculate_light_contrib(vec3 light_position, vec4 light_color,
                             vec3 frag_normal, vec3 frag_pos, vec4 frag_color,
                             vec3 view_dir) {
  if (light_color.w < 0) {
    return vec3(0);
  }

  float distance = length(light_position - fragPosWorld.xyz);

  vec3 light_to_frag = normalize(light_position - fragPosWorld.xyz);

  float diffuse_intensity = max(dot(fragNormalWorld.xyz, light_to_frag), 0.0);
  vec3 diffuse = diffuse_intensity * light_color.xyz;

  vec3 reflected_dir = reflect(light_to_frag, fragNormalWorld.xyz);
  float spec = pow(max(dot(view_dir, reflected_dir), 0.0), 32);
  vec3 specular = 0.5f * spec * light_color.xyz;

  float attenuation =
      1.0 / (1.0f + 0.007 * distance + 0.0002 * (distance * distance));

  vec3 ambient = vec3(0.05f, 0.05f, 0.05f);

  return (ambient * attenuation + diffuse * attenuation +
          specular * attenuation);
}

void main() {
  vec4 frag_color = texture(tex, uvIn);
  vec3 view_dir = normalize(c_data.view[3].xyz);
  vec3 out_color = calculate_light_contrib(
      light_sources[0].position.xyz, light_sources[0].color,
      fragNormalWorld.xyz, fragPosWorld.xyz, frag_color, view_dir);

  for (int i = 1; i < 6; i++) {
    out_color += calculate_light_contrib(
        light_sources[i].position, light_sources[i].color, fragNormalWorld.xyz,
        fragPosWorld.xyz, frag_color, view_dir);
  }

  outFragColor = vec4(out_color * frag_color.xyz, 1);
}
