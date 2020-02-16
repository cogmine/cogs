//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_IO_FILTER
#define COGS_HEADER_IO_FILTER


#include "cogs/env.hpp"
#include "cogs/function.hpp"
#include "cogs/io/composite_buffer.hpp"
#include "cogs/io/datasink.hpp"
#include "cogs/io/datasource.hpp"
#include "cogs/mem/object.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {
namespace io {


/// @ingroup IO
/// @brief A base class for an I/O filter.
///
/// A filter is both a datasource and a datasink.  Unlike a datastream, it's
/// not one end of a dual-stream channel.  Instead, it represents 1 channel
/// of communication, that is patched or filtered.  Data is read, modified by
/// the derived implementation of filter::filtering(), and
/// passed on to the next receiver in a chain.
class filter : public datasink, public datasource
{
public:
	COGS_IMPLEMENT_MULTIPLY_DERIVED_OBJECT_GLUE2(filter, datasink, datasource);

	typedef function<rcref<task<composite_buffer> >(composite_buffer&)> filter_func_t;
	typedef function<rcref<task<composite_buffer> >()> finalize_func_t;

private:
	class reader;
	class writer_base;
	class writer;
	class closer;
	class coupler;

	class state : public object
	{
	public:
		const weak_rcptr<filter> m_filter;
		rcptr<writer_base> m_nextWriter;
		rcptr<writer_base> m_writer;
		rcptr<reader> m_nextReader;
		rcptr<reader> m_reader;
		rcptr<closer> m_nextCloser;
		rcptr<coupler> m_nextCoupler;
		rcptr<coupler> m_coupler;
		rcptr<coupler> m_sinkCloseCoupler;
		rcptr<coupler> m_abortCoupler;
		rcptr<reader> m_abortReader;
		rcptr<writer> m_abortWriter;
		rcptr<datasink::writer> m_coupledWriter;
		composite_buffer m_filteredBuffer;
		bool m_finalized = false; // Once finalizing, filter sink should be already closed
		bool m_asyncInProgress = false;
		bool m_writeAborting = false;
		bool m_closing = false;
		bool m_couplerAborted = false;
		bool m_coupledSinkClosed = false;

		size_t preFilteringSize;
		rcptr<task<composite_buffer> > m_filteringTask;
		rcptr<task<composite_buffer> > m_finalizeTask;

		typedef void (state::* task_f)();

		volatile container_queue<task_f> m_completionSerializer;

		state(rc_obj_base& desc, const rcref<filter>& f)
			: object(desc),
			m_filter(f)
		{ }

		void process(task_f t)
		{
			if (m_completionSerializer.append(t))
			{
				bool wasLast;
				for (;;)
				{
					(this->*t)();
					m_completionSerializer.remove_first(wasLast);
					if (wasLast)
						break;
					m_completionSerializer.peek_first(t);
				}
			}
		}

		void processing()
		{
			if (!m_asyncInProgress)
			{
				rcptr<filter> f = m_filter; // filter will be in scope for duration of coupler_process/read_and_write
				if (!!f)
				{
					if (!!m_coupler)
						coupler_process();
					else if (!!m_reader)
						read_and_write();
				}
			}
		}

		void read()
		{
			rcptr<reader> nextReader = m_nextReader;
			m_nextReader.release();
			nextReader->read();
		}

		void read_abort()
		{
			rcptr<reader> abortReader = m_nextReader;
			m_abortReader.release();
			abortReader->read_abort();
		}

		void write()
		{
			rcptr<writer_base> nextWriter = m_nextWriter;
			m_nextWriter.release();
			nextWriter->write();
		}

		void write_abort()
		{
			rcptr<writer> abortWriter = m_abortWriter;
			m_abortWriter.release();
			abortWriter->write_abort();
			if (!m_filteringTask || (abortWriter != m_writer))
				abortWriter->write_abort();
			else
				m_writeAborting = true;
		}

		void close()
		{
			rcptr<closer> nextCloser = m_nextCloser;
			m_nextCloser.release();
			nextCloser->close();
		}

		void sink_closed()
		{
			rcptr<coupler> sinkCloseCoupler = m_sinkCloseCoupler;
			m_sinkCloseCoupler.release();
			sinkCloseCoupler->sink_closed();
		}

		void start()
		{
			m_coupler = m_nextCoupler;
			m_nextCoupler.release();
			m_coupler->start();
		}

		void coupler_abort()
		{
			rcptr<coupler> abortCoupler = m_abortCoupler;
			m_abortCoupler.release();
			abortCoupler->sink_closed();
		}

