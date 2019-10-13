//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, MayNeedCleanup

#ifndef COGS_HEADER_GUI_PANE
#define COGS_HEADER_GUI_PANE


#include "cogs/env.hpp"
#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/function.hpp"
#include "cogs/gfx/color.hpp"
#include "cogs/gui/frame.hpp"
#include "cogs/gui/mouse.hpp"
#include "cogs/gui/subsystem.hpp"
#include "cogs/math/boolean.hpp"
#include "cogs/math/const_min_int.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/resettable_event.hpp"
#include "cogs/sync/priority_queue.hpp"
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


/// @ingroup GUI
/// @brief The compositing behavior of a pane
///
/// A pane's compositing_behavior indicates whether or not it should use a backing buffer, and under
/// what conditions that backing buffer needs to be re-rendered.
///
/// The top-most pane implicitly buffers the drawing of itself and all children, to avoid flicker.
/// This is either supported automatically by the platform, rendering engine, or use of a compositing
/// buffer in the top-most pane.
enum compositing_behavior
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


class pane_container
{
public:
	void nest(const rcref<pane>& child, const rcptr<frame>& f = 0)
	{ return nest_last(child, f); }

	virtual void nest_last(const rcref<pane>& child, const rcptr<frame>& f = 0) = 0;
	virtual void nest_first(const rcref<pane>& child, const rcptr<frame>& f = 0) = 0;
	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis, const rcptr<frame>& f = 0) = 0;
	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis, const rcptr<frame>& f = 0) = 0;
};


// The only thread-safe methods of a pane are dispatch() and recompose().
// Access to all other methods must be serialized.  (Which can be done by dispatching against the pane itself).


class bridgeable_pane;

/// @ingroup GUI
/// @brief Base class for 2D visual UI elements
class pane : public object, public dispatcher, public cell, protected virtual gfx::canvas, protected virtual pane_container
{
public:
	friend class bridgeable_pane;
	friend class gui::subsystem;

	using cell = gfx::canvas::cell;

private:
	weak_rcptr<pane> m_parent;
	container_dlist<rcref<pane> > m_children;
	container_dlist<rcref<pane> >::remove_token m_siblingIterator; // our element in our parent's list of children panes

	// Changes to the UI subsystem are serialized using the pane's dispatcher.

	volatile rcptr<volatile gui::subsystem> m_uiSubSystem;
	volatile rcptr<volatile gui::subsystem> m_parentUISubSystem;

	rcptr<frame> m_frame;
	rcptr<pane> m_subFocus;
	ptrdiff_t m_hideShowState = 0;

	rcptr<bitmap> m_offScreenBuffer;

	compositing_behavior m_compositingBehavior;

	rcptr<gfx::canvas> m_bridgedCanvas;
	resettable_event m_closeEvent;

	bool m_hasFocus = false;
	bool m_isFocusable = true;
	bool m_installed = false;
	bool m_installing = false;
	bool m_uninstalling = false;
	bool m_initialReshapeDone = false;
	bool m_cursorWasWithin = false;
	bool m_needsDraw = false;
	bool m_recomposeDescendants = true;

	bounds m_lastVisibleBounds;
	point m_lastRenderOffset;

	range m_currentRange;
	size m_currentDefaultSize = { 0, 0 };

	container_dlist<rcref<pane> >::iterator m_childInstallItor;
	rcptr<pane> m_parentInstalling;
	bool m_topmostUninstall = false;

	volatile boolean m_recomposing;
	volatile boolean m_detaching;

	class serial_dispatched;
	priority_queue<int, ptr<serial_dispatched> > m_priorityQueue;

	const function<void(const rcptr<volatile gui::subsystem>&)> m_setSubsystemDelegate;

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

	rcref<volatile dispatcher> get_next_dispatcher() const volatile
	{
		return ((const pane*)this)->get_next_dispatcher();
	}

	class serial_dispatched : public dispatched
	{
	public:
		priority_queue<int, ptr<serial_dispatched> >::remove_token m_removeToken;
		bool m_async;

		// m_removeToken is accessed in a thread-safe way.  Cast away volatility
		priority_queue<int, ptr<serial_dispatched> >::remove_token& get_remove_token() volatile { return ((serial_dispatched*)this)->m_removeToken; }
		const priority_queue<int, ptr<serial_dispatched> >::remove_token& get_remove_token() const volatile { return ((const serial_dispatched*)this)->m_removeToken; }

		serial_dispatched(const ptr<rc_obj_base>& desc, bool async, const rcref<volatile dispatcher>& parentDispatcher, const rcref<task_base>& t, const priority_queue<int, ptr<serial_dispatched> >::remove_token& rt)
			: dispatched(desc, parentDispatcher, t),
			m_removeToken(rt),
			m_async(async)
		{ }

		// const.  Cast away volatility
		bool get_async() const volatile { return ((const serial_dispatched*)this)->m_async; }
	};

	virtual rcref<task<bool> > cancel_inner(volatile dispatched& d) volatile
	{
		volatile serial_dispatched& d2 = *(volatile serial_dispatched*)&d;
		const priority_queue<int, ptr<serial_dispatched> >::remove_token& rt = d2.get_remove_token();
		bool b = m_priorityQueue.remove(rt);
		if (b)
			serial_dispatch();
		return get_immediate_task(b);
	}

