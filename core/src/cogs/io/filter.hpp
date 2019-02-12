//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_IO_FILTER
#define COGS_IO_FILTER


#include "cogs/env.hpp"
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

private:
	class reader;
	class writer_base;
	class writer;
	class closer;
	class coupler;
	class state;

	class state : public object
	{
	public:
		const weak_rcptr<filter>	m_filter;
		rcptr<writer_base>			m_nextWriter;
		rcptr<writer_base>			m_writer;
		rcptr<reader>				m_nextReader;
		rcptr<reader>				m_reader;
		rcptr<closer>				m_nextCloser;
		rcptr<closer>				m_closer;
		rcptr<coupler>				m_nextCoupler;
		rcptr<coupler>				m_coupler;
		rcptr<datasink::writer>		m_coupledWriter;
		composite_buffer			m_filteredBuffer;
		bool						m_sinkClosing;
		bool						m_decoupling;
		bool						m_finalized;

		int releaseLocation = 0;

		typedef void (state::*task_f)();

		volatile container_queue<task_f>	m_completionSerializer;

		void process(task_f t)
		{
			if (m_completionSerializer.append(t))
			{
				for (;;)
				{
					(this->*t)();

					bool wasLast;
					m_completionSerializer.remove_first(wasLast);
					if (wasLast)
						break;
					m_completionSerializer.peek_first(t);
				}
			}
		}

		void read_abort()
		{
			m_reader->read_abort();
		}

		void read()
		{
			rcptr<reader> nextReader = m_nextReader;
			m_nextReader.release();
			nextReader->read();
		}

		void write_abort()
		{
			m_writer->write_abort();
		}

		void write()
		{
			rcptr<writer_base> nextWriter = m_nextWriter;
			m_nextWriter.release();
			nextWriter->write();
		}

		void close_abort()
		{
			m_closer->close_abort();
		}

		void close()
		{
			rcptr<closer> nextCloser = m_nextCloser;
			m_nextCloser.release();
			nextCloser->close();
		}

		void sink_aborted()
		{
			if (!!m_coupler)
				m_coupler->sink_aborted();
		}

		void start()
		{
			m_coupler = m_nextCoupler;
			m_nextCoupler.release();
			m_coupler->start();
		}

		void decouple_inner()
		{
			m_coupler->decouple();
		}

		state(const rcref<filter>& f) : m_filter(f), m_sinkClosing(false), m_finalized(false)
		{
		}

		// Called when a reader executes on the datasource queue, or from a writer or closer on the datasink queue while there is a reader
		void read_and_write()
		{
			//COGS_ASSERT(!!m_reader);
			//COGS_ASSERT(!m_writer || !m_closer);	// Zero or one of them, as datasink is a queue, so serial
			bool closing = false;
			bool completeWrite = false;
			bool completeRead = false;
			do {
				if (!m_filteredBuffer)	
				{
					for (;;)	// this loop never executes twice
					{
						if (!m_closer)
						{
							if (!m_writer)	// No closer or write, do nothing 
							{
								completeRead = (m_reader->get_read_mode() == datasource::read_now);
								break;	// all the way out? - !m_filteredBuffer
							}

							if (!m_writer->get_unwritten_buffer())
							{
								completeWrite = true;
								break;	// All the way out? - !m_filteredBuffer
							}

							if (m_writer->fill_buffer())
								break;	// inner - !!m_filteredBuffer
							//...
						}

						if (!m_finalized)
						{
							m_filteredBuffer = m_filter->finalize();
							m_finalized = true;
						}

						if (!m_filteredBuffer)
						{
							closing = true;
							completeRead = true;
							completeWrite = true;
							break; // All the way out? - !m_filteredBuffer
						}

						break;	// inner - !!m_filteredBuffer
					}

					if (!m_filteredBuffer)
						break;
				}

				composite_buffer& readBufferList = m_reader->get_buffer();
				composite_buffer curBufferList = m_filteredBuffer.split_off_before(m_reader->get_unread_size());
				readBufferList.append(curBufferList);

				completeRead = (readBufferList.get_length() == m_reader->get_requested_size())
					|| (m_reader->get_read_mode() == datasource::read_now)
					|| ((m_reader->get_read_mode() == datasource::read_some) && (m_reader->get_read_size() != 0));
			} while (!completeWrite && !completeRead);	// one, the other, or both, will complete.
			
			if (completeWrite)
			{
				if (!!m_closer)
				{
					m_closer->complete();
					m_closer.release();
				}
				else
				{
					if (closing)
						m_filter->close_sink();
					if (!!m_writer)
					{
						m_writer->complete();
						m_writer.release();
					}
				}
			}
			
			if (completeRead)
			{
				if (closing)
					m_filter->close_source();
				m_reader->complete();
				m_reader.release();
			}
		}

		// Called when coupler executes, or from a completing coupled write, or from a writer or closer on the datasink queue while there is a coupler
		void coupler_process(bool completingCoupledWrite = false)
		{
			if (!m_coupler)
				return;

			//COGS_ASSERT(!!m_reader);
			//COGS_ASSERT(!m_writer || !m_closer);	// Zero or one of them, as datasink is a queue, so serial
			bool closing = false;
			if (!!m_coupledWriter)
			{
				if (!completingCoupledWrite)
					return;	// Ignore situation where writer or closer is aborted then another is added.

				COGS_ASSERT(!!m_writer);
				size_t n = m_coupledWriter->get_write_size();
				if (!!n)
				{
					m_filteredBuffer.advance(n);
					if (m_coupler->m_hasMaxLength)
					{
						m_coupler->m_remainingLength -= n;
						if (!m_coupler->m_remainingLength)
							m_decoupling = true;
					}
				}
				
				if (m_coupledWriter->was_closed())
				{
					m_decoupling = true;
					closing = m_coupler->m_closeSourceOnSinkClose;
				}
				m_coupledWriter.release();
			}

			bool completeWrite = false;
			if (!m_decoupling)
			{
				for (;;)
				{
					if (!m_filteredBuffer)
					{
						for (;;)	// this loop never executes twice
						{
							if (!m_closer)
							{
								if (!m_writer)	// No closer or write, do nothing 
									break;	// all the way out? - !m_filteredBuffer

								if (!m_writer->get_unwritten_buffer())
								{
									completeWrite = true;
									break;	// All the way out? - !m_filteredBuffer
								}

								if (m_writer->fill_buffer())
									break;	// inner - !!m_filteredBuffer
								//...
							}

							if (!m_finalized)
							{
								m_filteredBuffer = m_filter->finalize();
								m_finalized = true;
							}

							if (!m_filteredBuffer)
							{
								closing = true;
								m_decoupling = true;
								completeWrite = true;
								if (m_coupler->m_closeSinkOnSourceClose)
									m_coupler->m_sink->close_sink();
								break; // All the way out? - !m_filteredBuffer
							}

							break;	// inner - !!m_filteredBuffer
						}

						if (!m_filteredBuffer)
							break;
					}

					composite_buffer* bufPtr = &m_filteredBuffer;
					composite_buffer tmpBuf;
					if (m_coupler->m_hasMaxLength && (m_coupler->m_remainingLength < m_filteredBuffer.get_length()))
					{
						tmpBuf = m_filteredBuffer.subrange(0, m_coupler->m_remainingLength.get_int<size_t>());
						bufPtr = &tmpBuf;
					}
					m_coupledWriter = m_coupler->m_sink->write(*bufPtr);
					m_coupledWriter->dispatch([this]()
					{
						process(&state::complete_coupled_write);
					});
					return;
				}
			}

			if (completeWrite)
			{
				if (!!m_closer)
				{
					m_closer->complete();
					m_closer.release();
				}
				else
				{
					if (closing)
						m_filter->close_sink();
					if (!!m_writer)
					{
						m_writer->complete();
						m_writer.release();
					}
				}
			}

			if (m_decoupling)
			{
				m_coupler->m_sinkAbortDispatched->cancel();
				if (closing)
					m_filter->close_source();
				m_coupler->complete();
				m_coupler.release();
				releaseLocation = 1;
				if (!!m_coupledWriter)
					m_coupledWriter->abort();
			}
		}

		void complete_coupled_write()
		{
			rcptr<filter> f = m_filter;
			if (!!f)
				coupler_process(true);
		}

		void coupled_sink_aborted()
		{
			rcptr<filter> f = m_filter;
			if (!!f)
			{
				m_decoupling = true;
				if (!m_coupledWriter)	// Otherwise, it will clean up itself
				{
					if (!!m_coupler)
					{
						if (m_coupler->m_closeSourceOnSinkClose)
							m_filter->close_source();
						m_coupler->complete();
						m_coupler.release();
						releaseLocation = 2;
					}
				}
			}
		}

		void shutdown()
		{
			// The filter is gone.  No subsequent events will be processed
			if (!!m_coupledWriter)			// Since the filter is gone, abort.
			{
				m_coupledWriter->abort();	// If it needed to finish, it should have been kept in scope.
				m_coupledWriter.release();
			}

			if (!!m_coupler)
			{
				m_coupler->m_sinkAbortDispatched->cancel();
				m_coupler->complete();
				m_coupler.release();
				releaseLocation = 3;
			}
			else if (!!m_reader)			// A reader would not be present if there were a coupler
			{
				m_reader->complete();
				m_reader.release();
			}
			else
				return;		// A writer would not be present if there were a reader

			if (!!m_writer)
			{
				m_writer->complete();
				m_writer.release();
			}
		}

		void decouple()
		{
			if (!!m_coupler)
			{
				m_decoupling = true;
				m_coupler->m_sinkAbortDispatched->cancel();
				if (!!m_coupledWriter)
					m_coupledWriter->abort();
				else
				{
					m_coupler->complete();
					m_coupler.release();
					releaseLocation = 4;
				}
			}
		}
	};

	const rcref<state>	m_state;

	class reader : public datasource::reader
	{
	public:
		friend class state;

		const rcref<filter::state>	m_state;

		reader(const rcref<datasource>& proxy, const rcref<filter::state>& s)
			: datasource::reader(proxy),
			m_state(s)
		{ }

		composite_buffer& get_buffer() { return datasource::reader::get_buffer(); }

		virtual void aborting()
		{ 
			m_state->process(&state::read_abort);
		}

		virtual void reading()
		{
			m_state->m_nextReader = this_rcref;
			m_state->process(&state::read);
		}

		void read_abort()
		{
			if (!is_complete())
			{
				// There is never both a reader and writer pending at the same time.  If there was a writer,
				// a reader wouldn't need to be waiting.
				// Just complete the reader.
				complete();
				m_state->m_reader.release();
			}
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
					m_state->read_and_write();
				}
			}
		}
	};

	class writer_base : public datasink::writer
	{
	public:
		friend class state;

		const rcref<filter::state>	m_state;

		writer_base(const rcref<datasink>& proxy, const rcref<filter::state>& s)
			: datasink::writer(proxy),
			m_state(s)
		{ }

		virtual bool fill_buffer() = 0;

		virtual void aborting()
		{
			m_state->process(&state::write_abort);
		}

		virtual void writing()
		{
			m_state->m_nextWriter = this_rcref;
			m_state->process(&state::write);
		}

		void write_abort()
		{
			if (!is_complete())
			{
				complete();
				m_state->m_writer.release();
			}
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
					if (!!m_state->m_reader)
						m_state->read_and_write();
					else if (!!m_state->m_coupler)
						m_state->coupler_process();
				}
			}
		}
	};

	class writer : public writer_base
	{
	public:
		writer(const rcref<datasink>& proxy, const rcref<filter::state>& s)
			: writer_base(proxy, s)
		{ }

		virtual bool fill_buffer()
		{
			bool anyWriteProgress = false;
			rcptr<filter> f = m_state->m_filter;
			if (!!f)
			{
				composite_buffer& writeBuf = get_buffer();
				while (!!writeBuf)
				{
					size_t preSize = writeBuf.get_length();
					m_state->m_filteredBuffer.append(f->filtering(writeBuf));
					anyWriteProgress = (!!m_state->m_filteredBuffer);
					if (anyWriteProgress || (preSize == writeBuf.get_length()))
						break;
				}
			}
			return anyWriteProgress;
		}
	};

	class bypass_writer : public writer_base
	{
	public:
		bypass_writer(const rcref<datasink>& proxy, const rcref<filter::state>& s)
			: writer_base(proxy, s)
		{ }

		/// Empty writes complete without calling writing(), so we know this will add to m_filteredBuffer
		virtual bool fill_buffer()
		{
			composite_buffer& writeBuf = get_buffer();
			m_state->m_filteredBuffer.append(writeBuf);
			writeBuf.clear();
			return true;
		}
	};

	class closer : public datasink::closer
	{
	public:
		friend class state;

		const rcref<filter::state>	m_state;
		bool						m_finalized;

		closer(const rcref<datasink>& proxy, const rcref<filter::state>& s)
			: datasink::closer(proxy),
			m_state(s),
			m_finalized(false)
		{ }

		virtual void aborting()
		{
			m_state->process(&state::close_abort);
		}

		virtual void closing()
		{
			m_state->m_nextCloser = this_rcref;
			m_state->process(&state::close);
		}

		void close_abort()
		{
			if (!is_complete())
			{
				complete();
				m_state->m_closer.release();
			}
		}

		void close()
		{
			if (!is_complete())
			{
				rcptr<filter> f = m_state->m_filter;
				if (!f)
				{
					complete();	
					m_state->m_closer.release();
				}
				else
				{
					m_state->m_closer = this_rcref;
					if (!!m_state->m_reader)
						m_state->read_and_write();
					else if (!!m_state->m_coupler)
						m_state->coupler_process();
				}
			}
		}
	};


	// One purpose of the coupler is to stop up our datasource io::queue
	// until a coupled datasink has closed or been decoupled, to ensure nothing else can read/write while coupled.
	class coupler : public io::queue::task<void>
	{
	public:
		friend class state;

		const rcref<datasink::transaction>	m_sink;
		const rcref<filter::state> m_state;
		const bool m_closeSinkOnSourceClose;
		const bool m_closeSourceOnSinkClose;
		const bool m_hasMaxLength;

		dynamic_integer m_remainingLength;
		rcptr<cogs::task<void> > m_sinkAbortDispatched;

		coupler(
			const rcref<filter::state>& s,
			const rcref<datasource>& src,
			const rcref<datasink>& snk,
			bool closeSinkOnSourceClose = false,
			bool closeSourceOnSinkClose = false,
			const dynamic_integer& maxLength = dynamic_integer(),
			size_t bufferBlockSize = COGS_DEFAULT_BLOCK_SIZE)
				: m_state(s),
				m_hasMaxLength(!!maxLength),
				m_remainingLength(maxLength),
				m_sink(datasink::transaction::create(snk)),
				m_closeSinkOnSourceClose(closeSinkOnSourceClose),
				m_closeSourceOnSinkClose(closeSourceOnSinkClose)
		{
			m_sinkAbortDispatched = m_sink->get_sink_close_event()->dispatch([this]()
			{
				m_state->process(&state::sink_aborted);
			});
		}
		
		virtual void start_coupler()
		{
			m_state->m_filter->source_enqueue(this_rcref);
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
					m_state->m_decoupling = false;
					m_state->m_coupler = this_rcref;
					m_state->coupler_process();
				}
			}
		}

		void decouple_inner()
		{
			if (!is_complete())
			{
				rcptr<filter> f = m_state->m_filter;
				if (!f)
				{
					complete();
					m_state->m_coupler.release();
					m_state->releaseLocation = 5;
				}
				else
					m_state->decouple();
			}
		}

		void sink_aborted()
		{
			if (!is_complete())
			{
				rcptr<filter> f = m_state->m_filter;
				if (!f)
				{
					complete();
					m_state->m_coupler.release();
					m_state->releaseLocation = 6;
				}
				else
					m_state->coupled_sink_aborted();
			}
		}

		virtual void aborting()
		{
			m_state->process(&state::decouple_inner);
		}

		virtual void executing()
		{
			m_state->m_nextCoupler = this_rcref;
			m_state->process(&state::start);
		}

		void decouple()
		{
			m_state->process(&state::decouple_inner);
		}

		virtual bool cancel() volatile
		{
			if (signallable_task<void>::cancel())
			{
				// we manage thread safety, so cast away volatility
				((coupler*)this)->decouple();
				return true;
			}

			return false;
		}
	};

	virtual rcref<datasource::reader> create_reader(const rcref<datasource>& proxy)	{ return rcnew(reader, proxy, m_state); }
	virtual rcref<datasink::writer> create_writer(const rcref<datasink>& proxy)		{ return rcnew(writer, proxy, m_state); }

	virtual rcref<datasink::closer> create_sink_closer(const rcref<datasink>& proxy)
	{
		return rcnew(closer, proxy, m_state);
	}

	virtual rcref<cogs::task<void> > create_coupler(
		const rcref<datasink>& snk, 
		bool closeSinkOnSourceClose = false,
		bool closeSourceOnSinkClose = false,
		const dynamic_integer& maxLength = dynamic_integer(),
		size_t bufferBlockSize = COGS_DEFAULT_BLOCK_SIZE)
	{
		rcref<cogs::task<void> > result = rcnew(coupler, m_state, this_rcref, snk, closeSinkOnSourceClose, closeSourceOnSinkClose, maxLength, bufferBlockSize);
		result.static_cast_to<coupler>()->start_coupler();
		return result;
	}

