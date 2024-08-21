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
FILE(GLOB_RECURSE codeFiles "src/*")
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

  # Create a custom target to compile shaders
  add_custom_target(compile_shaders ALL DEPENDS ${shaderOutputs})

  # Ensure that shaders are compiled before building the executable
  add_dependencies(TriangleVK compile_shaders)

endif()