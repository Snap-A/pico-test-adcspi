cmake_minimum_required(VERSION 3.17)
include(pico_sdk_import.cmake)

project(test_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
add_executable(adc0832
  adc0832.c
  )

pico_enable_stdio_usb(adc0832 1)
pico_enable_stdio_uart(adc0832 1)

pico_add_extra_outputs(adc0832)

target_link_libraries(adc0832 pico_cyw43_arch_none pico_stdlib)
