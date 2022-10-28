//
// Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, MayNeedCleanup

#ifndef COGS_HEADER_GUI_PANE
#define COGS_HEADER_GUI_PANE


#include "cogs/env.hpp"
#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/collections/backed_vector.hpp"
#include "cogs/function.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/gui/frame.hpp"
#include "cogs/gui/mouse.hpp"
#include "cogs/gui/subsystem.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/math/const_min_int.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/event.hpp"
#include "cogs/sync/priority_queue.hpp"
#include "cogs/sync/resettable_condition.hpp"
#include "cogs/sync/serial_dispatcher.hpp"
#include "cogs/sync/thread_pool.hpp"
#include "cogs/ui/keyboard.hpp"

namespace cogs {

/// @defgroup GUI Graphical User Interface
/// @{
/// @ingroup Graphics
/// @brief Graphical User Interface
/// @}


/// @ingroup GUI
/// @brief Namespace for GUI
namespace gui {


class subsystem;
class pane;


class pane_container
{
public:
	void nest(const rcref<pane>& child) { return nest_last(child); }

	virtual void nest_last(const rcref<pane>& child) = 0;
	virtual void nest_first(const rcref<pane>& child) = 0;
	virtual void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child) = 0;
	virtual void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child) = 0;
};


// The only thread-safe methods of a pane are dispatch() and recompose().
// Access to all other methods must be serialized.  (Which can be done by dispatching against the pane itself).


class bridgeable_pane;


typedef container_dlist<rcref<pane> > pane_list;

/// @ingroup GUI
/// @brief Base class for 2D visual UI elements
class pane : public dispatcher, public frameable, protected virtual gfx::canvas, protected virtual pane_container
{
public:
	/// @ingroup GUI
	/// @brief The compositing behavior of a pane
	///
	/// A pane's compositing_behavior indicates whether or not it should use a backing buffer, and under
	/// what conditions that backing buffer needs to be re-rendered.
	///
	/// The top-most pane implicitly buffers the drawing of itself and all children, to avoid flicker.
	/// This is either supported automatically by the platform, rendering engine, or use of a compositing
	/// buffer in the top-most pane.
	enum class compositing_behavior
	{
		/// Does not buffer drawing.  draw() may be called frequently, as parent, child, or overlapping
		/// sibling panes are invalidated and redrawn to a common backing buffer.  This is useful for
		/// panes that change every time drawn, or are very simple to draw, as the overhead (and additional
		/// blit) of a backing buffer would be unnecessary.
		no_buffer = 0,

		/// Uses a backing buffer for this pane's draw operation, but it is not shared with child panes.
		/// Overlapping invalidates from child or sibling panes would not trigger calls to draw().
		/// This is useful to allow parent/children/sibling panes to render completely independently.
		/// Only the layer that is invalidated is redrawn, then composited with the existing backing
		/// buffers of the other layers.
		buffer_self = 1,

		/// Uses a backing buffer for the drawing operation of this pane as well as its child panes.
		/// Overlapping invalidates from child panes would cause redraws of the parent, so the parent and
		/// then children can re-render themselves to a common backing buffer.  But, overlapping invalidates
		/// from sibling panes would not trigger calls to draw().  This is useful to allow one hierachy
		/// of panes to render independently of another.
		buffer_self_and_children = 2,
	};

	friend class bridgeable_pane;
	friend class gui::subsystem;

	typedef handler_list<const rcref<pane>&, wchar_t, const ui::modifier_keys_state&> character_type_handler_list_t;
	typedef handler_list<const rcref<pane>&, wchar_t, const ui::modifier_keys_state&> key_press_handler_list_t;
	typedef handler_list<const rcref<pane>&, wchar_t, const ui::modifier_keys_state&> key_release_handler_list_t;
	typedef handler_list<const rcref<pane>&, mouse_button, const point&, const ui::modifier_keys_state&> button_press_handler_list_t;
	typedef handler_list<const rcref<pane>&, mouse_button, const point&, const ui::modifier_keys_state&> button_release_handler_list_t;
	typedef handler_list<const rcref<pane>&, mouse_button, const point&, const ui::modifier_keys_state&> button_double_click_handler_list_t;
	typedef handler_list<const rcref<pane>&, double, const point&, const ui::modifier_keys_state&> wheel_move_handler_list_t;

	// The request_close_handler returns true to incidate close should be prevented/blocked, false to continue
	// executing other handlers.  If a function is returned, it will be called when handlers are complete and
	// indicate true if close was allowed, false if another handler prevented the close.
	typedef handler_list<const rcref<pane>&, const rcref<pane>&, function<void(bool)>&> request_close_handler_list_t;

	typedef event<const rcref<pane>&> hide_event_t;
	typedef event<const rcref<pane>&> show_event_t;
	typedef event<const rcref<pane>&, int> focus_event_t;
	typedef event<const rcref<pane>&> defocus_event_t;
	typedef event<const rcref<pane>&, const ui::modifier_keys_state&> modifier_keys_change_event_t;
	typedef event<const rcref<pane>&, const point&> cursor_enter_event_t;
	typedef event<const rcref<pane>&, const point&> cursor_move_event_t;
	typedef event<const rcref<pane>&> cursor_leave_event_t;
	typedef event<const rcref<pane>&> detach_event_t;

	request_close_handler_list_t& get_request_close_handlers() { return m_requestCloseHandlers; }
	hide_event_t& get_hide_event() { return m_hideEvent; }
	show_event_t& get_show_event() { return m_showEvent; }
	focus_event_t& get_focus_event() { return m_focusEvent; }
	defocus_event_t& get_defocus_event() { return m_defocusEvent; }
	modifier_keys_change_event_t& get_modifier_keys_change_event() { return m_modifierKeysChangeEvent; }
	cursor_enter_event_t& get_cursor_enterEvent() { return m_cursorEnterEvent; }
	cursor_move_event_t& get_cursor_move_event() { return m_cursorMoveEvent; }
	cursor_leave_event_t& get_cursor_leave_event() { return m_cursorLeaveEvent; }
	detach_event_t& get_detach_event() { return m_detachEvent; }

private:
	weak_rcptr<pane> m_parent;
	pane_list m_children;
	pane_list::remove_token m_siblingIterator; // our element in our parent's list of children panes

	character_type_handler_list_t m_characterTypeHandlers;
	key_press_handler_list_t m_keyPressHandlers;
	key_release_handler_list_t m_keyReleaseHandlers;
	button_press_handler_list_t m_buttonPressHandlers;
	button_release_handler_list_t m_buttonReleaseHandlers;
	button_double_click_handler_list_t m_buttonDoubleClickHandlers;
	wheel_move_handler_list_t m_wheelMoveHandlers;
	request_close_handler_list_t m_requestCloseHandlers;

	hide_event_t m_hideEvent;
	show_event_t m_showEvent;
	focus_event_t m_focusEvent;
	defocus_event_t m_defocusEvent;
	modifier_keys_change_event_t m_modifierKeysChangeEvent;
	cursor_enter_event_t m_cursorEnterEvent;
	cursor_move_event_t m_cursorMoveEvent;
	cursor_leave_event_t m_cursorLeaveEvent;
	detach_event_t m_detachEvent;

	// Changes to the UI subsystem are serialized using the pane's dispatcher.

	volatile rcptr<volatile gui::subsystem> m_uiSubSystem;
	volatile rcptr<volatile gui::subsystem> m_parentUISubSystem;

	rcptr<pane> m_subFocus;
	ptrdiff_t m_hideShowState = 0;

	rcptr<bitmap> m_offScreenBuffer;

	compositing_behavior m_compositingBehavior;

	rcptr<gfx::canvas> m_bridgedCanvas;
	resettable_condition m_closeCondition;

	bool m_hasFocus = false;
	bool m_isFocusable = true;
	bool m_installed = false;
	bool m_installing = false;
	bool m_uninstalling = false;
	bool m_initialReshapeDone = false;
	bool m_cursorWasWithin = false;
	bool m_needsDraw = false;
	bool m_recomposeDescendants = true;
	bool m_invalidateOnReshape = false;

	bounds m_lastVisibleBounds;

	range m_range;
	std::optional<size> m_defaultSize;

	pane_list::iterator m_childInstallItor;
	rcptr<pane> m_parentInstalling;
	bool m_topmostUninstall = false;

	volatile boolean m_recomposing;
	volatile boolean m_detaching;

	rcref<volatile dispatcher> get_next_dispatcher() const
	{
		rcptr<volatile dispatcher> subSystem = m_uiSubSystem;
		if (!!subSystem)
			return subSystem.dereference();
		subSystem = m_parentUISubSystem;
		if (!!subSystem)
			return subSystem.dereference();
		return thread_pool::get_default_or_immediate();
	}

	class dispatcher_proxy : public dispatcher
	{
	private:
		const pane& m_pane;

	public:
		dispatcher_proxy(const pane& p)
			: m_pane(p)
		{ }

		virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
		{
			dispatcher::dispatch_inner(*(m_pane.get_next_dispatcher()), t, priority);
		}
	};

	// The purpose of this proxy is to selectively defer tasks to either the UI subsystem the
	// pane is currently installed into, or the global thread pool if not installed.
	dispatcher_proxy m_dispatcherProxy;

	rcref<serial_dispatcher> m_serialDispatcher;

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		dispatcher::dispatch_inner(*m_serialDispatcher, t, priority);
	}

	rcptr<signallable_task<void> > m_installTask;

