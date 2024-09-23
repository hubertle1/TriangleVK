# CMakeList.txt : CMake project for TriangleVK, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.23)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("TriangleVK")

# Add source to this project's executable.
FILE(GLOB_RECURSE codeFiles "src/*" "assets/shaders/*")
add_executable (TriangleVK ${codeFiles})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET TriangleVK PROPERTY CXX_STANDARD 20)
endif()

find_package(Vulkan REQUIRED)
target_link_libraries(TriangleVK Vulkan::Vulkan)

FILE(GLOB_RECURSE shaderFiles "assets/shaders/*")

list(LENGTH shaderFiles fileCount)
if(NOT fileCount EQUAL 0)

  set(shaderOutputs)

  foreach(shaderFile IN LISTS shaderFiles)
    cmake_path(ABSOLUTE_PATH shaderFile NORMALIZE)
    cmake_path(GET shaderFile FILENAME shaderName)
    
    set(outputFile "assets/shaders/${shaderName}.spv")

    add_custom_command(
      OUTPUT ${outputFile}
      COMMAND Vulkan::glslc "${shaderFile}" -o "${outputFile}"
      DEPENDS "${shaderFile}"
      COMMENT "Compiling shader: ${shaderFile}"
    )

    list(APPEND shaderOutputs "${outputFile}")
  endforeach()

  # Add custom command to clean up old shaders
  add_custom_target(clean_shaders
    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning up previous shaders..."
    COMMAND ${CMAKE_COMMAND} -E rm -f ${CMAKE_CURRENT_BINARY_DIR}/*.spv
    COMMENT "Removing old shader files..."
  )

  # Ensure that shaders are cleaned and then compiled
  add_custom_target(compile_shaders ALL
    DEPENDS clean_shaders ${shaderOutputs}
  )

  # Ensure that shaders are compiled before building the executable
  add_dependencies(TriangleVK compile_shaders)

endif()

set(textureSourceDir "${CMAKE_SOURCE_DIR}/assets/textures")

add_custom_command(
  TARGET TriangleVK POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${textureSourceDir}" "${CMAKE_BINARY_DIR}/assets/textures"
  COMMENT "Copying textures to output directory..."
)

add_custom_target(copy_textures ALL
  DEPENDS ${textureSourceDir}
  COMMENT "Ensuring textures are copied to the output directory..."
)

add_dependencies(TriangleVK copy_textures)