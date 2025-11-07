# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/jonathan/esp/v5.5.1/esp-idf/components/bootloader/subproject"
  "/home/jonathan/Documents/Git/VISION-AI/esp_bengala/firmware/build/bootloader"
  "/home/jonathan/Documents/Git/VISION-AI/esp_bengala/firmware/build/bootloader-prefix"
  "/home/jonathan/Documents/Git/VISION-AI/esp_bengala/firmware/build/bootloader-prefix/tmp"
  "/home/jonathan/Documents/Git/VISION-AI/esp_bengala/firmware/build/bootloader-prefix/src/bootloader-stamp"
  "/home/jonathan/Documents/Git/VISION-AI/esp_bengala/firmware/build/bootloader-prefix/src"
  "/home/jonathan/Documents/Git/VISION-AI/esp_bengala/firmware/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/jonathan/Documents/Git/VISION-AI/esp_bengala/firmware/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/jonathan/Documents/Git/VISION-AI/esp_bengala/firmware/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
