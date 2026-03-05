#include "image.h"

#include <cstring>

#include "stb/image.h"

Image::Image(const u8* data, const uvec2& dimensions) : m_dimensions{dimensions}
{
  m_data = new u8[m_dimensions.x * m_dimensions.y * 4];
  for (u32 i = 0; i < m_dimensions.x * m_dimensions.y * 4; ++i)
  {
    m_data[i] = data[i];
  }
}

Image::Image(const std::filesystem::path& path) : m_stbi_loaded{true}
{
  stbi_set_flip_vertically_on_load(true);
  int width, height, channels;
  m_data = stbi_load(path.c_str(), &width, &height, &channels, 4);
  ASSERT(channels == 4, "Invalid image file.");
  m_dimensions = {static_cast<u32>(width), static_cast<u32>(height)};
}

Image::Image(Image&& other)
  : m_stbi_loaded{other.m_stbi_loaded}, m_data{other.m_data}, m_dimensions{other.m_dimensions}
{
  other.m_data = nullptr;
}

Image& Image::operator=(Image&& other)
{
  if (this == &other)
  {
    return *this;
  }
  if (m_stbi_loaded)
  {
    stbi_image_free(m_data);
  }
  else
  {
    delete[] m_data;
  }
  m_data = other.m_data;
  other.m_data = nullptr;
  m_dimensions = other.m_dimensions;
  m_stbi_loaded = other.m_stbi_loaded;
  return *this;
}

Image::~Image()
{
  if (m_stbi_loaded)
  {
    stbi_image_free(m_data);
  }
  else
  {
    delete[] m_data;
  }
}
