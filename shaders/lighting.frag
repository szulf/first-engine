#version 330 core

struct Material
{
  vec3 ambient;
  vec3 diffuse;
  sampler2D diffuse_map;
  bool use_diffuse_map;
  vec3 specular;
  float specular_exponent;
};

struct Light
{
  vec3 pos;
  vec3 color;

  // NOTE: for attenuation
  float constant;
  float linear;
  float quadratic;
};

in VERT_OUT
{
  vec2 uv;
  vec3 normal;
  vec3 frag_pos;
  vec4 tint;
}
vert_out;

out vec4 out_color;

uniform Material material;
uniform samplerCube shadow_map;
uniform float shadow_map_camera_far_plane;

layout(std140) uniform Camera
{
  mat4 proj_view;
  vec3 view_pos;
  float far_plane;
};

layout(std140) uniform Lights
{
  Light light;
};

void main()
{
  vec3 norm = normalize(vert_out.normal);
  // TODO: this shouldnt ignore the a component of tint or the diffuse map
  vec3 object_color;
  if (material.use_diffuse_map)
  {
    object_color = texture(material.diffuse_map, vert_out.uv).rgb;
  }
  else
  {
    object_color = material.diffuse;
  }
  object_color *= vert_out.tint.rgb;

  float dist = length(light.pos - vert_out.frag_pos);
  float attenuation =
    1.0f / (light.constant + light.linear * dist + light.quadratic * dist * dist);

  vec3 light_dir = normalize(light.pos - vert_out.frag_pos);
  float diff = max(dot(norm, light_dir), 0.0f);
  vec3 diffuse = diff * light.color;

  float specular_strength = 0.5f;
  vec3 view_dir = normalize(view_pos - vert_out.frag_pos);
  vec3 halfway = normalize(light_dir + view_dir);
  float spec = pow(max(dot(norm, halfway), 0.0f), material.specular_exponent);
  vec3 specular = spec * specular_strength * light.color;

  vec3 frag_to_light = vert_out.frag_pos - light.pos;
  float closest_depth = texture(shadow_map, frag_to_light).r;
  closest_depth *= shadow_map_camera_far_plane;
  float current_depth = length(frag_to_light);
  float bias = 0.05f;
  float shadow = current_depth - bias > closest_depth ? 1.0f : 0.0f;

  vec3 diffuse_specular = (diffuse + specular) * attenuation;

  vec3 color = (material.ambient + (1.0f - shadow) * diffuse_specular) * object_color;
  out_color = vec4(color, 1.0f);
}
