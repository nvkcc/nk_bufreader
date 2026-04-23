cmake_minimum_required(VERSION 3.14)

project(nk_bufreader LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build the test suite" ON)

add_library(nk_bufreader nk_bufreader.c log.c)
target_include_directories(nk_bufreader PUBLIC include/)

if(BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    nk_test
    GIT_REPOSITORY https://github.com/nvkcc/nk_test
    GIT_TAG        9b5e86dee066dfce8db73f007f68946adadbe74b
    DOWNLOAD_EXTRACT_TIMESTAMP false
  )
  FetchContent_MakeAvailable(nk_test)
  add_executable(nk_bufreader-test nk_bufreader.c log.c test.cpp)
  target_include_directories(nk_bufreader-test PRIVATE include/)
  target_link_libraries(nk_bufreader-test PRIVATE nk_test)
endif()
