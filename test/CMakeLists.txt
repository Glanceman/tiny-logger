cmake_minimum_required(VERSION 3.20)

project(TinyLoggerTests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -Wpedantic")
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Optional but handy for debugging builds
# -----------------------
# Test executable
# -----------------------
file(GLOB TEST_SOURCES "main.cpp")
add_executable(tests ${TEST_SOURCES})
target_include_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_directories(tests PRIVATE
    ${CMAKE_SOURCE_DIR}/dist_static_lib
)

target_link_libraries(tests PRIVATE logger)
