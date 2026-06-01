#ifndef GAME_ITEMS_H
#define GAME_ITEMS_H

#include <expected>
#include <string_view>
#include <array>

#include "base/base.h"

static constexpr u8 ITEMS_MAX_STACK_SIZE = 100;

enum ItemType
{
  ITEM_BLOCK,
  ITEM_LIGHT_BULB,
  ITEM_CONVEYOR,
  ITEM_STORAGE,
  ITEM_TYPE_COUNT,
};

static constexpr std::array<std::string_view, ITEM_TYPE_COUNT> ITEM_TYPE_STRING = []()
{
  std::array<std::string_view, ITEM_TYPE_COUNT> out{};
  out[ITEM_BLOCK] = "block";
  out[ITEM_LIGHT_BULB] = "light_bulb";
  out[ITEM_CONVEYOR] = "conveyor";
  out[ITEM_STORAGE] = "storage";
  return out;
}();

std::expected<ItemType, std::string_view> item_type_from_string(std::string_view str);

struct ItemSlot
{
  ItemType type{};
  u32 count{};
};

#endif
