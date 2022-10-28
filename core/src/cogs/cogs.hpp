//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER
#define COGS_HEADER

/// @mainpage Cogs Documentation
///
/// @section MainPageWorkInProgress Work In Progress
///
/// This documentation is a work in progress.
/// Check the 'Modules' section for classes organized into groups.
///
///
/// @section MainPageIntro Introduction
///
/// Cogs is a lock-free, cross-platform, C++ template class library and application framework.
///
/// @section GettingStarted Getting Started
///
/// (TODO)
///
/// @subsection MainPageDirectoryStructure Directory Structure and Include Search Paths
///
/// Cogs source code is organized into 4 main areas: Core, Architecture, Operating System, and Build Environment.
///
/// Some cross arch/OS/env support is accomplished by providing alternate headers and classes.
/// Projects should add the desired arch, env and OS specific include paths for the desired platform.
/// i.e.  To target AMD64, MS Windows, using Visual Studio, a project should add:
///
/// - core/src
/// - core/arch/AMD64/src
/// - core/os/windows/src
/// - core/env/VS/Windows/src
///
/// This addressed in the included Template projects, and CMakeList.txt files.
///
/// @section Concepts
/// @subsection MainPageVolatileCorrectness Volatile Correctness
///
/// Most C++ programmers should be aware of the concept of <a href="https://isocpp.org/wiki/faq/const-correctness#overview-const">const correctness</a>.
/// In C++, the 'const' qualifier can be added to a type, preventing non-const access to that data at compile time.
/// Using this language feature tends to have a cascading effect, and "correctness" refers to using const for
/// read-only references consistently throughout the code-base.
///
/// The const qualifier can also be added to member functions, causing a member function to treat it's
/// 'this' pointer as a pointer to a const type.  Methods of the same name can be overloaded with and without the const
/// qualifier, to indicate one version of the function is to be used when called on a non-const reference,
/// and the other is to be used when called on a const reference.
///
/// The const and volatile keywords are syntactically equivalent (defined together as "cv-qualifiers").  The same
/// language features that apply to distinguishing between const and non-const types, can be used
/// to distinguish between volatile and non-volatile types.  Cogs leverages this to provide thread
/// safe (volatile) and more efficient thread-unsafe (non-volatile) implementations of an algorithm
/// using a single a class.
///
/// @code{.cpp}
///	class A
///	{
///	private:
///		ptr<void> m_ptr;
///
///	public:
///		// Sees: ptr<void> m_ptr;
///		void foo(); // #1
///
///		// Sees: const ptr<void> m_ptr;
///		void foo() const; // #2
///
///		// Sees: volatile ptr<void> m_ptr;
///		void foo() volatile; // #3
///
///		// Sees: const volatile ptr<void> m_ptr;
///		void foo() const volatile; // #4
///	};
///
/// ...
///
///	void bar(A& a1, const A& a2, volatile A& a3, const volatile A& a4)
///	{
///		a1.foo(); // Calls #1
///		a2.foo(); // Calls #2
///		a3.foo(); // Calls #3
///		a4.foo(); // Calls #4
///	}
/// @endcode
///
///
/// @subsection MainPageAtomics Atomics
///
/// Included is a library of atomic operations that leverage compiler intrisics.  See: @ref cogs::atomic
///
/// @subsection MainPageRefCountedObjects Lock-Free RAII Reference-Counted Objects
///
/// Cogs uses it's own reference-counted pointer type, and new-like operator.
///
/// @code{.cpp}
///		volatile rcptr<A> a1 = rcnew(A)(constructorArgs);
///
///		// in another thread:
///		rcptr<A> a2 = a1;
///
///		// Encapsulated object is destructed and deallocated automatically once both a1 and a2 have gone out of scope.
/// @endcode
///
/// Classes intended to be allocated using rcnew can derive from @ref cogs::object to access their own reference-counted
/// container using @ref this_rcptr or @ref this_rcref.
///
/// A @ref cogs::rcref is similar to a @ref cogs::rcptr, but cannot refer to null.  @ref cogs::rcptr and @ref cogs::rcref are
/// both strong references.  (@ref cogs::reference_strength)  A @ref cogs::weak_rcptr can be used to
/// retain a conditional reference to an object, which automatically becomes null when there are no longer any strong references.
///
///
/// @subsection MainPageLockFreeAlgorithms Lock-Free Algorithms
///
/// Noteworthy lock-free algorithms:
///
/// @ref cogs::hazard (Hazard pointers) - Inspired by a paper by Maged M. Michael titled, "Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects".
///
/// @ref cogs::container_deque - A container (non-intrusive) deque/queue/stack.  Allows coalescing of equal nodes.  Inspired by a paper by Maged M. Michael titled, "CAS-Based Lock-Free Algorithm for Shared Deques"
///
/// @ref cogs::container_dlist - A container (non-intrusive) double-link list.  Supports lock-free traversal.
///
/// @ref cogs::container_skiplist "container_skiplist" - A sorted container (non-intrusive) with O(log n) insert and search.  Efficient insert at start/end.  Based loosely on a <a href="http://www.non-blocking.com/download/SunT03_PQueue_TR.pdf">paper</a> by Sundell and Tsigas.
/// This is used to provide lock-free implementations of standard associative containers such as
/// @ref cogs::set,
/// @ref cogs::multiset,
/// @ref cogs::map, and
/// @ref cogs::multimap,
/// as well as
/// @ref cogs::priority_dispatcher.
///
/// @ref cogs::transactable - A transactional type wrapper.  Encapsulates a type and provides access to it using simple atomic read/write transactions.
///
/// @ref cogs::freelist - A simple lock-free <a href="https://en.wikipedia.org/wiki/Free_list">free-list</a>.
///
/// @ref cogs::buddy_block_memory_manager - A memory manager that uses a set of cascading free-lists.  If a block of a required
/// size is not available, a block twice the size is allocated, split in half, and the other half is added to the smaller block's free-list.
/// When a block is freed, and it's associated (buddy) block is also freed, they are coalesced and released to the free-list for the coalesced block.
///
///
/// @subsection MainPageSynchronizationObjects Synchronization Objects
///
/// As a cross-platform framework, Cogs abstracts thread synchronization on the target platform.  As a lock-free
/// framework, use of blocking synchronization is discouraged.  Most cogs synchronization objects provide a way to be
/// notified of resource availability using a callback, which allows traditional thread synchronization objects
/// to be used in a lock-free manner.
///
/// Cogs synchronization objects avoid locks internally, and leverage only a simple OS-specific semaphore when
/// blocking is necessary.
///
/// @ref Events - Similar to Win32 Events and pthreads condition objects - @ref cogs::event, @ref cogs::count_down_condition, @ref cogs::resettable_condition, @ref cogs::single_fire_condition
///
/// @ref Timers - cogs::timer, @ref cogs::resettable_timer, @ref cogs::pulse_timer, @ref cogs::refireable_timer, @ref cogs::single_fire_timer
///
/// @ref cogs::mutex, @ref cogs::semaphore, @ref cogs::rwlock, @ref cogs::priority_queue
///
/// @ref cogs::thread, @ref cogs::thread_pool
///
///
/// @subsection MainPageFunctionsDispatchersTasks Function Objects, Dispatchers and Tasks
///
/// @ref cogs::function<> is similar to std::function<>, with the following differences:
/// - It's embbeded storage size (used to avoid dynamic allocations) can be specified
/// - It can contain a lambda with a different signature than provided, if the call is compatible.
///
/// @ref cogs::dispatcher is an interface for objects that dispatch invoke lambda, such as a @ref cogs::thread_pool or a @cogs::event .
///
/// @ref cogs::task is similar to a ppl task, or a JavScript promise.  It's a type returned by asynchronous operations, and can be used to cancel,
/// wait for, and potential change the priority of an operation.  cogs::task is a dispatcher, so can also return a task that will complete after it's
/// been invoked.  If a lamba returning a task is used, the next chained task will not complete until that returned task completes.
///
///
/// @subsection MainPageIO Asynchronous I/O
///
/// Cogs include a robust asynchronous I/O system.   The primary I/O classes are @ref cogs::io::datasource
/// and @ref cogs::io::datasink.
/// A @ref cogs::io::datasource and a @ref cogs::io::datasink can be 'coupled', such that data is automatically routed between them, until decoupled or
/// one of them is closed.  A @ref cogs::io::filter can be placed between a datasink and a datasource to patch a stream.
///
/// Cogs I/O makes use of composite buffers (@ref cogs::io::composite_buffer).  A composite buffer is a single logical buffer potentially
/// comprised of multiple incontiguous buffers.  This is intended to minimize large buffer copies.
///
/// Cogs includes scalable cross-platform Network I/O.
/// - On Windows, <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/aa365198(v=vs.85).aspx">IOCompletionPorts</a> are used.
/// - On Linux, <a href="https://en.wikipedia.org/wiki/Epoll">epoll</a> is used.
/// - On MacOS, <a href="https://en.wikipedia.org/wiki/Kqueue">kqueue</a> is used.
///
/// (File I/O with scatter/gather transactions, is currently TODO).
///
///
/// @subsection MainPageMath Math
///
/// Cogs includes classes for fixed, dynamic and compile-time constant <a href="https://en.wikipedia.org/wiki/Arbitrary-precision_arithmetic">arbitrary precision</a>
/// integers (@ref cogs::fixed_integer, @ref cogs::dynamic_integer).  Where applicable, mathematical operations generate results with increasing bits,
/// making range overflows avoidable.
///
/// @ref cogs::measure iencapsulates a numeric representation
/// (such as a @ref cogs::fixed_integer or @ref cogs::dynamic_integer) and a unit base (such as seconds, minutes, meters, liters).
/// Conversations are automatically applied for operations with mixed unit based.
///
///
/// @subsection ManiPageCrypto Cryptography, Hashes, CRCs and Checksums
///
/// Cogs includes a library of hash, CRC, and checksum algorithms.
///
/// Cryptographic cipher algorithms are TODO.  A goal is to implement the SSL/TLS protocol.
///
///
/// @subsection ManiPageGUI GUI
///
/// Cogs includes a cross-platform GUI framework.
///
/// - @ref cogs::gfx::canvas - an interface for basic 2D drawing.
/// - @ref cogs::gui::pane - an interface for 2D UI elements
///
/// Common UI elements are implemented using a <a href="https://en.wikipedia.org/wiki/Bridge_pattern">bridge pattern</a>
/// that facilitates both UI skinning and cross-platform support.  For example, a @ref cogs::gui::button represents a UI button.
/// When the @ref cogs::gui::button (or a pane it's nested within)
/// is installed into a @ref cogs::gui::subsystem, an additional subsystem-specific button object is created and bridged using the existing
/// @ref cogs::gui::button.  It's possible to uninstall a UI tree from one @ref cogs::gui::subsystem, and install it into another, for instance to change
/// the (skinned) appearance of the user interface.  Platform-specific UI is provided using a platform-specific @ref cogs::gui::subsystem.
///
/// @subsection MainPageCompatibility Conformance
///
/// Cogs is not fully C++ conformant.  For example, it assumes two's compliment integers.  Archaic systems may not supported.
///

