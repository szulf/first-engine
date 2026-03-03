#include "camera.h"

#include <cmath>

void Camera::update(f32 alpha)
{
  m_rendered_pos = m_pos * alpha + m_prev_pos * (1.0f - alpha);
}

void Camera::look_around(const vec2& offset)
{
  m_yaw += offset.x;
  m_pitch -= offset.y;
  m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
  update_vectors();
}

void Camera::move(const vec3& acceleration, const vec3& direction, f32 dt)
{
  m_prev_pos = m_rendered_pos = m_pos;
  m_pos += direction * (-acceleration.z * SPEED * dt);
  m_pos += m_right * (acceleration.x * SPEED * dt);
  m_pos += WORLD_UP * (acceleration.y * SPEED * dt);
}

mat4 Camera::look_at() const
{
  return ::look_at(m_rendered_pos, m_rendered_pos + m_front, m_up);
}

mat4 Camera::projection() const
{
  switch (m_type)
  {
    case CameraType::PERSPECTIVE:
    {
      return perspective(
        m_fov,
        (f32) m_viewport.x / (f32) m_viewport.y,
        m_near_plane,
        m_far_plane,
        m_using_vertical_fov
      );
    }
    break;
    case CameraType::ORTHOGRAPHIC:
    {
      static constexpr f32 world_height = 10.0f;
      static constexpr f32 half_height = world_height * 0.5f;
      f32 half_width = half_height * ((f32) m_viewport.x / (f32) m_viewport.y);

      return orthographic(
        half_width,
        -half_width,
        half_height,
        -half_height,
        m_near_plane,
        m_far_plane
      );
    }
    break;
  }
}

void Camera::update_vectors()
{
  m_front.x = std::cos(radians(m_yaw)) * std::cos(radians(m_pitch));
  m_front.y = std::sin(radians(m_pitch));
  m_front.z = std::sin(radians(m_yaw)) * std::cos(radians(m_pitch));
  m_front = m_front.normalize();

  m_right = cross(m_front, WORLD_UP).normalize();
  m_up = cross(m_right, m_front).normalize();
}