	virtual void change_priority_inner(volatile dispatched& d, int newPriority) volatile
	{
		volatile serial_dispatched& d2 = *(volatile serial_dispatched*)&d;
		priority_queue<int, ptr<serial_dispatched> >::remove_token& rt = d2.get_remove_token();
		m_priorityQueue.change_priority(rt, newPriority);
		serial_dispatch();
	}

	virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
	{
		priority_queue<int, ptr<serial_dispatched> >::preallocated_t i;
		auto r = m_priorityQueue.preallocate_key_with_aux<delayed_construction<serial_dispatched> >(priority, i);
		serial_dispatched* d = &(r->get());
		i.get_value() = d;
		new (d) serial_dispatched(r.get_desc(), false, this_rcref, t, i);
		m_priorityQueue.insert_preallocated(i);
		rcref<dispatched> d2(d, i.get_desc());
		t->set_dispatched(d2);
		i.disown();
		serial_dispatch();
	}

	class serial_dispatch_state
	{
	public:
		int m_scheduledPriority;
		unsigned int m_flags;
	};

	alignas (atomic::get_alignment_v<serial_dispatch_state>) serial_dispatch_state m_serialDispatchState;

	rcptr<task<void> > m_expireTask;
	boolean m_expireDone;

	enum serial_dispatch_state_flags
	{
		busy_flag = 0x01,      // 00001
		dirty_flag = 0x02,     // 00010
		scheduled_flag = 0x04, // 00100
		expired_flag = 0x08,   // 01000
		hand_off_flag = 0x10,  // 10000
	};

	void serial_dispatch() volatile
	{
		serial_dispatch_state oldState;
		atomic::load(m_serialDispatchState, oldState);
		for (;;)
		{
			if ((oldState.m_flags & dirty_flag) != 0)
				break;

			serial_dispatch_state newState = oldState;
			bool own = (oldState.m_flags & busy_flag) == 0;
			if (own)
				newState.m_flags |= busy_flag;
			else
				newState.m_flags |= dirty_flag;

			if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
				continue;

			if (!own)
				break;

			get_next_dispatcher()->dispatch([r{ this_rcref }]()
			{
				r->serial_update();
			}, const_min_int_v<int>); // best possible priority
			break;
		}
	}

	void serial_expire() volatile
	{
		if (!m_expireDone.compare_exchange(true, false))
			return;

		m_expireTask.release();
		serial_dispatch_state oldState;
		atomic::load(m_serialDispatchState, oldState);
		for (;;)
		{
			COGS_ASSERT((oldState.m_flags & expired_flag) == 0);
			serial_dispatch_state newState = oldState;
			newState.m_flags &= ~scheduled_flag & ~hand_off_flag & ~dirty_flag; // remove scheduled flag, and hand_off_flag if it was present
			newState.m_flags |= busy_flag | expired_flag;
			bool own = ((oldState.m_flags & busy_flag) == 0) || ((oldState.m_flags & hand_off_flag) != 0);
			if (!own)
				newState.m_flags |= dirty_flag;

			if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
				continue;

			if (own)
				serial_update();
			break;
		}
	}