		void filtering_done()
		{
			if (m_writeAborting)
			{
				m_writer->write_abort();
				m_writeAborting = false;
			}

			m_asyncInProgress = false;

			rcptr<filter> f = m_filter;
			if (!!f)
			{
				bool madeProgress = false;
				const composite_buffer& newBuf = m_filteringTask->get();
				if (!newBuf)
					madeProgress = (preFilteringSize != m_writer->get_buffer().get_length());
				else
				{
					madeProgress = true;
					m_filteredBuffer.append(newBuf);
				}

				m_filteringTask.release();

				if (!madeProgress)
				{
					f->abort_sink(); // If there is a read or write in progress, it will complete/abort with current progress
					start_finalize();
				}
				else
					processing();
			}
		}

		void start_finalize()
		{
			m_finalizeTask = m_filter->finalize();
			m_asyncInProgress = true;
			m_finalized = true;
			m_finalizeTask->dispatch([r{ this_rcref }]()
			{
				r->process(&filter::state::finalize_done);
			});
		}

		void finalize_done()
		{
			m_asyncInProgress = false;
			rcptr<filter> f = m_filter;
			if (!!f)
			{
				m_filteredBuffer.append(m_finalizeTask->get());
				m_finalizeTask.release();
				processing();
			}
		}

		void read_and_write()
		{
			bool completeRead = false;
			do {
				if (!m_filteredBuffer)
				{
					if (!m_writer) // No closer or write, do nothing
					{
						completeRead = (m_reader->get_read_mode() == datasource::read_mode::now);
						break;
					}
					for (;;) // this loop never executes twice
					{
						if (!m_writer->get_unwritten_buffer())
						{
							m_writer->complete();
							m_writer.release();
							return;
						}
						int i = m_writer->fill_buffer();
						if (i == 1)
						{
							if (!m_filteredBuffer)
								continue;
							break;
						}
						if (i == -1)
							return;
						if (!!m_finalized)
							m_filter->abort_source();
						else
						{
							m_filter->abort_sink(); // If there is a read or write in progress, it will complete/abort with current progress
							start_finalize();
						}
						return;
					}
				}

				composite_buffer & readBufferList = m_reader->get_buffer();
				composite_buffer curBufferList = m_filteredBuffer.split_off_before(m_reader->get_unread_size());
				readBufferList.append(curBufferList);

				completeRead = (readBufferList.get_length() == m_reader->get_requested_size())
					|| (m_reader->get_read_mode() == datasource::read_mode::now)
					|| ((m_reader->get_read_mode() == datasource::read_mode::some) && (m_reader->get_read_size() != 0));

			} while (!completeRead); // one, the other, or both, will complete.

			if (completeRead)
			{
				m_reader->complete();
				m_reader.release();
			}
		}

		void complete_coupled_write()
		{
			m_asyncInProgress = false;

			bool completeCoupler = m_couplerAborted || m_coupledSinkClosed;
			size_t n = m_coupledWriter->get_write_size();
			if (!!n)
			{
				m_filteredBuffer.advance(n);
				if (m_coupler->m_hasMaxLength)
				{
					m_coupler->m_remainingLength -= n;
					if (!m_coupler->m_remainingLength)
					{
						m_coupler->m_onSinkAbortTask->cancel();
						completeCoupler = true;
					}
				}
			}

			if (completeCoupler)
			{
				m_coupler->complete(); // Needed to wait to complete the coupler until the write completed
				m_coupler.release();
			}

			// If m_couplerAborted, we don't care if sink closed, as the user explicitly removed this coupler already.
			if (m_coupledSinkClosed) // source queue was aborted
			{
				if (m_coupler->m_closeSinkOnSourceClose)
					m_coupler->m_sink->close_sink();
				m_coupledSinkClosed = false;
			}
			else if (!m_couplerAborted && m_coupledWriter->was_closed())
			{
				if (m_coupler->m_closeSourceOnSinkClose)
				{
					rcptr<filter> f = m_filter;
					if (!!f)
						f->abort_source();
				}
			}

			m_couplerAborted = false;
			m_coupledWriter.release();

			processing();
		}

