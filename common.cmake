
target_compile_definitions(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:COGS_DEBUG>)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/core/src
)

# TODO: Add support for architectures other than x64 and x86
if (CMAKE_SIZEOF_VOID_P MATCHES 8)  # 64-bit
  set(X64 1)
  message(STATUS "Configured for x64")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/arch/x64/src
  )
elseif (CMAKE_SIZEOF_VOID_P MATCHES 4)  # 32-bit
  set(X86 1)
  message(STATUS "Configured for x86")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/arch/x86/src
  )
else()
  message(ERROR ": Unknown target architecture")
endif()

if (WIN32)
###################################################################### WIN32
  message(STATUS "Configured for Win32")

  target_compile_definitions(${EXECUTABLE_NAME} PUBLIC -DUNICODE -D_UNICODE -DWIN32 -DWINDOWS -DCOGS_DEFAULT_UI_SUBSYSTEM=COGS_GUI -DCOGS_DEFAULT_GUI_SUBSYSTEM=COGS_GDI)

  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/os/windows/src
  )

elseif (APPLE)
###################################################################### MACOS
  message(STATUS "Configured for Apple")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/os/macos/src
  )

  target_link_libraries(${EXECUTABLE_NAME} "-framework CoreFoundation" "-framework CoreGraphics" "-framework Cocoa")

elseif (LINUX OR UNIX)
###################################################################### LINUX/UNIX
  message(STATUS "Configured for Linux/Unix")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/os/linux/src
    ${CMAKE_CURRENT_LIST_DIR}/core/os/libs/pthreads
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
    # If clang + MSVC, MSVC intrinsics, headers, and libraries are used.
    include_directories(
      ${CMAKE_CURRENT_LIST_DIR}/core/env/VS/Windows/src
    )

    # clang 10 can't currently compile ATL headers, so disable use of ATL thunk
    target_compile_definitions(${EXECUTABLE_NAME} PUBLIC -DCOGS_USE_ATL_THUNK=0 -D_SILENCE_CLANG_CONCEPTS_MESSAGE)

    # On Windows, clang-cl and MSVC linker are used, so MSVC settings are used (below)
  else()
    set(CLANG 1)
    message(STATUS "Configured for clang")
    if (APPLE)
    ################################################################ CLANG MACOS
      target_compile_options(${EXECUTABLE_NAME} PUBLIC -fobjc-arc -ObjC++)
    endif()
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -std=gnu++17)
    include_directories(
      ${CMAKE_CURRENT_LIST_DIR}/core/env/gcc/src
    )
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
    ${CMAKE_CURRENT_LIST_DIR}/core/env/gcc/src
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
    ${CMAKE_CURRENT_LIST_DIR}/core/env/VS/Windows/src
  )
  if (WIN32)
  else()
    message(ERROR ": Unsupported target for MSVC")
  endif()

endif()
######################################################################

###################################################################### MSVC OR CLANG-CL
if (MSVC OR CLANGCL)
  target_compile_options(${EXECUTABLE_NAME} PUBLIC /W4 /wd4324 /std:c++latest /bigobj /Oi /Zi /permissive-)
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
