cmake_minimum_required(VERSION 3.15)

# This needs to be set before project()
if (WIN32)
  set(CMAKE_SYSTEM_VERSION 10.0.18362.0)
endif()

project (UISample C CXX)

add_executable(UISample)
set(EXECUTABLE_NAME "UISample")

target_sources(${EXECUTABLE_NAME} PUBLIC
  ./apps/UISample/UISample.cpp
)

target_compile_definitions(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:COGS_DEBUG>)

include_directories(
  core/src
)

# TODO: Add support for architectures other than x64 and x86
if (CMAKE_SIZEOF_VOID_P MATCHES 8)  # 64-bit
  set(X64 1)
  message(STATUS "Configured for x64")
  include_directories(
    core/arch/x64/src
  )
elseif (CMAKE_SIZEOF_VOID_P MATCHES 4)  # 32-bit
  set(X86 1)
  message(STATUS "Configured for x86")
  include_directories(
    core/arch/x86/src
  )
else()
  message(ERROR ": Unknown target architecture")
endif()

if (WIN32)
###################################################################### WIN32
  message(STATUS "Configured for Win32")

  target_compile_definitions(${EXECUTABLE_NAME} PUBLIC -DUNICODE -D_UNICODE -DWIN32 -DWINDOWS -DCOGS_DEFAULT_UI_SUBSYSTEM=COGS_GUI -DCOGS_DEFAULT_GUI_SUBSYSTEM=COGS_GDI)

  target_sources(${EXECUTABLE_NAME} PUBLIC
    ./apps/UISample/VS/UISample.manifest
    ./apps/UISample/VS/UISample.rc
  )
  
  include_directories(
    core/os/windows/src
  )

elseif (APPLE)
###################################################################### MACOS
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

  target_link_libraries(${EXECUTABLE_NAME} "-framework CoreFoundation" "-framework CoreGraphics" "-framework Cocoa")

  configure_file("./apps/UISample/CMake/Info.plist.in" "./Info.plist")
  target_link_libraries(${EXECUTABLE_NAME} "-sectcreate __TEXT __info_plist ./Info.plist")

elseif (LINUX OR UNIX)
###################################################################### LINUX/UNIX
  message(STATUS "Configured for Linux/Unix")
  include_directories(
    core/os/linux/src
    core/os/libs/pthreads
  )
else()
  message(ERROR ": Unknown Platform")

endif()
######################################################################


