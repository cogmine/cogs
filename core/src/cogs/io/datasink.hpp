//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_IO_DATASINK
#define COGS_IO_DATASINK


#include "cogs/function.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/io/composite_buffer.hpp"
#include "cogs/io/queue.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/thread_pool.hpp"

namespace cogs {


/// @defgroup IO I/O
/// @{
/// @brief Input and Output
/// @}

/// @ingroup IO
/// @brief Namespace for I/O
namespace io {

/// @ingroup IO
/// @brief A base class for objects that raw data can be written to serially, such as a file or network stream.
///
/// A datasink represents a uni-directional stream of data to which data can be written.
/// A derived datasink implements write functionality by deriving from datasource::writer,
/// and implementing datasource::create_writer().
///
/// A reference to an IO object (datasource or datasink) must be retained
/// to hold the connection open.  If it goes out of scope, the connection
/// will become closed and any
/// pending tasks will be canceled.  It's important that derived IO objects
/// ensure all pending tasks are completed rather than abandoned.
/// The task itself may own resources which will not be released until
/// the task is completed.  If an io::task needs to refer to the IO
/// object, it should use a weak reference, to avoid a circular reference
/// that would prevent the IO object from being released.  (reader and writer
/// contain a weak reference to their associated datasource/datasink.)
class datasink : public object
{
private:
	datasink(datasink&&) = delete;
	datasink(const datasink&) = delete;
	datasink& operator=(datasink&&) = delete;
	datasink& operator=(const datasink&) = delete;

	rcptr<io::queue>	m_ioQueue;	// OK that the rcptr is not volatile, since it's set on construction, and cleared in destructor

public:
	class transaction;

	/// @brief Base class for datasink I/O tasks
	template <typename result_t>
	class task : public io::queue::task<result_t>
	{
	private:
		task() = delete;
		task(task&&) = delete;
		task(const task&) = delete;
		task& operator=(task&&) = delete;
		task& operator=(const task&) = delete;

		const weak_rcptr<datasink>		m_sink;

	protected:
		/// @brief Constructor
		/// @param ds Datasink to associate with this task
		task(const rcref<datasink>& ds) : m_sink(ds)		{ }

		/// @brief Derived class implements executing() to execute the task
		virtual void executing() = 0;

		/// @brief Derived class implements canceling() to cancel a task that has not yet been executed. 
		using io::queue::task<const rcref<result_t> >::canceling;
		//virtual void canceling() { io::queue::task::canceling(); }

		/// @brief Derived class implements aborting() to cancel a task that has started executing.
		///
		/// aborting() is only called if execute() was called and returned without having completed synchronously.
		/// If the derived task executes synchronously, it does not need to override aborting(). 
		virtual void aborting() { COGS_ASSERT(false); }

		/// @brief Completes the task, and starts the next task, if any.  Called by a derived task.
		/// @param closeQueue If true the datasink is also closed and all subsequent tasks are canceled.  Default: false
		using io::queue::task<const rcref<result_t> >::complete;
		//void complete(bool closeQueue = false) { io::queue::task::complete(closeQueue); }

	public:
		/// @brief Gets a weak reference to the datasink associated with this task
		/// @return A weak reference to the datasink associated with this task
		const weak_rcptr<datasink>& get_datasink() const	{ return m_sink; }
	};

	/// @brief A flush task
	class flusher : public task<flusher>
	{
	private:
		friend class datasink;

		flusher() = delete;
		flusher(flusher&&) = delete;
		flusher(const flusher&) = delete;
		flusher& operator=(flusher&&) = delete;
		flusher& operator=(const flusher&) = delete;

		virtual void executing()	{ flushing(); }

	protected:
		/// @brief Constructor
		/// @param ds Datasink to associate with this flusher
		flusher(const rcref<datasink>& ds) : task(ds)	{ }

		/// @brief Derived class should implement flushing() to perform the flush operation.
		///
		/// It's unlikely that the default behavior needs to be changed.  A flush operation is
		/// simply a NOP that asynchronously notifies when all prior queued writes have completed.
		virtual void flushing()		{ complete(); }

