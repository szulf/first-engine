#include "items.h"

std::expected<ItemType, Error> item_type_from_string(std::string_view str)
{
  for (usize i = 0; i < ITEM_TYPE_COUNT; ++i)
  {
    if (str == ITEM_TYPE_TO_STRING[(ItemType) i])
    {
      return {(ItemType) i};
    }
  }
  return std::unexpected{ERROR("Invalid item type string: {}", str)};
}
