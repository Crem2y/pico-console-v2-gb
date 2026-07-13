message(STATUS "building middlewares...")

add_subdirectory(bridge_protocol)
add_subdirectory(bridge_control)
add_subdirectory(power)
add_subdirectory(charger)
add_subdirectory(gamepad)
add_subdirectory(imu)
add_subdirectory(ir_link)
add_subdirectory(led_control)
add_subdirectory(audio_system)
add_subdirectory(temperature)
add_subdirectory(touchscreen)
add_subdirectory(graphic_system)
add_subdirectory(vibration)


add_library(middleware INTERFACE)

target_link_libraries(middleware INTERFACE
  driver
  bridge_protocol
  bridge_control
  power
  charger
  gamepad
  imu
  ir_link
  led_control
  audio_system
  temperature
  touchscreen
  graphic_system
  vibration
)

message(STATUS "building middlewares completed!")
