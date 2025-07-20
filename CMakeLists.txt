cmake_minimum_required(VERSION 3.20)

# Define the project name and where to report bugs.

project(TinyLogger LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -Wpedantic")
elseif(MSVC)
    add_compile_options(/W4)
endif()

# Optional but handy for debugging builds
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist_static_lib")

message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "CMAKE_ARCHIVE_OUTPUT_DIRECTORY: ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")

# -----------------------
# Static library: logger
# -----------------------
file(GLOB LOGGER_SOURCES "src/logger/*.cpp")
add_library(logger STATIC ${LOGGER_SOURCES})
message(STATUS "Logger sources: ${LOGGER_SOURCES}")

target_include_directories(logger PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    )


add_subdirectory(${PROJECT_SOURCE_DIR}/test)


# include(GNUInstallDirs)
# message("GNU_INSTALL_BINDIR:  ${CMAKE_INSTALL_BINDIR}")
# install(TARGETS app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})