#version 330 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in mat4 a_model;
layout(location = 7) in vec3 a_tint;
layout(location = 8) in vec2 a_uv_scale;
layout(location = 9) in vec2 a_uv_offset;

out VERT_OUT
{
  vec2 uv;
  vec3 normal;
  vec3 frag_pos;
  vec3 tint;
}
vert_out;

layout(std140) uniform Camera
{
  mat4 proj_view;
  vec3 view_pos;
  float far_plane;
};

void main()
{
  vert_out.uv = (a_uv * a_uv_scale) + a_uv_offset;
  vert_out.frag_pos = vec3(a_model * vec4(a_pos, 1.0f));
  vert_out.normal = mat3(transpose(inverse(a_model))) * a_normal;
  vert_out.tint = a_tint;

  gl_Position = proj_view * vec4(vert_out.frag_pos, 1.0f);
}
