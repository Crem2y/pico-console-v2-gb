get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

message(STATUS "building lib \"${LIB_NAME}\"...")

set(OUTPUT_STATIC_LIB ${LIB_NAME})

add_library(${OUTPUT_STATIC_LIB} ${LIB_NAME}.c)

target_include_directories(${OUTPUT_STATIC_LIB} PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${OUTPUT_STATIC_LIB}
  pico_stdlib
#  pico_cyw43_arch_none # if using Pico W, 2W, CYW43 or Radio Module 2
)

message(STATUS "building lib \"${LIB_NAME}\" completed!")