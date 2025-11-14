cmake_minimum_required(VERSION 3.12)
project(threading-tasks VERSION 1.0.0 LANGUAGES CXX)

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

add_library(threading-tasks ${SOURCES})

target_include_directories(threading-tasks 
	PUBLIC 
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(threading-tasks PUBLIC cxx_std_23)

if(WIN32)
    target_compile_definitions(threading-tasks PUBLIC PLATFORM_WINDOWS)
    if(MSVC)
        target_compile_definitions(threading-tasks PUBLIC COMPILER_MSVC)
    endif()
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(threading-tasks PUBLIC PLATFORM_LINUX)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_definitions(threading-tasks PUBLIC COMPILER_GCC)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_definitions(threading-tasks PUBLIC COMPILER_CLANG)
    endif()
elseif(APPLE)
    target_compile_definitions(threading-tasks PUBLIC PLATFORM_MACOS)
endif()

find_package(Threads REQUIRED)
target_link_libraries(threading-tasks PRIVATE Threads::Threads)

install(TARGETS threading-tasks
    EXPORT threading-tasks-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

install(EXPORT threading-tasks-targets
    FILE threading-tasks-config.cmake
    DESTINATION lib/cmake/threading-tasks
)