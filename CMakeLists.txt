cmake_minimum_required(VERSION 3.30)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ZSTD REQUIRED IMPORTED_TARGET libzstd)
pkg_check_modules(ZLIB REQUIRED IMPORTED_TARGET zlib)
pkg_check_modules(JEMALLOC REQUIRED IMPORTED_TARGET jemalloc)

project(ts-server VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add_compile_options(-flto)
add_compile_options(-Wall)
add_compile_options(-Werror)
add_compile_options(-Wno-deprecated-declarations)
add_compile_options(-Wno-missing-braces)
add_compile_options(-Wno-unused)
add_compile_options(-Wno-expansion-to-defined)
add_compile_options(-Wno-error=address)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-O0 -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3 -ffast-math -fno-finite-math-only -fvisibility=hidden -DNDEBUG")

if(APPLE)
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -stdlib=libc++")
elseif(UNIX)
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-psabi")
endif()

include_directories(src/)
include_directories(third_party/)
include_directories(third_party/uWebSockets/uSockets/src/)

file(GLOB SOURCES src/*.cpp)
file(GLOB USOCKETS_OBJS third_party/uWebSockets/uSockets/*.o)

add_executable(ts-server ${SOURCES})

target_compile_definitions(ts-server PRIVATE "MAX_LOADED_CHUNKS=$<IF:$<CONFIG:Release>,16,4>")

target_link_libraries(ts-server PUBLIC PkgConfig::ZSTD PkgConfig::ZLIB PkgConfig::JEMALLOC ${USOCKETS_OBJS})
