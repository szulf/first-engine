#ifndef BASE_ERRORS_H
#define BASE_ERRORS_H

#include <vector>
#include <string_view>

#include "base.h"

// TODO: i really need to sort out error handling everywhere

// TODO: make this a sort of linked list that where the next node is a comment to this node?
struct Error
{
  std::string_view function{};
  usize line{};
  std::string_view message{};
};

extern std::vector<Error> g_error_list;

#define REPORT_ERROR(msg)                                                                          \
  do                                                                                               \
  {                                                                                                \
    g_error_list.push_back({.function = __func__, .line = __LINE__, .message = (msg)});            \
  }                                                                                                \
  while (false)

#endif
