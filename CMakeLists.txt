cmake_minimum_required(VERSION 3.14)

project(nk_bufreader LANGUAGES C CXX)

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

add_library(nk_bufreader nk_bufreader.c)
target_include_directories(nk_bufreader PUBLIC include/)
target_link_libraries(nk_bufreader PRIVATE nk_log)

if(BUILD_TESTS)
  FetchContent_Declare(
    nk_test
    GIT_REPOSITORY https://github.com/nvkcc/nk_test
    GIT_TAG        ef1bdfe8e38d56da41cd922ad5a3f93934aaedb2
    DOWNLOAD_EXTRACT_TIMESTAMP false
  )
  FetchContent_MakeAvailable(nk_test)
  set(log_level TRACE)
  add_executable(nk_bufreader-test nk_bufreader.c test.cpp)
  target_compile_definitions(nk_bufreader-test PRIVATE NK_LOG_LEVEL=nk${log_level})
  target_include_directories(nk_bufreader-test PRIVATE include/)
  target_link_libraries(nk_bufreader-test PRIVATE nk_log nk_test)
endif()