		/// @brief Derived class implements canceling() to cancel a flusher that has not yet been executed. 
		using task<flusher>::canceling;
		//virtual void canceling() { io::queue::task::canceling(); }

		/// @brief Derived class implements aborting() to cancel a flusher that has started executing.
		///
		/// aborting() is only called if flushing() was called and returned without having completed synchronously.
		/// If the derived flusher executes synchronously, it does not need to override aborting(). 
		virtual void aborting() { COGS_ASSERT(false); }

		/// @brief Completes the flusher, and starts the next task, if any.  Called by a derived flusher.
		/// @param closeQueue If true the datasink is also closed and all subsequent tasks are canceled.  Default: false
		using task<flusher>::complete;
		//void complete(bool closeQueue = false) { io::queue::task::complete(closeQueue); }
		
		virtual const flusher& get() const volatile { return *(const flusher*)this; }
	};

	/// @brief A close task
	class closer : public task<closer>
	{
	private:
		friend class datasink;

		closer() = delete;
		closer(closer&&) = delete;
		closer(const closer&) = delete;
		closer& operator=(closer&&) = delete;
		closer& operator=(const closer&) = delete;

		virtual void executing()	{ closing(); }

	protected:
		/// @brief Constructor
		closer(const rcref<datasink>& ds) : task(ds)	{ }

		/// @brief Derived class implements closing() to perform the close operation.
		///
		/// The default implementation completes the closer and closes the datasink's io::queue.
		virtual void closing()	{ task<closer>::complete(true); }

		/// @brief Derived class implements canceling() to cancel a closer that has not yet been executed. 
		using task<closer>::canceling;
		//virtual void canceling() { io::queue::task::canceling(); }

		/// @brief Derived class implements aborting() to cancel a closer that has started executing.
		///
		/// aborting() is only called if closing() was called and returned without having completed synchronously.
		/// If the derived closer executes synchronously, it does not need to override aborting(). 
		virtual void aborting() { COGS_ASSERT(false); }

		/// @brief Completes the closer, and starts the next task, if any (if the closer was aborted).  Called by a derived closer.
		/// @param closeQueue If true the datasink is also closed and all subsequent tasks are canceled.  Default: false
		using task<closer>::complete;
		//void complete(bool closeQueue = false) { io::queue::task::complete(closeQueue); }

		virtual const closer& get() const volatile { return *(const closer*)this; }
	};

	/// @brief A write task
	class writer : public task<writer>
	{
	private:
		friend class datasink;

		size_t				m_requestedSize;		// Just to remind caller how much their original request was for.
		composite_buffer	m_unwrittenBufferList;	// On Entry: Buffer to write.  On Exit: whatever couldn't be written

		writer() = delete;
		writer(writer&&) = delete;
		writer(const writer&) = delete;
		writer& operator=(writer&&) = delete;
		writer& operator=(const writer&) = delete;

		virtual void executing()
		{
			if (!m_requestedSize)
				complete();
			else
				writing();
		}

	protected:
		/// @brief Constructor
		/// @param ds Datasink to associate with this writer
		writer(const rcref<datasink>& ds) : task(ds)	{ }

		/// @brief Derived writers should implement writing() to perform the write operation
		/// 
		/// When the write is complete, the derived class should call complete().
		virtual void writing()		{ complete(); }

		/// @brief Derived class implements canceling() to cancel a writer that has not yet been executed. 
		using task<writer>::canceling;
		//virtual void canceling() { io::queue::task::canceling(); }

		/// @brief Derived class implements aborting() to cancel a writer that has started executing.
		///
		/// aborting() is only called if writing() was called and returned without having completed synchronously.
		/// If the derived writer executes synchronously, it does not need to override aborting(). 
		virtual void aborting() { COGS_ASSERT(false); }

		/// @brief Completes the writer, and starts the next task, if any.  Called by a derived writer.
		/// @param closeQueue If true the datasink is also closed and all subsequent tasks are canceled.  Default: false
		using task<writer>::complete;
		//void complete(bool closeQueue = false) { io::queue::task::complete(closeQueue); }

		/// @brief Gets the buffer to write
		/// @return The buffer to write
		composite_buffer& get_buffer() { return m_unwrittenBufferList; }

