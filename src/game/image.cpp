#include "image.h"

#include <cstring>
#include "stb/image.h"

Image image_init(const u8* data, const vec2& dimensions)
{
  Image img{};
  img.dimensions = dimensions;
  img.data.insert(img.data.end(), data, data + (u32) (dimensions.x * dimensions.y * 4));
  return img;
}

std::expected<Image, Error> image_from_file(const std::filesystem::path& path)
{
  int width, height, channels;
  u8* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
  if (!data)
  {
    return std::unexpected{ERROR("Failed to load image file at path: '{}'", path.string())};
  }
  Image img = image_init(data, {(f32) width, (f32) height});
  stbi_image_free(data);
  return img;
}
