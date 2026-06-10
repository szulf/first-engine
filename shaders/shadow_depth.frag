#version 330 core

in vec4 frag_pos;

layout(std140) uniform Camera
{
  mat4 proj_view;
  vec3 view_pos;
  float far_plane;
};

void main()
{
  float light_distance = length(frag_pos.xyz - view_pos);
  light_distance = light_distance / far_plane;
  gl_FragDepth = light_distance;
}
