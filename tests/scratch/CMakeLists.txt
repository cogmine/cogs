cmake_minimum_required(VERSION 3.15)

# This needs to be set before project()
if (WIN32)
  set(CMAKE_SYSTEM_VERSION 10.0.18362.0)
endif()

set(EXECUTABLE_NAME "scratch")
project (${EXECUTABLE_NAME} C CXX)
add_executable(${EXECUTABLE_NAME})

include(../../common.cmake)

target_sources(${EXECUTABLE_NAME} PUBLIC
  scratch.cpp
)

if (${COGS_HEADER_TEST})
set(HEADERS
  ../../core/src/cogs/assert.hpp
  ../../core/src/cogs/compatible.hpp
  ../../core/src/cogs/debug.hpp
  ../../core/src/cogs/dependency_property.hpp
  ../../core/src/cogs/function.hpp
  ../../core/src/cogs/function_list.hpp
  ../../core/src/cogs/load.hpp
  ../../core/src/cogs/macro_concat.hpp
  ../../core/src/cogs/macro_stringify.hpp
  ../../core/src/cogs/main.hpp
  ../../core/src/cogs/operators.hpp
  ../../core/src/cogs/collections/array_view.hpp
  ../../core/src/cogs/collections/avltree.hpp
  ../../core/src/cogs/collections/btree.hpp
  ../../core/src/cogs/collections/composite_string.hpp
  ../../core/src/cogs/collections/composite_vector.hpp
  ../../core/src/cogs/collections/container_deque.hpp
  ../../core/src/cogs/collections/container_dlist.hpp
  ../../core/src/cogs/collections/container_queue.hpp
  ../../core/src/cogs/collections/container_skiplist.hpp
  ../../core/src/cogs/collections/container_stack.hpp
  ../../core/src/cogs/collections/dlink.hpp
  ../../core/src/cogs/collections/dlist.hpp
  ../../core/src/cogs/collections/map.hpp
  ../../core/src/cogs/collections/multimap.hpp
  ../../core/src/cogs/collections/multiset.hpp
  ../../core/src/cogs/collections/no_aba_stack.hpp
  ../../core/src/cogs/collections/rbtree.hpp
  ../../core/src/cogs/collections/set.hpp
  ../../core/src/cogs/collections/simple_vector.hpp
  ../../core/src/cogs/collections/slink.hpp
  ../../core/src/cogs/collections/slist.hpp
  ../../core/src/cogs/collections/stack.hpp
  ../../core/src/cogs/collections/string.hpp
  ../../core/src/cogs/collections/tlink.hpp
  ../../core/src/cogs/collections/unicode.hpp
  ../../core/src/cogs/collections/vector.hpp
  ../../core/src/cogs/collections/vector_view.hpp
  ../../core/src/cogs/collections/weak_rcptr_list.hpp
  ../../core/src/cogs/crypto/adler32.hpp
  ../../core/src/cogs/crypto/cipher.hpp
  ../../core/src/cogs/crypto/crc.hpp
  ../../core/src/cogs/crypto/fletcher.hpp
  ../../core/src/cogs/crypto/fnv.hpp
  ../../core/src/cogs/crypto/hash.hpp
  ../../core/src/cogs/crypto/hash_int.hpp
  ../../core/src/cogs/crypto/haval.hpp
  ../../core/src/cogs/crypto/hmac.hpp
  ../../core/src/cogs/crypto/joaat.hpp
  ../../core/src/cogs/crypto/md2.hpp
  ../../core/src/cogs/crypto/md4.hpp
  ../../core/src/cogs/crypto/md5.hpp
  ../../core/src/cogs/crypto/ripemd.hpp
  ../../core/src/cogs/crypto/serial_hash.hpp
  ../../core/src/cogs/crypto/sha1.hpp
  ../../core/src/cogs/crypto/sha2.hpp
  ../../core/src/cogs/crypto/sha3.hpp
  ../../core/src/cogs/crypto/snefru.hpp
  ../../core/src/cogs/crypto/tiger.hpp
  ../../core/src/cogs/crypto/whirlpool.hpp
  ../../core/src/cogs/geometry/alignment.hpp
  ../../core/src/cogs/geometry/bounds.hpp
  ../../core/src/cogs/geometry/cell.hpp
  ../../core/src/cogs/geometry/dimension.hpp
  ../../core/src/cogs/geometry/direction.hpp
  ../../core/src/cogs/geometry/flow.hpp
  ../../core/src/cogs/geometry/margin.hpp
  ../../core/src/cogs/geometry/point.hpp
  ../../core/src/cogs/geometry/proportion.hpp
  ../../core/src/cogs/geometry/range.hpp
  ../../core/src/cogs/geometry/size.hpp
  ../../core/src/cogs/geometry/sizing_groups.hpp
  ../../core/src/cogs/gfx/canvas.hpp
  ../../core/src/cogs/gfx/color.hpp
  ../../core/src/cogs/gfx/font.hpp
  ../../core/src/cogs/gui/ansiterm.hpp
  ../../core/src/cogs/gui/background.hpp
  ../../core/src/cogs/gui/button.hpp
  ../../core/src/cogs/gui/button_box.hpp
  ../../core/src/cogs/gui/check_box.hpp
  ../../core/src/cogs/gui/editor.hpp
  ../../core/src/cogs/gui/frame.hpp
  ../../core/src/cogs/gui/grid.hpp
  ../../core/src/cogs/gui/label.hpp
  ../../core/src/cogs/gui/labeled_list.hpp
  ../../core/src/cogs/gui/list.hpp
  ../../core/src/cogs/gui/mouse.hpp
  ../../core/src/cogs/gui/native_container_pane.hpp
  ../../core/src/cogs/gui/pane.hpp
  ../../core/src/cogs/gui/pane_bridge.hpp
  ../../core/src/cogs/gui/bitmap_pane.hpp
  ../../core/src/cogs/gui/scroll_bar.hpp
  ../../core/src/cogs/gui/scroll_pane.hpp
  ../../core/src/cogs/gui/stack_panel.hpp
  ../../core/src/cogs/gui/subsystem.hpp
  ../../core/src/cogs/gui/table.hpp
  ../../core/src/cogs/gui/text_editor.hpp
  ../../core/src/cogs/gui/window.hpp
  ../../core/src/cogs/gui/wrap_list.hpp
  ../../core/src/cogs/io/buffer.hpp
  ../../core/src/cogs/io/composite_buffer.hpp
  ../../core/src/cogs/io/datasink.hpp
  ../../core/src/cogs/io/datasource.hpp
  ../../core/src/cogs/io/datastream.hpp
  ../../core/src/cogs/io/datastream_protocol.hpp
  ../../core/src/cogs/io/file.hpp
  ../../core/src/cogs/io/filter.hpp
  ../../core/src/cogs/io/limiter.hpp
  ../../core/src/cogs/io/permission.hpp
  ../../core/src/cogs/io/queue.hpp
  ../../core/src/cogs/io/net/address.hpp
  ../../core/src/cogs/io/net/connection.hpp
  ../../core/src/cogs/io/net/endpoint.hpp
  ../../core/src/cogs/io/net/http.hpp
  ../../core/src/cogs/io/net/ip.hpp
  ../../core/src/cogs/io/net/server.hpp
  ../../core/src/cogs/io/net/smtp.hpp
  ../../core/src/cogs/io/net/telnet.hpp
  ../../core/src/cogs/io/net/ip/address.hpp
  ../../core/src/cogs/io/net/ip/endpoint.hpp
  ../../core/src/cogs/io/net/ip/tcp.hpp
  ../../core/src/cogs/math/bits_to_bytes.hpp
  ../../core/src/cogs/math/bits_to_int.hpp
  ../../core/src/cogs/math/boolean.hpp
  ../../core/src/cogs/math/bytes_to_int.hpp
  ../../core/src/cogs/math/chars.hpp
  ../../core/src/cogs/math/const_extuadd.hpp
  ../../core/src/cogs/math/const_extudiv.hpp
  ../../core/src/cogs/math/const_extumul.hpp
  ../../core/src/cogs/math/const_gcd.hpp
  ../../core/src/cogs/math/const_lcm.hpp
  ../../core/src/cogs/math/const_max_int.hpp
  ../../core/src/cogs/math/const_min_int.hpp
  ../../core/src/cogs/math/const_upow.hpp
  ../../core/src/cogs/math/const_uroot.hpp
  ../../core/src/cogs/math/datetime.hpp
  ../../core/src/cogs/math/dynamic_integer.hpp
  ../../core/src/cogs/math/extumul.hpp
  ../../core/src/cogs/math/fixed_integer.hpp
  ../../core/src/cogs/math/fixed_integer_extended.hpp
  ../../core/src/cogs/math/fixed_integer_extended_const.hpp
  ../../core/src/cogs/math/fixed_integer_native.hpp
  ../../core/src/cogs/math/fixed_integer_native_const.hpp
  ../../core/src/cogs/math/fraction.hpp
  ../../core/src/cogs/math/is_arithmetic_type.hpp
  ../../core/src/cogs/math/is_const_type.hpp
  ../../core/src/cogs/math/is_integer_type.hpp
  ../../core/src/cogs/math/is_negative_value.hpp
  ../../core/src/cogs/math/is_signed_type.hpp
  ../../core/src/cogs/math/least_multiple_of.hpp
  ../../core/src/cogs/math/measure.hpp
  ../../core/src/cogs/math/measurement_types.hpp
  ../../core/src/cogs/math/negate_if_signed.hpp
  ../../core/src/cogs/math/next_exponent_of_two.hpp
  ../../core/src/cogs/math/next_multiple_of.hpp
  ../../core/src/cogs/math/prev_exponent_of_two.hpp
  ../../core/src/cogs/math/random.hpp
  ../../core/src/cogs/math/range_to_bits.hpp
  ../../core/src/cogs/math/range_to_bytes.hpp
  ../../core/src/cogs/math/range_to_int.hpp
  ../../core/src/cogs/math/time.hpp
  ../../core/src/cogs/math/value_to_bits.hpp
  ../../core/src/cogs/math/vec.hpp
  ../../core/src/cogs/mem/allocator.hpp
  ../../core/src/cogs/mem/allocator_container.hpp
  ../../core/src/cogs/mem/auto_handle.hpp
  ../../core/src/cogs/mem/bballoc.hpp
  ../../core/src/cogs/mem/bypass_constructor_permission.hpp
  ../../core/src/cogs/mem/bypass_strict_aliasing.hpp
  ../../core/src/cogs/mem/const_bit_rotate.hpp
  ../../core/src/cogs/mem/const_bit_scan.hpp
  ../../core/src/cogs/mem/const_set_bits.hpp
  ../../core/src/cogs/mem/count_args.hpp
  ../../core/src/cogs/mem/default_allocator.hpp
  ../../core/src/cogs/mem/delayed_construction.hpp
  ../../core/src/cogs/mem/endian.hpp
  ../../core/src/cogs/mem/freelist.hpp
  ../../core/src/cogs/mem/freelist_allocator.hpp
  ../../core/src/cogs/mem/int_parts.hpp
  ../../core/src/cogs/mem/is_pointer_type.hpp
  ../../core/src/cogs/mem/is_raw_pointer_type.hpp
  ../../core/src/cogs/mem/is_raw_reference_type.hpp
  ../../core/src/cogs/mem/is_rc_pointer_type.hpp
  ../../core/src/cogs/mem/is_rc_reference_type.hpp
  ../../core/src/cogs/mem/is_rc_type.hpp
  ../../core/src/cogs/mem/is_reference_type.hpp
  ../../core/src/cogs/mem/is_same_instance.hpp
  ../../core/src/cogs/mem/is_static_castable.hpp
  ../../core/src/cogs/mem/object.hpp
  ../../core/src/cogs/mem/placement.hpp
  ../../core/src/cogs/mem/placement_header.hpp
  ../../core/src/cogs/mem/ptr.hpp
  ../../core/src/cogs/mem/rcnew.hpp
  ../../core/src/cogs/mem/rcptr.hpp
  ../../core/src/cogs/mem/rcref.hpp
  ../../core/src/cogs/mem/rcref_freelist.hpp
  ../../core/src/cogs/mem/rc_container.hpp
  ../../core/src/cogs/mem/rc_obj.hpp
  ../../core/src/cogs/mem/rc_obj_base.hpp
  ../../core/src/cogs/mem/ref.hpp
  ../../core/src/cogs/mem/storage_union.hpp
  ../../core/src/cogs/mem/unowned.hpp
  ../../core/src/cogs/mem/weak_rcptr.hpp
  ../../core/src/cogs/parser/escseq.hpp
  ../../core/src/cogs/sync/atomic_alignment.hpp
  ../../core/src/cogs/sync/atomic_compare_exchange.hpp
  ../../core/src/cogs/sync/atomic_exchange.hpp
  ../../core/src/cogs/sync/atomic_load.hpp
  ../../core/src/cogs/sync/atomic_store.hpp
  ../../core/src/cogs/sync/auto_reset_event.hpp
  ../../core/src/cogs/sync/can_atomic.hpp
  ../../core/src/cogs/sync/cleanup_queue.hpp
  ../../core/src/cogs/sync/count_down_event.hpp
  ../../core/src/cogs/sync/default_atomic_operators.hpp
  ../../core/src/cogs/sync/defer_guard.hpp
  ../../core/src/cogs/sync/dispatcher.hpp
  ../../core/src/cogs/sync/dispatch_parallel.hpp
  ../../core/src/cogs/sync/hazard.hpp
  ../../core/src/cogs/sync/priority_dispatcher.hpp
  ../../core/src/cogs/sync/priority_queue.hpp
  ../../core/src/cogs/sync/pulse_timer.hpp
  ../../core/src/cogs/sync/quit_dispatcher.hpp
  ../../core/src/cogs/sync/refireable_timer.hpp
  ../../core/src/cogs/sync/resettable_event.hpp
  ../../core/src/cogs/sync/resettable_timer.hpp
  ../../core/src/cogs/sync/rwlock.hpp
  ../../core/src/cogs/sync/semaphore.hpp
  ../../core/src/cogs/sync/serial_defer_guard.hpp
  ../../core/src/cogs/sync/serial_dispatcher.hpp
  ../../core/src/cogs/sync/single_fire_event.hpp
  ../../core/src/cogs/sync/single_fire_timer.hpp
  ../../core/src/cogs/sync/thread.hpp
  ../../core/src/cogs/sync/thread_pool.hpp
  ../../core/src/cogs/sync/timer.hpp
  ../../core/src/cogs/sync/transactable.hpp
  ../../core/src/cogs/sync/versioned.hpp
  ../../core/src/cogs/sync/versioned_ptr.hpp
  ../../core/src/cogs/sync/versioned_ref.hpp
  ../../core/src/cogs/sync/wait_deque.hpp
  ../../core/src/cogs/sync/wait_priority_queue.hpp
  ../../core/src/cogs/sync/wait_queue.hpp
  ../../core/src/cogs/sync/wait_stack.hpp
  ../../core/src/cogs/ui/keyboard.hpp
)

# compile each header in isolation to ensure they include all of their own dependencies
target_sources(${EXECUTABLE_NAME} PUBLIC
  ${HEADERS}
)
set_source_files_properties(${HEADERS} PROPERTIES LANGUAGE CXX)
endif()

if (WIN32)

  target_sources(${EXECUTABLE_NAME} PUBLIC
    VS/scratch.manifest
    VS/scratch.rc
  )

elseif (APPLE)

  set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.cogmine.cogs.scratch"
    RESOURCE "guitar.bmp"
    XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS YES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
  )

    target_sources(${EXECUTABLE_NAME} PUBLIC
    MacOS/main.mm
    guitar.bmp
  )

  configure_file("CMake/Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/Info.plist")
  target_link_libraries(${EXECUTABLE_NAME} "-sectcreate __TEXT __info_plist ${CMAKE_CURRENT_BINARY_DIR}/Info.plist")

endif()
