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

in VERT_OUT
{
  vec2 uv;
  vec3 normal;
  vec3 frag_pos;
  vec4 tint;
}
vert_out;

out vec4 color;

uniform Material material;

void main()
{
  vec4 object_color;
  if (material.use_diffuse_map)
  {
    object_color = texture(material.diffuse_map, vert_out.uv);
  }
  else
  {
    object_color = vec4(material.diffuse, 1);
  }
  color = object_color * vert_out.tint;
}
