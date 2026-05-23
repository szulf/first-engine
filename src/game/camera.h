#ifndef GAME_CAMERA_H
#define GAME_CAMERA_H

#include "base/base.h"
#include "base/math.h"

enum CameraType
{
  CAMERA_TYPE_PERSPECTIVE,
  CAMERA_TYPE_ORTHOGRAPHIC,
};

enum FovType
{
  FOV_TYPE_VERTICAL,
  FOV_TYPE_HORIZONTAL,
};

#define CAMERA_WORLD_UP vec3{0.0f, 1.0f, 0.0f}
#define CAMERA_SPEED 4.0f
#define CAMERA_SENSITIVITY 0.1f

struct Camera
{
  CameraType type{};
  vec3 pos{};
  vec3 prev_pos{};
  f32 yaw{};
  f32 pitch{};
  FovType fov_type{};
  f32 fov{};
  f32 near_plane{};
  f32 far_plane{};
  vec2 viewport{};
  vec3 front{};
  vec3 up{};
  vec3 right{};
};

void camera_update_vectors(Camera& camera);
void camera_look_around(Camera& camera, const vec2& offset);
void camera_move(Camera& camera, const vec3& acceleration, const vec3& direction, f32 dt);
mat4 camera_view(const Camera& camera, f32 t);
mat4 camera_projection(const Camera& camera);

#endif
