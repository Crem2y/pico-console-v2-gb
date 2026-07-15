get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

message(STATUS "building lib \"${LIB_NAME}\"...")

set(OUTPUT_STATIC_LIB ${LIB_NAME})

add_library(${OUTPUT_STATIC_LIB} ${LIB_NAME}.c wave_table.c)

target_include_directories(${OUTPUT_STATIC_LIB} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${OUTPUT_STATIC_LIB} PUBLIC
        # compile time configuration of I2S
        # PICO_AUDIO_I2S_MONO_INPUT=1
        #define for our example code
        USE_AUDIO_I2S=1
        # PICO_AUDIO_I2S_DATA_PIN=6
        # PICO_AUDIO_I2S_CLOCK_PIN_BASE=7
)

target_link_libraries(${OUTPUT_STATIC_LIB}
  pico_stdlib
  pico_audio_i2s
  system_time
)

message(STATUS "building lib \"${LIB_NAME}\" completed!")