public:
	/// @brief Constructor
	filter()
		: m_state(rcnew(state, this_rcref))
	{ }

	~filter()
	{
		m_state->process(&state::shutdown);
	}

	/// @brief Writes data that bypasses the filter.
	rcref<datasink::writer> bypass(const composite_buffer& compBuf)
	{
		rcref<datasink::writer> w = rcnew(bypass_writer, this_rcref, m_state);
		datasink::write(compBuf, w);
		return w;
	}

protected:
	/// @brief Derived class must override filtering() to filter input data
	/// 
	/// If filtering() returns without reading the entire buffer,
	///	it will not be called again until the previously returned data has been entirely process.
	///
	/// If filtering() returns an empty block, the filter is closed.
	/// 
	/// @param src Input data to filter.  Data should be removed from this buffer to indicate progress.
	/// @return A buffer containing filtered data
	virtual composite_buffer filtering(composite_buffer& src) = 0;

	/// @brief Derived class may override finalize() to return a final buffer to terminate the filtered stream with.
	/// 
	/// The default implemention returns an empty buffer.
	/// @return A final buffer to terminate the filtered stream with.
	virtual composite_buffer finalize()
	{
		composite_buffer emptyBuffer;
		return emptyBuffer;
	}
};


class datasource_facade : public datasource
{
protected:
	volatile container_queue<function<void()> > m_serialQueue;

