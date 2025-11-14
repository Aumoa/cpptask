cmake_minimum_required(VERSION 3.12)
project(cpptask VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
	add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

file(GLOB_RECURSE SOURCES 
	"src/*.cpp"
	"src/*.cc"
	"src/*.cxx"
)

add_library(cpptask ${SOURCES})

target_include_directories(cpptask 
	PUBLIC 
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(cpptask PUBLIC cxx_std_23)

if(WIN32)
    target_compile_definitions(cpptask PUBLIC PLATFORM_WINDOWS)
    if(MSVC)
        target_compile_definitions(cpptask PUBLIC COMPILER_MSVC)
    endif()
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(cpptask PUBLIC PLATFORM_LINUX)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_definitions(cpptask PUBLIC COMPILER_GCC)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_definitions(cpptask PUBLIC COMPILER_CLANG)
    endif()
elseif(APPLE)
    target_compile_definitions(cpptask PUBLIC PLATFORM_MACOS)
endif()

find_package(Threads REQUIRED)
target_link_libraries(cpptask PRIVATE Threads::Threads)

install(TARGETS cpptask
    EXPORT cpptask-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(EXPORT cpptask-targets
    FILE cpptask-config.cmake
    DESTINATION lib/cmake/cpptask
)