protected:
	struct options
	{
		bool invalidateOnReshape = false;
		character_type_handler_list_t characterTypeHandlers;
		key_press_handler_list_t keyPressHandlers;
		key_release_handler_list_t keyReleaseHandlers;
		button_press_handler_list_t buttonPressHandlers;
		button_release_handler_list_t buttonReleaseHandlers;
		button_double_click_handler_list_t buttonDoubleClickHandlers;
		wheel_move_handler_list_t wheelMoveHandlers;
		request_close_handler_list_t requestCloseHandlers;
		hide_event_t hideEvent;
		show_event_t showEvent;
		focus_event_t focusEvent;
		defocus_event_t defocusEvent;
		modifier_keys_change_event_t modifierKeysChangeEvent;
		cursor_enter_event_t cursorEnterEvent;
		cursor_move_event_t cursorMoveEvent;
		cursor_leave_event_t cursorLeaveEvent;
		detach_event_t detachEvent;
		compositing_behavior compositingBehavior = compositing_behavior::no_buffer;
		frame_list frames;
		pane_list children;
	};

	pane()
		: pane(options())
	{ }

	explicit pane(options&& o)
		: frameable(std::move(o.frames)),
		m_children(std::move(o.children)),
		m_characterTypeHandlers(std::move(o.characterTypeHandlers)),
		m_keyPressHandlers(std::move(o.keyPressHandlers)),
		m_keyReleaseHandlers(std::move(o.keyReleaseHandlers)),
		m_buttonPressHandlers(std::move(o.buttonPressHandlers)),
		m_buttonReleaseHandlers(std::move(o.buttonReleaseHandlers)),
		m_buttonDoubleClickHandlers(std::move(o.buttonDoubleClickHandlers)),
		m_wheelMoveHandlers(std::move(o.wheelMoveHandlers)),
		m_requestCloseHandlers(std::move(o.requestCloseHandlers)),
		m_hideEvent(std::move(o.hideEvent)),
		m_showEvent(std::move(o.showEvent)),
		m_focusEvent(std::move(o.focusEvent)),
		m_defocusEvent(std::move(o.defocusEvent)),
		m_modifierKeysChangeEvent(std::move(o.modifierKeysChangeEvent)),
		m_cursorEnterEvent(std::move(o.cursorEnterEvent)),
		m_cursorMoveEvent(std::move(o.cursorMoveEvent)),
		m_cursorLeaveEvent(std::move(o.cursorLeaveEvent)),
		m_detachEvent(std::move(o.detachEvent)),
		m_compositingBehavior(o.compositingBehavior),
		m_invalidateOnReshape(o.invalidateOnReshape),
		m_dispatcherProxy(*this),
		m_serialDispatcher(rcnew(serial_dispatcher)(m_dispatcherProxy))
	{
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			COGS_ASSERT(!(*itor)->m_siblingIterator);
			(*itor)->m_siblingIterator = itor;
			(*itor)->m_parent = this_rcref;
			++itor;
		}
	}

	character_type_handler_list_t& get_character_type_handlers() { return m_characterTypeHandlers; }
	key_press_handler_list_t& get_key_press_handlers() { return m_keyPressHandlers; }
	key_release_handler_list_t& get_key_release_handlers() { return m_keyReleaseHandlers; }
	button_press_handler_list_t& get_button_press_handlers() { return m_buttonPressHandlers; }
	button_release_handler_list_t& get_button_release_handlers() { return m_buttonReleaseHandlers; }
	button_double_click_handler_list_t& get_button_double_click_handlers() { return m_buttonDoubleClickHandlers; }
	wheel_move_handler_list_t& get_wheel_move_handlers() { return m_wheelMoveHandlers; }

	void set_externally_drawn(const rcref<gfx::canvas>& externalCanvas)
	{
		COGS_ASSERT(m_installing); // This function should only be called when installing
		COGS_ASSERT(!m_bridgedCanvas);
		m_bridgedCanvas = externalCanvas;
	}

	// Changes to the UI subsystem are serialized using the pane's dispatcher.
	// get_subsystem() will likely not immediately reflect subsystem changes.
	// Only safe to call get_subsystem from the pane's own dispatcher.
	rcptr<volatile gui::subsystem> get_subsystem() const volatile
	{
		rcptr<volatile gui::subsystem> ss = m_uiSubSystem;
		if (!ss)
			return m_parentUISubSystem;
		return ss;
	}

	bool is_ui_thread_current() const volatile
	{
		rcptr<volatile gui::subsystem> ss = get_subsystem();
		if (!!ss)
			return ss->is_ui_thread_current();
		return false;
	}

	void clear_subsystem()
	{
		rcptr<volatile gui::subsystem> emptyRef;
		set_subsystem(emptyRef);
	}

	void set_subsystem(const rcptr<volatile gui::subsystem>& s)
	{
		self_acquire();
		dispatch([r{ this_rcptr }, s]()
		{
			r->set_subsystem_inner(s);
		});
	}

	const rcptr<bitmap>& peek_offscreen_buffer() const
	{
		COGS_ASSERT(m_installed);
		return m_offScreenBuffer;
	}

	const rcptr<bitmap>& get_offscreen_buffer()
	{
		COGS_ASSERT(m_installed);
		prepare_offscreen_buffer();
		return m_offScreenBuffer;
	}

	point get_render_position()
	{
		point offset(0, 0);
		if ((m_compositingBehavior == compositing_behavior::no_buffer) && (!is_externally_drawn()))
			offset = get_ancestor_render_position();
		return offset;
	}

	point get_ancestor_render_position() const
	{
		point offset(0, 0);
		rcptr<pane> p = m_parent;
		if (!!p)
		{
			offset += get_position();
			while ((p->m_compositingBehavior != compositing_behavior::buffer_self_and_children) && (!p->is_externally_drawn()))
			{
				rcptr<pane> p2 = p->get_parent();
				if (!p2)
					break;
				offset += p->get_position();
				p = p2;
			}
		}
		return offset;
	}

	rcptr<pane> get_render_pane()
	{
		if ((m_compositingBehavior == compositing_behavior::no_buffer) && (!is_externally_drawn()))
			return get_ancestor_render_pane();
		return this_rcptr;
	}

	rcptr<pane> get_render_pane(point& offset)
	{
		if ((m_compositingBehavior == compositing_behavior::no_buffer) && (!is_externally_drawn()))
			return get_ancestor_render_pane(offset);
		return this_rcptr;
	}

	rcptr<pane> get_render_pane(bounds& b)
	{
		b &= bounds(get_size());
		if ((m_compositingBehavior == compositing_behavior::no_buffer) && (!is_externally_drawn()))
			return get_ancestor_render_pane(b);
		return this_rcptr;
	}

	rcptr<pane> get_render_pane(point& offset, bounds& b)
	{
		if ((m_compositingBehavior == compositing_behavior::no_buffer) && (!is_externally_drawn()))
			return get_ancestor_render_pane(offset, b);
		b &= bounds(get_size());
		return this_rcptr;
	}

	rcptr<pane> get_ancestor_render_pane() const
	{
		rcptr<pane> parent = m_parent;
		while (!!parent)
		{
			if (parent->is_externally_drawn() || parent->m_compositingBehavior == compositing_behavior::buffer_self_and_children)
				break;
			parent = parent->m_parent;
		}
		return parent;
	}

	rcptr<pane> get_ancestor_render_pane(point& offset) const
	{
		const pane* child = this;
		rcptr<pane> parent = m_parent;
		while (!!parent)
		{
			offset += child->get_position();
			if (parent->is_externally_drawn() || parent->m_compositingBehavior == compositing_behavior::buffer_self_and_children)
				break;
			child = parent.get_ptr();
			parent = child->m_parent;
		}
		return parent;
	}

	rcptr<pane> get_ancestor_render_pane(bounds& b) const
	{
		const pane* child = this;
		rcptr<pane> parent = m_parent;
		while (!!parent)
		{
			point childPosition = child->get_position();
			b += childPosition;
			b &= bounds(parent->get_size());
			if (parent->is_externally_drawn() || parent->m_compositingBehavior == compositing_behavior::buffer_self_and_children)
				break;
			child = parent.get_ptr();
			parent = child->m_parent;
		}
		return parent;
	}

	rcptr<pane> get_ancestor_render_pane(point& offset, bounds& b) const
	{
		const pane* child = this;
		rcptr<pane> parent = m_parent;
		while (!!parent)
		{
			point childPosition = child->get_position();
			offset += childPosition;
			b += childPosition;
			b &= bounds(parent->get_size());
			if (parent->is_externally_drawn() || parent->m_compositingBehavior == compositing_behavior::buffer_self_and_children)
				break;
			child = parent.get_ptr();
			parent = child->m_parent;
		}
		return parent;
	}

	rcref<task<void> > install(const rcptr<volatile subsystem>& subSystem)
	{
		COGS_ASSERT(!m_installTask);
		rcref<task<void> > installTask = (m_installTask = rcnew(signallable_task<void>)).dereference();
		subSystem->dispatch([r{ this_rcref }, subSystem]()
		{
			r->m_uiSubSystem = subSystem;
			r->dispatch([r]()
			{
				rcptr<signallable_task<void> > installTask2 = r->m_installTask;
				r->install_inner2();
				return installTask2.dereference();
			});
		});
		return installTask;
	}

	rcref<task<void> > uninstall()
	{
		COGS_ASSERT(!m_installTask);
		rcref<task<void> > installTask = (m_installTask = rcnew(signallable_task<void>)).dereference();
		dispatch([r{ this_rcref }]()
		{
			rcptr<signallable_task<void> > installTask2 = r->m_installTask;
			r->uninstall_inner();
			return installTask2.dereference();
		});
		return installTask;
	}

	virtual void nest_last(const rcref<pane>& child)
	{
		child->m_parent = this_rcref;

		child->m_siblingIterator = m_children.append(child);

		COGS_ASSERT(!child->m_installed && !child->m_installing);
		if (m_installed)
			child->install(get_subsystem());
	}

	virtual void nest_first(const rcref<pane>& child)
	{
		child->m_parent = this_rcref;

		child->m_siblingIterator = m_children.prepend(child);

		COGS_ASSERT(!child->m_installed && !child->m_installing);
		if (m_installed)
			child->install(get_subsystem());
	}

	virtual void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child)
	{
		child->m_parent = this_rcref;

		child->m_siblingIterator = m_children.insert_before(beforeThis->m_siblingIterator, child);

		COGS_ASSERT(!child->m_installed && !child->m_installing);
		if (m_installed)
			child->install(get_subsystem());
	}

	virtual void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child)
	{
		child->m_parent = this_rcref;

		child->m_siblingIterator = m_children.insert_after(afterThis->m_siblingIterator, child);

		COGS_ASSERT(!child->m_installed && !child->m_installing);
		if (m_installed)
			child->install(get_subsystem());
	}

	void detach_children()
	{
		pane_list::iterator itor = m_children.get_first();
		if (!!itor)
		{
			do {
				if ((*itor)->is_visible())
					(*itor)->hiding();
				detaching_child(*itor);
			} while (!!++itor);
			if (get_desc()->get_strong_count() > 0) // Don't recompose if we're called from destructor
				recompose();
		}
	}

	compositing_behavior get_compositing_behavior() const { return m_compositingBehavior; }

	void set_compositing_behavior(compositing_behavior cb)
	{
		if (m_compositingBehavior != cb)
		{
			if (!m_installed)
				m_compositingBehavior = cb;
			else
			{
				if (cb == compositing_behavior::no_buffer)
					m_offScreenBuffer.release();

				if (cb == compositing_behavior::buffer_self) // invalidate before, to ensure children get invalidated.
					invalidate(get_size());

				m_compositingBehavior = cb;

				if (cb != compositing_behavior::buffer_self)
					invalidate(get_size());
			}
		}
	}

	void draw()
	{
		if (is_visible())
		{
			auto composite_offscreen_buffer = [&]()
			{
				if (!is_externally_drawn())
				{
					COGS_ASSERT(!!m_offScreenBuffer);
					point offset(0, 0);
					bounds visibleBounds = get_size();
					rcptr<pane> p = get_ancestor_render_pane(offset, visibleBounds);
					COGS_ASSERT(!!p);
					p->save_clip();
					p->clip_to(visibleBounds);
					p->clip_opaque_externals_after(*this, offset, visibleBounds);
					p->clip_opaque_external_descendants(*this, offset, visibleBounds);
					bounds b = get_bounds();
					p->draw_bitmap(*m_offScreenBuffer, b, b.get_size());
					p->restore_clip();
				}
			};

			prepare_offscreen_buffer();
			if (!!m_offScreenBuffer && !m_needsDraw) // --- Buffer already available:
			{
				composite_offscreen_buffer();                                   // Fully clip, Blit, Unclip
				if (m_compositingBehavior == compositing_behavior::buffer_self) // If buffering only this pane,
					draw_children();                                            // Draw children (they clip themselves)
			}
			else
			{
				m_needsDraw = false;
				if (m_compositingBehavior == compositing_behavior::no_buffer) // --- Not buffering:
				{                                                             // Fully clip
					point offset(0, 0);
					bounds visibleBounds = get_size();
					rcptr<pane> p = get_render_pane(offset, visibleBounds);
					COGS_ASSERT(!!p);
					p->save_clip();
					p->clip_to(visibleBounds);
					if (!is_externally_drawn())
						p->clip_opaque_after(*this, offset, visibleBounds);
					p->clip_opaque_descendants(*this, offset, visibleBounds);
					drawing();                            // Draw this pane
					p->restore_clip();                    // Unclip
					draw_children();                      // Draw children (they clip themselves)
				}
				else if (m_compositingBehavior == compositing_behavior::buffer_self) // --- Only buffering this pane:
				{
					size sz = get_size();
					bounds b = sz;
					save_clip();
					clip_to(b);                           // Clip to bounds
					fill(b, color::constant::transparent, false);
					drawing();                            // Draw this pane
					restore_clip();                       // Unclip
					composite_offscreen_buffer();         // Fully clip, Blit, Unclip
					draw_children();                      // Draw children (they clip themselves)
				}
				else //if (m_compositingBehavior == compositing_behavior::buffer_self_and_children) // --- Buffer this pane and children:
				{
					size sz = get_size();
					bounds b = sz;
					save_clip();
					point offset(0, 0);
					clip_opaque_descendants(*this, offset, b); // Clip descendants
					if (!!m_parent)
					{
						clip_to(b);                                // Clip to bounds
						fill(sz, color::constant::transparent, false);
					}
					drawing();                                 // Draw this pane
					restore_clip();                            // Unclip
					draw_children();                           // Draw children (they clip themselves)
					composite_offscreen_buffer();              // Fully clip, Blit, Unclip
				}
			}
		}
	}

	void set_focusable(bool isFocusable)
	{
		if (isFocusable != m_isFocusable)
		{
			m_isFocusable = isFocusable;
			if (!isFocusable && m_hasFocus)
			{
				// If we were focused, release focus, try to find new focus
				defocus();

				// Check parents for something that accepts focus.
				// Will actually reset focus order to first focusable under closes focusable parent.
				rcptr<pane> child = this_rcptr;
				rcptr<pane> parent = m_parent;
				while (!!parent)
				{
					if (parent->m_subFocus == child)
						parent->m_subFocus = 0;
					if (parent->is_focusable(child.get_ptr()))
					{
						parent->focus(1);
						break;
					}
					child = parent;
					parent = parent->get_parent();
				}
			}
		}
	}

	// A pane may have focus, but it may also contain a child pane that has subfocus.
	//
	// has_focus()         <- indicates true if focused, regardless of subfocus.
	// has_primary_focus() <- indicates true only if focused with no subfocus.

	bool has_focus() const { return m_hasFocus; }

	bool has_primary_focus() const { return (m_hasFocus && !m_subFocus); } // if false, a child has the primary focus

	bool is_focusable() const { return is_focusable(0); }

	// A call to focus should be synchronous.  If focusing() is overridden, it should call
	// pane::focusing() before returning.
	void focus(int direction = 0) // direction: -1 = prev/shift-tab, 0 = restore focus, 1 = next/tab
	{
		if (!!m_hasFocus)
		{
			if (!m_subFocus) // Maybe we're being prompted to notice a new subFocus
				focus_children(direction);
		}
		else
		{
			focus_parent(direction);
			m_hasFocus = true;
			focusing(direction);
		}
	}

	// defocus() is called internally to remove focus from a hierachy of UI objects, and
	// should generally get called externally only by a top-level window (usually platform
	// specific).  If a pane has focus and needs to make itself inelidgable, call set_focusable().
	void defocus()
	{
		if (m_hasFocus)
		{
			defocusing();
			m_hasFocus = false;
			if (!!m_subFocus)
				m_subFocus->defocus();
		}
	}

	// Call recompose if something changes which affects the min/max size of a pane,
	// to trigger calls to calculate_range() and reshape_frame().
	// Safe to call outside of this pane's UI thread.
	void recompose(bool recomposeDescendants = false)
	{
		if (m_recomposing.compare_exchange(true, false))
		{
			dispatch([r{ this_rcref }, recomposeDescendants]()
			{
				r->recomposing(recomposeDescendants);
			}, 1); // slightly lower priority, so recompose happens after whatever else is done
		}
	}

	void recomposing(bool recomposeDescendants)
	{
		m_recomposing = false; // only works to guard redundant calls to recompose() while in queue
		if (m_installed && !m_uninstalling)
		{
			rcptr<pane> parent = m_parent;
			for (;;)
			{
				if (!parent)
				{
					m_recomposeDescendants = recomposeDescendants;
					calculate_range();
					m_recomposeDescendants = true;
				}
				else
				{
					range oldParentRange = get_frame_range();
					std::optional<size> oldDefaultSize = get_frame_default_size();
					m_recomposeDescendants = recomposeDescendants;
					calculate_range();
					m_recomposeDescendants = true;
					range newParentRange = get_frame_range();
					std::optional<size> newDefaultSize = get_frame_default_size();
					bool defaultSizeChanged = newDefaultSize.has_value() != oldDefaultSize.has_value() || (newDefaultSize.has_value() && *newDefaultSize != *oldDefaultSize);
					if (defaultSizeChanged || newParentRange != oldParentRange) // if parent would be unaffected
					{
						parent->recompose();
						break;
					}
				}
				std::optional<size> sz = calculate_frame_size(get_size());
				size sz2;
				if (sz.has_value())
					sz2 = *sz;
				else
					sz2.clear();
				reshape_frame(sz2);
				break;
			}
		}
	}

	void invalidate(const bounds& b) // b in local coords
	{
		bounds b2 = b;
		rcptr<pane> renderPane = get_render_pane(b2);
		if (!!b)
		{
			renderPane->m_needsDraw = true;
			renderPane->invalidating(b2);
			renderPane->invalidating_up(b2);
		}
	}

	rcptr<pane> get_next_focusable(bool direction = true)
	{
		rcref<pane> child = this_rcref;
		for (;;)
		{
			rcptr<pane> parent = child->get_parent();
			if (!parent)
				return child;
			pane_list::iterator itor = child->m_siblingIterator;
			for (;;)
			{
				if (direction)
					++itor;
				else
					--itor;
				if (!itor) // hit end of the list
					break;
				if ((*itor)->is_focusable())
					return (*itor);
				//continue;
			}
			child = parent.dereference();
		}
	}

	rcptr<pane> get_prev_focusable() { return get_next_focusable(false); }

	size get_curent_size() const { return cell::get_size(); }

	template <bool topToButton, typename F>
	bool for_each_subtree(F&& f)
	{
		rcref<pane> top = this_rcref;
		rcref<pane> p = top;
		for (;;)
		{
			if (topToButton)
			{
				if (!f(p))
					break;
			}
			auto itor = p->get_children().get_first();
			if (!!itor)
			{
				p = *itor;
				continue;
			}
			if (!topToButton)
			{
				if (!f(p))
					break;
			}
			for (;;)
			{
				if (p == top)
					return true;
				itor = p->get_sibling_iterator();
				++itor;
				if (!!itor)
				{
					p = *itor;
					break;
				}
				rcptr<pane> p2 = p->get_parent();
				COGS_ASSERT(!!p2);
				p = std::move(p2).dereference();
			}
		}
		return false;
	}

	const waitable& get_close_condition() const
	{
		return m_closeCondition;
	}

	virtual void closing()
	{
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if (!(*itor)->m_closeCondition.is_signaled())
				(*itor)->closing();
			++itor;
		}
		m_closeCondition.signal();
	}

	virtual void close()
	{
		if (!m_closeCondition.is_signaled())
		{
			hide();
			closing();
		}
	}

	bool requesting_close()
	{
		if (!!m_closeCondition.is_signaled())
			return true;

		function_list<void(bool)> cb;
		bool allowClose = for_each_subtree<true>([&cb, closingPane{ this_rcref }](const rcref<pane>& p)
		{
			for (auto& h : p->m_requestCloseHandlers)
			{
				function<void(bool)> f;
				bool preventClose = h(p, closingPane, f);
				if (!!f)
					cb.append(std::move(f));
				if (preventClose)
					return false;
			}
			return true;
		});
		cb(allowClose);
		if (allowClose)
		{
			hide();
			closing();
			return true;
		}
		return false;
	}

	bool request_close()
	{
		return requesting_close();
	}

	// cell interface

	void calculate_range()
	{
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if (m_recomposeDescendants)
				(*itor)->calculate_range();
			++itor;
		}

		frameable::calculate_range();
	}

	virtual void calculating_range()
	{
		m_defaultSize.reset();
		m_range.clear();

		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			std::optional<size> childDefaultSize = (*itor)->get_frame_default_size();
			if (childDefaultSize.has_value())
			{
				if (m_defaultSize.has_value())
					*m_defaultSize |= *childDefaultSize;
				else
					m_defaultSize = *childDefaultSize;
			}
			m_range &= (*itor)->get_frame_range();
			++itor;
		}

		if (m_defaultSize.has_value())
			*m_defaultSize = m_range.get_limit(*m_defaultSize);
	}

	// canvas interface

	virtual void fill(const bounds& b, const color& c = color::constant::black, bool blendAlpha = true)
	{
		if (!!m_offScreenBuffer)
			m_offScreenBuffer->fill(b, c, blendAlpha);
		else if (!!m_bridgedCanvas)
			m_bridgedCanvas->fill(b, c, blendAlpha);
		else
		{
			point offset(0, 0);
			rcptr<pane> parent = get_ancestor_render_pane(offset);
			if (!!parent)
			{
				bounds r2 = b;
				r2 += offset;
				parent->fill(r2, c, true);
			}
		}
	}

	virtual void invert(const bounds& b)
	{
		if (!!m_offScreenBuffer)
			m_offScreenBuffer->invert(b);
		else if (!!m_bridgedCanvas)
			m_bridgedCanvas->invert(b);
		else
		{
			point offset(0, 0);
			rcptr<pane> parent = get_ancestor_render_pane(offset);
			if (!!parent)
			{
				bounds r2 = b;
				r2 += offset;
				parent->invert(r2);
			}
		}
	}

	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::constant::black, bool blendAlpha = true)
	{
		if (!!m_offScreenBuffer)
			m_offScreenBuffer->draw_line(startPt, endPt, width, c, blendAlpha);
		else if (!!m_bridgedCanvas)
			m_bridgedCanvas->draw_line(startPt, endPt, width, c, blendAlpha);
		else
		{
			point offset(0, 0);
			rcptr<pane> parent = get_ancestor_render_pane(offset);
			if (!!parent)
			{
				point startPt2 = startPt;
				point endPt2 = endPt;
				startPt2 += offset;
				endPt2 += offset;
				parent->draw_line(startPt2, endPt2, width, c, true);
			}
		}
	}

	virtual rcref<font> load_font(const gfx::font_parameters_list& fnt)
	{
		if (!!m_offScreenBuffer)
			return m_offScreenBuffer->load_font(fnt);

		if (!!m_bridgedCanvas)
			return m_bridgedCanvas->load_font(fnt);

		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Top level should have bridged canvas.  Should not be called if not installed.
		return parent->load_font(fnt);
	}

	virtual string get_default_font_name() const
	{
		if (!!m_offScreenBuffer)
			return m_offScreenBuffer->get_default_font_name();

		if (!!m_bridgedCanvas)
			return m_bridgedCanvas->get_default_font_name();

		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Top level should have bridged canvas.  Should not be called if not installed.
		return parent->get_default_font_name();
	}

	virtual void draw_text(const composite_string& s, const bounds& b, const rcptr<font>& fnt = 0, const color& c = color::constant::black)
	{
		if (!!m_offScreenBuffer)
			m_offScreenBuffer->draw_text(s, b, fnt, c);
		else if (!!m_bridgedCanvas)
			m_bridgedCanvas->draw_text(s, b, fnt, c);
		else
		{
			point offset(0, 0);
			rcptr<pane> parent = get_ancestor_render_pane(offset);
			if (!!parent)
			{
				bounds r2 = b;
				r2 += offset;
				parent->draw_text(s, r2, fnt, c);
			}
		}
	}

	virtual void draw_bitmap(const bitmap& src, const bounds& srcBounds, const bounds& dstBounds, bool blendAlpha = true)
	{
		if (!!m_offScreenBuffer)
			m_offScreenBuffer->draw_bitmap(src, srcBounds, dstBounds, blendAlpha);
		else if (!!m_bridgedCanvas)
			m_bridgedCanvas->draw_bitmap(src, srcBounds, dstBounds, blendAlpha);
		else
		{
			point offset(0, 0);
			rcptr<pane> parent = get_ancestor_render_pane(offset);
			if (!!parent)
			{
				bounds dstBounds2 = dstBounds;
				dstBounds2 += offset;
				parent->draw_bitmap(src, srcBounds, dstBounds2, true);
			}
		}
	}

	virtual void draw_bitmask(const bitmask& msk, const bounds& srcBounds, const bounds& dstBounds, const color& fore = color::constant::black, const color& back = color::constant::white, bool blendForeAlpha = true, bool blendBackAlpha = true)
	{
		if (!!m_offScreenBuffer)
			m_offScreenBuffer->draw_bitmask(msk, srcBounds, dstBounds, fore, back, blendForeAlpha, blendBackAlpha);
		else if (!!m_bridgedCanvas)
			m_bridgedCanvas->draw_bitmask(msk, srcBounds, dstBounds, fore, back, blendForeAlpha, blendBackAlpha);
		else
		{
			point offset(0, 0);
			rcptr<pane> parent = get_ancestor_render_pane(offset);
			if (!!parent)
			{
				bounds dstBounds2 = dstBounds;
				dstBounds2 += offset;
				parent->draw_bitmask(msk, srcBounds, dstBounds2, fore, back, true, true);
			}
		}
	}

	virtual void mask_out(const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool inverted = false)
	{
		if (!!m_offScreenBuffer)
			m_offScreenBuffer->mask_out(msk, mskBounds, dstBounds, inverted);
		else if (!!m_bridgedCanvas)
			m_bridgedCanvas->mask_out(msk, mskBounds, dstBounds, inverted);
		else
		{
			point offset(0, 0);
			rcptr<pane> parent = get_ancestor_render_pane(offset);
			if (!!parent)
			{
				bounds dstBounds2 = dstBounds;
				dstBounds2.get_position() += offset;
				parent->mask_out(msk, mskBounds, dstBounds2, inverted);
			}
		}
	}

	virtual void draw_bitmap_with_bitmask(const bitmap& src, const bounds& srcBounds, const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool blendAlpha = true, bool inverted = false)
	{
		if (!!m_offScreenBuffer)
			m_offScreenBuffer->draw_bitmap_with_bitmask(src, srcBounds, msk, mskBounds, dstBounds, blendAlpha, inverted);
		else if (!!m_bridgedCanvas)
			m_bridgedCanvas->draw_bitmap_with_bitmask(src, srcBounds, msk, mskBounds, dstBounds, blendAlpha, inverted);
		else
		{
			point offset(0, 0);
			rcptr<pane> parent = get_ancestor_render_pane(offset);
			if (!!parent)
			{
				bounds dstBounds2 = dstBounds;
				dstBounds2.get_position() += offset;
				parent->draw_bitmap_with_bitmask(src, srcBounds, msk, mskBounds, dstBounds2, true, inverted);
			}
		}
	}

	virtual rcref<bitmap> create_bitmap(const size& sz, std::optional<color> fillColor = std::nullopt)
	{
		if (!!m_offScreenBuffer)
			return m_offScreenBuffer->create_bitmap(sz, fillColor);

		if (!!m_bridgedCanvas)
			return m_bridgedCanvas->create_bitmap(sz, fillColor);

		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Top level should have bridged canvas.  Should not be called if not installed.
		return parent->create_bitmap(sz, fillColor);
	}

	virtual rcref<bitmap> load_bitmap(const composite_string& location)
	{
		if (!!m_offScreenBuffer)
			return m_offScreenBuffer->load_bitmap(location);

		if (!!m_bridgedCanvas)
			return m_bridgedCanvas->load_bitmap(location);

		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Top level should have bridged canvas.  Should not be called if not installed.
		return parent->load_bitmap(location);
	}

	virtual rcref<bitmask> create_bitmask(const size& sz, std::optional<bool> value = std::nullopt)
	{
		if (!!m_offScreenBuffer)
			return m_offScreenBuffer->create_bitmask(sz, value);

		if (!!m_bridgedCanvas)
			return m_bridgedCanvas->create_bitmask(sz, value);

		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Top level should have bridged canvas.  Should not be called if not installed.
		return parent->create_bitmask(sz, value);
	}

	virtual rcref<bitmask> load_bitmask(const composite_string& location)
	{
		if (!!m_offScreenBuffer)
			return m_offScreenBuffer->load_bitmask(location);

		if (!!m_bridgedCanvas)
			return m_bridgedCanvas->load_bitmask(location);

		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Top level should have bridged canvas.  Should not be called if not installed.
		return parent->load_bitmask(location);
	}

	virtual void save_clip()
	{
		if (!!m_offScreenBuffer)
			m_offScreenBuffer->save_clip();
		else if (!!m_bridgedCanvas)
			m_bridgedCanvas->save_clip();
		else
		{
			rcptr<pane> parent = get_ancestor_render_pane();
			if (!!parent)
				parent->save_clip();
		}
	}

	virtual void restore_clip()
	{
		if (!!m_offScreenBuffer)
			m_offScreenBuffer->restore_clip();
		else if (!!m_bridgedCanvas)
			m_bridgedCanvas->restore_clip();
		else
		{
			rcptr<pane> parent = get_ancestor_render_pane();
			if (!!parent)
				parent->restore_clip();
		}
	}

	virtual void clip_out(const bounds& b)
	{
		if (!!m_offScreenBuffer)
			m_offScreenBuffer->clip_out(b);
		else if (!!m_bridgedCanvas)
			m_bridgedCanvas->clip_out(b);
		else if (is_visible())
		{
			point offset;
			rcptr<pane> parent = get_ancestor_render_pane(offset);
			if (!!parent)
			{
				bounds r2 = b;
				r2 += offset;
				parent->clip_out(r2);
			}
		}
	}

	virtual void clip_to(const bounds& b)
	{
		if (!!m_offScreenBuffer)
			m_offScreenBuffer->clip_to(b);
		else if (!!m_bridgedCanvas)
			m_bridgedCanvas->clip_to(b);
		else if (is_visible())
		{
			point offset;
			rcptr<pane> parent = get_ancestor_render_pane(offset);
			if (!!parent)
			{
				bounds r2 = b;
				r2 += offset;
				parent->clip_to(r2);
			}
		}
	}

	virtual bool is_unclipped(const bounds& b) const
	{
		if (!!m_offScreenBuffer)
			return m_offScreenBuffer->is_unclipped(b);

		if (!!m_bridgedCanvas)
			return m_bridgedCanvas->is_unclipped(b);

		rcptr<pane> parent = get_parent();
		if (!!parent)
		{
			bounds r2 = b;
			r2 += get_position();
			return parent->is_unclipped(r2);
		}
		return true;
	}

	// notification interfaces (called internally, overriden)

	// pane_base interface - notifications

	virtual void installing()
	{
		m_installed = true;
		COGS_ASSERT(!m_childInstallItor);
		m_childInstallItor = m_children.get_first();
		if (!m_childInstallItor)
			install_done();
		else
		{
			COGS_ASSERT(!((*m_childInstallItor)->m_parentInstalling));
			(*m_childInstallItor)->m_parentInstalling = this_rcref; // extends reference through installation of child pane
			(*m_childInstallItor)->m_parentUISubSystem = get_subsystem();
			(*m_childInstallItor)->dispatch([r{ m_childInstallItor->dereference() }]()
			{
				COGS_ASSERT(!r->m_installTask);
				rcref<task<void> > installTask = (r->m_installTask = rcnew(signallable_task<void>)).dereference();
				r->install_inner2();
				return installTask;
			});
		}
	}

	// derived uninstalling() must call pane::uninstalling() before returning.
	virtual void uninstalling()
	{
		m_offScreenBuffer.release();
		m_installed = false;
		m_uninstalling = false;
		uninstall_done();
	}

	virtual bool character_typing(wchar_t c, const ui::modifier_keys_state& modifiers)
	{
		if (m_characterTypeHandlers(*this, c, modifiers))
			return true;
		return (!!m_subFocus) && m_subFocus->character_typing(c, modifiers);
	}

	virtual bool key_pressing(wchar_t c, const ui::modifier_keys_state& modifiers)
	{
		if (m_keyPressHandlers(*this, c, modifiers))
			return true;
		return (!!m_subFocus) && m_subFocus->key_pressing(c, modifiers);
	}

	virtual bool key_releasing(wchar_t c, const ui::modifier_keys_state& modifiers)
	{
		if (m_keyReleaseHandlers(*this, c, modifiers))
			return true;
		return (!!m_subFocus) && m_subFocus->key_releasing(c, modifiers);
	}

	virtual void modifier_keys_changing(const ui::modifier_keys_state& modifiers)
	{
		m_modifierKeysChangeEvent(this_rcref, modifiers);
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			(*itor)->modifier_keys_changing(modifiers);
			++itor;
		}
	}

	virtual bool button_pressing(mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		if (m_buttonPressHandlers(*this, btn, pt, modifiers))
			return true;
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			point newPt = pt;
			newPt -= (*itor)->get_position().to_size();
			if ((*itor)->get_size().contains(newPt))
			{
				if ((*itor)->button_pressing(btn, newPt, modifiers))
					return true;
			}
			++itor;
		}
		return false;
	}

	virtual bool button_releasing(mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		if (m_buttonReleaseHandlers(*this, btn, pt, modifiers))
			return true;
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			point newPt = pt;
			newPt -= (*itor)->get_position().to_size();
			if ((*itor)->get_size().contains(newPt))
			{
				if ((*itor)->button_releasing(btn, newPt, modifiers))
					return true;
			}
			++itor;
		}
		return false;
	}

	virtual bool button_double_clicking(mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		if (m_buttonDoubleClickHandlers(*this, btn, pt, modifiers))
			return true;
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			point newPt = pt;
			newPt -= (*itor)->get_position().to_size();
			if ((*itor)->get_size().contains(newPt))
			{
				if ((*itor)->button_double_clicking(btn, newPt, modifiers))
					return true;
			}
			++itor;
		}
		return false;
	}

	virtual bool wheel_moving(double distance, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		if (m_wheelMoveHandlers(*this, distance, pt, modifiers))
			return true;
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			point newPt = pt;
			newPt -= (*itor)->get_position().to_size();
			if ((*itor)->get_size().contains(newPt))
			{
				if ((*itor)->wheel_moving(distance, newPt, modifiers))
					return true;
			}
			++itor;
		}
		return false;
	}

	virtual void cursor_entering(const point& pt)
	{
		m_cursorWasWithin = true;
		m_cursorEnterEvent(this_rcref, pt);
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			point newPt = pt;
			newPt -= (*itor)->get_position().to_size();
			if ((*itor)->get_size().contains(newPt))
				(*itor)->cursor_entering(newPt);
			++itor;
		}
	}

	virtual void cursor_moving(const point& pt)
	{
		m_cursorMoveEvent(this_rcref, pt);
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			point newPt = pt;
			newPt -= (*itor)->get_position().to_size();
			if ((*itor)->get_size().contains(newPt))
			{
				if ((*itor)->m_cursorWasWithin)
					(*itor)->cursor_moving(newPt);
				else
					(*itor)->cursor_entering(newPt);
			}
			else
			{
				if ((*itor)->m_cursorWasWithin)
					(*itor)->cursor_leaving();
			}
			++itor;
		}
	}

	virtual void cursor_leaving()
	{
		m_cursorLeaveEvent(this_rcref);
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if ((*itor)->m_cursorWasWithin)
				(*itor)->cursor_leaving();
			++itor;
		}
		m_cursorWasWithin = false;
	}

	virtual void hiding()
	{
		m_hideEvent(this_rcref);
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if (!(*itor)->is_hidden())
				(*itor)->hiding();
			++itor;
		}
	}

	virtual void showing()
	{
		m_showEvent(this_rcref);
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if (!(*itor)->is_hidden())
				(*itor)->showing();
			++itor;
		}
	}

	virtual void drawing()
	{
	}

	// Override focusing()/defocusing() to customize on-focus behavior.
	// Calling the base pane::focusing() or pane::defocusing() will cause
	// focus propogation to continue.
	virtual void focusing(int direction)
	{
		m_focusEvent(this_rcref, direction);
		focus_children(direction);
	}

	virtual void defocusing()
	{
		m_defocusEvent(this_rcref);
	}

	// pane interface - notifications

	// This is invoked only for externally drawn panes, to trigger their own invalidation.
	virtual void invalidating(const bounds&) { }

	// A parent pane that keeps additional information about nested panes may need to override detaching_child(),
	// to refresh that data when a pane is removed.
	virtual void detaching_child(const rcref<pane>& child)
	{
		rcptr<pane> parent = get_ancestor_render_pane();
		if (child->m_installed)
		{
			child->m_detaching = true;
			child->uninstall();
		}
	}

	virtual void reshape(const bounds& newBounds, const point& oldOrigin = point(0, 0))
	{
		cell::reshape(newBounds, oldOrigin);
		bounds newVisibleBounds = get_size();
		rcptr<pane> p = get_ancestor_render_pane(newVisibleBounds);
		if (!m_initialReshapeDone)
			m_initialReshapeDone = true;
		else
		{
			// trim m_lastVisibleBounds to new rander pane bounds
			if (!p)
			{
				p = this;
				m_lastVisibleBounds &= get_size();
			}
			else
			{
				m_lastVisibleBounds &= p->get_size();
				// Invalidate visibly vacated portions of this pane, in the rendering anscestor pane.
				p->m_needsDraw = true;
				auto invalidBounds = m_lastVisibleBounds - newVisibleBounds;
				for (size_t i = 0; i < invalidBounds.second; i++)
				{
					bounds& b = invalidBounds.first[i];
					if (!!b)
					{
						p->invalidating(b);
						p->invalidating_up(b);
					}
				}
			}
			if (!!newVisibleBounds)
			{
				if (m_invalidateOnReshape)
				{
					// Invalidate entire pane.
					p->invalidating(newVisibleBounds);
					p->invalidating_up(newVisibleBounds);
				}
				else
				{
					// Invalidate visibly added portions of this pane.
					p->m_needsDraw = true;
					auto invalidBounds = newVisibleBounds - m_lastVisibleBounds;
					for (size_t i = 0; i < invalidBounds.second; i++)
					{
						bounds& b = invalidBounds.first[i];
						if (!!b)
						{
							p->invalidating(b);
							p->invalidating_up(b);
						}
					}
				}
			}
		}
		m_lastVisibleBounds = newVisibleBounds;

		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			(*itor)->reshape_frame(newBounds.get_size(), oldOrigin);
			++itor;
		}
	}

	virtual rcref<bitmap> create_offscreen_buffer(pane& forPane, const size& sz, std::optional<color> fillColor = std::nullopt)
	{
		rcptr<pane> parent = get_ancestor_render_pane();
		if (!parent)
			return forPane.create_bitmap(sz, fillColor);
		return parent->create_offscreen_buffer(forPane, sz, fillColor);
	}

	virtual void set_initial_shape()
	{
		std::optional<size> defaultSizeOpt = get_frame_default_size();
		size defaultSize = defaultSizeOpt.has_value() ? *defaultSizeOpt : size(0, 0);
		reshape_frame(defaultSize);
	}

	virtual gfx::font_parameters get_default_text_font() const
	{
		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Window should be installed.  Should not be called if not installed.
		return parent->get_default_text_font();
	}

	virtual color get_default_text_foreground_color() const
	{
		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Window should be installed.  Should not be called if not installed.
		return parent->get_default_text_foreground_color();
	}

	virtual color get_default_text_background_color() const
	{
		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Window should be installed.  Should not be called if not installed.
		return parent->get_default_text_background_color();
	}

	virtual color get_default_selected_text_foreground_color() const
	{
		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Window should be installed.  Should not be called if not installed.
		return parent->get_default_selected_text_foreground_color();
	}

	virtual color get_default_selected_text_background_color() const
	{
		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Window should be installed.  Should not be called if not installed.
		return parent->get_default_selected_text_background_color();
	}

	virtual color get_default_label_foreground_color() const
	{
		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Window should be installed.  Should not be called if not installed.
		return parent->get_default_label_foreground_color();
	}

	virtual color get_default_background_color() const
	{
		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Window should be installed.  Should not be called if not installed.
		return parent->get_default_background_color();
	}

