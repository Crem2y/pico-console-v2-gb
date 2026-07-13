get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

message(STATUS "building lib \"${LIB_NAME}\"...")

set(OUTPUT_STATIC_LIB ${LIB_NAME})

add_library(${OUTPUT_STATIC_LIB} ${LIB_NAME}.c)

pico_generate_pio_header(${LIB_NAME} ${CMAKE_CURRENT_LIST_DIR}/uart_tx.pio)

target_include_directories(${OUTPUT_STATIC_LIB} PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${OUTPUT_STATIC_LIB}
  pico_stdlib
  hardware_pio
)

message(STATUS "building lib \"${LIB_NAME}\" completed!")