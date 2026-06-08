#include "errors.h"

#include <format>

std::array<Error, ERROR_MAX_COUNT> g_error_list{};
usize g_error_count{};

Error error_add_trace(Error error, std::string_view function, u32 line)
{
  if (error.trace_count < ERROR_MAX_TRACE)
  {
    error.trace[error.trace_count] = ErrorPos{.function = function, .line = line};
    ++error.trace_count;
  }
  return error;
}

void report(Error error)
{
  if (g_error_count < ERROR_MAX_COUNT)
  {
    g_error_list[g_error_count] = std::move(error);
    ++g_error_count;
  }
}

std::string error_to_string(const Error& error)
{
  std::string str =
    std::format("Error occurred with the message:\n{}\nstack trace:\n", error.message);
  for (usize i = 0; i < error.trace_count; ++i)
  {
    str += std::format("[{}:{}]\n", error.trace[i].function, error.trace[i].line);
  }
  return str;
}
