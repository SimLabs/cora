PROJECT(cora_tests)
CMAKE_MINIMUM_REQUIRED(VERSION 3.11)
SET(CMAKE_CXX_STANDARD 17)

# override gtest default
SET(BUILD_GMOCK OFF CACHE BOOL "")
SET(gtest_force_shared_crt ON CACHE BOOL "" )

INCLUDE_DIRECTORIES("../..")

INCLUDE(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0
)

# cmake 3.11
FetchContent_GetProperties(googletest)
IF(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
ENDIF()

# IN MORE RECENT CMAKE (probably from 3.14) you can do the following
# instead of the previous paragraph
# FetchContent_MakeAvailable(googletest)

ADD_SUBDIRECTORY(json_io_tests)