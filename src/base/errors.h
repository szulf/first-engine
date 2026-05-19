#ifndef BASE_ERRORS_H
#define BASE_ERRORS_H

#include <vector>
#include <string_view>

#include "base.h"

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
