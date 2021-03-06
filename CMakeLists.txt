cmake_minimum_required(VERSION 3.13)
project(SpaceMemories VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(third_party/spdlog)

option(BUILD_SHARED_LIBS off)
option(SFML_BUILD_AUDIO off)
option(SFML_BUILD_NETWORK off)
add_subdirectory(third_party/SFML)

option(JSON_BuildTests off)
option(JSON_Install off)
option(JSON_ImplicitConversions off)
add_subdirectory(third_party/json)

option(IMGUI_SFML_FIND_SFML off)
set(IMGUI_SFML_IMGUI_DEMO on CACHE BOOL "")
set(IMGUI_DIR "${CMAKE_CURRENT_LIST_DIR}/third_party/imgui" CACHE PATH "")
add_subdirectory(third_party/imgui-sfml)

add_subdirectory(src/core)
add_subdirectory(src/main)
add_subdirectory(src/systems)
add_subdirectory(src/components)
add_subdirectory(src/services)
add_subdirectory(src/prototype_component_builders)
add_subdirectory(src/helpers)

function(add_resource_symlink_target)
  add_custom_target(
    create_resource_symlink ALL COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    ${CMAKE_CURRENT_BINARY_DIR}/resources
  )
  add_custom_target(
    create_prototypes_symlink ALL COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/prototypes
    ${CMAKE_CURRENT_BINARY_DIR}/prototypes
  )
  add_custom_target(
    create_maps_symlink ALL COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/maps
    ${CMAKE_CURRENT_BINARY_DIR}/maps
  )
  add_custom_target(
    create_system_exec_priority_symlink ALL COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_CURRENT_SOURCE_DIR}/systems-execute-priorities.txt
    ${CMAKE_CURRENT_BINARY_DIR}/systems-execute-priorities.txt
  )
endfunction()

if(WIN32)
  option(WINDOWS_CREATE_RESOURCE_SYMLINK "Requires admin privileges" off)
  if(${WINDOWS_CREATE_RESOURCE_SYMLINK})
    add_resource_symlink_target()
  endif()
else()
  add_resource_symlink_target()
endif()
