#include "items.h"

std::expected<ItemType, std::string_view> item_type_from_string(std::string_view str)
{
  if (str == "block")
  {
    return {ITEM_BLOCK};
  }
  else if (str == "light_bulb")
  {
    return {ITEM_LIGHT_BULB};
  }
  else if (str == "conveyor")
  {
    return {ITEM_CONVEYOR};
  }
  else if (str == "storage")
  {
    return {ITEM_STORAGE};
  }
  return std::unexpected{"Invalid item type string"};
}
