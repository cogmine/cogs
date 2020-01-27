## TODO

* Move these TODO items to GitHub issues.
* Add optional internal N elements for vector (Short vector will deep copy. Does not require dynamic allocation. Applies to string, buffer, composite_vector, composite_string, compositive_buffer, dynamic_integer)
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