		// Called when coupler executes, or from a completing coupled write, or from a writer or closer on the datasink queue while there is a coupler
		void coupler_process()
	{
			if (!m_filteredBuffer && !!m_writer) // No closer or write, do nothing
			{
				for (;;) // this loop never executes twice
				{
					if (!m_writer->get_unwritten_buffer())
					{
						m_writer->complete();
						m_writer.release();
						return;
					}
					int i = m_writer->fill_buffer();
					if (i == 1)
					{
						if (!m_filteredBuffer)
							continue;
						break;
					}
					if (i == -1)
						return;
					if (!!m_finalized)
						m_filter->abort_source();
					else
					{
						m_filter->abort_sink(); // If there is a read or write in progress, it will complete/abort with current progress
						start_finalize();
					}
					return;
				}
			}

			composite_buffer * bufPtr = &m_filteredBuffer;
			composite_buffer tmpBuf;
			if (m_coupler->m_hasMaxLength && (m_coupler->m_remainingLength < m_filteredBuffer.get_length()))
			{
				tmpBuf = m_filteredBuffer.subrange(0, m_coupler->m_remainingLength.get_int<size_t>());
				bufPtr = &tmpBuf;
			}
			m_coupledWriter = m_coupler->m_sink->write(*bufPtr);
			m_asyncInProgress = true;
			m_coupledWriter->dispatch([this]()
			{
				process(&filter::state::complete_coupled_write);
			});
		}

		void shutdown()
		{
			if (!!m_coupledWriter)
				m_coupledWriter->abort();

			if (!!m_finalizeTask)
				m_finalizeTask->cancel();
			else if (!!m_filteringTask)
			{
				// If filtering, a write would fail to complete aborting.
				// Cancel filtering, and if successful, pass along to allow cleanup
				m_filteringTask->cancel()->dispatch([r{ this_rcref }](bool isCancelled)
				{
					if (isCancelled)
					{
						r->process(&filter::state::filtering_done);
					}
				});
			}
		}
	};

	const rcref<state> m_state;

	class reader : public datasource::reader
	{
	public:
		friend class filter::state;

		const rcref<filter::state> m_state;

		reader(rc_obj_base& desc, const rcref<datasource>& proxy, const rcref<filter::state>& s)
			: datasource::reader(desc, proxy),
			m_state(s)
		{ }

		composite_buffer& get_buffer() { return datasource::reader::get_buffer(); }

		virtual void reading()
		{
			m_state->m_nextReader = this_rcref;
			m_state->process(&filter::state::read);
		}

		virtual void aborting()
		{
			m_state->m_abortReader = this_rcref;
			m_state->process(&filter::state::read_abort);
		}

		void read()
		{
			if (!is_complete())
			{
				rcptr<filter> f = m_state->m_filter;
				if (!f)
					complete();
				else
				{
					m_state->m_reader = this_rcref;
					m_state->processing();
				}
			}
		}

		void read_abort()
		{
			if (!is_complete())
			{
				complete();
				m_state->m_reader.release();
			}
		}
	};

	class writer_base : public datasink::writer
	{
	public:
		friend class filter::state;

		const rcref<filter::state> m_state;

		writer_base(rc_obj_base& desc, const rcref<datasink>& proxy, const rcref<filter::state>& s)
			: datasink::writer(desc, proxy),
			m_state(s)
		{ }

		// Returns 1 if progress is made, 0 if not, -1 if asynchronous
		virtual int fill_buffer() = 0;

		virtual void writing()
		{
			m_state->m_nextWriter = this_rcref;
			m_state->process(&filter::state::write);
		}

		virtual void aborting()
		{
			m_state->process(&filter::state::write_abort);
		}

		void write()
		{
			if (!is_complete())
			{
				rcptr<filter> f = m_state->m_filter;
				if (!f)
					complete();
				else
				{
					m_state->m_writer = this_rcref;
					m_state->processing();
				}
			}
		}

		void write_abort()
		{
			if (!is_complete())
			{
				complete();
				m_state->m_writer.release();
			}
		}
	};

	class writer : public writer_base
	{
	public:
		writer(rc_obj_base& desc, const rcref<datasink>& proxy, const rcref<filter::state>& s)
			: writer_base(desc, proxy, s)
		{ }

		virtual int fill_buffer()
		{
			rcptr<filter> f = m_state->m_filter;
			if (!!f)
			{
				composite_buffer& writeBuf = get_buffer();
				if (!!writeBuf)
				{
					m_state->preFilteringSize = writeBuf.get_length();
					m_state->m_filteringTask = f->filtering(writeBuf);
					m_state->m_asyncInProgress = true;
					m_state->m_filteringTask->dispatch([r{ this_rcref }]()
					{
						r->m_state->process(&filter::state::filtering_done);
					});
					return -1;
				}
			}
			return 0;
		}
	};

	class bypass_writer : public writer_base
	{
	public:
		bypass_writer(rc_obj_base& desc, const rcref<datasink>& proxy, const rcref<filter::state>& s)
			: writer_base(desc, proxy, s)
		{ }