private:
	void set_subsystem_inner(const rcptr<volatile gui::subsystem>& subSystem)
	{
		if (!m_installed)
			m_uiSubSystem = subSystem;
		else
		{
			uninstall();
			install(subSystem);
		}
		self_release();
	}

	void install_inner2()
	{
		self_acquire();
		m_installing = true;
		m_initialReshapeDone = false;
		installing();
	}

	void install_done()
	{
		m_installing = false;
		for (;;)
		{
			bool hasInstallingParent = !!m_parentInstalling;
			if (!hasInstallingParent)
				calculate_range();
			else
			{
				rcptr<pane> parentInstalling = m_parentInstalling;
				m_parentInstalling.release();

				pane_list::iterator itor = ++(parentInstalling->m_childInstallItor);
				if (!!itor)
				{
					(*itor)->m_parentInstalling = parentInstalling;
					rcptr<volatile subsystem> tmp = m_parentUISubSystem;
					(*itor)->m_parentUISubSystem = tmp;
					(*itor)->dispatch([r{ itor->dereference() }]()
					{
						COGS_ASSERT(!r->m_installTask);
						rcref<task<void> > installTask = (r->m_installTask = rcnew(signallable_task<void>)).dereference();
						r->install_inner2();
						return installTask;
					});
					break;
				}
				parentInstalling->install_done();
				break;
			}

			rcptr<pane> p = m_parent;
			if (!!p)
				p->recompose();
			else
				set_initial_shape();

			if (is_visible())
				showing();
			break;
		}

		rcptr<signallable_task<void> > installTask = m_installTask;
		m_installTask.release();
		COGS_ASSERT(!!installTask);
		installTask->signal();
		self_release();
	}

	void uninstall_inner()
	{
		m_topmostUninstall = true;
		uninstall_inner2();
	}

	void uninstall_inner2()
	{
		self_acquire();
		m_uninstalling = true;

		// Uninstall in the reverse order we installed in.  So, last child to first child, then ourselves.
		m_childInstallItor = m_children.get_last();
		if (!m_childInstallItor)
			uninstalling();
		else
		{
			rcref<pane>& child = *m_childInstallItor;
			child->m_parentInstalling = this_rcref;
			child->dispatch([r{ child.dereference() }]()
			{
				COGS_ASSERT(!r->m_installTask);
				rcref<task<void> > installTask = (r->m_installTask = rcnew(signallable_task<void>)).dereference();
				r->uninstall_inner2();
				return installTask;
			});
		}
	}

	void uninstall_done()
	{
		for (;;)
		{
			pane_list::iterator itor;
			rcptr<pane> parentInstalling = m_parentInstalling;
			bool hasUninstallingParent = !!parentInstalling;
			if (!!hasUninstallingParent)
			{
				m_parentInstalling.release();

				itor = --(parentInstalling->m_childInstallItor);
			}

			if (m_detaching)
			{
				m_parent->m_children.remove(m_siblingIterator);
				m_detaching = false;
			}
			m_bridgedCanvas.release();

			if (!!hasUninstallingParent)
			{
				if (!!itor)
				{
					(*itor)->m_parentInstalling = parentInstalling;
					(*itor)->dispatch([r{ itor->dereference() }]()
					{
						COGS_ASSERT(!r->m_installTask);
						rcref<task<void> > installTask = (r->m_installTask = rcnew(signallable_task<void>)).dereference();
						r->uninstall_inner2();
						return installTask;
					});
					break;
				}
				parentInstalling->uninstall_done();
				break;
			}
			if (m_topmostUninstall)
			{
				m_topmostUninstall = false;
				rcptr<pane> p = m_parent;
				if (!!p)
					p->recompose();
			}
			break;
		}

		m_parentUISubSystem.release();
		rcptr<signallable_task<void> > installTask = m_installTask;
		m_installTask.release();
		COGS_ASSERT(!!installTask);
		installTask->signal();
		self_release();
	}

	void invalidating_up(const bounds& b)
	{
		if (!is_externally_drawn())
		{
			bounds b2 = b;
			rcptr<pane> p = get_ancestor_render_pane(b2);
			COGS_ASSERT(!!p); // Topmost pane should be an externally drawn window.
			if (!!b2)
			{
				p->m_needsDraw = true;
				p->invalidating(b2);
				p->invalidating_up(b2);
			}
		}
	}

	void draw_children()
	{
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if (!(*itor)->is_externally_drawn())
				(*itor)->draw();
			++itor;
		}
	}

	void clip_opaque_external_descendants(pane& fromPane, const point& offset, const bounds& visibleBounds)
	{
		pane_list::iterator itor = fromPane.m_children.get_first();
		while (!!itor)
		{
			pane& p = **itor;
			bounds b = p.get_bounds() + offset;
			bounds newVisibleBounds = visibleBounds & b;
			if (!!newVisibleBounds)
			{
				if (p.is_externally_drawn())
					clip_out(newVisibleBounds);
				else
					clip_opaque_descendants(**itor, b.get_position(), newVisibleBounds);
			}
			++itor;
		}
	}

	void clip_opaque_descendants(pane& fromPane, const point& offset, const bounds& visibleBounds)
	{
		pane_list::iterator itor = fromPane.m_children.get_first();
		while (!!itor)
		{
			pane& p = **itor;
			bounds b = p.get_bounds() + offset;
			bounds newVisibleBounds = visibleBounds & b;
			if (!!newVisibleBounds)
			{
				if (p.is_opaque())
					clip_out(newVisibleBounds);
				else
					clip_opaque_descendants(**itor, b.get_position(), newVisibleBounds);
			}
			++itor;
		}
	}

	void clip_opaque_after(pane& fromPane, const point& offset, const bounds& visibleBounds)
	{
		COGS_ASSERT(!fromPane.is_externally_drawn());
		rcptr<pane> curPane = &fromPane;
		point parentOffset = offset;
		for (;;)
		{
			pane_list::iterator itor = curPane->m_siblingIterator;
			parentOffset += -curPane->get_position();
			while (!!++itor)
			{
				pane& p = **itor;
				bounds b = p.get_bounds() + parentOffset;
				bounds newVisibleBounds = visibleBounds & b;
				if (!!newVisibleBounds)
				{
					if (p.is_opaque())
						clip_out(newVisibleBounds);
					else
						clip_opaque_descendants(**itor, b.get_position(), newVisibleBounds);
				}
			}
			curPane = curPane->m_parent;
			if (!curPane || curPane->is_externally_drawn() || curPane->m_compositingBehavior == compositing_behavior::buffer_self_and_children)
				break;
		}
	}

	void clip_opaque_externals_after(pane& fromPane, const point& offset, const bounds& visibleBounds)
	{
		COGS_ASSERT(!fromPane.is_externally_drawn());
		rcptr<pane> curPane = &fromPane;
		point parentOffset = offset;
		for (;;)
		{
			pane_list::iterator itor = curPane->m_siblingIterator;
			parentOffset += -curPane->get_position();
			while (!!++itor)
			{
				pane& p = **itor;
				bounds b = p.get_bounds() + parentOffset;
				bounds newVisibleBounds = visibleBounds & b;
				if (!!newVisibleBounds)
				{
					if (p.is_externally_drawn())
						clip_out(newVisibleBounds);
					else
						clip_opaque_descendants(**itor, b.get_position(), newVisibleBounds);
				}
			}
			curPane = curPane->m_parent;
			if (!curPane || curPane->is_externally_drawn() || curPane->m_compositingBehavior == compositing_behavior::buffer_self_and_children)
				break;
		}
	}

	bool is_focusable(const pane* skipChild) const // override to indicate capable of receiving focus
	{
		if (this == skipChild)
			return false;

		if (m_isFocusable)
			return true;

		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		while (!!itor)
		{
			if ((*itor)->is_focusable(skipChild))
				return true;
			++itor;
		}
		return false;
	}

	void focus_children(int direction)
	{
		if ((direction == 0) && (!!m_subFocus)) // restore previously focused sub-frame
			m_subFocus->focus(0);
		else
		{
			bool iterateDirection = (direction >= 0);
			pane_list::iterator itor;
			if (iterateDirection)
				itor = m_children.get_first();
			else
				itor = m_children.get_last();
			while (!!itor)
			{
				if ((*itor)->is_focusable())
				{
					m_subFocus = (*itor);
					m_subFocus->m_hasFocus = true;
					m_subFocus->focusing(direction);
					break;
				}
				if (iterateDirection)
					++itor;
				else
					--itor;
			}
		}
	}

	void focus_parent(int direction)
	{
		rcptr<pane> parent = m_parent;
		if (!!parent)
		{
			if (!parent->m_hasFocus)
			{
				parent->focus_parent(direction);
				parent->m_hasFocus = true;
				parent->focusing(direction);
			}
			else if ((m_subFocus != this) && (!!m_subFocus))
				m_subFocus->defocus();
			parent->m_subFocus = this_rcref;
		}
	}

	void prepare_offscreen_buffer()
	{
		if (m_compositingBehavior != compositing_behavior::no_buffer)
		{
			size sz = get_size();
			if (!!m_offScreenBuffer)
				m_offScreenBuffer->set_size(sz, size(100, 100), false);
			else
			{
				m_offScreenBuffer = create_offscreen_buffer(*this, sz, is_opaque() ? color::constant::black : color::constant::transparent);
				m_needsDraw = true;
				invalidating(sz);
			}
		}
	}

	template <typename F>
	void for_each_child(F&& f)
	{
		pane_list::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if (!f(*itor)) // return false to stop processing
				break;
			++itor;
		}
	}

	template <typename F>
	void reverse_for_each_child(F&& f)
	{
		pane_list::iterator itor = m_children.get_last();
		while (!!itor)
		{
			if (!f(*itor)) // return false to stop processing
				break;
			--itor;
		}
	}

