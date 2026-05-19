#include "math.h"

#include <cmath>

bool f32_equal(f32 a, f32 b)
{
  return std::abs(a - b) <=
         (std::numeric_limits<f32>::epsilon() * std::max(std::abs(a), std::abs(b)));
}

f32 wrap_to_neg_pi_to_pi(f32 value)
{
  return std::atan2(std::sin(value), std::cos(value));
}

f32 radians(f32 deg)
{
  return deg * 0.01745329251994329576923690768489f;
}

vec2 operator-(const vec2& v)
{
  return {-v.x, -v.y};
}

vec2 operator+(const vec2& a, const vec2& b)
{
  return {a.x + b.x, a.y + b.y};
}

vec2 operator-(const vec2& a, const vec2& b)
{
  return {a.x - b.x, a.y - b.y};
}

vec2 operator*(const vec2& v, f32 scalar)
{
  return {v.x * scalar, v.y * scalar};
}

vec2 operator*(f32 scalar, const vec2& v)
{
  return {v.x * scalar, v.y * scalar};
}

vec2 operator*(const vec2& a, const vec2& b)
{
  return {a.x * b.x, a.y * b.y};
}

vec2 operator/(const vec2& v, f32 scalar)
{
  return {v.x / scalar, v.y / scalar};
}

vec2 operator/(const vec2& a, const vec2& b)
{
  return {a.x / b.x, a.y / b.y};
}

vec2& operator+=(vec2& a, const vec2& b)
{
  a.x += b.x;
  a.y += b.y;
  return a;
}

vec2& operator-=(vec2& a, const vec2& b)
{
  a.x -= b.x;
  a.y -= b.y;
  return a;
}

vec2& operator*=(vec2& v, f32 scalar)
{
  v.x += scalar;
  v.y += scalar;
  return v;
}

vec2& operator*=(vec2& a, const vec2& b)
{
  a.x *= b.x;
  a.y *= b.y;
  return a;
}

vec2& operator/=(vec2& v, f32 scalar)
{
  v.x /= scalar;
  v.y /= scalar;
  return v;
}

bool operator==(const vec2& a, const vec2& b)
{
  return f32_equal(a.x, b.x) && f32_equal(a.y, b.y);
}

bool operator!=(const vec2& a, const vec2& b)
{
  return !(a == b);
}

uvec2 operator+(const uvec2& a, const uvec2& b)
{
  return {a.x + b.x, a.y + b.y};
}

uvec2 operator-(const uvec2& a, const uvec2& b)
{
  return {a.x - b.x, a.y - b.y};
}

uvec2& operator+=(uvec2& v, u32 scalar)
{
  v.x += scalar;
  v.y += scalar;
  return v;
}

uvec2& operator+=(uvec2& a, const uvec2& b)
{
  a.x += b.x;
  a.y += b.y;
  return a;
}

uvec2& operator-=(uvec2& v, u32 scalar)
{
  v.x -= scalar;
  v.y -= scalar;
  return v;
}

uvec2& operator-=(uvec2& a, const uvec2& b)
{
  a.x -= b.x;
  a.y -= b.y;
  return a;
}

f32 length(const vec3& v)
{
  return std::hypot(v.x, v.y, v.z);
}

f32 length2(const vec3& v)
{
  return (v.x * v.x) + (v.y * v.y) + (v.z * v.z);
}

vec3 normalize(const vec3& v)
{
  auto len = length(v);
  if (f32_equal(len, 0))
  {
    return {};
  }
  return {v.x / len, v.y / len, v.z / len};
}

vec3 abs(const vec3& vec)
{
  return {std::abs(vec.x), std::abs(vec.y), std::abs(vec.z)};
}

f32 dot(const vec3& va, const vec3& vb)
{
  return va.x * vb.x + va.y * vb.y + va.z * vb.z;
}

