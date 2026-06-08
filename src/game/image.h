#ifndef GAME_IMAGE_H
#define GAME_IMAGE_H

#include <filesystem>
#include <expected>
#include <vector>

#include "base/base.h"
#include "base/math.h"
#include "base/errors.h"

struct Image
{
  std::vector<u8> data{};
  vec2 dimensions{};
};

Image image_init(const u8* data, const vec2& dimensions);
std::expected<Image, Error> image_from_file(const std::filesystem::path& path);

#endif
