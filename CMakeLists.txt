cmake_minimum_required(VERSION 3.20)

# Define the project name and where to report bugs.

project(TinyLogger LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_COMPILER g++)
    set(CMAKE_CXX_FLAGS "-std=c++2a -Wall -Wextra -Werror -Wpedantic")
elseif(MSVC)
    add_compile_options(/W4)
endif()
set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)



include_directories(${PROJECT_SOURCE_DIR}/src)

file(GLOB SOURCES "src/*.cpp")

add_executable(app ${SOURCES})

target_link_libraries(app PUBLIC ${LIBS})


include(GNUInstallDirs)
message("GNU_INSTALL_BINDIR:  ${CMAKE_INSTALL_BINDIR}")
install(TARGETS app RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})