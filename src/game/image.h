#ifndef GAME_IMAGE_H
#define GAME_IMAGE_H

#include <filesystem>
#include <expected>
#include <vector>

#include "base/base.h"
#include "base/math.h"

struct Image
{
  std::vector<u8> data{};
  uvec2 dimensions{};
};

Image image_init(const u8* data, const uvec2& dimensions);
std::expected<Image, std::string_view> image_from_file(const std::filesystem::path& path);

#endif
