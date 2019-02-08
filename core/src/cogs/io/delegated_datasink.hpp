//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

	
// Status: Good

#ifndef COGS_IO_DELEGATED_DATASINK
#define COGS_IO_DELEGATED_DATASINK


#include "cogs/function.hpp"
#include "cogs/io/datasink.hpp"
#include "cogs/mem/object.hpp"


namespace cogs {
namespace io {


/// @ingroup IO
/// @brief A derived datasink accepting a delegate to handle writing.
class delegated_datasink : public datasink
{
public:
	typedef function<bool(composite_buffer&)> process_write_delegate_t;

private:
	process_write_delegate_t	m_delegate;

	class writer : public datasink::writer
	{
	public:
		const weak_rcptr<delegated_datasink> m_delegatedDatasink;

		writer(const rcref<datasink>& proxy, const rcref<delegated_datasink>& dd)
			: datasink::writer(proxy),
			m_delegatedDatasink(dd)
		{
		}

		virtual void writing()
		{
			rcptr<delegated_datasink> dd = m_delegatedDatasink;
			complete(!dd || !dd->process_write(get_buffer()));
		}
	};

protected:
	// return false to close.
	virtual bool process_write(composite_buffer& compBuf)
	{ 
		return (!!m_delegate) && m_delegate(compBuf);
	}

	virtual rcref<datasink::writer> create_writer(const rcref<datasink>& proxy)
	{
		return rcnew(writer, proxy, this_rcref);
	}

public:
	delegated_datasink()
	{ }

	explicit delegated_datasink(const rcref<io::queue>& q)
		: datasink(q)
	{ }

	explicit delegated_datasink(const process_write_delegate_t& d)
		: m_delegate(d)
	{ }
};


}
}


#endif
