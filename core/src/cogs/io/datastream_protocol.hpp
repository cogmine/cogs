//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_IO_DATASTREAM_PROTOCOL
#define COGS_HEADER_IO_DATASTREAM_PROTOCOL


#include "cogs/env.hpp"
#include "cogs/io/datastream.hpp"
#include "cogs/io/filter.hpp"
#include "cogs/mem/rcnew.hpp"


namespace cogs {
namespace io {


/// @ingroup IO
/// @brief A base class for datastream protocols, which filter both directions of a datastream.
class datastream_protocol : public datastream
{
private:
	class source_filter : public filter
	{
	private:
		weak_rcptr<datastream_protocol> m_datastreamProtocol;

	public:
		explicit source_filter(const rcref<datastream_protocol>& dsp)
			: m_datastreamProtocol(dsp)
		{ }

		virtual rcref<task<composite_buffer> > filtering(composite_buffer& src)
		{
			rcptr<datastream_protocol> dsp = m_datastreamProtocol;
			if (!!dsp)
				return signaled(dsp->filtering_source(src));
			composite_buffer empty;
			return signaled(empty);
		}
	};

	class sink_filter : public filter
	{
	private:
		weak_rcptr<datastream_protocol> m_datastreamProtocol;

	public:
		explicit sink_filter(const rcref<datastream_protocol>& dsp)
			: m_datastreamProtocol(dsp)
		{ }

		virtual rcref<task<composite_buffer> > filtering(composite_buffer& src)
		{
			rcptr<datastream_protocol> dsp = m_datastreamProtocol;
			if (!!dsp)
				return signaled(dsp->filtering_sink(src));
			composite_buffer empty;
			return signaled(empty);
		}
	};

	rcref<datastream> m_datastream;
	rcref<filter> m_sourceFilter;
	rcref<filter> m_sinkFilter;

protected:
	virtual composite_buffer filtering_source(composite_buffer& src) = 0;
	virtual composite_buffer filtering_sink(composite_buffer& src) = 0;

public:
	explicit datastream_protocol(const rcref<datastream>& ds)
		: m_datastream(ds),
		m_sourceFilter(rcnew(source_filter)(this_rcref)),
		m_sinkFilter(rcnew(sink_filter)(this_rcref))
	{ }

	virtual void start()
	{
		couple(m_datastream, m_sourceFilter);
		couple(m_sinkFilter, m_datastream);
	}

	const rcref<filter>& get_source_filter() const { return m_sourceFilter; }
	const rcref<filter>& get_sink_filter() const { return m_sinkFilter; }

	const rcref<datastream>& get_datastream() const { return m_datastream; }

	// datastream overrides

	virtual void abort_source() { datastream::abort_source(); m_datastream->abort_source(); }
	virtual void abort_sink() { datastream::abort_sink(); m_datastream->abort_sink(); }

	virtual void abort() { datastream::abort(); m_datastream->abort(); }
};


}
}


#endif
