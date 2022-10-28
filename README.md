# Table of Contents
* [cogs](#cogs)
* [Concepts](#concepts)
  * [Non-blocking Algorithms](#non-blocking-algorithms)
  * [Volatile](#volatile)
  * [Volatile Decay](#volatile-decay)
* [Goals](#goals)
  * [Future Goals](#future-goals)
* [Core Features](#core-features)
  * [Future Feature Plans](#future-feature-plans)
* [Supported Platforms](#supported-platforms)
* [Supported Architectures](#supported-architectures)
* [Supported Compilers](#supported-compilers)
* [Quick Start](#quick-start)
  * [Using as a header-only library in another project](#using-as-a-header-only-library-in-another-project)
    * [Why multiple paths](#why-multiple-paths)
    * [Configuration for header-only usage](#configuration-for-header-only-usage)

# cogs
><em>"If you wish to make an apple pie from scratch, you must first invent the universe."</em>
> -- Carl Sagan

Cogs is a lock-free, cross-platform, C++ template class library and application framework, developed by Colen M. Garoutte-Carson.

Cogs has been a personal creative project for me for many years, to explore ideas and expand my knowledge.

This project is of pre-alpha quality. Some functionality is incomplete or missing.

# Concepts

## Non-blocking Algorithms

* https://en.wikipedia.org/wiki/Non-blocking_algorithm

## Volatile

Cogs leverages the `volatile` qualifier to qualify a type as thread-safe or atomic, in much the same way as the `const` qualifier qualifies a type as non-modifiable.

Like the `const` qualifier, `volatile` can be added to member functions, in which the object and its members are also treated as `volatile`.  Member functions with and without the `volatile` qualifier can be overloaded separately, to provide variations of the same algorithm with and without thread safety.

```cpp
class A
{
private:
   ptr<void> m_ptr;

public:
    // Sees: ptr<void> m_ptr;
    void foo(); // #1

    // Sees: const ptr<void> m_ptr;
    void foo() const;   // #2

    // Sees: volatile ptr<void> m_ptr;
    void foo() volatile;    // #3

    // Sees: const volatile ptr<void> m_ptr;
    void foo() const volatile;   // #4
};

void bar(A& a1, const A& a2, volatile A& a3, const volatile A& a4)
{
    a1.foo();   // Calls #1
    a2.foo();   // Calls #2
    a3.foo();   // Calls #3
    a4.foo();   // Calls #4
}
```

## Volatile Decay

Like `const`, `volatile` qualifiers can be added, and decayed.  An object can be created as non-volatile, and it's non-volatile member functions may be called.  As these would not require thread safety, they could be implemented efficiently, without concern for thread synchronization.  Then, the object may be passed somewhere or assigned, gaining a volatile qualification.  As that algorithm interacts with the object across multiple threads, the thread-safe `volatile` member functions are used.  Then, upon synchronizing with the release of the object from that algorithm, the calling context may again use their non-volatile instance of the object to access efficient non-volatile member functions.

This is in contrast to `std::atomic`, which must always be interacted with in an atomic manner.

# Goals
* To create a fully lock-free system, in which the ideal number of threads is equal to the number of logical processors, all potentially blocking operations are asynchronous, and no threads use `wait` operations to synchronize.

  * *UI is an exception to this.  Some platforms only allow use of UI APIs on a dedicated UI thread.  Therefor, most UI functionality is intended to be called only within the appropriate UI thread, and is not thread-safe.*

* Reusable algorithms abstracted using C++ templates,  template meta-programming, and SFINAE.

* Performance and scalability, with an emphasis on performing calculations at compile time, and exploiting compile time optimizations.

  * Avoiding overhead, such as kernel mode transitions on Windows.

* Alternatives to algorithms and approaches in the C++ standard library, where the standard implementation is not ideal, or to enable better integrate with other classes in the framework.

* Minimal external dependencies.

* 

## Future Goals

 * [C++ Coroutines](https://en.cppreference.com/w/cpp/language/coroutines)
 * [C++ Constraints and Concepts](https://en.cppreference.com/w/cpp/language/constraints)
 * [C++ Modules](https://en.cppreference.com/w/cpp/language/modules)

# Core Features

Cross-platform, cross-architecture, cross-compiler, lock-free (where applicable) :

* Memory management:

  * `hazard` - Inspired by a paper by Maged M. Michael titled, "Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects".
  
  * `batch_allocator` - An allocator that uses a lock-free [free-list](https://en.wikipedia.org/wiki/Free_list)'s with blocks of equal size, which allocates additional larger blocks from another allocator as needed.
  
  * `buddy_block_memory_manager` - An memory manager that uses a set of cascading lock-free [free-list](https://en.wikipedia.org/wiki/Free_list)'s. When a block of a required size is not available, a block twice that size is allocated, split in two, and the other half is added to a [free-list](https://en.wikipedia.org/wiki/Free_list) for the smaller block. When a block is freed, and its associated (buddy) block is also freed, they're coalesced and released to a [free-list](https://en.wikipedia.org/wiki/Free_list) for the larger block size.

  * Lock-free smart-pointers -
    * `rcptr` - Nullable reference-counted smart-pointer.
    * `rcref` - Non-nullable reference-counted smart-pointer.
    * `weak_rcptr` - Weakly referenced nullable reference-counted smart-pointer.
  
* Thread synchronization - Each of the following rely only on a single OS-specific semaphore and only when blocking is necessary. 
  * Conditions (aka Events in Win32)
    * `single_fire_condition`
    * `resettable_condition`
    * `count_down_condition`
  * Timers
    * `single_fire_timer`
    * `resettable_timer`
    * `pulse_timer`
    * `refireable_timer`
  * `mutex`
  * `semaphore`
  * `rwlock`
  * `priority_queue`
  * `task` (aka Promise) - Most similar to `Promise`/`Thenable` in TypeScript.

  * Lock-free containers

    * `container_deque` - A container (non-intrusive) deque/queue/stack. Allows coalescing of equal nodes. Inspired by a paper by Maged M. Michael titled, "CAS-Based Lock-Free Algorithm for Shared Deques".
    
    * `container_dlist` - A container (non-intrusive) double-link list. Supports lock-free traversal.
    
    * `container_skiplist` - A sorted container (non-intrusive) with O(log n) insert and search. This is used to provide lock-free associative containers such as `set`, `multiset`, `map`, and `multimap`, as well `priority_dispatcher`.  Based loosely on [a paper by Sundell and Tsigas](http://www.non-blocking.com/download/SunT03_PQueue_TR.pdf)
    
  * `transactable` - A transactional type wrapper. Encapsulates a type and provides atomic read/write transactions.

* Scalable Asynchronous I/O -
  * TCP/IP - Uses [`IOCompletionPorts`](https://msdn.microsoft.com/en-us/library/windows/desktop/aa365198(v=vs.85).aspx) on Windows, [`epoll`](https://en.wikipedia.org/wiki/Epoll) on Linux, and [`kqueue`](https://en.wikipedia.org/wiki/Kqueue) on macOS.

  * `datasource`, `datasink` - Can be 'coupled', such that data is automatically routed between them, until decoupled or one of them is closed.

  * `filter` - Both a `datasource` and a `datasink`.  It can couple between another `datasource` and `datasink` to transform data in one stream direction.

  * `datastream` - An object that consists of both a `datasource` and a `datasink`, representing a single connection with streams in both directions, such as a read/write file handle or a TCP/IP socket.

  * `datastream_protocol` - Similar to a `filter`, a `datastream` that patches another `datastream`, to transform data in both directions.

  * `composite_buffer` - A single logical buffer potentially composed of multiple in-contiguous buffers. This is intended to minimize large buffer copies.

* Math
  * `fixed_integer` - Fixed-sized [Arbitrary precision](https://en.wikipedia.org/wiki/Arbitrary-precision_arithmetic) integers.
  
  * `dynamic_integer` - Dynamic-sized [Arbitrary precision](https://en.wikipedia.org/wiki/Arbitrary-precision_arithmetic) integer.
  
  * `fixed_integer_const` - Constant fixed-sized [Arbitrary precision](https://en.wikipedia.org/wiki/Arbitrary-precision_arithmetic) integers.  Can be used to perform big-integer calculations at compile time.
  
  * `measure` - Associates a numeric type with a unit of measurement (such as seconds, minutes, meters, liters).  Unit conversions occur implicitly.

* Cryptography, Hashes, CRCs and Checksums - `adler32`, `crc` (various), `fletcher`, `fnv`, `gost`, `haval`, `joaat`, `keccak`, `md2`, `md4`, `md5`, `ripemd`, `sha1`, `sha2`, `sha3`/`keccak`/`shake`, `snefru`, `tiger`, `whirlpool`

* Standard collections
  * AVL trees
  * Red-black trees
  * skip-lists
  * vectors
  * strings
  * stacks
  * lists
  * maps
  * sets
  
* Miscellaneous things
  * HTTP chunking protocol

* GUI
  * Uses a [bridge pattern](https://en.wikipedia.org/wiki/Bridge_pattern) that facilitates both cross-platform support and UI skinning.  
  * Uses GDI/GDI+ on Windows, Cocoa on macOS
  * 2D drawing canvas
  * Efficient 2D compositing
  * Sizes and resizing behavior can be defined abstractly, independent of the coordinate system.
  * `frame` - Controls resizing behavior of a enclosed pane.


## Future Feature Plans

* Async DNS
* File I/O with scatter/gather transactions.
* Cryptography - ciphers, encryptions/decryption, SSL/TSL.
* Linux GUI support
* Windows Direct2D GUI/Drawing
* iPhone support
* Android support
* 3D APIs
* Video APIs
* Audio APIs
* REST APIs
* GraphQL APIs

# Supported Architectures
x86, x64, arm and arm64.  Though, other architectures might be supportable with minimal effort, provided certain instructions are available using compiler intrinsics.  An atomic wide compare/exchange instruction is required (i.e. cmpxchg8b on x86, cmpxchg16b on x64).

# Supported Platforms
Windows 10+, macOS, Linux.  Currently, a sufficient Windows 10 SDK is required for DPI scaling APIs.  GUI classes are not currently supported on Linux.  Some classes can be used on unknown platforms by specifying `std` as the platform.

# Supported Compilers
MSVC, Clang, GCC. On Windows, Cygwin and MinGW are also supported.

# Quick Start
A Hello World is included in `apps/Template`.  A Visual Studio solution is included, as well as `CMakeLists.txt` files for building with [CMake](https://cmake.org) across platforms.

## Using as a header-only library in another project
Cogs can be used from other project.  It requires 4 include paths, a primary path, and paths for architecture, platform, and development environment.  For example, to build for x64 Windows using MSVC, use the following include paths:
- cogs/core/src
- cogs/core/arch/x64/src
- cogs/core/os/windows/src
- cogs/core/env/VS/Windows/src

### Why multiple paths?

  Some functionality is varied using different files of the same name, rather than putting implementations for multiple platforms into the same file. The intent is to avoid excessive use of preprocessor conditions, and allow support for architectures, platforms, or compilers to be added in a module manner.

The architecture path is a remnant from when assembly implementations were required.  These have since been moved to compiler intrinsics.  The architecture path may be removed soon.

### Configuration for header-only usage
Some features require specific compiler options to be enabled.  For example:
* `-mcx16` is required with gcc/clang, as a lot of functionality requires atomic wide compare/exchange.
* `-fzero-initialized-in-bss` is required with gcc/clang, to guards against order of initialization issues that arise during global/static construction.
* Disabling C++ Exceptions is recommended. ("`-fno-exceptions`" for gcc/clang, "`/EHsc -D_HAS_EXCEPTIONS=0`" for MSVC)  Technically, exceptions are part of C++ and disabling them is non-standard.  Cogs does not use exceptions, and may not correctly handle being unwound.  Any callback, lambda, member function overload, etc., invoked by cogs should catch any non-fatal exceptions it might incur.
* `-Wno-deprecated` is useful with gcc 12, to disable warnings related to [p1152](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1152r0.html).  Hopefully someone will realize the how much of a mistake those changes are.  If some of those changes are not undone, they will mean the end of this project, and likely the end of my nearly life-long use of C++.
