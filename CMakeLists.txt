cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(game
    VERSION 1.0
    LANGUAGES CXX
)

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES
      "Debug;Release"
      CACHE STRING "" FORCE
  )
elseif(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

# TODO: can this also be used for gcc?
set(clang_flags_common
  -Weverything -Werror -pedantic
  -Wno-c++20-compat
  -Wno-c++98-compat -Wno-c++98-compat-pedantic
  -Wno-padded -Wno-switch-default -Wno-unsafe-buffer-usage
  -Wno-unreachable-code-break -Wno-cast-function-type-strict
  -Wno-weak-vtables -Wno-exit-time-destructors -Wno-ctad-maybe-unsupported
  -Wno-unused-macros -Wno-missing-noreturn -Wno-covered-switch-default
  -Wno-undefined-func-template -Wno-missing-designated-field-initializers
  -Wno-global-constructors -Wno-unused-function -Wno-unused-template
  -Wno-format-nonliteral -Wno-shadow-uncaptured-local
  -Wno-old-style-cast -Wno-missing-prototypes -Wno-double-promotion
  -Wno-documentation-unknown-command -Wno-reserved-macro-identifier
  -Wno-gnu-anonymous-struct -Wno-nested-anon-types
  -Wno-reserved-identifier -Wno-missing-field-initializers
  -Wno-c2y-extensions
)
set(clang_flags_debug
  -g3 -O0
)
set(clang_flags_release
  -O3
)

set(compiler_clang "$<CXX_COMPILER_ID:Clang>")
set(compiler_definitions
  "$<${compiler_clang}:COMPILER_CLANG>"
)

set(os_linux "$<PLATFORM_ID:Linux>")
set(os_definitions
  "$<${os_linux}:OS_LINUX>"
)

set(mode_debug "$<CONFIG:Debug>")
set(mode_release "$<CONFIG:Release>")
set(mode_definitions
  "$<${mode_debug}:MODE_DEBUG>"
)

set(definitions
  ${mode_definitions}
  ${compiler_definitions}
  ${os_definitions}
)

set(flags
  "$<${compiler_clang}:${clang_flags_common}>"
  "$<$<AND:${compiler_clang},${mode_debug}>:${clang_flags_debug}>"
  "$<$<AND:${compiler_clang},${mode_release}>:${clang_flags_release}>"
)

set(include_dirs src vendor)

find_package(OpenGL REQUIRED)
include(vendor/vendor.cmake)

# add_compile_options(-fsanitize=address,undefined)
# add_link_options(-fsanitize=address,undefined)

add_library(base STATIC
  src/base/base.h
  src/base/math.h src/base/math.cpp
  src/base/enum_array.h
  src/base/spsc_queue.h
)
target_compile_options(base PRIVATE ${flags})
target_compile_definitions(base PRIVATE ${definitions})

add_library(platform STATIC
  src/os/os.h src/os/os_sdl3.cpp
  src/os/gl_functions.h src/os/gl_functions_sdl3.cpp
)
target_include_directories(platform PRIVATE ${include_dirs})
target_compile_options(platform PRIVATE ${flags})
target_compile_definitions(platform PRIVATE ${definitions})
target_link_libraries(platform PRIVATE base SDL3::SDL3 OpenGL)

add_executable(game
  src/main.cpp
  src/game/game.h src/game/game.cpp
  src/game/vertex.h src/game/vertex.cpp
  src/game/assets.h src/game/assets.cpp
  src/game/camera.h src/game/camera.cpp
  src/game/entity.h src/game/entity.cpp
  src/game/image.h src/game/image.cpp
  src/game/renderer.h src/game/renderer.cpp
  src/game/parser.h src/game/parser.cpp
  src/game/sound.h src/game/sound.cpp
  src/game/ui.h src/game/ui.cpp
)
target_include_directories(game PRIVATE ${include_dirs})
target_compile_options(game PRIVATE ${flags})
target_compile_definitions(game PRIVATE ${definitions})
target_link_libraries(game PRIVATE base platform stb OpenGL)

# NOTE: run a python script to collect todos from source files
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  find_package(Python3 REQUIRED)

  set(FIND_TODOS_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/find_todos.py")
  add_custom_target(
      run_find_todos ALL
      COMMAND ${Python3_EXECUTABLE} "${FIND_TODOS_SCRIPT}"
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      COMMENT "Gathering todos from source files (find_todos.py)"
      VERBATIM
  )
  add_dependencies(run_find_todos game)
endif()
