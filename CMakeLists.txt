cmake_minimum_required(VERSION 3.12)
project (agaEngine)

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED FATAL_ERROR)

include_directories(.)

file(GLOB SOURCES "core/*.cpp" "main/*.cpp" "render/*.cpp")

add_executable(agaEngine ${SOURCES})

target_include_directories (agaEngine 
    PUBLIC ${VULKAN_INCLUDE_DIRS}
)

target_link_libraries (agaEngine ${Vulkan_LIBRARIES})