#ifndef IMAGE_H
#define IMAGE_H

#include <filesystem>

#include "base/base.h"
#include "base/math.h"

struct Image
{
  Image() {}
  Image(const Image& other) = delete;
  Image& operator=(const Image& other) = delete;
  Image(Image&& other);
  Image& operator=(Image&& other);
  ~Image();

  static Image from_file(const std::filesystem::path& path);
  static Image error_placeholder();

  [[nodiscard]] constexpr inline u32 width() const
  {
    return m_dimensions.x;
  }

  [[nodiscard]] constexpr inline u32 height() const
  {
    return m_dimensions.y;
  }

  [[nodiscard]] constexpr inline const uvec2& dimensions() const
  {
    return m_dimensions;
  }

  [[nodiscard]] constexpr inline void* data() const
  {
    return m_data;
  }

private:
  u8* m_data{};
  uvec2 m_dimensions{};
};

#endif
