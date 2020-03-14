cmake_minimum_required(VERSION 3.15)

# This needs to be set before project()
if (WIN32)
  set(CMAKE_SYSTEM_VERSION 10.0.18362.0)
endif()

set(EXECUTABLE_NAME "Template")
project (${EXECUTABLE_NAME} C CXX)
add_executable(${EXECUTABLE_NAME})

include(../../common.cmake)

target_sources(${EXECUTABLE_NAME} PUBLIC
  Template.cpp
)

if (WIN32)

  target_sources(${EXECUTABLE_NAME} PUBLIC
    VS/Template.manifest
  )

elseif (APPLE)

  set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.cogmine.cogs.Template"
    XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS YES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
  )

    target_sources(${EXECUTABLE_NAME} PUBLIC
    MacOS/main.mm
  )

  configure_file("CMake/Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
  target_link_libraries(${EXECUTABLE_NAME} "-sectcreate __TEXT __info_plist ${CMAKE_CURRENT_BINARY_DIR}/Info.plist")

endif()
