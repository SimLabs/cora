ADD_EXECUTABLE(json_io_tests tests.cpp)

SET(RAPIDJSON_DIR "" CACHE STRING "rapidjson location")

TARGET_INCLUDE_DIRECTORIES(json_io_tests PRIVATE ${RAPIDJSON_DIR})

TARGET_LINK_LIBRARIES(json_io_tests gtest gtest_main)
