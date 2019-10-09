//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_GUI_NATIVE_CONTAINER_PANE
#define COGS_HEADER_GUI_NATIVE_CONTAINER_PANE


#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"


namespace cogs {
namespace gui {


class native_container_pane : public pane_bridge, public virtual pane_container
{
protected:
	virtual void installing()
	{
		pane_bridge::install_bridged(get_subsystem()->create_native_pane());
	}

public:
	explicit native_container_pane(const ptr<rc_obj_base>& desc)
		: pane_bridge(desc)
	{
	}

	using pane_container::nest;
	virtual void nest_last(const rcref<pane>& child, const rcptr<frame>& f = 0) { pane::nest_last(child, f); }
	virtual void nest_first(const rcref<pane>& child, const rcptr<frame>& f = 0) { pane::nest_first(child, f); }
	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis, const rcptr<frame>& f = 0) { pane::nest_before(child, beforeThis, f); }
	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis, const rcptr<frame>& f = 0) { pane::nest_after(child, afterThis, f); }
};


}
}


#endif
