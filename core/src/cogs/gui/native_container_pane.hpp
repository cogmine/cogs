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
	using pane_container::nest;
	virtual void nest_last(const rcref<pane>& child) { pane::nest_last(child); }
	virtual void nest_first(const rcref<pane>& child) { pane::nest_first(child); }
	virtual void nest_before(const rcref<pane>& child, const rcref<pane>& beforeThis) { pane::nest_before(child, beforeThis); }
	virtual void nest_after(const rcref<pane>& child, const rcref<pane>& afterThis) { pane::nest_after(child, afterThis); }
};


}
}


#endif
