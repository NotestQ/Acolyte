add_library(safetyhook OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/safetyhook.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Zydis.c
)

target_include_directories(safetyhook
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

install(TARGETS safetyhook
  ARCHIVE DESTINATION lib OPTIONAL
)
