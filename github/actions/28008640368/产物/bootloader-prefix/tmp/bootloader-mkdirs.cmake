# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/opt/esp/idf/components/bootloader/subproject"
  "/app/XUE2026/MIDI-Link/esp32-midi-gateway/build/bootloader"
  "/app/XUE2026/MIDI-Link/esp32-midi-gateway/build/bootloader-prefix"
  "/app/XUE2026/MIDI-Link/esp32-midi-gateway/build/bootloader-prefix/tmp"
  "/app/XUE2026/MIDI-Link/esp32-midi-gateway/build/bootloader-prefix/src/bootloader-stamp"
  "/app/XUE2026/MIDI-Link/esp32-midi-gateway/build/bootloader-prefix/src"
  "/app/XUE2026/MIDI-Link/esp32-midi-gateway/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/app/XUE2026/MIDI-Link/esp32-midi-gateway/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/app/XUE2026/MIDI-Link/esp32-midi-gateway/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