		/// Empty writes complete without calling writing(), so we know this will add to m_filteredBuffer
		virtual int fill_buffer()
		{
			rcptr<filter> f = m_state->m_filter;
			if (!!f)
			{
				composite_buffer& writeBuf = get_buffer();
				if (!!writeBuf)
				{
					m_state->m_filteredBuffer.append(writeBuf);
					writeBuf.clear();
					return 1;
				}
			}

			return 0;
		}
	};

	class closer : public datasink::closer
	{
	public:
		friend class filter::state;

		const rcref<filter::state> m_state;

		closer(rc_obj_base& desc, const rcref<datasink>& proxy, const rcref<filter::state>& s)
			: datasink::closer(desc, proxy),
			m_state(s)
		{ }

		virtual void closing()
		{
			m_state->m_nextCloser = this_rcref;
			m_state->process(&filter::state::close);
		}

		void close()
		{
			rcptr<filter> f = m_state->m_filter;
			if (!!f)
			{
				if (!m_state->m_finalized)
					m_state->start_finalize();
			}
			complete(true); // close filter sink queue
		}
	};


	// One purpose of the coupler is to stop up our datasource io::queue
	// until a coupled datasink has closed or been decoupled, to ensure nothing else can read/write while coupled.
	class coupler : public io::queue::io_task<void>
	{
	public:
		friend class filter::state;

		const rcref<datasink::transaction> m_sink;
		const rcref<filter::state> m_state;
		const bool m_closeSinkOnSourceClose; // If the filter's source queue closes, close the coupled sink.
		const bool m_closeSourceOnSinkClose; // If the coupled sink closes, close the filter.
		const bool m_hasMaxLength;

		dynamic_integer m_remainingLength;
		rcptr<task<void> > m_onSinkAbortTask;
		bool isCompleting = true;

		coupler(
			rc_obj_base& desc,
			const rcref<filter::state>& s,
			const rcref<datasink>& snk,
			bool closeSinkOnSourceClose = false,
			bool closeSourceOnSinkClose = false,
			const dynamic_integer& maxLength = dynamic_integer())
				: io::queue::io_task<void>(desc),
				m_sink(rcnew(datasink::transaction, snk)),
				m_state(s),
				m_closeSinkOnSourceClose(closeSinkOnSourceClose),
				m_closeSourceOnSinkClose(closeSourceOnSinkClose),
				m_hasMaxLength(!!maxLength),
				m_remainingLength(maxLength)
		{
			m_onSinkAbortTask = m_sink->get_sink_close_event().dispatch([r{ this_weak_rcptr }]()
			{
				rcptr<coupler> r2 = r;
				if (!!r2)
				{
					r2->m_state->m_sinkCloseCoupler = r2;
					r2->m_state->process(&filter::state::sink_closed);
				}
			});
		}

		void sink_closed()
		{
			if (!is_complete())
			{
				COGS_ASSERT(!m_state->m_coupledSinkClosed);
				if (!m_state->m_couplerAborted)
				{
					if (m_state->m_coupler == this)
					{
						if (!!m_state->m_coupledWriter) // If there is a write in progress, it will handle sink close
							m_state->m_coupledSinkClosed = true; // The sink closed, so no need to abort.
						else
						{
							complete(m_closeSourceOnSinkClose);
							m_state->m_coupler.release();
						}
					}
				}
			}
		}

		virtual void start_coupler()
		{
			m_state->m_filter->source_enqueue(this_rcref);
		}

		virtual void executing()
		{
			m_state->m_nextCoupler = this_rcref;
			m_state->process(&filter::state::start);
		}

		virtual void aborting()
		{
			m_state->m_abortCoupler = this_rcref;
			m_state->process(&filter::state::coupler_abort);
		}

		void start()
		{
			if (!is_complete())
			{
				rcptr<filter> f = m_state->m_filter;
				if (!f)
					complete();
				else
				{
					m_state->m_coupler = this_rcref;
					m_state->processing();
				}
			}
		}

		void coupler_abort()
		{
			if (!is_complete())
			{
				m_onSinkAbortTask->cancel();

				// If wasClosed, then the whole source filter got closed unexpectedly.  (We would have completed coupler, if we closed it)
				// If !wasClosed, then the caller is cancelling the coupler.  DONT PROP
				bool wasClosed = was_closed();
				if (!m_state->m_coupledWriter)
				{
					if (wasClosed && m_closeSinkOnSourceClose)
						m_sink->close_sink();
					complete();
					m_state->m_coupler.release();
				}
				else
				{
					if (wasClosed) // Needed to wait to complete the coupler until the write completes
						m_state->m_coupledSinkClosed = true;
					else
						m_state->m_couplerAborted = true;
					m_state->m_coupledWriter->abort();
				}
			}
		}
	};

