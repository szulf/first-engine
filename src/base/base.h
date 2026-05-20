#ifndef BASE_BASE_H
#define BASE_BASE_H

#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <print>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usize = std::size_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using isize = std::ptrdiff_t;

using f32 = float;
using f64 = double;

#define UNUSED(x) ((void) (x))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*arr))

#define ASSERT(expr, ...)                                                                          \
  do                                                                                               \
  {                                                                                                \
    if (!(expr))                                                                                   \
    {                                                                                              \
      std::println("assertion failed on expression: '{}' with message:", #expr);                   \
      std::println(__VA_ARGS__);                                                                   \
      std::abort();                                                                                \
    }                                                                                              \
  }                                                                                                \
  while (false)

template <typename F>
struct privDefer
{
  F f;
  privDefer(F _f) : f(_f) {}
  ~privDefer()
  {
    f();
  }
};

template <typename F>
privDefer<F> defer_func(F f)
{
  return privDefer<F>(f);
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define DEFER_3(x) DEFER_2(x, __COUNTER__)
#define defer(code)                                                                                \
  auto DEFER_3(_defer_) = defer_func(                                                              \
    [&]()                                                                                          \
    {                                                                                              \
      code;                                                                                        \
    }                                                                                              \
  )

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)

enum AtomicMemoryOrder
{
  ATOMIC_MEMORY_ORDER_RELAXED = __ATOMIC_RELAXED,
  ATOMIC_MEMORY_ORDER_ACQUIRE = __ATOMIC_ACQUIRE,
  ATOMIC_MEMORY_ORDER_RELEASE = __ATOMIC_RELEASE,
};

#  define atomic_load_32(ptr, memory_order) __atomic_load_n((ptr), (memory_order))
#  define atomic_store_32(ptr, value, memory_order) __atomic_store_n((ptr), (value), (memory_order))

#elif defined(COMPILER_MSVC)
#  error Compiler intrinsics for atomic operations not supported for this compiler
#else
#  error Compiler intrinsics for atomic operations not supported for this compiler
#endif

#endif
