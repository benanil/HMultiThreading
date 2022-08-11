cmake_minimum_required(VERSION 3.10)

project(HMultiThreading)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
	add_link_options(/DEBUG:FULL)
	add_compile_options(/Oi /GR- /D_HAS_EXCEPTIONS=0)
endif()

file(GLOB_RECURSE HSOURCES readerwriterqueue/*.h HContainers/HContainers/*.hpp HMultiThreading/*.hpp HContainers/HContainers/*.cpp HMultiThreading/*.cpp)

add_executable(HMultiThreading ${HSOURCES})
target_include_directories(HMultiThreading PUBLIC HMultiThreading)

target_compile_definitions(HMultiThreading PUBLIC NOMINMAX)