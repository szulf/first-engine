#ifndef BASE_ERRORS_H
#define BASE_ERRORS_H

#include <array>
#include <string>
#include <string_view>
#include <format>

#include "base.h"

static constexpr usize ERROR_MAX_COUNT = 64;
static constexpr usize ERROR_MAX_TRACE = 8;

struct ErrorPos
{
  std::string_view function{};
  u32 line{};
};

struct Error
{
  std::array<ErrorPos, ERROR_MAX_TRACE> trace{};
  usize trace_count{};
  std::string message{};
};

Error error_add_trace(Error error, std::string_view function, u32 line);
void report(Error error);
std::string error_to_string(const Error& error);

#define ERROR(...) error_add_trace(Error{.message = std::format(__VA_ARGS__)}, __func__, __LINE__)
#define FORWARD(error) error_add_trace(error, __func__, __LINE__)

#define TRY(code)                                                                                  \
  do                                                                                               \
  {                                                                                                \
    if (auto try_test = (code))                                                                    \
    {                                                                                              \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      return std::unexpected{FORWARD(try_test.error())};                                           \
    }                                                                                              \
  }                                                                                                \
  while (false)

#define TRY_ASSIGN(var, code)                                                                      \
  do                                                                                               \
  {                                                                                                \
    if (auto try_assign_test = (code))                                                             \
    {                                                                                              \
      var = std::move(*try_assign_test);                                                           \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      return std::unexpected{FORWARD(try_assign_test.error())};                                    \
    }                                                                                              \
  }                                                                                                \
  while (false)

#define TRY_ASSIGN_CAST(var, type, code)                                                           \
  do                                                                                               \
  {                                                                                                \
    if (auto try_assign_test = (code))                                                             \
    {                                                                                              \
      var = (type) std::move(*try_assign_test);                                                    \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      return std::unexpected{FORWARD(try_assign_test.error())};                                    \
    }                                                                                              \
  }                                                                                                \
  while (false)

#define TRY_ASSIGN_REPORT(var, code)                                                               \
  do                                                                                               \
  {                                                                                                \
    if (auto try_assign_test = (code))                                                             \
    {                                                                                              \
      var = std::move(*try_assign_test);                                                           \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      report(FORWARD(try_assign_test.error()));                                                    \
    }                                                                                              \
  }                                                                                                \
  while (false)

extern std::array<Error, ERROR_MAX_COUNT> g_error_list;
extern usize g_error_count;

#endif
