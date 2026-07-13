message(STATUS "building drivers...")

add_subdirectory(uart_rx)
add_subdirectory(uart_tx)
add_subdirectory(uart_log)
add_subdirectory(uart_bridge)
add_subdirectory(led_builtin)
add_subdirectory(led_status)
add_subdirectory(psram_apsxx04)
add_subdirectory(ili9488_40_hstx)
add_subdirectory(system_time)
add_subdirectory(temp_builtin)
add_subdirectory(xpt2046)
add_subdirectory(sd_card)
add_subdirectory(usb_device)


add_library(driver INTERFACE)

target_link_libraries(driver INTERFACE
  lib
  uart_rx
  uart_tx
  uart_log
  uart_bridge
  led_builtin
  led_status
  psram_apsxx04
  ili9488_40_hstx
  system_time
  temp_builtin
  xpt2046
  sd_card
  usb_device
)

message(STATUS "building drivers completed!")
