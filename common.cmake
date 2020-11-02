set(CMAKE_HOST_SYSTEM_VERSION "10.0.19041")

target_compile_definitions(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:COGS_DEBUG>)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/core/src
)

if (CMAKE_SIZEOF_VOID_P MATCHES 8) # 64-bit
  set(IS64BIT 1)
  message(STATUS "${EXECUTABLE_NAME} - Configured for 64-bit")
elseif (CMAKE_SIZEOF_VOID_P MATCHES 4) # 32-bit
  set(IS32BIT 1)
  message(STATUS "${EXECUTABLE_NAME} - Configured for 32-bit")
else()
  message(ERROR ": ${EXECUTABLE_NAME} - Unsupported bitness")
endif()

if (NOT X86 AND NOT X64 AND NOT ARM AND NOT ARM64)
  # First, try architecture passed to CMake, if any.
  if ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64")
    set(X64 1)
  elseif ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "win32")
    set(X86 1)
  elseif ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "arm")
    set(ARM 1)
  elseif ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "arm64")
    set(ARM64 1)
  elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
    set(X64 1)
  elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "X86" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
    set(X86 1)
  elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "ARM")
    set(ARM 1)
  elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "ARM64")
    set(ARM64 1)
  else()
    message(ERROR ": ${EXECUTABLE_NAME} - Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
  endif()
endif()

if (X64)
  message(STATUS "${EXECUTABLE_NAME} - Configuring for x64")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/arch/x64/src
  )
elseif (X86)
  message(STATUS "${EXECUTABLE_NAME} - Configuring for x86")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/arch/x86/src
  )
elseif (ARM)
  message(STATUS "${EXECUTABLE_NAME} - Configuring for ARM")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/arch/arm/src
  )
elseif (ARM64)
  message(STATUS "${EXECUTABLE_NAME} - Configuring for ARM64")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/arch/arm64/src
  )
else()
  message(ERROR ": ${EXECUTABLE_NAME} - Unknown target architecture")
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  message(STATUS "${EXECUTABLE_NAME} - Configuring for MSVC")
  set(MSVC 1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  message(STATUS "${EXECUTABLE_NAME} - Configuring for gcc")
  set(GCC 1)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    message(STATUS "${EXECUTABLE_NAME} - Configuring for clang-cl")
    set(CLANGCL 1)
  else()
    message(STATUS "${EXECUTABLE_NAME} - Configuring for clang")
    set(CLANG 1)
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
  message(STATUS "${EXECUTABLE_NAME} - Configuring for AppleClang")
  set(APPLECLANG 1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  message(STATUS "${EXECUTABLE_NAME} - Configuring for Intel")
  set(INTEL 1)
else()
  message(ERROR ": ${EXECUTABLE_NAME} - Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

if (WIN32)
###################################################################### WIN32
  message(STATUS "${EXECUTABLE_NAME} - Configured for Win32")

  target_compile_definitions(${EXECUTABLE_NAME} PUBLIC -DUNICODE -D_UNICODE -DWIN32 -DWINDOWS -DCOGS_DEFAULT_UI_SUBSYSTEM=COGS_GUI -DCOGS_DEFAULT_GUI_SUBSYSTEM=COGS_GDI -D_HAS_EXCEPTIONS=0)

  target_link_libraries(${EXECUTABLE_NAME} advapi32 comctl32 comdlg32 dwmapi gdi32 gdiplus kernel32 msimg32 mswsock normaliz odbc32 odbccp32 ole32 oleaut32 shcore shell32 user32 uuid uxtheme winmm winspool ws2_32)

  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/os/windows/src
  )

elseif (APPLE)
###################################################################### MACOS
  message(STATUS "${EXECUTABLE_NAME} - Configured for Apple")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/os/macos/src
  )

  target_link_libraries(${EXECUTABLE_NAME} "-framework CoreFoundation" "-framework CoreGraphics" "-framework Cocoa")

elseif (LINUX OR UNIX)
###################################################################### LINUX/UNIX
  message(STATUS "${EXECUTABLE_NAME} - Configured for Linux/Unix")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/os/linux/src
    ${CMAKE_CURRENT_LIST_DIR}/core/os/libs/pthreads
  )
else()
  message(ERROR ": ${EXECUTABLE_NAME} - Unknown Platform")
endif()


if (APPLECLANG)
###################################################################### APPLECLANG
  target_compile_options(${EXECUTABLE_NAME} PUBLIC -fobjc-arc -ObjC++ -std=gnu++2a)
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/env/gcc/src
  )
  if (X64)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16 -m64)
  elseif (X86)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16 -m32)
  elseif (ARM)
    message(ERROR ": ${EXECUTABLE_NAME} - No ARM args - compiler must be architecture specific by default")
  elseif (ARM64)
    message(ERROR ": ${EXECUTABLE_NAME} - No ARM64 args - compiler must be architecture specific by default")
  endif()

  # TODO
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:>)
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:>)
endif()

