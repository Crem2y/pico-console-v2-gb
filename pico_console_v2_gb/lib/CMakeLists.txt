add_subdirectory(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/src)

add_library(lib INTERFACE)

target_link_libraries(lib INTERFACE
  no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
)