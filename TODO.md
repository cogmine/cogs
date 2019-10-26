## TODO

* Remove use of unowned_t default arg pattern in favor of move assignments
* Rework allocators to align with allocators in std
* Create separte high vs low level implementation of lock-free types.
    * Low-level = simpler and prioritize performance and simplicity.
    * High-level = more robust.  i.e. Support thread-safe assignments including allocator. 
* exchange/compare_exchange overloads with move semantics
* Drag and flick scrolling
* Single/static dependency property for preferred scroll bar style
* scroll_pane::scroll_to()
* Console UI subsystem
* Revisit DNS - Are there proper async APIs for it now?
* Replace hide/show state with visibile/invisible/collapsed
* Finish porting all UI elements to MacOS
* Implement open_window_full_screen
* Implement support for setting full screen video resolution and modes
* Port to UWP
* Port to WPF
* Port to iOS
* Port to Android
* Add GUI support on Linux
* Implement UI element focus/defucus processing
* Rework dependency_property to use tasks for async set()
* Implement more UI properties as dependency_property's
* Rework serial dispatcher (currently embedded in pane) to use task completion to kick off next task, instead of call to serial_resume().
* Add internal single elements for vector<> (so single element vector does not require dynamic allocation.  Useful to collapse single-element composite_single, composite_vector, compositive_buffer)
* Get ANSI terminal w/telnet up and running again
* Add gui::stack_panel
* Add/rework gui::wrap_list
* Add/rework gui::grid
* Add/rework gui::list
* Add/rework gui::labeled_list
* Add/rework gui::button_box
* Add any missing cryptographic hashes (GOST? LLVM?)
* Add cryptographic ciphers (all)
* Implement SSL/TLS IO classes
* Implement HTTP related classes
* Implement REST related classes
* Implement GraphQL server/client classes
