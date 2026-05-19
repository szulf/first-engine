#include "camera.h"

#include <cmath>

void camera_update_vectors(Camera& camera)
{
  camera.front.x = std::cos(radians(camera.yaw)) * std::cos(radians(camera.pitch));
  camera.front.y = std::sin(radians(camera.pitch));
  camera.front.z = std::sin(radians(camera.yaw)) * std::cos(radians(camera.pitch));
  camera.front = normalize(camera.front);

  camera.right = normalize(cross(camera.front, CAMERA_WORLD_UP));
  camera.up = normalize(cross(camera.right, camera.front));
}

void camera_update(Camera& camera, f32 alpha)
{
  camera.rendered_pos = camera.pos * alpha + camera.prev_pos * (1.0f - alpha);
}

void camera_look_around(Camera& camera, const vec2& offset)
{
  camera.yaw += offset.x;
  camera.pitch -= offset.y;
  camera.pitch = std::clamp(camera.pitch, -89.0f, 89.0f);
  camera_update_vectors(camera);
}

void camera_move(Camera& camera, const vec3& acceleration, const vec3& direction, f32 dt)
{
  camera.prev_pos = camera.rendered_pos = camera.pos;
  camera.pos += direction * (-acceleration.z * CAMERA_SPEED * dt);
  camera.pos += camera.right * (acceleration.x * CAMERA_SPEED * dt);
  camera.pos += CAMERA_WORLD_UP * (acceleration.y * CAMERA_SPEED * dt);
}

mat4 camera_look_at(const Camera& camera)
{
  return look_at(camera.rendered_pos, camera.rendered_pos + camera.front, camera.up);
}

mat4 camera_projection(const Camera& camera)
{
  switch (camera.type)
  {
    case CAMERA_TYPE_PERSPECTIVE:
    {
      return perspective(
        camera.fov,
        (f32) camera.viewport.x / (f32) camera.viewport.y,
        camera.near_plane,
        camera.far_plane,
        camera.fov_type == FOV_TYPE_VERTICAL
      );
    }
    break;
    case CAMERA_TYPE_ORTHOGRAPHIC:
    {
      return orthographic(
        (f32) camera.viewport.x,
        0.0f,
        0.0f,
        (f32) camera.viewport.y,
        camera.near_plane,
        camera.far_plane
      );
    }
    break;
  }
  ASSERT(false, "Invalid camera type");
  return {};
}
