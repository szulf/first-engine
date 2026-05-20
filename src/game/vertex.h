#ifndef GAME_VERTEX_H
#define GAME_VERTEX_H

#include "base/math.h"

struct Vertex
{
  vec3 pos;
  vec3 normal;
  vec2 uv;
};

template <>
struct std::hash<Vertex>
{
  std::size_t operator()(const Vertex& v) const
  {
    usize h = FNV_OFFSET;
    hash_fnv1(h, &v.pos.x, sizeof(v.pos.x));
    hash_fnv1(h, &v.pos.y, sizeof(v.pos.y));
    hash_fnv1(h, &v.pos.z, sizeof(v.pos.z));
    hash_fnv1(h, &v.normal.x, sizeof(v.normal.x));
    hash_fnv1(h, &v.normal.y, sizeof(v.normal.y));
    hash_fnv1(h, &v.normal.z, sizeof(v.normal.z));
    hash_fnv1(h, &v.uv.x, sizeof(v.uv.x));
    hash_fnv1(h, &v.uv.y, sizeof(v.uv.y));
    return h;
  }
};

inline bool operator==(const Vertex& va, const Vertex& vb)
{
  return va.pos == vb.pos && va.normal == vb.normal && va.uv == vb.uv;
}

#endif
