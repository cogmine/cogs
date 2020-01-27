//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_GUI_PANE_BRIDGE
#define COGS_HEADER_GUI_PANE_BRIDGE


#include "cogs/gui/pane.hpp"


namespace cogs {
namespace gui {


class bridgeable_pane;

class pane_bridge;


// pane_bridge can bridge in a bridgeable_pane.
// platform specific OS controls are derived from bridgeable_pane,
// and hosted in a pane_bridge.

/// @ingroup GUI
/// @brief Base class for bridgeable panes that can be bridged by pane_bridge.
class bridgeable_pane
{
private:
	friend class pane_bridge;

	weak_rcptr<pane> m_paneBridge;

protected:
	const weak_rcptr<pane>& get_bridge() const { return m_paneBridge; }

	static const weak_rcptr<pane>& get_bridge(const bridgeable_pane& p) { return p.m_paneBridge; }

	static const rcptr<bridgeable_pane>& get_bridged(const pane_bridge& p);

	void install(rcref<pane>&& paneBridge)
	{
		m_paneBridge = std::move(paneBridge);
		installing();
	}

	void install(const rcref<pane>& paneBridge)
	{
		m_paneBridge = paneBridge;
		installing();
	}

	size get_size() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::get_size();
	}

	point get_position() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::get_position();
	}

	bool is_drawing_needed() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::is_drawing_needed();
	}

	void invalidate(const bounds& b)
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::invalidate(b);
	}

	void hide()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::hide();
	}

	void show()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::show();
	}

	void focus(int direction = 0)
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::focus(direction);
	}

	void defocus()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::defocus();
	}

	rcptr<volatile gui::subsystem> get_subsystem() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::get_subsystem();
	}

	const rcptr<bitmap>& peek_offscreen_buffer()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::peek_offscreen_buffer();
	}

	const rcptr<bitmap>& get_offscreen_buffer()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::get_offscreen_buffer();
	}

	void draw()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::draw();
	}

	const weak_rcptr<pane>& get_parent() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::get_parent();
	}

	const container_dlist<rcref<pane> >& get_children() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::get_children();
	}

	const container_dlist<rcref<pane> >::remove_token& get_sibling_iterator() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::get_sibling_iterator();
	}

	void set_compositing_behavior(compositing_behavior cb)
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::set_compositing_behavior(cb);
	}

	void set_externally_drawn(const rcref<gfx::canvas>& externalCanvas)
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::set_externally_drawn(externalCanvas);
	}

	bool is_hidden() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::is_hidden();
	}

	bool is_visible() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::is_visible();
	}

	void recompose(bool recomposeDescendants = false)
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::recompose(recomposeDescendants);
	}

	// TODO: Need more functions of pane exposed here???


	// pane_base interface - public

	virtual bool is_opaque() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::is_opaque();
	}

	virtual bool is_unclipped(const bounds& b) const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::is_unclipped(b);
	}

	// reshapable interface - public

	virtual size get_default_size() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::get_default_size();
	}

	virtual range get_range() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::get_range();
	}

	virtual dimension get_primary_flow_dimension() const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::get_primary_flow_dimension();
	}

	virtual cell::propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), cell::size_mode horizontalMode = cell::size_mode::both, cell::size_mode verticalMode = cell::size_mode::both) const
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::propose_size(sz, resizeDimension, r, horizontalMode, verticalMode);
	}

	virtual void calculate_range()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::calculate_range();
	}

	// notification interfaces (called internally, overriden)

	// pane_base interface - notifications

	// bridgeable pane should override installing().  install can be async.
	virtual void installing()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::installing();
	}

	virtual void uninstalling()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::uninstalling();
	}

	virtual bool character_typing(wchar_t c, const ui::modifier_keys_state& modifiers) { return false; }

	virtual bool key_pressing(wchar_t c, const ui::modifier_keys_state& modifiers) { return false; }
	virtual bool key_releasing(wchar_t c, const ui::modifier_keys_state& modifiers) { return false; }

	virtual bool button_pressing(mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers) { return false; }
	virtual bool button_releasing(mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers) { return false; }
	virtual bool button_double_clicking(mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers) { return false; }

	virtual bool wheel_moving(double distance, const point& pt, const ui::modifier_keys_state& modifiers) { return false; }

	virtual void modifier_keys_changing(const ui::modifier_keys_state& modifiers)
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::modifier_keys_changing(modifiers);
	}

	virtual void cursor_entering(const point& pt)
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::cursor_entering(pt);
	}

	virtual void cursor_moving(const point& pt)
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::cursor_moving(pt);
	}

	virtual void cursor_leaving()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::cursor_leaving();
	}

	virtual void hiding()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::hiding();
	}

	virtual void showing()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::showing();
	}

	virtual void drawing()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::drawing();
	}

	// if overridden, base bridgeable_pane::focusing() should be invoked.
	virtual void focusing(int direction)
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::focusing(direction);
	}

	// if overridden, base bridgeable_pane::defocusing() should be invoked.
	virtual void defocusing()
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::defocusing();
	}

	// if overridden, base bridgeable_pane::invalidating() should be invoked.
	virtual void invalidating(const bounds& b)
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::invalidating(b);
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		paneBridge->pane::reshape(b, oldOrigin);
	}

	virtual rcref<bitmap> create_offscreen_buffer(pane& forPane, const size& sz, std::optional<color> fillColor = std::nullopt)
	{
		rcptr<pane> paneBridge = m_paneBridge;
		COGS_ASSERT(!!paneBridge);
		return paneBridge->pane::create_offscreen_buffer(forPane, sz, fillColor);
	}
};