	void process()
	{
		for (;;)
		{
			function<void()> f;
			m_serialQueue.peek(f);
			f();
			bool wasLast;
			m_serialQueue.remove_first(wasLast);
			if (wasLast)
				break;
			// continue
		}
	}

	class facade_reader : public datasource::reader
	{
	public:
		weak_rcptr<datasource_facade> m_facade;

		facade_reader(const rcref<datasource>& proxy, const rcref<datasource_facade>& f)
			: datasource::reader(proxy),
			m_facade(f)
		{
		}
		
		virtual void reading()
		{
			rcptr<datasource_facade> f = m_facade;
			if (!f)
			{
				complete(true);
				return;
			}

			bool wasEmpty = f->m_serialQueue.append([r{ this_rcref }]()
			{
				rcptr<datasource_facade> f2 = r->m_facade;
				if (!!f2)
				{
					COGS_ASSERT(!f2->m_pendingReader);
					f2->m_pendingReader = r;
					r->process_read(*f2);
				}
			});
			if (wasEmpty)
				f->process();
		}

		virtual void aborting()
		{
			rcptr<datasource_facade> f = m_facade;
			if (!f)
			{
				complete(true);
				return;
			}

			bool wasEmpty = f->m_serialQueue.append([r{ this_rcref }]()
			{
				rcptr<datasource_facade> f2 = r->m_facade;
				if (!f2)
					r->complete(true);
				else
				{
					if (f2->m_pendingReader == r)	// If abort didn't arrive after completion
					{
						r->complete(false);
						f2->m_pendingReader.release();
					}
				}
			});
			if (wasEmpty)
				f->process();
		}

