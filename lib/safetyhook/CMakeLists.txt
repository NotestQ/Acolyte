add_library(safetyhook OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/safetyhook.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Zydis.c
)

target_include_directories(safetyhook
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(safetyhook PRIVATE SAFETYHOOK_SHARED_LIB=1 SAFETYHOOK_BUILDING=1)

install(FILES safetyhook.hpp DESTINATION include)

install(TARGETS safetyhook
  ARCHIVE DESTINATION lib OPTIONAL
)