		virtual const writer& get() const volatile { return *(const writer*)this; }

	public:
		/// @brief Gets the unwritten portion of the buffer, if any.  Intended to be called after the writer completes.
		/// @return The unwritten portion of the buffer, if any.
		const composite_buffer& get_unwritten_buffer() const			{ return m_unwrittenBufferList; }

		/// @brief Gets the requested size of the write operation.
		/// @return The requested size of the write operation.
		size_t get_requested_size() const								{ return m_requestedSize; }

		/// @brief Gets the actual number of bytes writen.  Intended to be called after the writer completes.
		/// @return The actaul number of bytes written.
		const size_t get_write_size() const								{ return m_requestedSize - m_unwrittenBufferList.get_length(); }

		/// @brief Tests if any bytes were written.  Intended to be called after the writer completes.
		/// @return True if any bytes were written.
		bool was_any_written() const									{ return m_requestedSize != m_unwrittenBufferList.get_length(); }
	};

	/// @brief Issues a write.
	/// @param compBuf The buffer to write
	/// @return A rcref to a writer
	rcref<writer> write(const composite_buffer& compBuf)
	{
		rcref<writer> w = create_writer(this_rcref);
		write(compBuf, w);
		return w;
	}

	/// @brief Issues a flush of the datasink
	/// @return A rcref to a flusher
	rcref<flusher> flush_sink()
	{
		rcref<flusher> f = create_sink_flusher(this_rcref);
		sink_enqueue(f);
		return f;
	}

	/// @brief Issues an asynchronous close of the datasink
	/// @return A rcref to a closer
	virtual rcref<closer> close_sink()
	{
		rcref<closer> c = create_sink_closer(this_rcref);
		sink_enqueue(c);
		return c;
	}

	/// @brief Cancels any pending I/O operations on the datasink and closes it immediately.
	virtual void abort_sink()				{ m_ioQueue->close(); }

	/// @brief Gets the close event associated with the datasink.
	/// @return A rcref to a waitable that will become signaled when the datasink is closed.
	rcref<waitable> get_sink_close_event() const	{ return m_ioQueue->get_close_event(); }

	~datasink()
	{
		if (!!m_ioQueue)
			m_ioQueue->close();
	}

protected:
	datasink()
		: m_ioQueue(queue::create())
	{ }

	/// @brief Datasink constructor
	/// @param ioQueue The io::queue to use.  Default: creates a new one
	explicit datasink(const rcref<io::queue>& ioQueue)
		: m_ioQueue(ioQueue)
	{ }

	/// @brief Arbitrarily enqueues an io::queue::task in the datasink's IO queue
	/// @param t A reference to the io::queue::task to enqueue
	virtual void sink_enqueue(const rcref<io::queue::task_base>& t)	{ m_ioQueue->enqueue(t); }

	/// @brief Associate the specified buffer with the specified writer and enqueue it on the datasink's IO queue
	/// @param compBuf Buffer to write
	/// @param w datasink::writer to enqueue
	void write(const composite_buffer& compBuf, const rcref<writer>& w)
	{
		w->m_unwrittenBufferList = compBuf;
		w->m_requestedSize = compBuf.get_length();
		sink_enqueue(w);
	}
	
	/// @brief Create a datasink::flusher.  Used by derived datasinks to create derived flushers
	/// @param ds A datasink reference to encapsulate in the flusher.  This may different from this datasink, if
	/// another datasink is acting as a facade.
	/// @return A reference to a new flusher
	virtual rcref<flusher> create_sink_flusher(const rcref<datasink>& ds)	{ return rcnew(bypass_constructor_permission<flusher>, ds); }

	/// @brief Create a datasink::closer.  Used by derived datasinks to create derived closer
	/// @param ds A datasink reference to encapsulate in the closer.  This may different from this datasink, if
	/// another datasink is acting as a facade.
	/// @return A reference to a new closer
	virtual rcref<closer> create_sink_closer(const rcref<datasink>& ds)		{ return rcnew(bypass_constructor_permission<closer>, ds); }

