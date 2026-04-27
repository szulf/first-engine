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

bool out_of_circle(float x, float y)
{
  return pow(x, 2.0f) + pow(y, 2.0f) > pow(vert_out.corner_radius * 0.5f, 2.0f);
}

void main()
{
  // NOTE: corner radius test
  {
    float r = vert_out.corner_radius * 0.5f;
    bool top_left = vert_out.uv.x < r && vert_out.uv.y < r &&
        out_of_circle(vert_out.uv.x - r, vert_out.uv.y - r);
    bool top_right = vert_out.uv.x > 1. - r && vert_out.uv.y < r &&
        out_of_circle(vert_out.uv.x - 1. + r, vert_out.uv.y - r);
    bool bottom_left = vert_out.uv.x < r && vert_out.uv.y > 1. - r &&
        out_of_circle(vert_out.uv.x - r, vert_out.uv.y - 1. + r);
    bool bottom_right = vert_out.uv.x > 1. - r && vert_out.uv.y > 1. - r &&
        out_of_circle(vert_out.uv.x - 1. + r, vert_out.uv.y - 1. + r);
    if (top_left || top_right || bottom_left || bottom_right)
    {
      discard;
    }
  }

  vec4 texture_color = texture(diffuse_map, vert_out.uv);
  vec4 object_color = texture_color * vert_out.tint;
  color = object_color;
}
