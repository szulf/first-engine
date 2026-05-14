#pragma once

#include "base.h"

#define G_F32 9.81f
#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL

bool f32_equal(f32 a, f32 b);
f32 wrap_to_neg_pi_to_pi(f32 value);
f32 radians(f32 deg);

struct vec2
{
  f32 x{};
  f32 y{};
};
vec2 operator-(const vec2& v);
vec2 operator+(const vec2& a, const vec2& b);
vec2 operator-(const vec2& a, const vec2& b);
vec2 operator*(const vec2& v, f32 scalar);
vec2 operator*(f32 scalar, const vec2& v);
vec2 operator*(const vec2& a, const vec2& b);
vec2 operator/(const vec2& v, f32 scalar);
vec2 operator/(const vec2& a, const vec2& b);
vec2& operator+=(vec2& a, const vec2& b);
vec2& operator-=(vec2& a, const vec2& b);
vec2& operator*=(vec2& v, f32 scalar);
vec2& operator*=(vec2& a, const vec2& b);
vec2& operator/=(vec2& v, f32 scalar);
bool operator==(const vec2& a, const vec2& b);
bool operator!=(const vec2& a, const vec2& b);

struct uvec2
{
  u32 x{};
  u32 y{};
};
uvec2 operator+(const uvec2& a, const uvec2& b);
uvec2 operator-(const uvec2& a, const uvec2& b);
uvec2& operator+=(uvec2& v, u32 scalar);
uvec2& operator+=(uvec2& a, const uvec2& b);
uvec2& operator-=(uvec2& v, u32 scalar);
uvec2& operator-=(uvec2& a, const uvec2& b);

struct vec3
{
  f32 x{};
  f32 y{};
  f32 z{};
};
vec3 operator-(const vec3& v);
vec3 operator+(const vec3& a, const vec3& b);
vec3 operator-(const vec3& a, const vec3& b);
vec3 operator*(const vec3& v, f32 scalar);
vec3 operator*(f32 scalar, const vec3& v);
vec3 operator*(const vec3& a, const vec3& b);
vec3 operator/(const vec3& v, f32 scalar);
vec3& operator+=(vec3& a, const vec3& b);
vec3& operator-=(vec3& a, const vec3& b);
vec3& operator*=(vec3& v, f32 scalar);
vec3& operator*=(vec3& a, const vec3& b);
vec3& operator/=(vec3& v, f32 scalar);
bool operator==(const vec3& a, const vec3& b);
bool operator!=(const vec3& a, const vec3& b);

f32 length(const vec3& v);
f32 length2(const vec3& v);
vec3 normalize(const vec3& v);
vec3 abs(const vec3& vec);
f32 dot(const vec3& va, const vec3& vb);
vec3 cross(const vec3& va, const vec3& vb);

union vec4
{
  struct
  {
    f32 x{};
    f32 y{};
    f32 z{};
    f32 w{};
  };
  f32 data[4];
};
vec4 operator*(const vec4& a, const vec4& b);
vec4& operator*=(vec4& a, const vec4& b);
bool operator==(const vec4& a, const vec4& b);
bool operator!=(const vec4& a, const vec4& b);

// NOTE: column major
struct mat4
{
  f32 data[4][4]{};
};
mat4 operator*(const mat4& a, const mat4& b);
vec4 operator*(const mat4& m, const vec4& v);

mat4 unit_matrix();
mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far, bool vertical);
mat4 orthographic(f32 right, f32 left, f32 top, f32 bottom, f32 near, f32 far);
mat4 look_at(const vec3& pos, const vec3& target, const vec3& up);

mat4 scale(mat4 mat, f32 scale);
mat4 scale(mat4 mat, const vec3& scale);
mat4 translate(mat4 mat, const vec3& position);
mat4 rotate(mat4 mat, f32 rad, const vec3& axis);

void hash_fnv1(usize& out, const void* data, usize n);

struct Rectangle
{
  vec2 pos{};
  vec2 dimensions{};
};
bool operator==(const Rectangle& a, const Rectangle& b);