#include "cogs/env.hpp"

#include "cogs/assert.hpp"
#include "cogs/compatible.hpp"
#include "cogs/debug.hpp"
#include "cogs/dependency_property.hpp"
#include "cogs/function.hpp"
#include "cogs/load.hpp"
#include "cogs/macro_concat.hpp"
#include "cogs/macro_stringify.hpp"
#include "cogs/main.hpp"
#include "cogs/operators.hpp"
#include "cogs/collections/array_view.hpp"
#include "cogs/collections/avltree.hpp"
#include "cogs/collections/btree_node.hpp"
#include "cogs/collections/btree.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/composite_vector.hpp"
#include "cogs/collections/container_deque.hpp"
#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/container_queue.hpp"
#include "cogs/collections/container_skiplist.hpp"
#include "cogs/collections/container_stack.hpp"
#include "cogs/collections/dlink.hpp"
#include "cogs/collections/dlist.hpp"
#include "cogs/collections/backed_vector.hpp"
#include "cogs/collections/function_list.hpp"
#include "cogs/collections/map.hpp"
#include "cogs/collections/multimap.hpp"
#include "cogs/collections/multiset.hpp"
#include "cogs/collections/no_aba_stack.hpp"
#include "cogs/collections/rbtree.hpp"
#include "cogs/collections/set.hpp"
#include "cogs/collections/simple_vector.hpp"
#include "cogs/collections/slink.hpp"
#include "cogs/collections/slist.hpp"
#include "cogs/collections/stack.hpp"
#include "cogs/collections/string.hpp"
#include "cogs/collections/unicode.hpp"
#include "cogs/collections/vector.hpp"
#include "cogs/collections/vector_view.hpp"
#include "cogs/collections/weak_rcptr_list.hpp"
#include "cogs/crypto/adler32.hpp"
#include "cogs/crypto/cipher.hpp"
#include "cogs/crypto/crc.hpp"
#include "cogs/crypto/fletcher.hpp"
#include "cogs/crypto/fnv.hpp"
#include "cogs/crypto/gost.hpp"
#include "cogs/crypto/hash.hpp"
#include "cogs/crypto/hash_int.hpp"
#include "cogs/crypto/haval.hpp"
#include "cogs/crypto/hmac.hpp"
#include "cogs/crypto/joaat.hpp"
#include "cogs/crypto/md2.hpp"
#include "cogs/crypto/md4.hpp"
#include "cogs/crypto/md5.hpp"
#include "cogs/crypto/ripemd.hpp"
#include "cogs/crypto/serial_hash.hpp"
#include "cogs/crypto/sha1.hpp"
#include "cogs/crypto/sha2.hpp"
#include "cogs/crypto/sha3.hpp"
#include "cogs/crypto/snefru.hpp"
#include "cogs/crypto/tiger.hpp"
#include "cogs/crypto/whirlpool.hpp"
#include "cogs/geometry/alignment.hpp"
#include "cogs/geometry/bounds.hpp"
#include "cogs/geometry/cell.hpp"
#include "cogs/geometry/dimension.hpp"
#include "cogs/geometry/direction.hpp"
#include "cogs/geometry/flow.hpp"
#include "cogs/geometry/margin.hpp"
#include "cogs/geometry/point.hpp"
#include "cogs/geometry/proportion.hpp"
#include "cogs/geometry/range.hpp"
#include "cogs/geometry/size.hpp"
#include "cogs/geometry/sizing_groups.hpp"
#include "cogs/gfx/canvas.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/gui/ansiterm.hpp"
#include "cogs/gui/background.hpp"
#include "cogs/gui/button.hpp"
#include "cogs/gui/button_box.hpp"
#include "cogs/gui/check_box.hpp"
#include "cogs/gui/editor.hpp"
#include "cogs/gui/frame.hpp"
#include "cogs/gui/grid.hpp"
#include "cogs/gui/label.hpp"
#include "cogs/gui/labeled_list.hpp"
#include "cogs/gui/list.hpp"
#include "cogs/gui/mouse.hpp"
#include "cogs/gui/native_container_pane.hpp"
#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"
#include "cogs/gui/bitmap_pane.hpp"
#include "cogs/gui/scroll_bar.hpp"
#include "cogs/gui/scroll_pane.hpp"
#include "cogs/gui/stack_panel.hpp"
#include "cogs/gui/subsystem.hpp"
#include "cogs/gui/table.hpp"
#include "cogs/gui/text_editor.hpp"
#include "cogs/gui/window.hpp"
#include "cogs/gui/wrap_list.hpp"
#include "cogs/io/buffer.hpp"
#include "cogs/io/composite_buffer.hpp"
#include "cogs/io/datasink.hpp"
#include "cogs/io/datasource.hpp"
#include "cogs/io/datastream.hpp"
#include "cogs/io/datastream_protocol.hpp"
#include "cogs/io/file.hpp"
#include "cogs/io/filter.hpp"
#include "cogs/io/limiter.hpp"
#include "cogs/io/permission.hpp"
#include "cogs/io/queue.hpp"
#include "cogs/io/net/address.hpp"
#include "cogs/io/net/connection.hpp"
#include "cogs/io/net/endpoint.hpp"
#include "cogs/io/net/http.hpp"
#include "cogs/io/net/ip.hpp"
#include "cogs/io/net/server.hpp"
#include "cogs/io/net/smtp.hpp"
#include "cogs/io/net/telnet.hpp"
#include "cogs/io/net/ip/address.hpp"
#include "cogs/io/net/ip/endpoint.hpp"
#include "cogs/io/net/ip/tcp.hpp"
#include "cogs/math/bits_to_bytes.hpp"
#include "cogs/math/bits_to_int.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/math/chars.hpp"
#include "cogs/math/const_extuadd.hpp"
#include "cogs/math/const_extudiv.hpp"
#include "cogs/math/const_extumul.hpp"
#include "cogs/math/const_gcd.hpp"
#include "cogs/math/const_lcm.hpp"
#include "cogs/math/const_max_int.hpp"
#include "cogs/math/const_min_int.hpp"
#include "cogs/math/const_upow.hpp"
#include "cogs/math/const_uroot.hpp"
#include "cogs/math/datetime.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/math/extumul.hpp"
#include "cogs/math/fixed_integer.hpp"
#include "cogs/math/fixed_integer_extended.hpp"
#include "cogs/math/fixed_integer_extended_const.hpp"
#include "cogs/math/fixed_integer_native.hpp"
#include "cogs/math/fixed_integer_native_const.hpp"
#include "cogs/math/fraction.hpp"
#include "cogs/math/is_arithmetic_type.hpp"
#include "cogs/math/is_const_type.hpp"
#include "cogs/math/is_integer_type.hpp"
#include "cogs/math/is_negative_value.hpp"
#include "cogs/math/is_signed_type.hpp"
#include "cogs/math/least_multiple_of.hpp"
#include "cogs/math/measure.hpp"
#include "cogs/math/measurement_types.hpp"
#include "cogs/math/negate_if_signed.hpp"
#include "cogs/math/next_exponent_of_two.hpp"
#include "cogs/math/next_multiple_of.hpp"
#include "cogs/math/prev_exponent_of_two.hpp"
#include "cogs/math/random.hpp"
#include "cogs/math/range_to_bits.hpp"
#include "cogs/math/range_to_bytes.hpp"
#include "cogs/math/range_to_int.hpp"
#include "cogs/math/time.hpp"
#include "cogs/math/value_to_bits.hpp"
#include "cogs/math/vec.hpp"
#include "cogs/mem/allocator_base.hpp"
#include "cogs/mem/memory_manager_base.hpp"
#include "cogs/mem/auto_handle.hpp"
#include "cogs/mem/batch_allocator.hpp"
#include "cogs/mem/bballoc.hpp"
#include "cogs/mem/bypass_constructor_permission.hpp"
#include "cogs/mem/bypass_strict_aliasing.hpp"
#include "cogs/mem/const_bit_rotate.hpp"
#include "cogs/mem/const_bit_scan.hpp"
#include "cogs/mem/const_set_bits.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/default_memory_manager.hpp"
#include "cogs/mem/endian.hpp"
#include "cogs/mem/flag_enum.hpp"
#include "cogs/mem/freelist.hpp"
#include "cogs/mem/int_parts.hpp"
#include "cogs/mem/is_pointer_type.hpp"
#include "cogs/mem/is_raw_pointer_type.hpp"
#include "cogs/mem/is_raw_reference_type.hpp"
#include "cogs/mem/is_rc_pointer_type.hpp"
#include "cogs/mem/is_rc_reference_type.hpp"
#include "cogs/mem/is_rc_type.hpp"
#include "cogs/mem/is_reference_type.hpp"
#include "cogs/mem/is_same_instance.hpp"
#include "cogs/mem/is_static_castable.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/placement_header.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/mem/rcptr.hpp"
#include "cogs/mem/rcref.hpp"
#include "cogs/mem/rcref_freelist.hpp"
#include "cogs/mem/rc_container.hpp"
#include "cogs/mem/rc_obj.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/mem/ref.hpp"
#include "cogs/mem/storage_union.hpp"
#include "cogs/mem/unowned.hpp"
#include "cogs/mem/weak_rcptr.hpp"
#include "cogs/parser/escseq.hpp"
//#include "cogs/parser/regexp.hpp"
#include "cogs/sync/atomic_alignment.hpp"
#include "cogs/sync/atomic_compare_exchange.hpp"
#include "cogs/sync/atomic_exchange.hpp"
#include "cogs/sync/atomic_load.hpp"
#include "cogs/sync/atomic_store.hpp"
#include "cogs/sync/can_atomic.hpp"
#include "cogs/sync/cleanup_queue.hpp"
#include "cogs/sync/count_down_condition.hpp"
#include "cogs/sync/default_atomic_operators.hpp"
#include "cogs/sync/defer_guard.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/dispatch_parallel.hpp"
#include "cogs/sync/event.hpp"
#include "cogs/sync/hazard.hpp"
#include "cogs/sync/priority_dispatcher.hpp"
#include "cogs/sync/priority_queue.hpp"
#include "cogs/sync/pulse_timer.hpp"
#include "cogs/sync/quit_dispatcher.hpp"
#include "cogs/sync/refireable_timer.hpp"
#include "cogs/sync/resettable_condition.hpp"
#include "cogs/sync/resettable_timer.hpp"
#include "cogs/sync/rwlock.hpp"
#include "cogs/sync/semaphore.hpp"
#include "cogs/sync/serial_defer_guard.hpp"
#include "cogs/sync/serial_dispatcher.hpp"
#include "cogs/sync/single_fire_condition.hpp"
#include "cogs/sync/single_fire_timer.hpp"
#include "cogs/sync/thread.hpp"
#include "cogs/sync/thread_pool.hpp"
#include "cogs/sync/timer.hpp"
#include "cogs/sync/transactable.hpp"
#include "cogs/sync/versioned.hpp"
#include "cogs/sync/versioned_ptr.hpp"
#include "cogs/sync/versioned_ref.hpp"
#include "cogs/sync/wait_deque.hpp"
#include "cogs/sync/wait_priority_queue.hpp"
#include "cogs/sync/wait_queue.hpp"
#include "cogs/sync/wait_stack.hpp"
#include "cogs/ui/keyboard.hpp"


#endif