		void process_read(datasource_facade& f)
		{
			composite_buffer& buf = f.m_accumulated;
			size_t bufSize = buf.get_length();
			for (;;)
			{
				if (!bufSize && (get_read_mode() == read_now))
					break;

				size_t unreadSize = get_unread_size();
				size_t n = (unreadSize < bufSize) ? unreadSize : bufSize;
				get_buffer().append(buf.split_off_before(n));
				if ((unreadSize == n) || (get_read_mode() != read_all))
					break;

				return;
			}
			complete(false);
			f.m_pendingReader.release();
		}
	};

	rcptr<facade_reader> m_pendingReader;
	composite_buffer m_accumulated;

	virtual rcref<reader> create_reader(const rcref<datasource>& ds)
	{
		return rcnew(facade_reader, ds, this_rcref);
	}

	void queue_to_datasource(const composite_buffer& buf)
	{
		bool wasEmpty = m_serialQueue.append([f{ this_weak_rcptr }, buf2{ buf }]()
		{
			rcptr<datasource_facade> f2 = f;
			if (!!f2)
			{
				f2->m_accumulated.append(buf2);
				if (!!f2->m_pendingReader)	// If set, m_accumulated will be empty
					f2->m_pendingReader->process_read(*f2);
			}
		});
		if (wasEmpty)
			process();
	}

	void queue_to_datasource(const buffer& buf)
	{
		bool wasEmpty = m_serialQueue.append([f{ this_weak_rcptr }, buf2{ buf }]()
		{
			rcptr<datasource_facade> f2 = f;
			if (!!f2)
			{
				f2->m_accumulated.append(buf2);
				if (!!f2->m_pendingReader)	// If set, m_accumulated will be empty
					f2->m_pendingReader->process_read(*f2);
			}
		});
		if (wasEmpty)
			process();
	}
};




}
}


#endif
