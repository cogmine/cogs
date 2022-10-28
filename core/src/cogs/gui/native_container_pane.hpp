//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
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
	typedef pane_bridge::options options;

	native_container_pane()
		: native_container_pane(options())
	{ }

	explicit native_container_pane(options&& o)
		: pane_bridge(std::move(o))
	{ }

	using pane_container::nest;
	virtual void nest_last(const rcref<pane>& child) { pane_bridge::nest_last(child); }
	virtual void nest_first(const rcref<pane>& child) { pane_bridge::nest_first(child); }
	virtual void nest_before(const rcref<pane>& beforeThis, const rcref<pane>& child) { pane_bridge::nest_before(beforeThis, child); }
	virtual void nest_after(const rcref<pane>& afterThis, const rcref<pane>& child) { pane_bridge::nest_after(afterThis, child); }
};


}
}


#endif