protected:
	virtual gfx::canvas& get_canvas() { return *(gfx::canvas*)this; }
	virtual const gfx::canvas& get_canvas() const { return *(const gfx::canvas*)this; }

	virtual rcref<gfx::canvas> get_canvas_ref() { return get_self_rcref((gfx::canvas*)this); }
	virtual rcref<const gfx::canvas> get_canvas_ref() const { return get_self_rcref((const gfx::canvas*)this); }

	virtual pane_container& get_pane_container() { return *(pane_container*)this; }
	virtual const pane_container& get_pane_container() const { return *(pane_container*)this; }

	virtual rcref<pane_container> get_pane_container_ref() { return get_self_rcref((pane_container*)this); }
	virtual rcref<const pane_container> get_pane_container_ref() const { return get_self_rcref((pane_container*)this); }

	virtual cell& get_pane_cell() { return *(cell*)this; }
	virtual const cell& get_pane_cell() const { return *(cell*)this; }

	virtual rcref<cell> get_pane_cell_ref() { return get_self_rcref((cell*)this); }
	virtual rcref<const cell> get_pane_cell_ref() const { return get_self_rcref((cell*)this); }

	friend class frame;

public:
	virtual void insert_before_frame(const rcref<frame>& f, const rcref<frame>& beforeThis)
	{
		frameable::insert_before_frame(f, beforeThis);
		if (m_installed && !m_uninstalling)
			recompose();
	}

	virtual void insert_after_frame(const rcref<frame>& f, const rcref<frame>& afterThis)
	{
		frameable::insert_after_frame(f, afterThis);
		if (m_installed && !m_uninstalling)
			recompose();
	}

	virtual void append_frame(const rcref<frame>& f)
	{
		frameable::append_frame(f);
		if (m_installed && !m_uninstalling)
			recompose();
	}

	virtual void prepend_frame(const rcref<frame>& f)
	{
		frameable::prepend_frame(f);
		if (m_installed && !m_uninstalling)
			recompose();
	}

	virtual void remove_frame(frame& f)
	{
		frameable::remove_frame(f);
		if (m_installed && !m_uninstalling)
			recompose();
	}

	rcref<pane> get_top_pane()
	{
		rcptr<pane> p = this_rcptr;
		while (true)
		{
			rcptr<pane> parent = p->get_parent();
			if (!parent)
				break;
			p = std::move(parent);
		}
		return p.dereference();
	}

	rcref<pane> get_top_pane(point& offset)
	{
		rcptr<pane> p = this_rcptr;
		while (true)
		{
			rcptr<pane> parent = p->get_parent();
			if (!parent)
				break;
			offset += p->get_position();
			p = std::move(parent);
		}
		return p.dereference();
	}

	void detach()
	{
		m_detachEvent(this_rcref);
		rcptr<pane> p = get_parent();
		if (!!p)
		{
			if (is_visible())
				hiding();
			p->detaching_child(this_rcref);
			p->recompose();
		}
		else if (!!m_installed)
		{
			if (is_visible())
				hiding();
			uninstall();
		}
	}

	// is_hidden() and is_visible() are NOT inverses of eachother, they are distinct.
	//
	// is_hidden() will indicate true, if this specific frame has been hidden.
	// is_hidden() will indicate false, if this specific frame has not been hidden,
	// even if it is not visible due to a parent pane being hidden.
	//
	// is_visible() will indicate true, if not hidden and all parent panes are not hidden.
	// is_visible() will indicate false, if not visible due to being hidden or a parent pane being hidden.
	bool is_hidden() const { return m_hideShowState < 0; }
	bool is_visible() const
	{
		if (is_hidden())
			return false;
		rcptr<pane> parent = get_parent();
		if (!!parent)
			return parent->is_visible();
		return true;
	}

	virtual void hide()
	{
		if (m_hideShowState-- == 0) // if it is becoming hidden
		{
			rcptr<pane> parent = get_parent();
			if (!parent)
				hiding();
			else if (parent->is_visible())
			{
				hiding();
				parent->invalidate(get_bounds());
			}
		}
	}

	virtual void show()
	{
		COGS_ASSERT(m_hideShowState != 0); // It's an error to show something already visible.  Counting is only for hiding.
		if (++m_hideShowState == 0) // if it is becoming visible
		{
			m_closeCondition.reset();
			rcptr<pane> parent = get_parent();
			if (!parent)
				showing();
			else if (parent->is_visible())
			{
				showing();
				invalidate(get_size());
			}
		}
	}

	void show_or_hide(bool showIt)
	{
		if (showIt)
			show();
		else
			hide();
	}

	virtual size get_size() const { return cell::get_size(); }

	bounds get_bounds() const
	{
		bounds b(get_position(), get_size());
		return b;
	}

	range get_range() const { return m_range; }
	std::optional<size> get_default_size() const { return m_defaultSize; }

	virtual dimension get_primary_flow_dimension() const
	{
		rcptr<pane> parent = m_parent;
		if (!!parent)
			return parent->get_primary_flow_dimension();
		return cell::get_primary_flow_dimension();
	}

	virtual collaborative_sizes calculate_collaborative_sizes(
		const size& sz,
		const range& r = range::make_unbounded(),
		const std::optional<quadrant_mask>& quadrants = std::nullopt,
		const std::optional<dimension>& resizeDimension = std::nullopt) const
	{
		if (m_children.is_empty())
			return cell::calculate_collaborative_sizes(sz, r, quadrants, resizeDimension);

		collaborative_sizes result;
		range r2 = r & get_range();
		if (!r2.is_invalid())
		{
			// Since the default behavior is to constrain the parent,
			// we need to give children a chance to consider the proposed size.
			if (m_children.size() == 1)
				return (*m_children.get_first())->calculate_collaborative_frame_sizes(sz, r2, quadrants, resizeDimension);

			size sz2 = r2.get_limit(sz);
			bool sizeChanged = sz2 != sz;

			struct state
			{
				quadrant_mask mode;
				size sz;
				pane* paneToSkip;
			};

			quadrant_mask initialMask = quadrants.has_value() ? *quadrants : all_quadrants;
			backed_vector<state, 4> states(1, { initialMask, sz2, nullptr });
			for (size_t stateIndex = 0; stateIndex < states.get_length(); stateIndex++)
			{
				state& currentState = states.get_ptr()[stateIndex];
				pane_list::iterator itor = m_children.get_first();
				collaborative_sizes result2;
				while (!!itor)
				{
					pane* p = itor->get_ptr();
					if (p == currentState.paneToSkip)
					{
						++itor;
						continue;
					}
					result2 = (*itor)->calculate_collaborative_frame_sizes(currentState.sz, r2, currentState.mode, resizeDimension);
					quadrant_mask differentSizeMask = quadrant_mask::none;
					for (auto sizingType : currentState.mode)
					{
						auto bit_index = get_flag_index(sizingType);
						if (!result2.indexed_sizes[bit_index].has_value()) // null is a dead end, remove.
						{
							// It's ok to modify prior or current entries when iterating a flag_mask.
							currentState.mode -= sizingType;
							continue;
						}
						if (*result2.indexed_sizes[bit_index] == currentState.sz)
							continue;
						differentSizeMask += sizingType;
					}
					if (currentState.mode == quadrant_mask::none)
						break;
					if (differentSizeMask == quadrant_mask::none) // If all were accepted
					{
						++itor;
						continue;
					}
					size_t startScanStateIndex = stateIndex;
					if (differentSizeMask == currentState.mode) // None were accepted
					{
						states.erase(stateIndex);
						itor = m_children.get_first();
					}
					else // If at least one was accepted
					{
						currentState.paneToSkip = p;
						startScanStateIndex++;
						++itor;
					}
					for (auto differentSizeType : differentSizeMask)
					{
						auto bit_index = get_flag_index(differentSizeType);
						size& differentSize = *result2.indexed_sizes[bit_index];
						// Check if already present in the vector.
						bool found = false;
						for (size_t i = startScanStateIndex; i < states.get_length(); i++)
						{
							state& cmpState = states.get_ptr()[i];
							if (cmpState.sz == differentSize)
							{
								cmpState.mode += differentSizeType;
								found = true;
								break;
							}
						}
						if (!found)
							states.append(1, { *differentSizeType, differentSize, p });
					}
					//continue;
				}
				// Add to result.
				for (auto sizingType : currentState.mode)
				{
					auto bit_index = get_flag_index(sizingType);
					result.indexed_sizes[bit_index] = *result2.indexed_sizes[bit_index];
				}
			}
			if (sizeChanged)
				result.update_relative_to(sz, get_primary_flow_dimension(), resizeDimension);
		}
		return result;
	}

	const weak_rcptr<pane>& get_parent() const { return m_parent; }
	const pane_list& get_children() const { return m_children; }
	const pane_list::remove_token& get_sibling_iterator() const { return m_siblingIterator; }

	virtual bool is_opaque() const { return false; }

	bool is_installed() const { return m_installed; }
	bool is_installing() const { return m_installing; }
	bool is_uninstalling() const { return m_uninstalling; }

	// is_drawing_needed() is only valid for a pane with a backing buffer.
	// Calls to invalidate() will set this to true.  When false, the backing buffer can be used and drawing() will not be called.
	bool is_drawing_needed() const { return m_needsDraw; }

	// If a pane is 'externally' drawn, that implies it's rendered by something
	// other than the drawing of it's immediate parent, such as an OS callback.
	// All Windows HWND and MacOS NSViews are externally drawn.
	bool is_externally_drawn() const
	{
		return !!m_bridgedCanvas;
	}

	friend class pane_orchestrator;
};


