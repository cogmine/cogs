set(CMAKE_HOST_SYSTEM_VERSION "10.0.19041")

target_compile_definitions(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:COGS_DEBUG>)

set_property(TARGET ${EXECUTABLE_NAME} PROPERTY CXX_STANDARD 20)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/core/src
)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  message(STATUS "${EXECUTABLE_NAME} - Target compiler: MSVC")
  set(MSVC 1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  message(STATUS "${EXECUTABLE_NAME} - Target compiler: gcc")
  set(GCC 1)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    message(STATUS "${EXECUTABLE_NAME} - Target compiler: clang-cl")
    set(CLANGCL 1)
  else()
    message(STATUS "${EXECUTABLE_NAME} - Target compiler: clang")
    set(CLANG 1)
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
  message(STATUS "${EXECUTABLE_NAME} - Target compiler: AppleClang")
  set(APPLECLANG 1)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  message(STATUS "${EXECUTABLE_NAME} - Target compiler: Intel")
  set(INTEL 1)
else()
  message(ERROR ": ${EXECUTABLE_NAME} - Target compiler: Unknown: ${CMAKE_CXX_COMPILER_ID}")
endif()

message(STATUS "${EXECUTABLE_NAME} - CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "AMD64" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
  set(HOST_X64 1)
elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "X86" OR "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "i686")
  set(HOST_X86 1)
elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "ARM")
  set(HOST_ARM 1)
elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "ARM64")
  set(HOST_ARM64 1)
endif()

if (NOT X86 AND NOT X64 AND NOT ARM AND NOT ARM64)

  if (APPLECLANG AND NOT "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
    list(LENGTH CMAKE_OSX_ARCHITECTURES NUM_CMAKE_OSX_ARCHITECTURES)
    if (NUM_CMAKE_OSX_ARCHITECTURES GREATER 1)
      message(FATAL_ERROR ": ${EXECUTABLE_NAME} - Multiple architectures specified to CMAKE_OSX_ARCHITECTURES.  Only 1 is supported.")
    elseif ("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "x86_64")
      set(HOST_X64 1)
    elseif ("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "arm64")
      set(HOST_ARM64 1)
    elseif ("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "i386")
      set(HOST_X86 1)
    else()
      # On macOS, if CMAKE_OSX_ARCHITECTURES is not provided to CMake, we can only build for the host system.
      set(X64 HOST_X64)
      set(X86 HOST_X86)
      set(ARM HOST_ARM)
      set(ARM64 HOST_ARM64)
    endif()
  elseif ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64") # Try architecture passed to CMake, if any.
    set(X64 1)
  elseif ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "win32")
    set(X86 1)
  elseif ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "arm")
    set(ARM 1)
  elseif ("${CMAKE_GENERATOR_PLATFORM}" STREQUAL "arm64")
    set(ARM64 1)
  elseif (HOST_X64)
    set(USING_SYSTEM_PROCESSOR 1)
    set(X64 1)
  elseif (HOST_X86)
    set(USING_SYSTEM_PROCESSOR 1)
    set(X86 1)
  elseif (HOST_ARM)
    set(USING_SYSTEM_PROCESSOR 1)
    set(ARM 1)
  elseif (HOST_ARM64)
    set(USING_SYSTEM_PROCESSOR 1)
    set(ARM64 1)
  else()
    message(ERROR ": ${EXECUTABLE_NAME} - Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
  endif()
endif()


if (NOT X86 AND NOT X64 AND NOT ARM AND NOT ARM64)
  message(FATAL_ERROR ": ${EXECUTABLE_NAME} - Unknown or unsupported target architecture.")
endif()

if (X64)
  message(STATUS "${EXECUTABLE_NAME} - Target architecture: x64")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/arch/x64/src
  )
elseif (X86)
  message(STATUS "${EXECUTABLE_NAME} - Target architecture: x86")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/arch/x86/src
  )
elseif (ARM)
  message(STATUS "${EXECUTABLE_NAME} - Target architecture: ARM")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/arch/arm/src
  )
