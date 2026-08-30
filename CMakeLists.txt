cmake_minimum_required(VERSION 3.23)
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
project(GazolandGame
  VERSION 1.0.0
)
add_library(glfw STATIC IMPORTED)
add_library(vulkan STATIC IMPORTED)
if(WIN32)
  set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION "${GLFW_PATH}/lib-vc2022/glfw3.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${GLFW_PATH}/include"
  )
  set_target_properties(vulkan PROPERTIES
    IMPORTED_LOCATION "${VULKAN_PATH}/Lib/vulkan-1.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_PATH}/Include"
  )
endif()
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE
  ${GLM_PATH}
)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/assets/spirv)
set(VERT_SHADER_SOURCES
  vert_3d
  vert_basic
  vert_gazo
  vert_no_uv_map
  vulkan_demo_vert
)
foreach(src IN LISTS VERT_SHADER_SOURCES)
  add_custom_command(
    OUTPUT "assets/spirv/${src}.spv"
    DEPENDS "assets/glsl/${src}.glsl"
    COMMAND ${VULKAN_PATH}/Bin/glslc -c -fshader-stage=vertex
        -o ${CMAKE_CURRENT_BINARY_DIR}/assets/spirv/${src}.spv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/glsl/${src}.glsl)
endforeach()

set(FRAG_SHADER_SOURCES
  frag_basic
  frag_gamma
  frag_gui
  vulkan_demo_frag
)
foreach(src IN LISTS FRAG_SHADER_SOURCES)
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/assets/spirv/${src}.spv"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/glsl/${src}.glsl"
    COMMAND ${VULKAN_PATH}/Bin/glslc -c -fshader-stage=fragment -o ${CMAKE_CURRENT_BINARY_DIR}/assets/spirv/${src}.spv ${CMAKE_CURRENT_SOURCE_DIR}/assets/glsl/${src}.glsl
)
endforeach()

add_custom_target(spirv_shaders
  DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/assets/spirv/vert_3d.spv"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/spirv/vert_basic.spv"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/spirv/vert_gazo.spv"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/spirv/vert_no_uv_map.spv"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/spirv/frag_basic.spv"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/spirv/frag_gamma.spv"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/spirv/frag_gui.spv"
)

add_executable(GazolandGame)
add_dependencies(GazolandGame spirv_shaders)
target_sources(GazolandGame
  PRIVATE
    src/embeds.c
    src/gazo.cpp
    src/gazoland_vulkan.cpp
    src/level.cpp
    src/main.cpp
    src/path.cpp
    src/platform.cpp
)
target_compile_definitions(GazolandGame PUBLIC GAZOLAND_VULKAN)
target_link_libraries(GazolandGame PRIVATE
  glfw
  vulkan
  $<$<PLATFORM_ID:Windows>:pathcch.lib>
)

add_executable(VulkanDemo)
target_sources(VulkanDemo
  PRIVATE
    src/path.cpp
    src/vulkan_demo.cpp
)
target_link_libraries(VulkanDemo
  PRIVATE
    glm
    glfw
    vulkan
    $<$<PLATFORM_ID:Windows>:pathcch.lib>
)
add_custom_target(vulkan_demo_shaders
  DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/assets/spirv/vulkan_demo_frag.spv"
    "${CMAKE_CURRENT_BINARY_DIR}/assets/spirv/vulkan_demo_vert.spv"
)
add_dependencies(VulkanDemo vulkan_demo_shaders)