	/// @brief Create a datasink::writer.  Used by derived datasinks to create derived writer
	/// @param ds A datasink reference to encapsulate in the writer.  This may different from this datasink, if
	/// another datasink is acting as a facade.
	/// @return A reference to a new writer
	virtual rcref<writer> create_writer(const rcref<datasink>& ds)			{ return rcnew(bypass_constructor_permission<writer>, ds); }
};


/// @brief A transaction on a datasink
class datasink::transaction	: public datasink
{
public:
	/// @brief Indicates what happens to the target datasink when a datasink::transaction closes or aborts.
	enum close_propagation_mode
	{
		/// @brief No close propagation.  The target datasink will not be closed if the datasink::transaction closes or aborts
		no_close_propagation = 0,

		/// @brief The target datasink will aborted if the datasink::transaction is aborted, but will not be closed if the datasink::transaction closes.
		propagate_abort_only = 1,

		/// @brief The target datasink will close if the datasink::transaction closes or aborts.
		propagate_close_and_abort = 2,
	};

private:
	const weak_rcptr<datasink>	m_sink;

	// This IO task represents the entire transaction.
	// It posts to the original datasink, and does not complete until
	// the transaction is complete and all data written.
	class transaction_task : public io::queue::task<transaction_task>
	{
	public:
		// This IO task is a dummy element placed at the start of the
		// transaction's queue, to ensure queue'ed tasks do not trigger IO.
		class plug : public io::queue::task<plug>
		{
		public:
			volatile boolean m_transactionRunning;	// Which of the 2 fails to set from false to true, completes the operation.

			using io::queue::task<plug>::complete;

			virtual void executing()
			{
				if (!m_transactionRunning.compare_exchange(true, false))
					io::queue::task<plug>::complete();
			}

			virtual void aborting()		{ io::queue::task<plug>::complete(true); }

			virtual const plug& get() const volatile { return *(const plug*)this; }
		};

		rcptr<io::queue>				m_transactionIoQueue;	// extends the scope of the transaction's io queue
		volatile rcptr<plug>			m_plug;
		volatile boolean				m_completeOrAbortGuard;
		const close_propagation_mode	m_closePropagationMode;
		volatile boolean				m_transactionAborted;

		virtual void executing()
		{
			rcptr<plug> p;
			p.set_mark(1);
			m_plug.swap(p);
			if (!!p && !p->m_transactionRunning.compare_exchange(true, false))
				p->complete();
		}

		transaction_task(const rcref<io::queue>& ioQueue, close_propagation_mode closePropagationMode)
			: m_transactionIoQueue(ioQueue),
			m_closePropagationMode(closePropagationMode),
			m_transactionAborted(false)
		{ }

		void queue_plug()
		{
			rcptr<plug> p = m_plug;
			if (!p)
			{
				p = rcnew(plug);
				if (m_plug.compare_exchange(p, rcptr<plug>()))
					m_transactionIoQueue->enqueue(p.dereference());
			}
		}

		void complete()
		{
			if (m_completeOrAbortGuard.compare_exchange(true, false))
			{
				m_transactionIoQueue->close();
				io::queue::task<transaction_task>::complete();
			}
		}

		void aborted()
		{
			switch (m_closePropagationMode)
			{
			case no_close_propagation:
				io::queue::task<transaction_task>::complete();
				break;
			case propagate_abort_only:
				if (!m_transactionAborted)
				{
					io::queue::task<transaction_task>::complete();
					break;
				}
				// fall through
			default:
			//case propagate_close_and_abort:
				io::queue::task<transaction_task>::complete(true);
				break;
			}
		}

		virtual void aborting()
		{
			if (m_completeOrAbortGuard.compare_exchange(true, false))
			{
				m_transactionAborted = true;
				m_transactionIoQueue->close();
				thread_pool::get_default_or_immediate()->dispatch([r{ this_rcref }]()
				{
					r->aborted();
				});
			}
		}

		virtual void canceling()	// transaction_task got canceled before anything executed.
		{
			m_transactionIoQueue->close();
			io::queue::task<transaction_task>::canceling();
		}

		virtual const transaction_task& get() const volatile { return *(const transaction_task*)this; }
	};

	rcref<transaction_task> m_transactionTask;

