cmake_minimum_required(VERSION 3.15)

project (UISample)

include_directories(
    core/src
    core/arch/x64/src
    core/env/VS/Windows/src
    core/os/windows/src
)

add_executable(UISample
    ./apps/UISample/UISample.cpp
)

# TODO: Add support for targetting other platforms.  x86/x64/arm/etc, may require passing different compiler args

# TODO: Add support for targetting other platforms.  Currently always settings -DWIN32 and -DWINDOWS
target_compile_definitions(UISample PUBLIC -DUNICODE -D_UNICODE -DCOGS_DEBUG -DDEBUG -DWIN32 -DWINDOWS -DCOGS_DEFAULT_GUI_SUBSYSTEM=GDI)


#if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
#
#  # using Clang

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # using regular Clang or AppleClang

target_compile_options(UISample PUBLIC /std:c++17 -mcx16)

#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -mcx16")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mcx16")

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")

  # using GCC
  # TODO

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")

  # using Intel C++
  # TODO

elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

  # using Visual Studio C++

# target_compile_options(UISample PUBLIC /std:c++latest /bigobj /MP3 /Oi /Gm- /Zi /permissive-)
# Can't use /permissive- yet, due to a compiler bug(?): https://developercommunity.visualstudio.com/content/problem/643831/use-of-permissive-results-in-unresolved-external-s.html
target_compile_options(UISample PUBLIC /W3 /std:c++latest /bigobj /MP3 /Oi /Gm- /Zi /Zc:referenceBinding /Zc:strictStrings /Zc:rvalueCast /Zc:ternary)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /GS /Od")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zc:inline /Gd /Oy /MD /FC /O2 /GS- /GL")

target_link_options(UISample PUBLIC /INCREMENTAL:NO /DEBUG /NXCOMPAT /DYNAMICBASE /OPT:REF /OPT:ICF)

endif()


set(CMAKE_VERBOSE_MAKEFILE on)


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

