#include "image.h"

#include <cstring>
#include "stb/image.h"

Image image_init(const u8* data, const uvec2& dimensions)
{
  Image img = {.dimensions = dimensions};
  img.data.insert(img.data.end(), data, data + (dimensions.x * dimensions.y * 4));
  return img;
}

std::expected<Image, std::string_view> image_from_file(const std::filesystem::path& path)
{
  int width, height, channels;
  u8* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
  if (!data)
  {
    return std::unexpected{"Failed to load image file"};
  }
  Image img = image_init(data, {(u32) width, (u32) height});
  stbi_image_free(data);
  return img;
}