if (CLANG)
###################################################################### CLANG
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:)
  target_compile_options(${EXECUTABLE_NAME} PUBLIC -std=gnu++2a)
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/env/gcc/src
  )
  if (X64)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16 -m64 -Wa,-mbig-obj)
  elseif (X86)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16 -m32)
  elseif (ARM)
    message(ERROR ": ${EXECUTABLE_NAME} - No ARM args - compiler must be architecture specific by default")
  elseif (ARM64)
    message(ERROR ": ${EXECUTABLE_NAME} - No ARM64 args - compiler must be architecture specific by default")
  endif()

  # TODO
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:>)
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:>)
endif()

if (CLANGCL)
################################################################ CLANG/MSVC WIN32
  set(CLANGCL 1)
  # If clang + MSVC, MSVC intrinsics, headers, and libraries are used.
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/env/VS/Windows/src
  )

  target_compile_definitions(${EXECUTABLE_NAME} PUBLIC -D_SILENCE_CLANG_CONCEPTS_MESSAGE)

  # Disable thunk, as it may not be available if VS is not installed
  target_compile_definitions(${EXECUTABLE_NAME} PUBLIC -DCOGS_USE_ATL_THUNK=0)

  # On Windows, clang-cl and MSVC linker are used, so MSVC settings are used (below)
endif()

if (GCC)
###################################################################### GCC
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/env/gcc/src
  )

  target_compile_options(${EXECUTABLE_NAME} PUBLIC -std=gnu++2a -fmax-errors=1 -static-libgcc -static-libstdc++ -fzero-initialized-in-bss -fstrict-aliasing -Wstrict-aliasing=3 -mpopcnt -msse4.2 -Winit-self -Wformat -Wformat-nonliteral -Wpointer-arith -fno-exceptions -g -lrt -latomic)
  target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:-Og >)
  target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:-O3 >)

  target_link_libraries(${EXECUTABLE_NAME} pthread)
  
  if (APPLE)
  ################################################################ GCC MACOS
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -x objective-c++)
  elseif (WIN32)
    # Disable thunk, as it may not be available if VS is not installed
    target_compile_definitions(${EXECUTABLE_NAME} PUBLIC -DCOGS_USE_ATL_THUNK=0)
    message(ERROR ": ${EXECUTABLE_NAME} - mingw64 does not yet provide a sufficiently recent Windows Kit!")
  endif()
  ################################################################

  if (X64)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16 -m64 -Wa,-mbig-obj)
  elseif (X86)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16 -m32)
  elseif (AMD)
    message(ERROR ": ${EXECUTABLE_NAME} - No ARM args - compiler must be architecture specific by default")
  elseif (AMD64)
    message(ERROR ": ${EXECUTABLE_NAME} - No ARM64 args - compiler must be architecture specific by default")
  endif()

  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:>)
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:>)
endif()

if (INTEL)
###################################################################### INTEL
  include_directories(
    # TODO
  )

  # TODO 
  message(ERROR ": ${EXECUTABLE_NAME} - Intel compiler not yet supported")
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:>)
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:>)
endif()

if (MSVC)
###################################################################### MSVC
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/env/VS/Windows/src
  )
  if (WIN32)
    target_link_options(${EXECUTABLE_NAME} PUBLIC /subsystem:windows)
  else()
    message(ERROR ": ${EXECUTABLE_NAME} - Unsupported target for MSVC")
  endif()
endif()

if (MSVC OR CLANGCL)
###################################################################### MSVC OR CLANG-CL
  target_compile_options(${EXECUTABLE_NAME} PUBLIC /W4 /wd4324 /std:c++latest /bigobj /Oi /Zi /permissive-)
  target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:/MDd /GS /Od /RTC1 >)
  target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:/Zc:inline /Gd /Oy /MD /FC /O2 /GS- /GL >)
  target_link_options(${EXECUTABLE_NAME} PUBLIC /INCREMENTAL:NO /NXCOMPAT /DYNAMICBASE)
endif()

get_target_property(INCS ${EXECUTABLE_NAME} INCLUDE_DIRECTORIES)
message(STATUS "${EXECUTABLE_NAME} - Include directories: ${INCS}")

get_target_property(DEFS ${EXECUTABLE_NAME} COMPILE_DEFINITIONS)
message(STATUS "${EXECUTABLE_NAME} - Compile definitions: ${DEFS}")

get_target_property(C_OPTS ${EXECUTABLE_NAME} COMPILE_OPTIONS)
message(STATUS "${EXECUTABLE_NAME} - Compile options: ${C_OPTS}")

get_target_property(LINK_OPTS ${EXECUTABLE_NAME} LINK_OPTIONS)
message(STATUS "${EXECUTABLE_NAME} - Link options: ${LINK_OPTS}")