	void serial_update() volatile
	{
		serial_dispatch_state oldState;
		serial_dispatch_state newState;
		priority_queue<int, ptr<serial_dispatched> >::value_token vt;
		atomic::load(m_serialDispatchState, oldState);
		for (;;)
		{
			COGS_ASSERT((oldState.m_flags & hand_off_flag) == 0);
			if ((oldState.m_flags & dirty_flag) != 0) // Immediately remove the retry tripwire
			{
				newState = oldState;
				newState.m_flags &= ~dirty_flag;
				if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
					continue;
				oldState.m_flags = newState.m_flags;
			}

			vt = m_priorityQueue.peek();
			int priority = 0;
			bool removeScheduled = false;
			if (!vt)
			{
				if ((oldState.m_flags & scheduled_flag) == 0) // Nothing scheduled, done if we can transition out
				{
					newState = oldState;
					newState.m_flags &= ~busy_flag & ~expired_flag;
					if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
						continue;
					break;
				}

				removeScheduled = true;
			}
			else
			{
				priority = vt.get_key();
				if ((oldState.m_flags & scheduled_flag) != 0)
				{
					if (priority == oldState.m_scheduledPriority) // Something the same priority is already scheduled, done if we can transition out
					{
						newState = oldState;
						newState.m_flags &= ~busy_flag;
						if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
							continue;
						break;
					}
					removeScheduled = true;
				}
			}

			if (removeScheduled)
			{
				bool canceled = m_expireDone.compare_exchange(true, false); 
				if (canceled) // This is only thread that will schedule/unschedule
				{
					m_expireTask->cancel();
					m_expireTask.release();
					for (;;) // Nothing is scheduled now.  Remove scheduled bit foracbly.
					{
						newState = oldState;
						newState.m_flags &= ~scheduled_flag;
						newState.m_flags &= ~dirty_flag; // slight efficiency
						if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
							continue;
						oldState.m_flags = newState.m_flags;
						break;
					}
					continue; // Start from the beginning, in case other flags have changed
				}
				for (;;) // Try to release update to expiring thread, if it doesn't expire before we get the chance
				{
					newState = oldState;
					newState.m_flags |= hand_off_flag;
					newState.m_flags &= ~dirty_flag; // slight efficiency
					if (atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
						return;
					if (((oldState.m_flags & expired_flag) != 0)) // Already expired
						break;
					//continue;
				}
				continue; // Start from the beginning, in case other flags have changed
			}

			if ((oldState.m_flags & expired_flag) != 0)
			{
				if (priority <= oldState.m_scheduledPriority)
				{
					COGS_ASSERT(!!vt);
					if (!m_priorityQueue.remove(vt))
						continue;

					const ptr<serial_dispatched>& d = *vt;
					const rcref<task_base>& taskBase = d->get_task_base();
					if (!taskBase->signal())
						continue;

					if (d->get_async() == false)
						serial_resume();
					return;
				}

				for (;;) // Expired, but too soon.  Need to reschedule anyway.  Remove expired bit foracbly.
				{
					newState = oldState;
					newState.m_flags &= ~expired_flag;
					newState.m_flags &= ~dirty_flag; // slight efficiency
					if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
						continue;
					oldState.m_flags = newState.m_flags;
					break;
				}
				continue; // Start from the beginning, in case other flags have changed
			}

			COGS_ASSERT(!!vt);
			newState.m_scheduledPriority = priority;
			newState.m_flags = oldState.m_flags & ~busy_flag;
			newState.m_flags |= scheduled_flag;
			if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
				continue;

			((pane*)this)->m_expireDone = false; // don't need to set with atomicity, so cast away
			m_expireTask = get_next_dispatcher()->dispatch([r{ this_rcref }]()
			{
				r->serial_expire();
			}, priority);
			break;
		}
	}

	void serial_resume() volatile
	{
		serial_dispatch_state oldState;
		atomic::load(m_serialDispatchState, oldState);
		for (;;)
		{
			COGS_ASSERT((oldState.m_flags & expired_flag) != 0);
			COGS_ASSERT((oldState.m_flags & busy_flag) != 0);

			serial_dispatch_state newState = oldState;
			newState.m_flags &= ~dirty_flag & ~expired_flag;
			if (!atomic::compare_exchange(m_serialDispatchState, newState, oldState, oldState))
				continue;

			get_next_dispatcher()->dispatch([r{ this_rcref }]()
			{
				r->serial_update();
			}, const_min_int_v<int>); // highest possible priority
			break;
		}
	}

	template <typename F, std::enable_if_t<std::is_invocable_v<F> >...>
	auto dispatch_async(F&& f, int priority = 0) volatile
	{
		typedef std::invoke_result_t<F> result_t2;
		typedef forwarding_function_task<result_t2, void> task_t;
		rcref<task<result_t2> > result = rcnew(task_t, std::forward<F>(f), priority).template static_cast_to<task<result_t2> >();
		priority_queue<int, ptr<serial_dispatched> >::preallocated_t i;
		auto r = m_priorityQueue.preallocate_key_with_aux<delayed_construction<serial_dispatched> >(priority, i);
		serial_dispatched* d = &(r->get());
		i.get_value() = d;
		new(d) serial_dispatched(r.get_desc(), true, this_rcref, result.template static_cast_to<task_t>().template static_cast_to<task_base>(), i);
		m_priorityQueue.insert_preallocated(i);
		rcref<dispatched> d2(d, i.get_desc());
		result.template static_cast_to<task_t>().template static_cast_to<task_base>()->set_dispatched(d2);
		i.disown();
		serial_dispatch();
		return result;
	}

protected:
	explicit pane(const ptr<rc_obj_base>& desc, compositing_behavior cb = compositing_behavior::no_buffer)
		: object(desc),
		m_compositingBehavior(cb),
		m_setSubsystemDelegate([r{ this_weak_rcptr }](const rcptr<volatile gui::subsystem>& s)
		{
			rcptr<pane> r2 = r;
			if (!!r2)
				r2->set_subsystem_inner(s);
		})
	{
		m_serialDispatchState.m_flags = 0;
	}

	~pane()
	{
		for (;;)
		{
			priority_queue<int, ptr<serial_dispatched> >::value_token vt = m_priorityQueue.peek();
			if (!vt)
				break;
			(*vt)->get_task_base()->cancel();
		}

		// release frames from children (or they will leak, due to circular reference)
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
		while (!!itor)
		{
			(*itor)->m_frame.release();
			++itor;
		}
	}

	void set_externally_drawn(const rcref<gfx::canvas>& externalCanvas)
	{
		COGS_ASSERT(m_installing); // This function should also be called when installing
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
			r->m_setSubsystemDelegate(s);
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
		if ((m_compositingBehavior == no_buffer) && (!is_externally_drawn()))
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
			while ((p->m_compositingBehavior != buffer_self_and_children) && (!p->is_externally_drawn()))
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
		if ((m_compositingBehavior == no_buffer) && (!is_externally_drawn()))
			return get_ancestor_render_pane();
		return this_rcptr;
	}

	rcptr<pane> get_render_pane(point& offset)
	{
		if ((m_compositingBehavior == no_buffer) && (!is_externally_drawn()))
			return get_ancestor_render_pane(offset);
		return this_rcptr;
	}

	rcptr<pane> get_render_pane(point& offset, bounds& visibleBounds)
	{
		if ((m_compositingBehavior == no_buffer) && (!is_externally_drawn()))
			return get_ancestor_render_pane(offset, visibleBounds);
		visibleBounds = get_size();
		return this_rcptr;
	}

	rcptr<pane> get_ancestor_render_pane() const
	{
		rcptr<pane> p = m_parent;
		if (!!p)
		{
			while ((p->m_compositingBehavior != buffer_self_and_children) && (!p->is_externally_drawn()))
			{
				rcptr<pane> p2 = p->get_parent();
				if (!p2)
					break;
				p = p2;
			}
		}
		return p;
	}

	rcptr<pane> get_ancestor_render_pane(point& offset) const
	{
		const pane* child = this;
		rcptr<pane> parent = m_parent;
		while (!!parent)
		{
			offset += child->get_position();
			if (parent->is_externally_drawn() || parent->m_compositingBehavior == buffer_self_and_children)
				break;
			child = parent.get_ptr();
			parent = child->m_parent;
		}

		return parent;
	}

	rcptr<pane> get_ancestor_render_pane(point& offset, bounds& visibleBounds) const
	{
		visibleBounds = get_size();
		const pane* child = this;
		rcptr<pane> parent = m_parent;
		while (!!parent)
		{
			point childPosition = child->get_position();
			offset += childPosition;
			visibleBounds += childPosition;
			visibleBounds &= bounds(parent->get_size());
			if (parent->is_externally_drawn() || parent->m_compositingBehavior == buffer_self_and_children)
				break;
			child = parent.get_ptr();
			parent = child->m_parent;
		}

		return parent;
	}

	void install(const rcptr<volatile subsystem>& subSystem)
	{
		subSystem->dispatch([r{ this_rcref }, subSystem]()
		{
			r->m_uiSubSystem = subSystem;
			r->dispatch_async([r]()
			{
				r->install_inner2();
			});
		});
	}

	void uninstall()
	{
		dispatch_async([r{ this_rcref }]()
		{
			r->uninstall_inner();
		});
	}

	virtual void nest_last(const rcref<pane>& child, const rcptr<frame>& f = 0)
	{
		child->m_parent = this_rcref;
		child->m_frame = f;

		child->m_siblingIterator = m_children.append(child);

		COGS_ASSERT(!child->m_installed && !child->m_installing);
		if (m_installed)
			child->install(get_subsystem());
	}

	virtual void nest_first(const rcref<pane>& child, const rcptr<frame>& f = 0)
	{
		child->m_parent = this_rcref;
		child->m_frame = f;

		child->m_siblingIterator = m_children.prepend(child);

		COGS_ASSERT(!child->m_installed && !child->m_installing);
		if (m_installed)
			child->install(get_subsystem());
	}

	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis, const rcptr<frame>& f = 0)
	{
		child->m_parent = this_rcref;
		child->m_frame = f;

		child->m_siblingIterator = m_children.insert_before(child, beforeThis->m_siblingIterator);

		COGS_ASSERT(!child->m_installed && !child->m_installing);
		if (m_installed)
			child->install(get_subsystem());
	}

	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis, const rcptr<frame>& f = 0)
	{
		child->m_parent = this_rcref;
		child->m_frame = f;

		child->m_siblingIterator = m_children.insert_after(child, afterThis->m_siblingIterator);

		COGS_ASSERT(!child->m_installed && !child->m_installing);
		if (m_installed)
			child->install(get_subsystem());
	}

	void detach_children()
	{
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
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
				if (cb == no_buffer)
					m_offScreenBuffer.release();

				if (cb == buffer_self) // invalidate before, to ensure children get invalidated.
					invalidate(get_size());

				m_compositingBehavior = cb;

				if (cb != buffer_self)
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
					bounds visibleBounds;
					rcptr<pane> p = get_ancestor_render_pane(offset, visibleBounds);
					COGS_ASSERT(!!p);
					p->save_clip();
					p->clip_to(visibleBounds);
					p->clip_opaque_after(*this, offset, visibleBounds);
					p->clip_opaque_descendants(*this, offset, visibleBounds, true);
					bounds b = get_bounds();
					p->draw_bitmap(*m_offScreenBuffer, b, b.get_size());
					p->restore_clip();
				}
			};

			prepare_offscreen_buffer();
			if (!!m_offScreenBuffer && !m_needsDraw)      // --- Buffer already available:
			{
				composite_offscreen_buffer();             // Fully clip, Blit, Unclip
				if (m_compositingBehavior == buffer_self) // If buffering only this pane,
					draw_children();                      // Draw children (they clip themselves)
			}
			else
			{
				m_needsDraw = false;
				if (m_compositingBehavior == no_buffer)   // --- Not buffering:
				{                                         // Fully clip
					point offset(0, 0);
					bounds visibleBounds;
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
				else if (m_compositingBehavior == buffer_self) // --- Only buffering this pane:
				{
					size sz = get_size();
					bounds b = sz;
					save_clip();
					clip_to(b);                           // Clip to bounds
					fill(b, color::transparent, false);
					drawing();                            // Draw this pane
					restore_clip();                       // Unclip
					composite_offscreen_buffer();         // Fully clip, Blit, Unclip
					draw_children();                      // Draw children (they clip themselves)
				}
				else //if (m_compositingBehavior == buffer_self_and_children) // --- Buffer this pane and children:
				{
					size sz = get_size();
					bounds b = sz;
					save_clip();
					clip_to(b);                                // Clip to bounds
					point offset(0, 0);
					clip_opaque_descendants(*this, offset, b); // Clip descendants
					fill(sz, color::transparent, false);
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
	// to trigger calls to calculate_range() and reshape().
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
		COGS_ASSERT(!m_installing);
		COGS_ASSERT(!m_uninstalling);
		m_recomposing = false; // only works to guard redundant calls to recompose() while in queue
		cell& r = get_outermost_cell();
		rcptr<pane> parent = m_parent;
		if (!parent)
		{
			m_recomposeDescendants = recomposeDescendants;
			cell::calculate_range(r);
			m_recomposeDescendants = true;
			cell::reshape(r, r.propose_size(get_size()));
		}
		else
		{
			range oldParentRange = r.get_range();
			m_recomposeDescendants = recomposeDescendants;
			cell::calculate_range(r);
			m_recomposeDescendants = true;
			range newParentRange = r.get_range();
			if (newParentRange == oldParentRange) // if parent would be unaffected
				cell::reshape(r, r.propose_size(get_size()));
			else
				parent->recompose();
		}
	}

	void invalidate(const bounds& b) // b in local coords
	{
		bounds b2 = b & bounds(get_size()); // clip
		if (!!b2.get_height() && !!b2.get_width())
		{
			m_needsDraw = true;
			invalidating(b2);
			invalidating_up(b2);
			if (m_compositingBehavior != buffer_self) // no need to invalidate children if we're buffering only ourselves.
				invalidating_children(b2);
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
			container_dlist<rcref<pane> >::iterator itor = child->m_siblingIterator;
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
		rcptr<pane> top = this_rcref;
		rcptr<pane> p = top;
		for (;;)
		{
			if (topToButton)
			{
				if (!f(*p))
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
				if (!f(*p))
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
				p = p->get_parent();
				COGS_ASSERT(!!p);
			}
		}
		return false;
	}

	const waitable& get_close_event() const
	{
		return m_closeEvent;
	}

	virtual void closing()
	{
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if (!(*itor)->m_closeEvent.is_signaled())
				(*itor)->closing();
			++itor;
		}
		m_closeEvent.signal();
	}

	virtual void close()
	{
		if (!m_closeEvent.is_signaled())
		{
			hide();
			closing();
		}
	}

	priority_queue<int, function<function<void()>(bool&)> > m_requestCloseHandlers;
	typedef priority_queue<int, function<function<void()>(bool&)> >::remove_token request_close_handler_token;

	template <typename F>
	request_close_handler_token register_request_close_handler(F&& f, int priority = 0)
	{
		return m_requestCloseHandlers.insert(priority, std::move(f));
	}

	bool deregister_request_close_handler(request_close_handler_token& t)
	{
		return m_requestCloseHandlers.remove(t);
	}

	bool requesting_close()
	{
		if (!!m_closeEvent.is_signaled())
			return true;

		container_dlist<function<void(bool)> > cb;
		bool b = for_each_subtree<true>([&cb](pane& p)
		{
			for (;;)
			{
				auto vt = p.m_requestCloseHandlers.get();
				if (!vt)
					return true;
				bool allowClose = true;
				auto f = (*vt)(allowClose);
				if (!!f)
					cb.append(std::move(f));
				if (!allowClose)
					return false;
			}
		});
		auto itor = cb.get_first();
		while (!!itor)
		{
			(*itor)(b);
			++itor;
		}
		if (!!b)
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

	virtual void calculate_range()
	{
		m_currentDefaultSize.set(0, 0);
		m_currentRange.clear();

		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
		while (!!itor)
		{
			cell& r = (*itor)->get_outermost_cell();
			if (m_recomposeDescendants)
				cell::calculate_range(r);
			m_currentDefaultSize |= r.get_default_size();
			m_currentRange &= r.get_range();
			++itor;
		}

		m_currentDefaultSize = m_currentRange.limit(m_currentDefaultSize);
	}

	// canvas interface

	virtual void fill(const bounds& b, const color& c = color::black, bool blendAlpha = true)
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

	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::black, bool blendAlpha = true)
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

	virtual rcref<font> load_font(const gfx::font& fnt)
	{
		if (!!m_offScreenBuffer)
			return m_offScreenBuffer->load_font(fnt);

		if (!!m_bridgedCanvas)
			return m_bridgedCanvas->load_font(fnt);

		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Top level should have bridged canvas.  Should not be called if not installed.
		return parent->load_font(fnt);
	}

	virtual gfx::font get_default_font() const
	{
		if (!!m_offScreenBuffer)
			return m_offScreenBuffer->get_default_font();

		if (!!m_bridgedCanvas)
			return m_bridgedCanvas->get_default_font();

		rcptr<pane> parent = get_ancestor_render_pane();
		COGS_ASSERT(!!parent); // Top level should have bridged canvas.  Should not be called if not installed.
		return parent->get_default_font();
	}

	virtual void draw_text(const composite_string& s, const bounds& b, const rcptr<font>& fnt = 0, const color& c = color::black)
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

	virtual void draw_bitmask(const bitmask& msk, const bounds& srcBounds, const bounds& dstBounds, const color& fore = color::black, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true)
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
			(*m_childInstallItor)->dispatch_async([r{ m_childInstallItor->dereference() }]()
			{
				r->install_inner2();
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

	virtual bool character_typing(wchar_t c, const ui::modifier_keys_state& modifiers) { return (!!m_subFocus) && m_subFocus->character_typing(c, modifiers); }
	virtual bool key_pressing(wchar_t c, const ui::modifier_keys_state& modifiers) { return (!!m_subFocus) && m_subFocus->key_pressing(c, modifiers); }
	virtual bool key_releasing(wchar_t c, const ui::modifier_keys_state& modifiers) { return (!!m_subFocus) && m_subFocus->key_releasing(c, modifiers); }

	virtual void modifier_keys_changing(const ui::modifier_keys_state& modifiers)
	{
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
		while (!!itor)
		{
			(*itor)->modifier_keys_changing(modifiers);
			++itor;
		}
	}

	virtual bool button_pressing(mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
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
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
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
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
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
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
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
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
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
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
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
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
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
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if (!(*itor)->is_hidden())
				(*itor)->hiding();
			++itor;
		}
	}

	virtual void showing()
	{
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
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
	virtual void focusing(int direction) { focus_children(direction); }

	virtual void defocusing() { }

	// pane interface - notifications

	virtual void invalidating(const bounds&) { }

	// A parent pane that keeps additional information about nested panes may need to override detaching_child(),
	// to refresh that data when a pane is removed.
	virtual void detaching_child(const rcref<pane>& child)
	{
		if (child->m_installed)
		{
			child->m_detaching = true;
			child->uninstall();
		}
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		size oldSize = get_size();
		cell::reshape(b, oldOrigin);
		size newSize = get_size();
		bool sizeChanged = oldSize != newSize;

		point renderOffset(0, 0);
		bounds newVisibleBounds;
		rcptr<pane> p = get_ancestor_render_pane(renderOffset, newVisibleBounds);
		if (!m_initialReshapeDone)
			m_initialReshapeDone = true;
		else
		{
			//bool positionChanged = renderOffset != m_lastRenderOffset;
			bool isBuffered = m_compositingBehavior != no_buffer;

			// Maybe an assumption here that any externally drawn pane that is only repositioned, will have own invalidate/redraw.
			if (!!p)
			{
				// Invalidate visibly vacated portions of this pane, in the rendering anscestor pane
				auto invalidBounds = m_lastVisibleBounds - newVisibleBounds;
				for (size_t i = 0; i < invalidBounds.second; i++)
					p->invalidate(invalidBounds.first[i]);

				if (!is_externally_drawn())
				{
					auto invalidBounds = newVisibleBounds - m_lastVisibleBounds;
					for (size_t i = 0; i < invalidBounds.second; i++)
						p->invalidate(invalidBounds.first[i]);
				}
			}

			if (sizeChanged && (!p || isBuffered || is_externally_drawn()))
			{
				if (oldSize.get_width() < newSize.get_width())
					invalidate(bounds(point(oldSize.get_width(), 0), size(newSize.get_width() - oldSize.get_width(), newSize.get_height())));

				if (oldSize.get_height() < newSize.get_height())
					invalidate(bounds(point(0, oldSize.get_height()), size(newSize.get_width(), newSize.get_height() - oldSize.get_height())));
			}
		}
		m_lastVisibleBounds = newVisibleBounds;
		m_lastRenderOffset = renderOffset;

		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
		while (!!itor)
		{
			cell::reshape((*itor)->get_outermost_cell(), b.get_size(), oldOrigin);
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

	virtual void reshape_top()
	{
		cell& r = get_outermost_cell();
		cell::reshape(r, r.get_default_size());
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
				cell::calculate_range(get_outermost_cell());
			else
			{
				rcptr<pane> parentInstalling = m_parentInstalling;
				m_parentInstalling.release();

				container_dlist<rcref<pane> >::iterator itor = ++(parentInstalling->m_childInstallItor);
				if (!!itor)
				{
					(*itor)->m_parentInstalling = parentInstalling;
					rcptr<volatile subsystem> tmp = m_parentUISubSystem;
					(*itor)->m_parentUISubSystem = tmp;
					(*itor)->dispatch_async([r{ itor->dereference() }]()
					{
						r->install_inner2();
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
				reshape_top();

			if (is_visible())
				showing();
			break;
		}

		serial_resume();
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
			child->dispatch_async([r{ child.dereference() }]()
			{
				r->uninstall_inner2();
			});
		}
	}

	void uninstall_done()
	{
		for (;;)
		{
			container_dlist<rcref<pane> >::iterator itor;
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
					(*itor)->dispatch_async([r{ itor->dereference() }]()
					{
						return r->uninstall_inner2();
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
		serial_resume();
		self_release();
	}

	void invalidating_up(const bounds& b)
	{
		if (!is_externally_drawn())
		{
			bounds parentRect = b;
			rcptr<pane> child = this_rcptr;
			rcptr<pane> parent = m_parent;
			for (;;)
			{
				if (!parent)
					break;
				parentRect += child->get_position();
				parentRect &= bounds(point(0, 0), parent->get_size());
				if (!!parentRect.get_width() && !!parentRect.get_height())
				{
					if ((parent->m_compositingBehavior != buffer_self_and_children) && (!parent->is_externally_drawn()))
					{
						child = parent;
						parent = parent->m_parent;
						continue;
					}
					parent->m_needsDraw = true;
					parent->invalidating(parentRect);
					parent->invalidating_up(parentRect);
				}
				break;
			}
		}
	}

	void invalidating_children(const bounds& b) // b is in own coords
	{
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
		if (!!itor)
			do {
				rcref<pane>& child = (*itor);
				if (!child->is_opaque()) // Only invalidate along with parent if child is self-drawn and not opaque
				{
					bounds r2 = b;
					r2 -= child->get_position(); // convert to child coords
					r2 &= bounds(child->get_size()); // intersections of child and invalid
					if (!!r2.get_height() && !!r2.get_width())
					{
						if (child->is_externally_drawn())
							child->invalidating(r2);
						child->invalidating_children(r2);
					}
				}
			} while (!!++itor);
	}

	void draw_children()
	{
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if (!(*itor)->is_externally_drawn())
				(*itor)->draw();
			++itor;
		}
	}

	void clip_opaque_descendants(pane& fromPane, const point& offset, const bounds& visibleBounds, bool onlyIfExternal = false)
	{
		container_dlist<rcref<pane> >::iterator itor = fromPane.m_children.get_first();
		while (!!itor)
		{
			pane& p = **itor;
			bounds b = p.get_bounds() + offset;
			bounds newVisibleBounds = visibleBounds & b;
			if (!!newVisibleBounds)
			{
				if (p.is_opaque() && (!onlyIfExternal || p.is_externally_drawn()))
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
			container_dlist<rcref<pane> >::iterator itor = curPane->m_siblingIterator;
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
			if (!curPane || curPane->is_externally_drawn() || curPane->m_compositingBehavior == buffer_self_and_children)
				break;
		}
	}

	bool is_focusable(const pane* skipChild) const // override to indicate capable of receiving focus
	{
		if (this == skipChild)
			return false;

		if (m_isFocusable)
			return true;

		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
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
			container_dlist<rcref<pane> >::iterator itor;
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
		if (m_compositingBehavior != no_buffer)
		{
			size sz = get_size();
			if (!!m_offScreenBuffer)
				m_offScreenBuffer->set_size(sz, size(100, 100), false);
			else
			{
				m_offScreenBuffer = create_offscreen_buffer(*this, sz, is_opaque() ? color::black : color::transparent);
				m_needsDraw = true;
				invalidating(sz);
			}
		}
	}

	template <typename F>
	void for_each_child(F&& f)
	{
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
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
		container_dlist<rcref<pane> >::iterator itor = m_children.get_last();
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

		m_frame.release();
	}

	const cell& get_const_cell() const
	{
		if (!!m_frame)
			return *m_frame;
		return *this;
	}

	rcref<const cell> get_const_cell_ref() const
	{
		if (!!m_frame)
			return m_frame.dereference();
		return get_self_rcref((const cell*)this);
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
			m_closeEvent.reset();
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

	point get_position() const
	{
		point pt(0, 0);
		rcptr<frame> f = get_outermost_frame();
		if (!!f)
			pt = f->get_innermost_child_position();
		return pt;
	}

	bounds get_bounds() const
	{
		bounds b(get_position(), get_size());
		return b;
	}

	virtual range get_range() const { return m_currentRange; }
	virtual size get_default_size() const { return m_currentDefaultSize; }

	virtual size propose_lengths(dimension d, const size& proposedSize) const
	{
		// Since the default behavior is to constrain the parent,
		// we need to give children a chance to consider the proposed size.
		// There is no need to limit to m_currentRange, as it's solely based on child ranges
		size currentProposed = proposedSize;
		container_dlist<rcref<pane> >::iterator itorToSkip;
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if (itor != itorToSkip)
			{
				size newProposed = (*itor)->get_outermost_cell().propose_lengths(d, currentProposed);
				if (currentProposed != newProposed) // If any subframe doesn't like our size, we need to RE propose the new size to all subframes.
				{
					if (!newProposed.get_height() || !newProposed.get_width())
						break; // give up
					currentProposed = newProposed;
					itorToSkip = itor;
					itor = m_children.get_first();
					continue; // skip the ++itor
				}
			}
			++itor;
		}

		return currentProposed;
	}

	virtual size propose_size(const size& proposedSize) const
	{
		// Since the default behavior is to constrain the parent,
		// we need to give children a chance to consider the proposed size.
		// There is no need to limit to m_currentRange, as it's solely based on child ranges
		size currentProposed = proposedSize;
		container_dlist<rcref<pane> >::iterator itorToSkip;
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if (itor != itorToSkip)
			{
				size newProposed = (*itor)->get_outermost_cell().propose_size(currentProposed);
				if (currentProposed != newProposed) // If any subframe doesn't like our size, we need to RE propose the new size to all subframes.
				{
					if (!newProposed.get_height() || !newProposed.get_width())
						break; // give up
					currentProposed = newProposed;
					itorToSkip = itor;
					itor = m_children.get_first();
					continue; // skip the ++itor
				}
			}
			++itor;
		}

		return currentProposed;
	}

	virtual double propose_length(dimension d, double proposed, range::linear_t& rtnOtherRange) const
	{
		double rtn = proposed;
		rtnOtherRange.clear();

		double currentProposed = proposed;
		container_dlist<rcref<pane> >::iterator itorToSkip;
		container_dlist<rcref<pane> >::iterator itor = m_children.get_first();
		while (!!itor)
		{
			if (itor != itorToSkip)
			{
				range::linear_t newOtherRange;
				rtn = (*itor)->get_outermost_cell().propose_length(d, currentProposed, newOtherRange);
				if (rtn == currentProposed)
					rtnOtherRange &= newOtherRange;
				else // If any subframe doesn't like our size, we need to RE propose the new size to all subframes.
				{
					if (!rtn)
					{ // give up
						rtnOtherRange.set_empty();
						break;
					}
					rtnOtherRange.clear(); // clean rtnOtherRange and start over
					itorToSkip = itor;
					itor = m_children.get_first();
					continue; // skip the ++itor
				}
			}
			++itor;
		}

		return rtn;
	}

	const rcptr<frame>& get_outermost_frame() const { return m_frame; } // will return null if not nested in a parent frame

	const cell& get_outermost_cell() const
	{
		if (!!m_frame)
			return *m_frame;
		return *this;
	}

	cell& get_outermost_cell()
	{
		if (!!m_frame)
			return *m_frame;
		return *this;
	}

	const weak_rcptr<pane>& get_parent() const { return m_parent; }
	const container_dlist<rcref<pane> >& get_children() const { return m_children; }
	const container_dlist<rcref<pane> >::remove_token& get_sibling_iterator() const { return m_siblingIterator; }

	virtual bool is_opaque() const { return false; }

	bool is_installed() const { return m_installed; }
	bool is_installing() const { return m_installing; }
	bool is_uninstalling() const { return m_uninstalling; }

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
	explicit container_pane(const ptr<rc_obj_base>& desc)
		: pane(desc, compositing_behavior::no_buffer)
	{
	}

	container_pane(const ptr<rc_obj_base>& desc, compositing_behavior cb)
		: pane(desc, cb)
	{
	}

	static rcref<container_pane> create(compositing_behavior cb = compositing_behavior::no_buffer)
	{
		return rcnew(container_pane, cb);
	}

	using pane_container::nest;
	virtual void nest_last(const rcref<pane>& child, const rcptr<frame>& f = 0) { pane::nest_last(child, f); }
	virtual void nest_first(const rcref<pane>& child, const rcptr<frame>& f = 0) { pane::nest_first(child, f); }
	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis, const rcptr<frame>& f = 0) { pane::nest_before(child, beforeThis, f); }
	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis, const rcptr<frame>& f = 0) { pane::nest_after(child, afterThis, f); }
};


class canvas_pane : public pane, public virtual pane_container, public virtual gfx::canvas
{
public:
	typedef function<void(const rcref<canvas_pane>&)> draw_delegate_t;

private:
	draw_delegate_t m_drawDelegate;
	bool m_invalidateOnReshape;

protected:
	virtual void drawing()
	{
		if (!!m_drawDelegate)
			m_drawDelegate(this_rcref);
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		pane::reshape(b, oldOrigin);
		if (m_invalidateOnReshape)
			invalidate(get_size());
	}

public:
	canvas_pane(const ptr<rc_obj_base>& desc, const draw_delegate_t& d = draw_delegate_t(), compositing_behavior cb = compositing_behavior::no_buffer, bool invalidateOnReshape = true)
		: pane(desc, cb),
		m_drawDelegate(d),
		m_invalidateOnReshape(invalidateOnReshape)
	{
	}

	virtual void fill(const bounds& b, const color& c = color::black, bool blendAlpha = true) { pane::fill(b, c, blendAlpha); }
	virtual void invert(const bounds& b) { pane::invert(b); }
	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::black, bool blendAlpha = true) { pane::draw_line(startPt, endPt, width, c, blendAlpha); }
	virtual rcref<font> load_font(const gfx::font& guiFont = gfx::font()) { return pane::load_font(guiFont); }
	virtual gfx::font get_default_font() const { return pane::get_default_font(); }
	virtual void draw_text(const composite_string& s, const bounds& b, const rcptr<font>& f = 0, const color& c = color::black) { pane::draw_text(s, b, f, c); }
	virtual void draw_bitmap(const bitmap& src, const bounds& srcBounds, const bounds& dstBounds, bool blendAlpha = true) { return pane::draw_bitmap(src, srcBounds, dstBounds, blendAlpha); }
	virtual void draw_bitmask(const bitmask& src, const bounds& srcBounds, const bounds& dstBounds, const color& fore = color::black, const color& back = color::white, bool blendForeAlpha = true, bool blendBackAlpha = true) { pane::draw_bitmask(src, srcBounds, dstBounds, fore, back, blendForeAlpha, blendBackAlpha); }
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
	virtual void nest_last(const rcref<pane>& child, const rcptr<frame>& f = 0) { pane::nest_last(child, f); }
	virtual void nest_first(const rcref<pane>& child, const rcptr<frame>& f = 0) { pane::nest_first(child, f); }
	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis, const rcptr<frame>& f = 0) { pane::nest_before(child, beforeThis, f); }
	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis, const rcptr<frame>& f = 0) { pane::nest_after(child, afterThis, f); }
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


#include "cogs/gui/window.hpp"


#endif