elseif (ARM64)
  message(STATUS "${EXECUTABLE_NAME} - Target architecture: ARM64")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/arch/arm64/src
  )
else()
  message(ERROR ": ${EXECUTABLE_NAME} - Unknown target architecture")
endif()

if (WIN32)
###################################################################### WIN32
  message(STATUS "${EXECUTABLE_NAME} - Target platform: Win32")

  target_compile_definitions(${EXECUTABLE_NAME} PUBLIC -DUNICODE -D_UNICODE -DWIN32 -DWINDOWS -DCOGS_DEFAULT_UI_SUBSYSTEM=COGS_GUI -DCOGS_DEFAULT_GUI_SUBSYSTEM=COGS_GDI -D_HAS_EXCEPTIONS=0)

  target_link_libraries(${EXECUTABLE_NAME} advapi32 comctl32 comdlg32 dwmapi gdi32 gdiplus kernel32 msimg32 mswsock normaliz odbc32 odbccp32 ole32 oleaut32 shcore shell32 user32 uxtheme winmm ws2_32)

  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/os/windows/src
  )

elseif (APPLE)
###################################################################### MACOS
  message(STATUS "${EXECUTABLE_NAME} - Target platform: Apple")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/os/macos/src
  )

  target_link_libraries(${EXECUTABLE_NAME} "-framework CoreFoundation" "-framework CoreGraphics" "-framework Cocoa")

elseif (LINUX OR UNIX)
###################################################################### LINUX/UNIX
  target_link_libraries(${EXECUTABLE_NAME} pthread)
  message(STATUS "${EXECUTABLE_NAME} - Target platform: Linux/Unix")
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/os/linux/src
    ${CMAKE_CURRENT_LIST_DIR}/core/os/libs/pthreads
  )
else()
  message(ERROR ": ${EXECUTABLE_NAME} - Target platform: Unknown")
endif()


if (APPLECLANG)
###################################################################### APPLECLANG
  target_compile_options(${EXECUTABLE_NAME} PUBLIC -fobjc-arc -ObjC++)
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/env/gcc/src
  )
  if (X64)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16)
  elseif (X86)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16)
  endif()

  # TODO
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:>)
  #target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:>)
endif()