/// @ingroup GUI
/// @brief Bridges a bridgeable_pane.
class pane_bridge : public pane
{
private:
	friend class bridgeable_pane;

	rcptr<bridgeable_pane> m_bridgedPane;

protected:
	explicit pane_bridge(rc_obj_base& desc)
		: pane(desc)
	{ }

	const rcptr<bridgeable_pane>& get_bridged() const { return m_bridgedPane; }

	void install_bridged(const rcref<bridgeable_pane>& bp)
	{
		m_bridgedPane = bp;
		bp->install(this_rcref);
	}


	// pane_base interface - public

	virtual bool is_opaque() const
	{
		COGS_ASSERT(!!m_bridgedPane);
		return m_bridgedPane->is_opaque();
	}

	virtual bool is_unclipped(const bounds& b) const
	{
		COGS_ASSERT(!!m_bridgedPane);
		return m_bridgedPane->is_unclipped(b);
	}

	// reshapable interface - public

	virtual size get_default_size() const
	{
		COGS_ASSERT(!!m_bridgedPane);
		return m_bridgedPane->get_default_size();
	}

	virtual dimension get_primary_flow_dimension() const
	{
		COGS_ASSERT(!!m_bridgedPane);
		return m_bridgedPane->get_primary_flow_dimension();
	}

	virtual cell::propose_size_result propose_size(const size& sz, std::optional<dimension> resizeDimension = std::nullopt, const range& r = range::make_unbounded(), cell::size_mode horizontalMode = cell::size_mode::both, cell::size_mode verticalMode = cell::size_mode::both) const
	{
		COGS_ASSERT(!!m_bridgedPane);
		return m_bridgedPane->propose_size(sz, resizeDimension, r, horizontalMode, verticalMode);
	}

