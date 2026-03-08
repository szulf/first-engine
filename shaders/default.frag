#version 330 core

struct Material
{
  vec3 ambient;
  vec3 diffuse;
  sampler2D diffuse_map;
  vec3 specular;
  float specular_exponent;
};

in VERT_OUT
{
  vec2 uv;
  vec3 normal;
  vec3 frag_pos;
  vec3 tint;
}
vert_out;

out vec4 color;

uniform Material material;

void main()
{
  // TODO: this is still not perfect, what if there is no diffuse_map and diffuse color is transparent?
  // so diffuse color is a vec3 lol, but should it really be that?
  vec4 texture_color = texture(material.diffuse_map, vert_out.uv);
  vec4 object_color = vec4(texture_color.rgb + material.diffuse, texture_color.a) * vec4(vert_out.tint, 1.0f);
  color = object_color;
}
