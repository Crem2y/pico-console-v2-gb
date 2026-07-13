get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

message(STATUS "building lib \"${LIB_NAME}\"...")

set(OUTPUT_STATIC_LIB ${LIB_NAME})

add_library(${OUTPUT_STATIC_LIB} ${LIB_NAME}.c usb_descriptors.c)

target_include_directories(${OUTPUT_STATIC_LIB} PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${OUTPUT_STATIC_LIB}
  uart_log
  tinyusb_device
  tinyusb_board
)

message(STATUS "building lib \"${LIB_NAME}\" completed!")