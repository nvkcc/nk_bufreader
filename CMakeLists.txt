cmake_minimum_required(VERSION 3.14)

project(nk_bufread LANGUAGES C CXX)

set(CMAKE_C_STANDARD 90)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build the test suite" ON)

include(FetchContent)
FetchContent_Declare(
  nk_log
  GIT_REPOSITORY https://github.com/nvkcc/nk_log
  GIT_TAG        d29e6da6d7caefe26d7b628afc9233dd3d6615b9
  DOWNLOAD_EXTRACT_TIMESTAMP false
)
FetchContent_MakeAvailable(nk_log)

set(NK_BUFREAD_SOURCES
  nk_bufread.c
)
add_library(nk_bufread ${NK_BUFREAD_SOURCES})
target_include_directories(nk_bufread PRIVATE include/)
target_link_libraries(nk_bufread PRIVATE nk_log)

if(BUILD_TESTS)
  FetchContent_Declare(
    nk_test
    GIT_REPOSITORY https://github.com/nvkcc/nk_test
    GIT_TAG        ef1bdfe8e38d56da41cd922ad5a3f93934aaedb2
    DOWNLOAD_EXTRACT_TIMESTAMP false
  )
  FetchContent_MakeAvailable(nk_test)
  add_executable(nk_bufread-test ${NK_BUFREAD_SOURCES} test.cpp)
  target_include_directories(nk_bufread-test PRIVATE include/)
  target_link_libraries(nk_bufread-test PRIVATE nk_log nk_test)
endif()

# Create compile_commands.json for clangd to latch on to.
file(CREATE_LINK ${CMAKE_BINARY_DIR}/compile_commands.json compile_commands.json SYMBOLIC)
