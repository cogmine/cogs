//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_GUI_NATIVE_CONTAINER_PANE
#define COGS_GUI_NATIVE_CONTAINER_PANE


#include "cogs/gui/pane.hpp"
#include "cogs/gui/pane_bridge.hpp"


namespace cogs {
namespace gui {

#pragma warning(push)
#pragma warning (disable: 4250)

class native_container_pane : public pane_bridge, public virtual pane_container
{
protected:
	virtual void installing()
	{
		pane_bridge::install_bridged(get_subsystem()->create_native_pane());
	}

	native_container_pane(const alignment& a)
		: pane_bridge(a)
	{
	}

public:
	static rcref<native_container_pane> create(const alignment& a = alignment::center())
	{
		return rcnew(bypass_constructor_permission<native_container_pane>, a);
	}

	using pane::nest;
	using pane::nest_last;
	using pane::nest_first;
	using pane::nest_before;
	using pane::nest_after;

	using pane::get_pane_container;
	using pane::get_pane_container_ref;
};

#pragma warning(pop)

}
}


#endif