	virtual void calculate_range()
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->calculate_range();
	}

	// notification interfaces (called internally, overriden)

	// pane_base interface - notifications

	// derived pane must override installing(), and call install_bridged(const rcref<bridgeable_pane_t>&) 
	// passing in the bridgeable_pane_t to use.
	virtual void installing() = 0;
	// the pane_bridge's pane::installing() gets called by the bridgable_pane

	virtual void uninstalling()
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->uninstalling();
		m_bridgedPane = 0;
	}

	virtual bool character_typing(wchar_t c, const ui::modifier_keys_state& modifiers)
	{
		bool result = false;
		COGS_ASSERT(!!m_bridgedPane);
		result = m_bridgedPane->character_typing(c, modifiers);
		if (!result)
			result = pane::character_typing(c, modifiers);
		return result;
	}

	virtual bool key_pressing(wchar_t c, const ui::modifier_keys_state& modifiers)
	{
		bool result = false;
		COGS_ASSERT(!!m_bridgedPane);
		result = m_bridgedPane->key_pressing(c, modifiers);
		if (!result)
			result = pane::key_pressing(c, modifiers);
		return result;
	}

	virtual bool key_releasing(wchar_t c, const ui::modifier_keys_state& modifiers)
	{
		bool result = false;
		COGS_ASSERT(!!m_bridgedPane);
		result = m_bridgedPane->key_releasing(c, modifiers);
		if (!result)
			result = pane::key_releasing(c, modifiers);
		return result;
	}



	virtual bool button_pressing(mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		bool result = false;
		COGS_ASSERT(!!m_bridgedPane);
		result = m_bridgedPane->button_pressing(btn, pt, modifiers);
		if (!result)
			result = pane::button_pressing(btn, pt, modifiers);
		return result;
	}

	virtual bool button_releasing(mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		bool result = false;
		COGS_ASSERT(!!m_bridgedPane);
		result = m_bridgedPane->button_releasing(btn, pt, modifiers);
		if (!result)
			result = pane::button_releasing(btn, pt, modifiers);
		return result;
	}

	virtual bool button_double_clicking(mouse_button btn, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		bool result = false;
		COGS_ASSERT(!!m_bridgedPane);
		result = m_bridgedPane->button_double_clicking(btn, pt, modifiers);
		if (!result)
			result = pane::button_double_clicking(btn, pt, modifiers);
		return result;
	}

	virtual bool wheel_moving(double distance, const point& pt, const ui::modifier_keys_state& modifiers)
	{
		bool result = false;
		COGS_ASSERT(!!m_bridgedPane);
		result = m_bridgedPane->wheel_moving(distance, pt, modifiers);
		if (!result)
			result = pane::wheel_moving(distance, pt, modifiers);
		return result;
	}

	virtual void modifier_keys_changing(const ui::modifier_keys_state& modifiers)
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->modifier_keys_changing(modifiers);
	}

	virtual void cursor_entering(const point& pt)
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->cursor_entering(pt);
	}

	virtual void cursor_moving(const point& pt)
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->cursor_moving(pt);
	}

	virtual void cursor_leaving()
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->cursor_leaving();
	}

	virtual void hiding()
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->hiding();
	}

	virtual void showing()
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->showing();
	}

	virtual void drawing()
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->drawing();
	}

	virtual void focusing(int direction)
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->focusing(direction);
	}

	virtual void defocusing()
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->defocusing();
	}

	virtual void invalidating(const bounds& b)
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->invalidating(b);
	}

	virtual void reshape(const bounds& b, const point& oldOrigin = point(0, 0))
	{
		COGS_ASSERT(!!m_bridgedPane);
		m_bridgedPane->reshape(b, oldOrigin);
	}

	virtual rcref<bitmap> create_offscreen_buffer(pane& forPane, const size& sz, std::optional<color> fillColor = std::nullopt)
	{
		COGS_ASSERT(!!m_bridgedPane);
		return m_bridgedPane->create_offscreen_buffer(forPane, sz, fillColor);
	}

	using pane_container::nest;
	virtual void nest_last(const rcref<pane>& child, const rcptr<frame>& f = 0) { pane::nest_last(child, f); }
	virtual void nest_first(const rcref<pane>& child, const rcptr<frame>& f = 0) { pane::nest_first(child, f); }
	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis, const rcptr<frame>& f = 0) { pane::nest_before(child, beforeThis, f); }
	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis, const rcptr<frame>& f = 0) { pane::nest_after(child, afterThis, f); }

public:
	using pane::get_size;

};

inline const rcptr<bridgeable_pane>& bridgeable_pane::get_bridged(const pane_bridge& p)
{
	return p.get_bridged();
}


inline rcref<bridgeable_pane> subsystem::create_native_pane() volatile
{
	return rcnew(bridgeable_pane);
}

}
}



#endif

