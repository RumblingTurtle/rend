cmake_minimum_required(VERSION 3.15)

project(rend)

set(CMAKE_CXX_STANDARD 17)

add_compile_definitions(ASSET_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/assets")

find_package(Vulkan REQUIRED)
find_package(assimp REQUIRED)
find_package(Eigen3 REQUIRED)

add_subdirectory(vk-bootstrap)
add_subdirectory(VulkanMemoryAllocator)

# Shader compilation targets
file(GLOB VERT_SHADER_SOURCES 
    "assets/shaders/*.vert"
    CONFIGUE_DEPENDS
)

file(GLOB  FRAG_SHADER_SOURCES 
"assets/shaders/*.frag"
CONFIGUE_DEPENDS
)

message(STATUS "Vertex shaders:\n ${VERT_SHADER_SOURCES}")
message(STATUS "Fragment shaders:\n ${FRAG_SHADER_SOURCES}")

set(SHADER_SOURCES ${VERT_SHADER_SOURCES} ${FRAG_SHADER_SOURCES})
set(SHADER_TARGETS)
foreach(SHADER_SOURCE ${SHADER_SOURCES})
    message("Shader source: ${SHADER_SOURCE}")
    get_filename_component(SHADER_FILENAME ${SHADER_SOURCE} NAME)
    get_filename_component(SHADER_NAME_STEM ${SHADER_SOURCE} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER_SOURCE} EXT)
    
    message(STATUS "Shader name: ${SHADER_FILENAME}")

    string(REPLACE "." "" EXT_NO_DOT ${SHADER_EXT})
    add_custom_target( ${SHADER_NAME_STEM}_${EXT_NO_DOT}_shader
        COMMAND glslangValidator 
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/${SHADER_FILENAME} 
        -V 
        -o 
        ${SHADER_NAME_STEM}_${EXT_NO_DOT}.spv 
        -I${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/bin
        COMMENT "Compiling ${SHADER_NAME_STEM}_${EXT_NO_DOT}_shader"
    )

    add_custom_target( ${SHADER_NAME_STEM}_${EXT_NO_DOT}_shader_copy
        COMMAND cp ${SHADER_NAME_STEM}_${EXT_NO_DOT}.spv ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/bin
        COMMENT "Copying ${SHADER_NAME_STEM}_${EXT_NO_DOT}_shader"
        DEPENDS ${SHADER_NAME_STEM}_${EXT_NO_DOT}_shader
        
    )

    list(APPEND SHADER_TARGETS ${SHADER_NAME_STEM}_${EXT_NO_DOT}_shader_copy)
endforeach()

message(STATUS "Generated targets:\n ${SHADER_TARGETS}")
message(STATUS "Sources:\n ${${CMAKE_PROJECT_NAME}_SOURCES}")

# LIB
set(${CMAKE_PROJECT_NAME}_SOURCES
    rend/src/Renderer.cpp
    rend/src/InputHandler.cpp
    rend/src/Shader.cpp
    rend/src/Mesh.cpp    
    rend/src/Material.cpp  
    rend/src/vma_impl.cpp  
)

add_library(${CMAKE_PROJECT_NAME} 
    ${${CMAKE_PROJECT_NAME}_SOURCES}
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    ${Vulkan_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    VulkanMemoryAllocator/include
    rend/include/
    vk-bootstrap/src
)

target_link_libraries(${CMAKE_PROJECT_NAME}  PUBLIC
    ${Vulkan_LIBRARIES}
    /usr/lib/x86_64-linux-gnu/libSDL2.so
    vk-bootstrap::vk-bootstrap
    VulkanMemoryAllocator
    assimp
)

add_dependencies(${CMAKE_PROJECT_NAME} ${SHADER_TARGETS})


# Example
add_executable(${CMAKE_PROJECT_NAME}_example
    examples/main.cpp
)  

target_link_libraries(${CMAKE_PROJECT_NAME}_example
    ${CMAKE_PROJECT_NAME}
)
