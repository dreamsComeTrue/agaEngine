cmake_minimum_required(VERSION 3.12)
project (agaEngine)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)

find_package(Vulkan REQUIRED FATAL_ERROR)
find_package(XCB REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
    
include_directories(.)

file(GLOB_RECURSE SOURCES "core/*.cpp" "main/*.cpp" "platform/*.cpp" "render/*.cpp")

add_executable(agaEngine ${SOURCES})

target_include_directories (agaEngine 
    PUBLIC ${VULKAN_INCLUDE_DIRS}
)

execute_process (
    WORKING_DIRECTORY "../data/shaders/"
    COMMAND bash -c "./compile_shaders.sh"
)

execute_process (
    WORKING_DIRECTORY "../data/shaders/"
    COMMAND bash -c "cp *.spv ../../build/data/shaders/"
)

execute_process (
    WORKING_DIRECTORY "../data/textures/"
    COMMAND bash -c "cp -a . ../../build/data/textures/"
)

target_link_libraries (agaEngine ${XCB_LIBRARIES} ${Vulkan_LIBRARIES})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
