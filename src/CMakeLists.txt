# src/CMakeLists.txt

add_library(Acolyte SHARED 
  "version.cpp" "core.cpp" "util/logger.cpp" "util/parser.cpp" "util/sdk_registry.cpp" "sdk/playerclass.cpp" "sdk/player.cpp"  "util/vaganteutils.cpp" "util/utils.cpp" "sdk/item.cpp")

# Public headers for consumers
target_include_directories(Acolyte
  PUBLIC
    ${CMAKE_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    "${CMAKE_SOURCE_DIR}/lib/minhook"
)

# Link Safetyhook
target_sources(Acolyte
  PRIVATE
    $<TARGET_OBJECTS:safetyhook>
)
target_include_directories(Acolyte PRIVATE ${CMAKE_SOURCE_DIR}/lib/safetyhook)

# Link MinHook
add_library(MinHook STATIC IMPORTED)
set_target_properties(MinHook PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/minhook/libMinHook.x86.lib"
)
target_link_libraries(Acolyte
  PRIVATE
    MinHook
)

# Exporting
target_compile_definitions(Acolyte PRIVATE ACOLYTE_EXPORTS)
set_target_properties(Acolyte PROPERTIES OUTPUT_NAME "version")

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/Acolyte
  DESTINATION include
  FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

add_custom_command(
    TARGET Acolyte
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/out/release/include"
)