// By default a canvas_pane will invalidate whenever reshaped, allowing it to be repainted at the new size.
// A container_pane does not invalidate on reshape, allowing contained panes to manage own invalidation.

class container_pane : public pane, public virtual pane_container
{
public:
	typedef pane::options options;

	container_pane()
		: container_pane(options())
	{ }

	explicit container_pane(options&& o)
		: pane(std::move(o))
	{ }

	using pane_container::nest;
	virtual void nest_last(const rcref<pane>& child) { pane::nest_last(child); }
	virtual void nest_first(const rcref<pane>& child) { pane::nest_first(child); }
	virtual void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child) { pane::nest_before(beforeThis, child); }
	virtual void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child) { pane::nest_after(afterThis, child); }
};


class canvas_pane : public pane, public virtual pane_container, public virtual gfx::canvas
{
public:
	typedef event<const rcref<canvas_pane>&> draw_event_t;

private:
	draw_event_t m_drawEvent;

protected:
	virtual void drawing()
	{
		m_drawEvent(this_rcref);
	}

public:
	struct options
	{
		bool invalidateOnReshape = true;
		draw_event_t drawEvent;
		compositing_behavior compositingBehavior = compositing_behavior::no_buffer;
		frame_list frames;
		pane_list children;
	};

	canvas_pane()
		: canvas_pane(options())
	{ }