if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
###################################################################### CLANG
  if (CMAKE_CXX_SIMULATE_ID MATCHES "MSVC")
  ################################################################ CLANG/MSVC WIN32
    set(CLANGCL 1)
    message(STATUS "Configured for clang-cl")
    # TODO: Create a separate env layer for Clang on Win32?
    include_directories(
      core/env/VS/Windows/src
    )
    # On Windows, clang-cl and MSVC linker are used, so MSVC settings are used (below)
  else()
    set(CLANG 1)
    message(STATUS "Configured for clang")
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -std=c++17 -fmodules)
    if (APPLE)
    ################################################################ CLANG MACOS
      # TODO: Create a separate env layer for Clang on MacOS?
      include_directories(
        core/env/XCode/MacOS/src
      )
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -fobjc-arc -ObjC++)
    elseif (LINUX OR UNIX)
    ################################################################ CLANG LINUX/UNIX
      # TODO: Create a separate env layer for Clang on *nux?
      include_directories(
        core/env/gcc/src
      )
    endif()
  endif()
  ################################################################
 
  if (X64)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16) 
  elseif (X32)
  endif()

  # TODO
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:>)
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:>)

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
###################################################################### GCC
  set(GCC 1)
  message(STATUS "Configured for gcc")

  include_directories(
    core/env/gcc/src
  )

  target_compile_options(${EXECUTABLE_NAME} PUBLIC -std=gnu++17 -fmax-errors=1 -fzero-initialized-in-bss -fstrict-aliasing -Wstrict-aliasing=3 -mpopcnt -msse4.2 -Winit-self -Wformat-nonliteral -Wpointer-arith -fno-exceptions -g -lrt -latomic) 

  target_link_libraries(${EXECUTABLE_NAME} pthread)
  
  if (APPLE)
  ################################################################ GCC MACOS
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -x objective-c++) 
  endif()
  ################################################################

  if (X64)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16 -m64) 
  elseif (X86)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16 -m32) 
  endif()

  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:>)
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:>)

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
###################################################################### INTEL
  set(INTEL 1)
  message(STATUS "Configured for Intel")

  include_directories(
    # TODO
  )

  # TODO 
  message(ERROR ": Intel compiler not yet supported")
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:>)
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:>)

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC") 
###################################################################### MSVC
  set(MSVC 1)
  message(STATUS "Configured for msvc")
  
  include_directories(
    core/env/VS/Windows/src
  )
  if (WIN32)
    # Can't use /permissive- yet, due to a compiler bug: https://developercommunity.visualstudio.com/content/problem/643831/use-of-permissive-results-in-unresolved-external-s.html
    #target_compile_options(${EXECUTABLE_NAME} PUBLIC /W3 /std:c++latest /bigobj /MP3 /Oi /Gm- /Zi /permissive-)
    #target_compile_options(${EXECUTABLE_NAME} PUBLIC /W3 /std:c++latest /bigobj /MP3 /Oi /Gm- /Zi /Zc:referenceBinding /Zc:strictStrings /Zc:rvalueCast /Zc:ternary)
    #target_compile_options(${EXECUTABLE_NAME} PUBLIC /JMC /MP /W3 /Zc:wchar_t /Zi /Gm- /Ob0 /Zc:inline /fp:precise /WX- /Zc:forScope /Gd /Oy- /Oi /std:c++latest /FC /EHa /nologo /bigobj /MP3 /Zc:referenceBinding /Zc:strictStrings /Zc:rvalueCast /Zc:ternary)

    #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:/MDd /GS /Od /RTC1 >)
    #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:/Zc:inline /Gd /Oy /MD /FC /O2 /GS- /GL >)

    #target_link_options(${EXECUTABLE_NAME} PUBLIC /NXCOMPAT /DYNAMICBASE /DEBUG /MACHINE:X64 /INCREMENTAL:NO /SUBSYSTEM:WINDOWS /TLBID:1)
  else()
    message(ERROR ": Unsupported target for MSVC")
  endif()

endif()
######################################################################

###################################################################### MSVC OR CLANG-CL
if (MSVC OR CLANGCL)
  target_compile_options(${EXECUTABLE_NAME} PUBLIC /W3 /std:c++latest /bigobj /MP3 /Oi /Gm- /Zi /Zc:referenceBinding /Zc:strictStrings /Zc:rvalueCast /Zc:ternary)
  target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:/MDd /GS /Od /RTC1 >)
  target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:/Zc:inline /Gd /Oy /MD /FC /O2 /GS- /GL >)
  target_link_options(${EXECUTABLE_NAME} PUBLIC /INCREMENTAL:NO /NXCOMPAT /DYNAMICBASE)
endif()
######################################################################


set(CMAKE_VERBOSE_MAKEFILE on)

get_target_property(INCS ${EXECUTABLE_NAME} INCLUDE_DIRECTORIES)
message("Include directories: ${INCS}")

get_target_property(DEFS ${EXECUTABLE_NAME} COMPILE_DEFINITIONS)
message("Compile definitions: ${DEFS}")

get_target_property(C_OPTS ${EXECUTABLE_NAME} COMPILE_OPTIONS)
message("Compile options: ${C_OPTS}")

get_target_property(LINK_OPTS ${EXECUTABLE_NAME} LINK_OPTIONS)
message("Link options: ${LINK_OPTS}")