	// This IO task is posted to the end of a transaction's io::queue
	// to trigger the completion of the transaction_task.
	class terminator : public io::queue::task<terminator>
	{
	public:
		rcref<transaction_task>	m_transactionTask;
		
		terminator(const rcref<transaction_task>& t)
			: m_transactionTask(t)
		{ }
		
		virtual void executing()
		{
			rcref<transaction_task>	tt = m_transactionTask;
			io::queue::task<terminator>::complete();
			tt->complete();
		}

		virtual void canceling()
		{
			// queue was closed, pass along close.
			rcref<transaction_task>	tt = m_transactionTask;
			io::queue::task<terminator>::canceling();
			tt->abort();
		}

		virtual const terminator& get() const volatile { return *(const terminator*)this; }
	};

	virtual rcref<writer> create_writer(const rcref<datasink>& proxy)
	{
		rcptr<datasink> ds = m_sink;
		if (!!ds)
			return ds->create_writer(proxy);
		abort_sink();
		return datasink::create_writer(proxy);
	}

	virtual rcref<flusher> create_sink_flusher(const rcref<datasink>& proxy)
	{
		rcptr<datasink> ds = m_sink;
		if (!!ds)
			return ds->create_sink_flusher(proxy);
		abort_sink();
		return datasink::create_sink_flusher(proxy);
	}

	virtual rcref<closer> create_sink_closer(const rcref<datasink>& proxy)
	{
		rcptr<datasink> ds = m_sink;
		if (!ds)
			abort_sink();
		else if (m_transactionTask->m_closePropagationMode == propagate_close_and_abort)
			return ds->create_sink_closer(proxy);
		return datasink::create_sink_closer(proxy);
	}

	virtual void sink_enqueue(const rcref<io::queue::task_base>& t)
	{
		m_transactionTask->queue_plug();
		datasink::sink_enqueue(t);
	}

	volatile boolean m_started;

protected:
	friend class datasink;

	/// @brief Transaction constructor.
	/// @param ds Target datasink
	/// @param startImmediately Indicates whether to start the transaction immediately.  If false, start() must be called
	/// at some point to queue the transaction to the datasink.
	/// @param closePropagationMode Indicates what happens to the target datasink when a datasink::transaction closes or aborts.
	transaction(const rcref<datasink>& ds, bool startImmediately, close_propagation_mode closePropagationMode)
		:	m_sink(ds),
			m_started(startImmediately),
			m_transactionTask(rcnew(transaction_task, m_ioQueue.dereference(), closePropagationMode))
	{
		if (startImmediately)
			ds->sink_enqueue(m_transactionTask);
	}

public:
	/// @brief Creates a datasink::transaction
	///
	/// @param snk Target datasink
	/// @param startImmediately Indicates whether to start the transaction immediately.  If false, start() must be called
	/// at some point to queue the transaction to the datasink.
	/// @param closePropagationMode Indicates what happens to the target datasink when a datasink::transaction closes or aborts.
	static rcref<transaction> create(const rcref<datasink>& snk, bool startImmediately = true, close_propagation_mode closePropagationMode = propagate_close_and_abort)
	{
		return rcnew(bypass_constructor_permission<transaction>, snk, startImmediately, closePropagationMode);
	}

	/// @brief Starts processing of tasks queued to the transaction
	void start()
	{
		rcptr<datasink> ds = m_sink;
		if (!!ds && m_started.compare_exchange(true, false))
			ds->sink_enqueue(m_transactionTask);
	}

	/// @brief Cancels any pending I/O operations on the datasink and closes it immediately.
	virtual void abort_sink()
	{
		if (m_transactionTask->m_transactionAborted.compare_exchange(true, false))
		{
			if (m_transactionTask->m_closePropagationMode == no_close_propagation)
				m_ioQueue->close();
			else
			{
				rcptr<datasink> ds = m_sink;
				if (!!ds)
					ds->abort_sink();
			}
		}
	}

	~transaction()
	{
		if (!!m_started)
		{
			rcref<terminator> t = rcnew(terminator, m_transactionTask);
			m_ioQueue->try_enqueue(t);
		}

		m_ioQueue.release();	// Prevents datasink destructor from closing it yet.
	}
};




}
}


#endif