	virtual rcref<datasource::reader> create_reader(const rcref<datasource>& proxy) { return rcnew(reader, proxy, m_state); }
	virtual rcref<datasink::writer> create_writer(const rcref<datasink>& proxy) { return rcnew(writer, proxy, m_state); }

	virtual rcref<datasink::closer> create_sink_closer(const rcref<datasink>& proxy)
	{
		return rcnew(closer, proxy, m_state);
	}

	virtual rcref<task<void> > create_coupler(
		const rcref<datasink>& snk,
		bool closeSinkOnSourceClose = false,
		bool closeSourceOnSinkClose = false,
		const dynamic_integer& maxLength = dynamic_integer(),
		size_t = COGS_DEFAULT_BLOCK_SIZE)
	{
		rcref<task<void> > result = rcnew(coupler, m_state, snk, closeSinkOnSourceClose, closeSourceOnSinkClose, maxLength);
		result.template static_cast_to<coupler>()->start_coupler();
		return result;
	}

	filter_func_t m_filterFunc;
	finalize_func_t m_finalizeFunc;

public:
	~filter()
	{
		// Maybe we don't need shutdown, as coupler will do the right thing?
		// Any pending reads and write will also get cancelled, as queues are gone
		m_state->process(&filter::state::shutdown);
	}

	/// @brief Writes data that bypasses the filter.
	rcref<datasink::writer> bypass(const composite_buffer& compBuf)
	{
		rcref<datasink::writer> w = rcnew(bypass_writer, this_rcref, m_state);
		datasink::write(compBuf, w);
		return w;
	}

	/// @brief Constructor
	explicit filter(rc_obj_base& desc)
		: datasink(desc),
		datasource(desc),
		m_state(rcnew(state, this_rcref))
	{ }

	filter(rc_obj_base& desc, const filter_func_t& f)
		: datasink(desc),
		datasource(desc),
		m_state(rcnew(state, this_rcref)),
		m_filterFunc(f)
	{ }

	filter(rc_obj_base& desc, filter_func_t&& f)
		: datasink(desc),
		datasource(desc),
		m_state(rcnew(state, this_rcref)),
		m_filterFunc(std::move(f))
	{ }

	filter(rc_obj_base& desc, const filter_func_t& f, const finalize_func_t& fn)
		: datasink(desc),
		datasource(desc),
		m_state(rcnew(state, this_rcref)),
		m_filterFunc(f),
		m_finalizeFunc(fn)
	{ }

	filter(rc_obj_base& desc, filter_func_t&& f, const finalize_func_t& fn)
		: datasink(desc),
		datasource(desc),
		m_state(rcnew(state, this_rcref)),
		m_filterFunc(std::move(f)),
		m_finalizeFunc(fn)
	{ }

	filter(rc_obj_base& desc, const filter_func_t& f, finalize_func_t&& fn)
		: datasink(desc),
		datasource(desc),
		m_state(rcnew(state, this_rcref)),
		m_filterFunc(f),
		m_finalizeFunc(fn)
	{ }

	filter(rc_obj_base& desc, filter_func_t&& f, finalize_func_t&& fn)
		: datasink(desc),
		datasource(desc),
		m_state(rcnew(state, this_rcref)),
		m_filterFunc(std::move(f)),
		m_finalizeFunc(fn)
	{ }

protected:
	/// @brief Derived class must override filtering() to filter input data
	///
	/// If filtering() returns without reading the entire buffer,
	/// it will not be called again until the previously returned data has been entirely process.
	///
	/// If filtering() returns an empty block, the filter is closed.
	///
	/// @param src Input data to filter.  Data should be removed from this buffer to indicate progress.
	/// @return A buffer containing filtered data
	virtual rcref<task<composite_buffer> > filtering(composite_buffer& src)
	{
		if (!!m_filterFunc)
			return m_filterFunc(src);
		return signaled(src);
	}

	/// @brief Derived class may override finalize() to return a final buffer to terminate the filtered stream with.
	///
	/// The default implemention returns an empty buffer.
	/// @return A final buffer to terminate the filtered stream with.
	virtual rcref<task<composite_buffer> > finalize()
	{
		if (!!m_finalizeFunc)
			return m_finalizeFunc();

		composite_buffer emptyBuffer;
		return signaled(emptyBuffer);
	}
};


}
}


#endif
