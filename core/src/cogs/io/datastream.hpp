//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_IO_DATASTREAM
#define COGS_HEADER_IO_DATASTREAM


#include "cogs/io/datasource.hpp"
#include "cogs/io/datasink.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/sync/count_down_event.hpp"


namespace cogs {
namespace io {


// A datastream is both a datasource and a datasink.  It represents a single end-point
// of a bi-directional channel (2 different streams, one in each direction).
// A datastream has no repositionable cursor.  Reading from a datastream
// removes the data from the datastream.

/// @ingroup IO
/// @brief A base class for objects that are both a datasource and a datasink, such as a file or network stream.
class datastream : public datasource,  public datasink
{
public:
	COGS_IMPLEMENT_MULTIPLY_DERIVED_OBJECT_GLUE2(datastream, datasource, datasink);

	class closer : public waitable, public object
	{
	private:
		closer() = delete;
		closer(closer&&) = delete;
		closer(const closer&) = delete;
		closer& operator=(closer&&) = delete;
		closer& operator=(const closer&) = delete;

		const weak_rcptr<datastream> m_stream;
		delayed_construction<count_down_event> m_event;

	protected:
		friend class datastream;

		closer(const rcref<datastream>& ds)
			: m_stream(ds)
		{
			placement_rcnew(this_desc, (bypass_constructor_permission<count_down_event>*)&m_event.get(), 2);
		}

		void closing()	{ m_event->count_down(); }

		virtual void dispatch_inner(const rcref<task_base>& t, int priority) volatile
		{
			return dispatcher::dispatch_inner(*m_event, t, priority);
		}

	public:
		const weak_rcptr<datastream>& get_datastream() const				{ return m_stream; }

		virtual int timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile			{ return m_event->timed_wait(timeout, spinCount); }
	};

	virtual void abort()
	{
		abort_source();
		abort_sink();
	}

	rcref<closer> close()
	{
		rcref<closer> c = create_closer(this_rcref);

		close_source()->dispatch([c]()
		{
			c->closing();
		});

		close_sink()->dispatch([c]()
		{
			c->closing();
		});

		return c;
	}

protected:
	virtual rcref<closer> create_closer(const rcref<datastream>& proxy)	{ return rcnew(bypass_constructor_permission<closer>, proxy); }
};


}
}


#endif