	explicit canvas_pane(options&& o)
		: pane({
			.invalidateOnReshape = o.invalidateOnReshape,
			.compositingBehavior = o.compositingBehavior,
			.frames = std::move(o.frames),
			.children = std::move(o.children),
		}),
		m_drawEvent(std::move(o.drawEvent))
	{ }

	virtual void fill(const bounds& b, const color& c = color::constant::black, bool blendAlpha = true) { pane::fill(b, c, blendAlpha); }
	virtual void invert(const bounds& b) { pane::invert(b); }
	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::constant::black, bool blendAlpha = true) { pane::draw_line(startPt, endPt, width, c, blendAlpha); }
	virtual rcref<font> load_font(const gfx::font_parameters_list& f = gfx::font_parameters_list()) { return pane::load_font(f); }
	virtual string get_default_font_name() const { return pane::get_default_font_name(); }
	virtual void draw_text(const composite_string& s, const bounds& b, const rcptr<font>& f = 0, const color& c = color::constant::black) { pane::draw_text(s, b, f, c); }
	virtual void draw_bitmap(const bitmap& src, const bounds& srcBounds, const bounds& dstBounds, bool blendAlpha = true) { return pane::draw_bitmap(src, srcBounds, dstBounds, blendAlpha); }
	virtual void draw_bitmask(const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, const color& fore = color::constant::black, const color& back = color::constant::white, bool blendForeAlpha = true, bool blendBackAlpha = true) { pane::draw_bitmask(msk, mskBounds, dstBounds, fore, back, blendForeAlpha, blendBackAlpha); }
	//virtual void composite_vector_image(const vector_image& src, const bounds& dstBounds) { pane::composite_vector_image(src, dstBounds); }
	virtual void mask_out(const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool inverted = false) { pane::mask_out(msk, mskBounds, dstBounds, inverted); }
	virtual void draw_bitmap_with_bitmask(const bitmap& src, const bounds& srcBounds, const bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, bool blendAlpha = true, bool inverted = false) { pane::draw_bitmap_with_bitmask(src, srcBounds, msk, mskBounds, dstBounds, blendAlpha, inverted); }
	virtual rcref<bitmap> create_bitmap(const size& sz, std::optional<color> fillColor = std::nullopt)
	{
		return pane::create_bitmap(sz, fillColor);
	}
	virtual rcref<bitmap> load_bitmap(const composite_string& location)
	{
		return pane::load_bitmap(location);
	}
	virtual rcref<bitmask> create_bitmask(const size& sz, std::optional<bool> value = std::nullopt)
	{
		return pane::create_bitmask(sz, value);
	}
	virtual rcref<bitmask> load_bitmask(const composite_string& location)
	{
		return pane::load_bitmask(location);
	}
	//virtual rcptr<vector_image> load_vector_image(const composite_string& location) { return pane::load_vector_image(location); }
	virtual void save_clip() { pane::save_clip(); }
	virtual void restore_clip() { pane::restore_clip(); }
	virtual void clip_out(const bounds& b) { pane::clip_out(b); }
	virtual void clip_to(const bounds& b) { pane::clip_to(b); }
	virtual bool is_unclipped(const bounds& b) const { return pane::is_unclipped(b); }

	using pane_container::nest;
	virtual void nest_last(const rcref<pane>& child) { pane::nest_last(child); }
	virtual void nest_first(const rcref<pane>& child) { pane::nest_first(child); }
	virtual void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child) { pane::nest_before(beforeThis, child); }
	virtual void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child) { pane::nest_after(afterThis, child); }

	virtual gfx::font_parameters get_default_text_font() const { return pane::get_default_text_font(); }
	virtual color get_default_text_foreground_color() const { return pane::get_default_text_foreground_color(); }
	virtual color get_default_text_background_color() const { return pane::get_default_text_background_color(); }
	virtual color get_default_selected_text_foreground_color() const { return pane::get_default_selected_text_foreground_color(); }
	virtual color get_default_selected_text_background_color() const { return pane::get_default_selected_text_background_color(); }
	virtual color get_default_label_foreground_color() const { return pane::get_default_label_foreground_color(); }
	virtual color get_default_background_color() const { return pane::get_default_background_color(); }
};


