#pragma once

#include "base.h"

#include <array>
#include <concepts>

template <typename T>
struct constants;

template <>
struct constants<f32>
{
  static constexpr f32 G = 9.81f;
};

template <>
struct constants<u64>
{
  static constexpr u64 FNV_OFFSET = 14695981039346656037UL;
  static constexpr u64 FNV_PRIME = 1099511628211UL;
};

bool f32_equal(f32 a, f32 b);
f32 wrap_to_neg_pi_to_pi(f32 value);
f32 radians(f32 deg);

struct vec2
{
  vec2 operator-() const;
  vec2 operator+(const vec2& other) const;
  vec2 operator-(const vec2& other) const;
  vec2 operator*(f32 scalar) const;
  vec2 operator*(const vec2& other) const;
  vec2 operator/(f32 scalar) const;
  vec2 operator/(const vec2& other) const;
  vec2& operator+=(const vec2& other);
  vec2& operator-=(const vec2& other);
  vec2& operator*=(f32 scalar);
  vec2& operator*=(const vec2& other);
  vec2& operator/=(f32 scalar);
  bool operator==(const vec2& other) const;
  bool operator!=(const vec2& other) const;

  f32 x{};
  f32 y{};
};
vec2 operator*(f32 scalar, const vec2& vec);

struct uvec2
{
  u32 x{};
  u32 y{};
};

struct vec3
{
  f32 length() const;
  f32 length2() const;
  vec3 normalize() const;

  vec3 operator-() const;
  vec3 operator+(const vec3& other) const;
  vec3 operator-(const vec3& other) const;
  vec3 operator*(f32 scalar) const;
  vec3 operator*(const vec3& other) const;
  vec3 operator/(f32 scalar) const;
  vec3& operator+=(const vec3& other);
  vec3& operator-=(const vec3& other);
  vec3& operator*=(f32 scalar);
  vec3& operator*=(const vec3& other);
  vec3& operator/=(f32 scalar);
  bool operator==(const vec3& other) const;
  bool operator!=(const vec3& other) const;

  f32 x{};
  f32 y{};
  f32 z{};
};
vec3 operator*(f32 scalar, const vec3& vec);

vec3 abs(const vec3& vec);
f32 dot(const vec3& va, const vec3& vb);
vec3 cross(const vec3& va, const vec3& vb);

struct vec4
{
  template <std::integral I>
  constexpr inline f32& operator[](I idx)
  {
    switch (idx)
    {
      case 0:
        return x;
      case 1:
        return y;
      case 2:
        return z;
      case 3:
        return w;
    }
    ASSERT(false, "Invalid index in vec4.");
    return x;
  }

  template <std::integral I>
  constexpr inline f32 operator[](I idx) const
  {
    switch (idx)
    {
      case 0:
        return x;
      case 1:
        return y;
      case 2:
        return z;
      case 3:
        return w;
    }
    ASSERT(false, "Invalid index in vec4.");
    return 0.0f;
  }

  f32 x{};
  f32 y{};
  f32 z{};
  f32 w{};
};

// NOTE: column major
struct mat4
{
  mat4() {}
  mat4(f32 v)
    : data{
        {{v, 0, 0, 0}, {0, v, 0, 0}, {0, 0, v, 0}, {0, 0, 0, v}}
  }
  {
  }

  mat4 operator*(const mat4& other) const;

  std::array<std::array<f32, 4>, 4> data{};
};

mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far, bool vertical);
mat4 orthographic(f32 right, f32 left, f32 top, f32 bottom, f32 near, f32 far);
mat4 look_at(const vec3& pos, const vec3& target, const vec3& up);

[[nodiscard]] mat4 scale(mat4 mat, f32 scale);
[[nodiscard]] mat4 scale(mat4 mat, const vec3& scale);
[[nodiscard]] mat4 translate(mat4 mat, const vec3& position);
[[nodiscard]] mat4 rotate(mat4 mat, f32 rad, const vec3& axis);

void hash_fnv1(usize& out, const void* data, usize n);
