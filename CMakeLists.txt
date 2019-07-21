cmake_minimum_required(VERSION 3.15)

project (UISample)

add_executable(UISample
    ./apps/UISample/UISample.cpp
)

include_directories(
    core/src
)

if (WIN32)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /GS /Od /DDEBUG /DCOGS_DEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zc:inline /Gd /Oy /MD /FC /O2 /GS- /GL")

  target_compile_definitions(UISample PUBLIC -DUNICODE -D_UNICODE -DWIN32 -DWINDOWS -DCOGS_DEFAULT_GUI_SUBSYSTEM=GDI)

  # Can't use /permissive- yet, due to a compiler bug: https://developercommunity.visualstudio.com/content/problem/643831/use-of-permissive-results-in-unresolved-external-s.html
  #target_compile_options(UISample PUBLIC /W3 /std:c++latest /bigobj /MP3 /Oi /Gm- /Zi /permissive-)
  target_compile_options(UISample PUBLIC /W3 /std:c++latest /bigobj /MP3 /Oi /Gm- /Zi /Zc:referenceBinding /Zc:strictStrings /Zc:rvalueCast /Zc:ternary)

  include_directories(
      core/os/windows/src
      core/env/VS/Windows/src
  )

  if (CMAKE_SIZEOF_VOID_P MATCHES 8)
    message(STATUS "Configured for Win32 x64")
    include_directories(
        core/arch/x64/src
    )
  elseif (CMAKE_SIZEOF_VOID_P MATCHES 4)
    message(STATUS "Configured for Win32 x86")
    include_directories(
        core/arch/x86/src
    )
  else()
    message(ERROR "Unknown target architecture")
  endif()

  target_link_options(UISample PUBLIC /INCREMENTAL:NO /DEBUG /NXCOMPAT /DYNAMICBASE /OPT:REF /OPT:ICF)

  target_link_libraries(UISample
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
elseif (LINUX)
endif()

set(CMAKE_VERBOSE_MAKEFILE on)
