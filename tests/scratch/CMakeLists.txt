cmake_minimum_required(VERSION 3.15)

# This needs to be set before project()
if (WIN32)
  set(CMAKE_SYSTEM_VERSION 10.0.18362.0)
endif()

set(EXECUTABLE_NAME "scratch")
project (${EXECUTABLE_NAME} C CXX)
add_executable(${EXECUTABLE_NAME})

include(../../common.cmake)

target_sources(${EXECUTABLE_NAME} PUBLIC
  scratch.cpp
)

if (WIN32)

  target_sources(${EXECUTABLE_NAME} PUBLIC
    VS/scratch.manifest
    VS/scratch.rc
  )

elseif (APPLE)

  set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.cogmine.cogs.scratch"
    RESOURCE "guitar.bmp"
    XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS YES
    MACOSX_BUNDLE_INFO_PLIST ./Info.plist
  )

    target_sources(${EXECUTABLE_NAME} PUBLIC
    MacOS/main.mm
    guitar.bmp
  )

  configure_file("CMake/Info.plist.in" "./Info.plist")
  target_link_libraries(${EXECUTABLE_NAME} "-sectcreate __TEXT __info_plist ./Info.plist")

endif()
