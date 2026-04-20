cmake_minimum_required(VERSION 3.14)

project(nk_bufread LANGUAGES C CXX)

set(CMAKE_C_STANDARD 90)

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
  bufread.c
)
add_library(nk_bufread ${NK_BUFREAD_SOURCES})
target_include_directories(nk_bufread PRIVATE include/)
target_link_libraries(nk_bufread PRIVATE nk_log)

if(BUILD_TESTS)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip # 1.17.0
    DOWNLOAD_EXTRACT_TIMESTAMP false
  )
  FetchContent_MakeAvailable(googletest)
  enable_testing()
  include(GoogleTest)

  add_executable(nk_bufread-test ${NK_BUFREAD_SOURCES} test.cpp)
  target_link_libraries(nk_bufread-test PRIVATE nk_log GTest::gtest_main GTest::gmock_main)
  gtest_discover_tests(nk_bufread-test)
endif()
