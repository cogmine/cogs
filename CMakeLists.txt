cmake_minimum_required(VERSION 3.15)

project (UISample)

add_executable(UISample
    ./apps/UISample/UISample.cpp
)

set(EXECUTABLE_NAME "UISample")

include_directories(
    core/src
)

# TODO: Add support for architetures other than x64 and x86
if (CMAKE_SIZEOF_VOID_P MATCHES 8)  # 64-bit
  message(STATUS "Configured for x64")
  include_directories(
      core/arch/x64/src
  )
elseif (CMAKE_SIZEOF_VOID_P MATCHES 4)  # 32-bit
  message(STATUS "Configured for x86")
  include_directories(
      core/arch/x86/src
  )
else()
  message(ERROR ": Unknown target architecture")
endif()

if (WIN32) 
  message(STATUS "Configured for Win32")

  target_compile_definitions(${EXECUTABLE_NAME} PUBLIC -DUNICODE -D_UNICODE -DWIN32 -DWINDOWS -DCOGS_DEFAULT_GUI_SUBSYSTEM=GDI)

  include_directories(
      core/os/windows/src
  )

  target_link_libraries(${EXECUTABLE_NAME}
    Shcore.lib
    Gdiplus.lib
    Normaliz.lib
    Mswsock.lib
    ws2_32.lib
    dwmapi.lib
    msimg32.lib
    ComCtl32.lib
    WINMM.LIB
    kernel32.lib
    user32.lib
    gdi32.lib
    winspool.lib
    comdlg32.lib
    advapi32.lib
    shell32.lib
    ole32.lib
    oleaut32.lib
    uuid.lib
    odbc32.lib
    odbccp32.lib
  )

elseif (APPLE)
  message(STATUS "Configured for Apple")
  include_directories(
      core/os/macos/src
  )

  target_sources(${EXECUTABLE_NAME} PUBLIC
    ./apps/UISample/XCode/UISample/UISample/AppDelegate.m
    ./apps/UISample/XCode/UISample/UISample/main.m
    ./apps/UISample/XCode/UISample/UISample/Base.lproj/MainMenu.xib
  )

  set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.cogmine.cogs.UISample"
    RESOURCE "./apps/UISample/XCode/UISample/UISample/Base.lproj/MainMenu.xib"
    XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS YES
    MACOSX_BUNDLE_INFO_PLIST ./Info.plist
    BUNDLE TRUE
  )

  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") 
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -x objective-c++) 
  endif()

  target_link_libraries(${EXECUTABLE_NAME} "-framework CoreFoundation" "-framework CoreGraphics" "-framework Cocoa")

  configure_file("./apps/UISample/CMake/Info.plist.in" "./Info.plist")
  target_link_libraries(${EXECUTABLE_NAME} "-sectcreate __TEXT __info_plist ./Info.plist")

elseif (LINUX)
  message(STATUS "Configured for Linux")
  include_directories(
      core/os/linux/src
  )

endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang") 
  message(STATUS "Configured for Clang")

  # using regular Clang or AppleClang
  include_directories(
      core/env/XCode/MacOS/src
  )

  target_compile_options(${EXECUTABLE_NAME} PUBLIC -std=c++17 -fobjc-arc -fmodules -ObjC++) 
 
  if (CMAKE_SIZEOF_VOID_P MATCHES 8)  # 64-bit
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16) 
  elseif (CMAKE_SIZEOF_VOID_P MATCHES 4)  # 32-bit
  endif()

  # TODO
  #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ") 
  #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ") 

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") 
  message(STATUS "Configured for GCC")

  include_directories(
      core/env/gcc/src
  )

  target_compile_options(${EXECUTABLE_NAME} PUBLIC -std=gnu++17 -fmax-errors=1 -fzero-initialized-in-bss -fstrict-aliasing -Wstrict-aliasing=3 -mpopcnt -msse4.2 -Winit-self -Wformat-nonliteral -Wpointer-arith -fno-exceptions -g -lrt -pthread -latomic) 

  if (CMAKE_SIZEOF_VOID_P MATCHES 8)  # 64-bit
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16) 
  elseif (CMAKE_SIZEOF_VOID_P MATCHES 4)  # 32-bit
  endif()

  #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ") 
  #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ") 

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel") 
  message(STATUS "Configured for Intel")

  include_directories(
    # TODO
  )

  # TODO 
  message(ERROR ": Intel compiler not yet supported")
  #set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ") 
  #set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ") 

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC") 
  message(STATUS "Configured for MSVC")
  
  include_directories(
      core/env/VS/Windows/src
  )
  if (WIN32)
    # Can't use /permissive- yet, due to a compiler bug: https://developercommunity.visualstudio.com/content/problem/643831/use-of-permissive-results-in-unresolved-external-s.html
    #target_compile_options(${EXECUTABLE_NAME} PUBLIC /W3 /std:c++latest /bigobj /MP3 /Oi /Gm- /Zi /permissive-)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC /W3 /std:c++latest /bigobj /MP3 /Oi /Gm- /Zi /Zc:referenceBinding /Zc:strictStrings /Zc:rvalueCast /Zc:ternary)
    
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /GS /Od")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zc:inline /Gd /Oy /MD /FC /O2 /GS- /GL")

    target_link_options(${EXECUTABLE_NAME} PUBLIC /INCREMENTAL:NO /DEBUG /NXCOMPAT /DYNAMICBASE /OPT:REF /OPT:ICF)
  else()
    message(ERROR ": Unsupported target for MSVC")
  endif()

endif()

set(CMAKE_VERBOSE_MAKEFILE on)