if (CLANG)
###################################################################### CLANG
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:)
  include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/core/env/gcc/src
  )
  if (X64)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16 -m64)
    target_link_options(${EXECUTABLE_NAME} PUBLIC -m64)
    if (WIN32)
      target_compile_options(${EXECUTABLE_NAME} PUBLIC -Wa,-mbig-obj)
    endif()
  elseif (X86)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16 -m32)
      target_link_options(${EXECUTABLE_NAME} PUBLIC -m32)
  elseif (ARM)
    message(MESSAGE ": ${EXECUTABLE_NAME} - No auto-configure for arm - Compiler must be architeture-specific or targetted using CMake args")
  elseif (ARM64)
    message(MESSAGE ": ${EXECUTABLE_NAME} - No auto-configure for arm64 - Compiler must be architeture-specific or targetted using CMake args")
  else()
    message(MESSAGE ": ${EXECUTABLE_NAME} - No known architecture specified - Compiler must be architeture-specific or targetted using CMake args")
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
  if (X64)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC /clang:-mcx16 /clang:-m64)
  elseif (X86)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC /clang:-mcx16 /clang:-m32)
  elseif (ARM)
    message(MESSAGE ": ${EXECUTABLE_NAME} - No auto-configure for arm - Compiler must be architeture-specific or targetted using CMake args")
  elseif (ARM64)
    message(MESSAGE ": ${EXECUTABLE_NAME} - No auto-configure for arm64 - Compiler must be architeture-specific or targetted using CMake args")
  else()
    message(MESSAGE ": ${EXECUTABLE_NAME} - No known architecture specified - Compiler must be architeture-specific or targetted using CMake args")
  endif()

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

  target_compile_options(${EXECUTABLE_NAME} PUBLIC -fmax-errors=1 -static -static-libgcc -static-libstdc++ -fzero-initialized-in-bss -fno-exceptions -g -Wno-deprecated)

  if (X64 OR X86)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mpopcnt)
  endif()

  target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:-Og >)
  target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:-O3 >)

  if (APPLE)
  ################################################################ GCC MACOS
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -x objective-c++)
  elseif (WIN32)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mwindows )
    # Disable thunk, as it may not be available if VS is not installed
    target_compile_definitions(${EXECUTABLE_NAME} PUBLIC -DCOGS_USE_ATL_THUNK=0)
  endif()
  ################################################################

  if (X64)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16 -m64)
    target_link_options(${EXECUTABLE_NAME} PUBLIC -m64)
    if (WIN32)
      target_compile_options(${EXECUTABLE_NAME} PUBLIC -Wa,-mbig-obj)
    endif()
  elseif (X86)
    target_compile_options(${EXECUTABLE_NAME} PUBLIC -mcx16 -m32)
    target_link_options(${EXECUTABLE_NAME} PUBLIC -m32)
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
  target_compile_options(${EXECUTABLE_NAME} PUBLIC /W4 /wd4324 /bigobj /Oi /Zi /permissive-)
  target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Debug>:/MDd /GS /Od /RTC1 >)
  target_compile_options(${EXECUTABLE_NAME} PUBLIC $<$<CONFIG:Release>:/Zc:inline /Gd /Oy /MD /FC /O2 /GS- /GL >)
  target_link_options(${EXECUTABLE_NAME} PUBLIC /INCREMENTAL:NO /NXCOMPAT /DYNAMICBASE)
endif()

get_target_property(S_DIR ${EXECUTABLE_NAME} SOURCE_DIR)
get_target_property(B_DIR ${EXECUTABLE_NAME} BINARY_DIR)
get_target_property(INCS ${EXECUTABLE_NAME} INCLUDE_DIRECTORIES)
get_target_property(DEFS ${EXECUTABLE_NAME} COMPILE_DEFINITIONS)
get_target_property(C_OPTS ${EXECUTABLE_NAME} COMPILE_OPTIONS)
get_target_property(L_OPTS ${EXECUTABLE_NAME} LINK_OPTIONS)
get_target_property(LIBS ${EXECUTABLE_NAME} LINK_LIBRARIES)

message(STATUS "${EXECUTABLE_NAME} - Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "${EXECUTABLE_NAME} - Target source directory: ${S_DIR}")
message(STATUS "${EXECUTABLE_NAME} - Target binary output directory: ${B_DIR}")
message(STATUS "${EXECUTABLE_NAME} - Target link libraries: ${LIBS}")
message(STATUS "${EXECUTABLE_NAME} - Tagget Link options: ${L_OPTS}")
message(STATUS "${EXECUTABLE_NAME} - Target include directories: ${INCS}")
message(STATUS "${EXECUTABLE_NAME} - Target compile definitions: ${DEFS}")
message(STATUS "${EXECUTABLE_NAME} - Target compile options: ${C_OPTS}")
message(STATUS "${EXECUTABLE_NAME} - CMAKE_CXX_COMPILE_FLAGS: ${CMAKE_CXX_COMPILE_FLAGS}")
message(STATUS "${EXECUTABLE_NAME} - CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "${EXECUTABLE_NAME} - CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "${EXECUTABLE_NAME} - CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "${EXECUTABLE_NAME} - CMAKE_CXX_FLAGS_MINSIZEREL: ${CMAKE_CXX_FLAGS_MINSIZEREL}")
message(STATUS "${EXECUTABLE_NAME} - CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
if (APPLECLANG)
  message(STATUS "${EXECUTABLE_NAME} - CMAKE_OSX_ARCHITECTURES: ${CMAKE_OSX_ARCHITECTURES}")
endif()
