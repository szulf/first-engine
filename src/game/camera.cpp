#include "camera.h"

#include <cmath>

void camera_update_vectors(Camera& camera)
{
  camera.front.x = std::cos(camera.yaw) * std::cos(camera.pitch);
  camera.front.y = std::sin(camera.pitch);
  camera.front.z = std::sin(camera.yaw) * std::cos(camera.pitch);
  camera.front = normalize(camera.front);

  camera.right = normalize(cross(camera.front, CAMERA_WORLD_UP));
  camera.up = normalize(cross(camera.right, camera.front));
}

void camera_look_around(Camera& camera, const vec2& offset)
{
  camera.yaw += offset.x;
  camera.pitch -= offset.y;
  camera.pitch =
    std::clamp(camera.pitch, -0.49f * std::numbers::pi_v<f32>, 0.49f * std::numbers::pi_v<f32>);
  camera_update_vectors(camera);
}

void camera_move(Camera& camera, const vec3& acceleration, const vec3& direction, f32 dt)
{
  camera.prev_pos = camera.pos;
  camera.pos += direction * (-acceleration.z * CAMERA_SPEED * dt);
  camera.pos += camera.right * (acceleration.x * CAMERA_SPEED * dt);
  camera.pos += CAMERA_WORLD_UP * (acceleration.y * CAMERA_SPEED * dt);
}

mat4 camera_view(const Camera& camera, f32 t)
{
  vec3 render_pos = camera.pos * t + camera.prev_pos * (1.0f - t);
  return look_at_matrix(render_pos, render_pos + camera.front, camera.up);
}

mat4 camera_projection(const Camera& camera)
{
  switch (camera.type)
  {
    case CAMERA_TYPE_PERSPECTIVE:
    {
      return perspective_matrix(
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
      return orthographic_matrix(
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
