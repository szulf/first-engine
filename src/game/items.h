#ifndef GAME_ITEMS_H
#define GAME_ITEMS_H

#include <expected>
#include <string_view>
#include <array>

#include "base/base.h"
#include "base/errors.h"

static constexpr u8 ITEMS_MAX_STACK_SIZE = 100;

#define ITEM_TYPES                                                                                 \
  X(ITEM_BLOCK, "block")                                                                           \
  X(ITEM_LIGHT_BULB, "light_bulb")                                                                 \
  X(ITEM_CONVEYOR, "conveyor")                                                                     \
  X(ITEM_STORAGE, "storage")

#define X(type, str) type,
enum ItemType
{
  ITEM_TYPES ITEM_TYPE_COUNT,
};
#undef X

#define X(type, str) out[type] = str;
static constexpr std::array<std::string_view, ITEM_TYPE_COUNT> ITEM_TYPE_TO_STRING = []()
{
  std::array<std::string_view, ITEM_TYPE_COUNT> out{};
  ITEM_TYPES
  return out;
}();
#undef X

std::expected<ItemType, Error> item_type_from_string(std::string_view str);

struct ItemSlot
{
  ItemType type{};
  u32 count{};
};

#endif
