#version 330 core

in VERT_OUT
{
  vec2 uv;
  vec3 frag_pos;
  vec4 tint;
  float corner_radius;
}
vert_out;

out vec4 color;

uniform sampler2D diffuse_map;

void main()
{
  vec4 texture_color = texture(diffuse_map, vert_out.uv);
  vec4 object_color = texture_color * vert_out.tint;
  color = object_color;
}