class pane_orchestrator
{
protected:
	static void cursor_enter(pane& p, const point& pt)
	{
		p.cursor_entering(pt);
	}

	static void cursor_move(pane& p, const point& pt)
	{
		p.cursor_moving(pt);
	}

	static void cursor_leave(pane& p)
	{
		p.cursor_leaving();
	}

	static bool character_type(pane& p, wchar_t c, const ui::modifier_keys_state& modifiers)
	{
		return p.character_typing(c, modifiers);
	}

	static bool key_press(pane& p, wchar_t c, const ui::modifier_keys_state& modifiers)
	{
		return p.key_pressing(c, modifiers);
	}

	static bool key_release(pane& p, wchar_t c, const ui::modifier_keys_state& modifiers)
	{
		return p.key_releasing(c, modifiers);
	}

	static void modifier_keys_change(pane& p, const ui::modifier_keys_state& modifiers)
	{
		p.modifier_keys_changing(modifiers);
	}

	static bool button_press(pane& p, mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		return p.button_pressing(btn, pt, modifiers);
	}

	static bool button_release(pane& p, mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		return p.button_releasing(btn, pt, modifiers);
	}

	static bool button_double_click(pane& p, mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		return p.button_double_clicking(btn, pt, modifiers);
	}

	static bool wheel_move(pane& p, double distance, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		return p.wheel_moving(distance, pt, modifiers);
	}

	static void invalidate(pane& p, const bounds& sz)
	{
		p.invalidate(sz);
	}

	static bool request_close(pane& p)
	{
		return p.requesting_close();
	}

	static void close(pane& p)
	{
		p.close();
	}

	static void focus(pane& p, int direction = 0)
	{
		p.focus(direction);
	}
};


inline void gui::subsystem::install(pane& p, const rcptr<volatile gui::subsystem>& subSystem)
{
	p.install(subSystem);
}

}
}


#include "cogs/gui/pane_bridge.hpp"


#endif