vec3 cross(const vec3& va, const vec3& vb)
{
  return {
    (va.y * vb.z) - (va.z * vb.y),
    (va.z * vb.x) - (va.x * vb.z),
    (va.x * vb.y) - (va.y * vb.x),
  };
}

vec3 operator-(const vec3& v)
{
  return {-v.x, -v.y, -v.z};
}

vec3 operator+(const vec3& a, const vec3& b)
{
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

vec3 operator-(const vec3& a, const vec3& b)
{
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

vec3 operator*(const vec3& v, f32 scalar)
{
  return {v.x * scalar, v.y * scalar, v.z * scalar};
}

vec3 operator*(f32 scalar, const vec3& v)
{
  return {v.x * scalar, v.y * scalar, v.z * scalar};
}

vec3 operator*(const vec3& a, const vec3& b)
{
  return {a.x * b.x, a.y * b.y, a.z * b.z};
}

vec3 operator/(const vec3& v, f32 scalar)
{
  return {v.x / scalar, v.y / scalar, v.z / scalar};
}

vec3& operator+=(vec3& a, const vec3& b)
{
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;
  return a;
}

vec3& operator-=(vec3& a, const vec3& b)
{
  a.x -= b.x;
  a.y -= b.y;
  a.z -= b.z;
  return a;
}

vec3& operator*=(vec3& v, f32 scalar)
{
  v.x *= scalar;
  v.y *= scalar;
  v.z *= scalar;
  return v;
}

vec3& operator*=(vec3& a, const vec3& b)
{
  a.x *= b.x;
  a.y *= b.y;
  a.z *= b.z;
  return a;
}

vec3& operator/=(vec3& v, f32 scalar)
{
  v.x /= scalar;
  v.y /= scalar;
  v.z /= scalar;
  return v;
}

bool operator==(const vec3& a, const vec3& b)
{
  return f32_equal(a.x, b.x) && f32_equal(a.y, b.y) && f32_equal(a.z, b.z);
}

bool operator!=(const vec3& a, const vec3& b)
{
  return !(a == b);
}

vec4 operator*(const vec4& a, const vec4& b)
{
  return {a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w};
}

vec4& operator*=(vec4& a, const vec4& b)
{
  a.x *= b.x;
  a.y *= b.y;
  a.z *= b.z;
  a.w *= b.w;
  return a;
}

bool operator==(const vec4& a, const vec4& b)
{
  return f32_equal(a.x, b.x) && f32_equal(a.y, b.y) && f32_equal(a.z, b.z) && f32_equal(a.w, b.w);
}

bool operator!=(const vec4& a, const vec4& b)
{
  return !(a == b);
}

mat4 unit_matrix()
{
  return {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};
}

mat4 perspective(f32 fov, f32 aspect, f32 near, f32 far, bool vertical)
{
  f32 tangent = std::tan(fov / 2.0f);
  f32 top;
  f32 right;
  if (vertical)
  {
    top = tangent * near;
    right = top * aspect;
  }
  else
  {
    right = tangent * near;
    top = right / aspect;
  }
  mat4 out = {};
  out.data[0][0] = near / right;
  out.data[0][0] = near / right;
  out.data[1][1] = near / top;
  out.data[2][2] = -(far + near) / (far - near);
  out.data[2][3] = -1;
  out.data[3][2] = -(2 * near * far) / (far - near);
  out.data[3][3] = 0;
  return out;
}

mat4 orthographic(f32 right, f32 left, f32 top, f32 bottom, f32 near, f32 far)
{
  mat4 out = {};
  out.data[0][0] = 2 / (right - left);
  out.data[1][1] = 2 / (top - bottom);
  out.data[2][2] = -2 / (far - near);
  out.data[3][0] = -(right + left) / (right - left);
  out.data[3][1] = -(top + bottom) / (top - bottom);
  out.data[3][2] = -(far + near) / (far - near);
  out.data[3][3] = 1;
  return out;
}

mat4 look_at(const vec3& pos, const vec3& target, const vec3& up)
{
  vec3 f = normalize(target - pos);
  vec3 s = normalize(cross(f, up));
  vec3 u = cross(s, f);

  mat4 out = {};

  out.data[0][0] = s.x;
  out.data[1][0] = s.y;
  out.data[2][0] = s.z;
  out.data[3][0] = -dot(s, pos);

  out.data[0][1] = u.x;
  out.data[1][1] = u.y;
  out.data[2][1] = u.z;
  out.data[3][1] = -dot(u, pos);

  out.data[0][2] = -f.x;
  out.data[1][2] = -f.y;
  out.data[2][2] = -f.z;
  out.data[3][2] = dot(f, pos);

  out.data[0][3] = 0.0f;
  out.data[1][3] = 0.0f;
  out.data[2][3] = 0.0f;
  out.data[3][3] = 1.0f;

  return out;
}

mat4 scale(mat4 mat, f32 scale)
{
  mat.data[0][0] *= scale;
  mat.data[1][0] *= scale;
  mat.data[2][0] *= scale;

  mat.data[0][1] *= scale;
  mat.data[1][1] *= scale;
  mat.data[2][1] *= scale;

  mat.data[0][2] *= scale;
  mat.data[1][2] *= scale;
  mat.data[2][2] *= scale;
  return mat;
}

mat4 scale(mat4 mat, const vec3& scale)
{
  mat.data[0][0] *= scale.x;
  mat.data[0][1] *= scale.x;
  mat.data[0][2] *= scale.x;

  mat.data[1][0] *= scale.y;
  mat.data[1][1] *= scale.y;
  mat.data[1][2] *= scale.y;

  mat.data[2][0] *= scale.z;
  mat.data[2][1] *= scale.z;
  mat.data[2][2] *= scale.z;
  return mat;
}

mat4 translate(mat4 mat, const vec3& position)
{
  mat.data[3][0] = position.x;
  mat.data[3][1] = position.y;
  mat.data[3][2] = position.z;
  return mat;
}

mat4 rotate(mat4 mat, f32 rad, const vec3& axis)
{
  f32 s = std::sin(rad);
  f32 c = std::cos(rad);
  f32 t = 1.0f - c;
  vec3 u = normalize(axis);

  mat.data[0][0] = (u.x * u.x) * t + c;
  mat.data[0][1] = (u.x * u.y) * t - u.z * s;
  mat.data[0][2] = (u.x * u.z) * t + u.y * s;

  mat.data[1][0] = (u.x * u.y) * t + u.z * s;
  mat.data[1][1] = (u.y * u.y) * t + c;
  mat.data[1][2] = (u.y * u.z) * t - u.x * s;

  mat.data[2][0] = (u.x * u.z) * t - u.y * s;
  mat.data[2][1] = (u.y * u.z) * t + u.x * s;
  mat.data[2][2] = (u.z * u.z) * t + c;
  return mat;
}

mat4 operator*(const mat4& a, const mat4& b)
{
  mat4 out = {};
  for (usize i = 0; i < 4; ++i)
  {
    for (usize j = 0; j < 4; ++j)
    {
      for (usize k = 0; k < 4; ++k)
      {
        out.data[j][i] += a.data[k][i] * b.data[j][k];
      }
    }
  }
  return out;
}

vec4 operator*(const mat4& m, const vec4& v)
{
  vec4 out = {};
  for (usize i = 0; i < 4; ++i)
  {
    for (usize j = 0; j < 4; ++j)
    {
      out.data[i] += m.data[i][j] * v.data[j];
    }
  }
  return out;
}

void hash_fnv1(usize& out, const void* data, usize n)
{
  if (out == 0)
  {
    out = FNV_OFFSET;
  }
  const u8* bytes = (const u8*) data;
  for (usize i = 0; i < n; ++i)
  {
    out ^= bytes[i];
    out *= FNV_PRIME;
  }
}

bool operator==(const Rectangle& a, const Rectangle& b)
{
  return a.pos == b.pos && a.dimensions == b.dimensions;
}
