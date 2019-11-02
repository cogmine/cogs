//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_IO_DATASOURCE
#define COGS_HEADER_IO_DATASOURCE


#include "cogs/env.hpp"
#include "cogs/collections/container_deque.hpp"
#include "cogs/collections/container_dlist.hpp"
#include "cogs/collections/container_queue.hpp"
#include "cogs/function.hpp"
#include "cogs/io/composite_buffer.hpp"
#include "cogs/io/datasink.hpp"
#include "cogs/io/queue.hpp"
#include "cogs/math/dynamic_integer.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/sync/thread_pool.hpp"
#include "cogs/sync/transactable.hpp"


#ifndef COGS_DEFAULT_BLOCK_SIZE

/// @brief The default maximum block size used by I/O objects
///
/// When reading an unknown number of bytes, blocks of size COGS_DEFAULT_BLOCK_SIZE
/// are used.  This is done instead of polling for an available number of bytes
/// (which is documented to be inefficient in some scenarios, and may be impossible
/// with some datasources).  
/// http://support.microsoft.com/kb/192599
/// This approach involves allocating buffers that may be unnecessarily large for the
/// contained data.  Generally, buffers should be very short-lived.
#define COGS_DEFAULT_BLOCK_SIZE (1024 * 8)
#endif


namespace cogs {
namespace io {


/// @ingroup IO
/// @brief A base class for objects that raw data can be read from serially, such as a file or network stream.
///
/// A datasource represents a uni-directional stream of data from which data can be read.
/// A derived datasource implements read functionality by deriving from datasource::reader,
/// and implementing datasource::create_reader().
///
/// A reference to an I/O object (datasource or datasink) must be retained
/// to hold the connection open.  If it goes out of scope, the connection
/// will become closed and any
/// pending tasks will be canceled.  It's important that derived IO objects
/// ensure all pending tasks are completed rather than abandoned.
/// The task itself may own resources which will not be released until
/// the task is completed.  If an io::queue::io_task needs to refer to the I/O
/// object, it should use a weak reference, to avoid a circular reference
/// that would prevent the IO object from being released.  (reader and writer
/// contain a weak reference to their associated datasource/datasink.)
class datasource : public object
{
public:
	class transaction;

	/// @brief Mode of a read operation
	enum read_mode
	{
		/// @brief Read data if available, completing immediately
		///
		/// The read may complete with less than the requested number of bytes.
		/// If no data is available, a result of 0 bytes is returned.
		/// An empty read does not indicate the datasource is no longer readable (closed, or EOF).
		/// If there is data available, a read of this type must not complete with a result of 0 bytes.
		read_now = 1, // 01

		/// @brief Read some data
		///
		/// The read will wait for data to become available before completing.
		/// It may complete with less than the requested number of bytes, but will wait until at least 1 byte is available.
		/// If not aborted, an empty read would indicate the datasource is no longer readable (closed, or EOF).
		read_some = 2, // 10

		/// @brief Read all data requested
		///
		/// The read will wait for all requested data to be available before returning.
		/// Fewer bytes may be returned if the read is aborted or the datasource is no longer readable (closed, or EOF)
		/// If not aborted, an empty read would indicate the datasource is no longer readable (closed, or EOF).
		read_all = 3, // 11
	}; 

private:
	datasource(datasource&&) = delete;
	datasource(const datasource&) = delete;
	datasource& operator=(datasource&&) = delete;
	datasource& operator=(const datasource&) = delete;

	friend class reader;

	rcptr<io::queue> m_ioQueue; // OK that the rcptr is not volatile, since it's set on construction, and cleared in destructor

	// m_overflow is not thread-safe.  It can be prepended to by a reader, to return data
	// to the datasource, only if there are no readers and no reads in progress.
	const rcref<composite_buffer> m_overflow;

	// Storage for read buffers, if needed by a derived class.
	// Splits smaller buffers from a larger buffer, reducing allocations and allowing multiple consecutive
	// reads to be coalesced if appended to a composite_buffer.
	buffer m_internalBuffer;
	size_t m_internalBufferSize;

public:
	/// @brief Base class for datasource I/O tasks
	template <typename result_t>
	class datasource_task : public io::queue::io_task<result_t>
	{
	private:
		datasource_task() = delete;
		datasource_task(datasource_task&&) = delete;
		datasource_task(const datasource_task&) = delete;
		datasource_task& operator=(datasource_task&&) = delete;
		datasource_task& operator=(const datasource_task&) = delete;

		const weak_rcptr<datasource> m_source;

	protected:
		/// @brief Constructor
		/// @param ds Datasource to associate with this task
		datasource_task(rc_obj_base& desc, const rcref<datasource>& ds) : io::queue::io_task<result_t>(desc), m_source(ds) { }

		/// @brief Derived class implements executing() to executing the task
		virtual void executing() = 0;

		/// @brief Derived class implements canceling() to cancel a task that has not yet been executed. 
		virtual void canceling() { io::queue::io_task<result_t>::canceling(); }

		/// @brief Derived class implements aborting() to cancel a task that has started executing.
		///
		/// aborting() is only called if execute() was called and returned without having completed synchronously.
		/// If the derived task executes synchronously, it does not need to override aborting(). 
		virtual void aborting() { COGS_ASSERT(false); }

		/// @brief Completes the task, and starts the next task, if any.  Called by a derived task.
		/// @param closeQueue If true the datasource is also closed and all subsequent tasks are canceled.  Default: false
		void complete(bool closeQueue = false) { io::queue::io_task<result_t>::complete(closeQueue); }

	public:
		/// @brief Gets a weak reference to the datasource associated with this task
		/// @return A weak reference to the datasource associated with this task
		const weak_rcptr<datasource>& get_datasource() const { return m_source; }
	};

	/// @brief A flush task
	class flusher : public datasource_task<flusher>
	{
	private:
		friend class datasource;

		flusher() = delete;
		flusher(flusher&&) = delete;
		flusher(const flusher&) = delete;
		flusher& operator=(flusher&&) = delete;
		flusher& operator=(const flusher&) = delete;

		virtual void executing() { flushing(); }

	protected:
		/// @brief Constructor
		/// @param ds Datasource to associate with this flusher
		flusher(rc_obj_base& desc, const rcref<datasource>& ds) : datasource_task<flusher>(desc, ds) { }

		/// @brief Derived class should implement flushing() to perform the flush operation.
		///
		/// It's unlikely that the default behavior needs to be changed.  A flush operation is
		/// simply a NOP that asynchronously notifies when all prior queued writes have completed.
		virtual void flushing() { complete(); }

		/// @brief Derived class implements canceling() to cancel a flusher that has not yet been executed. 
		virtual void canceling() { datasource_task<flusher>::canceling(); }

		/// @brief Derived class implements aborting() to cancel a flusher that has started executing.
		///
		/// aborting() is only called if flushing() was called and returned without having completed synchronously.
		/// If the derived flusher executes synchronously, it does not need to override aborting(). 
		virtual void aborting() { COGS_ASSERT(false); }

		/// @brief Completes the flusher, and starts the next task, if any.  Called by a derived flusher.
		/// @param closeQueue If true the datasource is also closed and all subsequent tasks are canceled.  Default: false
		void complete(bool closeQueue = false) { datasource_task<flusher>::complete(closeQueue); }

		virtual const flusher& get() const volatile { return *(const flusher*)this; }
	};

	/// @brief A close task
	class closer : public datasource_task<closer>
	{
	private:
		friend class datasource;

		closer() = delete;
		closer(closer&&) = delete;
		closer(const closer&) = delete;
		closer& operator=(closer&&) = delete;
		closer& operator=(const closer&) = delete;

		virtual void executing() { closing(); }

	protected:
		/// @brief Constructor
		closer(rc_obj_base& desc, const rcref<datasource>& ds) : datasource_task<closer>(desc, ds) { }

		/// @brief Derived class implements closing() to perform the close operation.
		///
		/// The default implementation completes the closer and closes the datasource's io::queue.
		virtual void closing() { datasource_task<closer>::complete(true); }

		/// @brief Derived class implements canceling() to cancel a closer that has not yet been executed. 
		virtual void canceling() { datasource_task<closer>::canceling(); }

		/// @brief Derived class implements aborting() to cancel a closer that has started executing.
		///
		/// aborting() is only called if closing() was called and returned without having completed synchronously.
		/// If the derived closer executes synchronously, it does not need to override aborting(). 
		virtual void aborting() { COGS_ASSERT(false); }

		/// @brief Completes the closer, and starts the next task, if any (if the closer was aborted).  Called by a derived closer.
		/// @param closeQueue If true the datasource is also closed and all subsequent tasks are canceled.  Default: false
		void complete(bool closeQueue = false) { datasource_task<closer>::complete(closeQueue); }

		virtual const closer& get() const volatile { return *(const closer*)this; }
	};

	/// @brief A read task
	class reader : public datasource_task<reader>
	{
	private:
		friend class datasource;

		size_t m_requestedSize;
		composite_buffer m_readBuffer; // all read results.
		rcref<composite_buffer> m_overflow;
		read_mode m_mode;

		reader() = delete;
		reader(reader&&) = delete;
		reader(const reader&) = delete;
		reader& operator=(reader&&) = delete;
		reader& operator=(const reader&) = delete;

		virtual void executing()
		{
			if (!m_requestedSize)
				datasource_task<reader>::complete();
			else
			{
				size_t availableOverflowSize = m_overflow->get_length();
				if (availableOverflowSize > 0)
				{
					size_t unreadSize = m_requestedSize - m_readBuffer.get_length();
					if (availableOverflowSize > unreadSize)
						availableOverflowSize = unreadSize;
					m_readBuffer.append(m_overflow->split_off_before(availableOverflowSize));
					if ((unreadSize == availableOverflowSize) || (m_mode != read_all))
					{
						datasource_task<reader>::complete();
						return;
					}
				}

				reading();
			}
		}

	protected:
		reader(rc_obj_base& desc, const rcref<datasource>& ds)
			: datasource_task<reader>(desc, ds),
			m_overflow(ds->m_overflow),
			m_mode(read_all)
		{ }

		/// @brief Derived readers should implement reading() to perform the read operation
		/// 
		/// When the read is complete, the derived class should call complete().
		virtual void reading() { complete(); }

		/// @brief Derived class implements canceling() to cancel a reader that has not yet been executed. 
		virtual void canceling() { datasource_task<reader>::canceling(); }

		/// @brief Derived class implements aborting() to cancel a reader that has started executing.
		///
		/// aborting() is only called if reading() was called and returned without having completed synchronously.
		/// If the derived reader executes synchronously, it does not need to override aborting(). 
		virtual void aborting() { COGS_ASSERT(false); }

		/// @brief Completes the reader, and starts the next task, if any.  Called by a derived reader.
		/// @param closeQueue If true the datasink is also closed and all subsequent tasks are canceled.  Default: false
		void complete(bool closeQueue = false) { datasource_task<reader>::complete(closeQueue); }

		/// @brief Restores data to the datasource.  It will be read by the next reader.
		///
		/// Only valid to call while a reader is executing.
		/// @param b The buffer to prepend 
		void prepend_overflow(const buffer& b) { m_overflow->prepend(b); }
		void prepend_overflow(const composite_buffer& b) { m_overflow->prepend(b); }

		/// @brief Allocate a buffer to read data into.
		///
		/// allocate_buffer() will internally allocate one large buffer, and suballocate
		/// smaller buffers from it.  This allows larger blocks to be coalesced, if 
		/// recombined in a composite_buffer in the course of processing I/O.
		///
		/// If the read_mode is not read_all, it's possible a smaller block than requested will be returned.
		/// @param n The size of the buffer to allocate
		/// @return A buffer of the required size, but possibly smaller.
		buffer allocate_buffer(size_t n)
		{
			buffer result;
			if (!!n)
			{
				rcptr<datasource> src = get_datasource();
				if (!!src)
				{
					size_t internalBufferRemaining = src->m_internalBuffer.get_length();
					if (internalBufferRemaining < n)
					{
						if ((internalBufferRemaining > 0) && (m_mode != read_all))
							n = internalBufferRemaining;
						else
						{
							size_t sz = src->m_internalBufferSize;
							if (n > sz)
								sz = n;
							src->m_internalBuffer = buffer(sz);
						}
					}
					result = src->m_internalBuffer.split_off_before(n);
				}
			}
			return result;
		}

		composite_buffer& get_buffer() { return m_readBuffer; }

		virtual const reader& get() const volatile { return *(const reader*)this; }

	public:
		/// @brief Gets the read mode of the read operation
		/// @return The read mode of the reader
		read_mode get_read_mode() const { return m_mode; }

		/// @brief Gets the requested size of the read operation
		/// @return The requested size of the read operation
		size_t get_requested_size() const { return m_requestedSize; }

		/// @brief Gets the number of bytes succesfully read.  This is intended to be called after the read operation has completed.
		/// @return The number of bytes successfully read.
		size_t get_read_size() const { return m_readBuffer.get_length(); }

		/// @brief Gets the remaining number of bytes not yet read.  This is intended to be called after the read operation has completed.
		/// @return The number of bytes successfully read.
		size_t get_unread_size() const { return m_requestedSize - m_readBuffer.get_length(); }

		/// @brief Gets the buffer that was read.  This is intended to be called after ther ead operaiton has completed.
		/// @return The buffer that was read.
		const composite_buffer& get_read_buffer() const { return m_readBuffer; }
	};

	/// @brief Issues a flush of the datasource
	/// @return A rcref to a flusher
	rcref<flusher> flush_source()
	{
		rcref<flusher> f = create_source_flusher(this_rcref);
		source_enqueue(f);
		return f;
	}

	/// @brief Issues an asynchronous close of the datasource
	/// @return A rcref to a closer
	rcref<closer> close_source()
	{
		rcref<closer> c = create_source_closer(this_rcref);
		source_enqueue(c);
		return c;
	}

	/// @brief Reads some data
	/// @return A rcref to a reader
	rcref<reader> read() { return read(COGS_DEFAULT_BLOCK_SIZE, read_some); }

	/// @brief Reads data
	/// @param n Number of bytes to read
	/// @param m Mode of the read operation
	/// @return A rcref to a reader
	rcref<reader> read(size_t n, read_mode m = read_all)
	{
		rcref<reader> r = create_reader(this_rcref);
		read(n, m, r);
		return r;
	}

	/// @brief Couples a datasink and a datasource
	///
	/// @param src The datasource to couple.  The datasource must remain in scope.  Scope is not extended by the coupler.
	/// @param snk The datasink to couple.  The datasink must remain in scope.  Scope is not extended by the coupler.
	/// @param closeSinkOnSourceClose If true, the datasink will be closed if the datasource closes.  If false, the coupler
	/// will be decoupled if the datasink closes.  Default: false
	/// @param closeSourceOnSinkClose If true, the datasource will be closed if the datasink closes.  If false, the coupler
	/// will be decoupled if the datasource closes.  Default: false
	/// @param maxLength A value indicating the maximum number of bytes to process until automatically decoupled.
	/// If zero, the coupler does not automatically decouple.  Default: 0
	/// @param bufferBlockSize The maximum size of the buffer used to read from the datasource and write to the datasink.  Default: COGS_DEFAULT_BLOCK_SIZE
	/// @return A reference to a coupler that can be used to synchronize with completion of the coupler, or to decoupler the coupler.
	/// The reference to the coupler does not extend the scope of the coupled association.  If it goes out of scope, the coupled association
	/// remains until the maximum length is reached or either the datasink or datasource is closed.
	friend rcref<task<void> > couple(
		const rcref<datasource>& src,
		const rcref<datasink>& snk,
		bool closeSinkOnSourceClose = false,
		bool closeSourceOnSinkClose = false,
		const dynamic_integer& maxLength = dynamic_integer(),
		size_t bufferBlockSize = COGS_DEFAULT_BLOCK_SIZE)
	{
		return src->create_coupler(snk, closeSinkOnSourceClose, closeSourceOnSinkClose, maxLength, bufferBlockSize);
	}

	/// @brief Cancels any pending I/O operations on the datasource and closes it immediately.
	virtual void abort_source() { m_ioQueue->close(); }

	/// @brief Gets the close event associated with the datasource.
	/// @return A rcref to a waitable that will become signaled when the datasource is closed.
	const waitable& get_source_close_event() const { return m_ioQueue->get_close_event(); }

	bool is_source_closed() const
	{
		if (!!m_ioQueue)
			m_ioQueue->is_closed();
		return true;
	}

	~datasource()
	{
		if (!!m_ioQueue)
			m_ioQueue->close();
	}

protected:
	class default_coupler;

	/// @{
	/// @brief Prepend data to the datasource's overflow buffer
	///
	/// If access to the datasource is otherwise known to be safe (no readers running concurrently), it is
	/// possible to safely "put back" data, where it will be read by the next reader.
	/// @param b Data to prepend to the overflow buffer
	void prepend_overflow(const buffer& b) { m_overflow->prepend(b); }
	void prepend_overflow(const composite_buffer& b) { m_overflow->prepend(b); }
	/// @}

	/// @brief Associate the specified parameters with the specified reader and enqueue it on the datasource's IO queue
	/// @param n Number of bytes to read
	/// @param m Mode of the read operation
	/// @param r datasource::reader to enqueue
	void read(size_t n, read_mode m, const rcref<reader>& r)
	{
		r->m_requestedSize = n;
		r->m_mode = m;
		source_enqueue(r);
	}

	/// @brief Arbitrarily enqueues an io::queue::io_task in the datasource's IO queue
	/// @param t A reference to the io::queue::io_task to enqueue
	virtual void source_enqueue(const rcref<io::queue::task_base>& t)
	{
		m_ioQueue->enqueue(t);
	}

	/// @brief Create a datasink::flusher.  Can be used by derived datasources to create customize couplers.
	///
	/// @param snk The datasink to couple this datasource to.  The datasink must remain in scope.  Scope is not extended by the coupler.
	/// @param closeSinkOnSourceClose If false, the coupler will be decoupled if the datasource closes.  If true, the datasink
	/// will be closed if the datasource closes.  Default: false
	/// @param closeSourceOnSinkClose If false, the coupler will be decoupled if the datasink closes.  If true, the datasource
	/// will be closed if the datasink closes.  This option is useful to chain cleanup if a datasink may close unexpectedly and any
	/// data in transit may be abandoned, or to close gracefully without losing data if the datasink is known only to close after 
	/// all available data has been read from the datasource.  Otherwise, it's possible that this option may result in data loss.
	/// i.e. If the datasource is a filter and the datasink has closed before all filtered data was processed.  The filtered data
	/// cannot be restored to the original (unfiltered) datasource, so is lost.  Default: false
	/// @param maxLength A value indicating the maximum number of bytes to process until automatically decoupled.
	/// If zero, the coupler does not automatically decouple.  Default: 0
	/// @param bufferBlockSize The maximum size of the buffer used to read from the datasource and write to the datasink.  Default: COGS_DEFAULT_BLOCK_SIZE
	/// @return A reference to a coupler that can be used to synchronize with completion of the coupler, or to decoupler the coupler.
	/// The reference to the coupler does not extend the scope of the coupled association.  If it goes out of scope, the coupled association
	/// remains until the maximum length is reached or either the datasink or datasource is closed.
	virtual rcref<task<void> > create_coupler(
		const rcref<datasink>& snk,
		bool closeSinkOnSourceClose = false,
		bool closeSourceOnSinkClose = false,
		const dynamic_integer& maxLength = dynamic_integer(),
		size_t bufferBlockSize = COGS_DEFAULT_BLOCK_SIZE);

	/// @{
	/// @brief Constructor

	explicit datasource(rc_obj_base& desc)
		: object(desc),
		m_ioQueue(rcnew(queue)),
		m_overflow(rcnew(composite_buffer)),
		m_internalBufferSize(COGS_DEFAULT_BLOCK_SIZE)
	{ }

	/// @brief Constructor
	/// @param internalBufferSize The size of a buffer that datasource manages internally for suballocated
	/// read buffers.  If the derived datasource manages its own read buffers, this value is ignored.
	/// A value of zero implies the default buffer size (COGS_DEFAULT_BLOCK_SIZE).
	datasource(rc_obj_base& desc, size_t internalBufferSize)
		: object(desc),
		m_ioQueue(rcnew(queue)),
		m_overflow(rcnew(composite_buffer)),
		m_internalBufferSize((internalBufferSize > 0) ? internalBufferSize : COGS_DEFAULT_BLOCK_SIZE)
	{ }

	/// @brief Constructor
	/// @param overflow An overflow buffer to be used by the datasource.
	/// @param internalBufferSize The size of a buffer that datasource manages internally for suballocated
	/// read buffers.  If the derived datasource manages its own read buffers, this value is ignored.
	/// A value of zero implies the default buffer size.  Default: COGS_DEFAULT_BLOCK_SIZE
	datasource(rc_obj_base& desc, const rcref<composite_buffer>& overflow, size_t internalBufferSize = COGS_DEFAULT_BLOCK_SIZE)
		: object(desc),
		m_ioQueue(rcnew(queue)),
		m_overflow(overflow),
		m_internalBufferSize((internalBufferSize > 0) ? internalBufferSize : COGS_DEFAULT_BLOCK_SIZE)
	{ }

	/// @brief Constructor
	/// @param ioQueue An io::queue to be used by the datasource.
	/// @param internalBufferSize The size of a buffer that datasource manages internally for suballocated
	/// read buffers.  If the derived datasource manages its own read buffers, this value is ignored.
	/// A value of zero implies the default buffer size.  Default: COGS_DEFAULT_BLOCK_SIZE
	datasource(rc_obj_base& desc, const rcref<io::queue>& ioQueue, size_t internalBufferSize = COGS_DEFAULT_BLOCK_SIZE)
		: object(desc),
		m_ioQueue(ioQueue),
		m_overflow(rcnew(composite_buffer)),
		m_internalBufferSize((internalBufferSize > 0) ? internalBufferSize : COGS_DEFAULT_BLOCK_SIZE)
	{ }

	/// @brief Constructor
	/// @param ioQueue An io::queue to be used by the datasource.
	/// @param overflow An overflow buffer to be used by the datasource.
	/// @param internalBufferSize The size of a buffer that datasource manages internally for suballocated
	/// read buffers.  If the derived datasource manages its own read buffers, this value is ignored.
	/// A value of zero implies the default buffer size.  Default: COGS_DEFAULT_BLOCK_SIZE
	datasource(rc_obj_base& desc, const rcref<io::queue>& ioQueue, const rcref<composite_buffer>& overflow, size_t internalBufferSize = COGS_DEFAULT_BLOCK_SIZE)
		: object(desc),
		m_ioQueue(ioQueue),
		m_overflow(overflow),
		m_internalBufferSize((internalBufferSize > 0) ? internalBufferSize : COGS_DEFAULT_BLOCK_SIZE)
	{ }
	/// @}

	/// @brief Create a datasource::flusher.  Used by derived datasources to create derived flushers
	/// @param ds A datasource reference to encapsulate in the flusher.  This may different from this datasource, if
	/// another datasource is acting as a facade.
	/// @return A reference to a new flusher
	virtual rcref<flusher> create_source_flusher(const rcref<datasource>& ds) { return rcnew(bypass_constructor_permission<flusher>, ds); }

	/// @brief Create a datasource::closer.  Used by derived datasources to create derived closer
	/// @param ds A datasource reference to encapsulate in the closer.  This may different from this datasource, if
	/// another datasource is acting as a facade.
	/// @return A reference to a new closer
	virtual rcref<closer> create_source_closer(const rcref<datasource>& ds) { return rcnew(bypass_constructor_permission<closer>, ds); }

	/// @brief Create a datasource::reader.  Used by derived datasources to create derived reader
	/// @param ds A datasource reference to encapsulate in the reader.  This may different from this datasource, if
	/// another datasource is acting as a facade.
	/// @return A reference to a new writer
	virtual rcref<reader> create_reader(const rcref<datasource>& ds) { return rcnew(bypass_constructor_permission<reader>, ds); }

};


/// @brief A transaction on a datasource
class datasource::transaction : public datasource
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
	const weak_rcptr<datasource> m_source;
	volatile boolean m_started;

	// This IO task represents the entire transaction.
	// It posts to the original datasource, and does not complete until
	// the transaction is complete and all data read.
	class transaction_task : public io::queue::io_task<transaction_task>
	{
	public:
		// This IO task is a dummy element placed at the start of the
		// transaction's queue, to ensure queue'ed tasks do not trigger IO.
		class plug : public io::queue::io_task<plug>
		{
		public:
			explicit plug(rc_obj_base& desc)
				: io::queue::io_task<plug>(desc)
			{ }

			volatile boolean m_transactionRunning; // Which of the 2 fails to set from false to true, completes the operation.

			using io::queue::io_task<plug>::complete;

			virtual void executing()
			{
				if (!m_transactionRunning.compare_exchange(true, false))
					io::queue::io_task<plug>::complete();
			}

			virtual void aborting() { io::queue::io_task<plug>::complete(true); }

			virtual const plug& get() const volatile { return *(const plug*)this; }
		};

		rcptr<io::queue> m_transactionIoQueue; // extends the scope of the transaction's io queue
		volatile rcptr<plug> m_plug;
		volatile boolean m_completeOrAbortGuard;
		const close_propagation_mode m_closePropagationMode;
		volatile boolean m_transactionAborted;

		virtual void executing()
		{
			rcptr<plug> p;
			p.set_mark(1);
			m_plug.swap(p);
			if (!!p && !p->m_transactionRunning.compare_exchange(true, false))
				p->complete();
		}

		transaction_task(rc_obj_base& desc, const rcref<io::queue>& ioQueue, close_propagation_mode closePropagationMode)
			: io::queue::io_task<transaction_task>(desc),
			m_transactionIoQueue(ioQueue),
			m_closePropagationMode(closePropagationMode)
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
				io::queue::io_task<transaction_task>::complete();
			}
		}

		void aborted()
		{
			switch (m_closePropagationMode)
			{
			case no_close_propagation:
				io::queue::io_task<transaction_task>::complete();
				break;
			case propagate_abort_only:
				if (!m_transactionAborted)
				{
					io::queue::io_task<transaction_task>::complete();
					break;
				}
				// fall through
			default:
			//case propagate_close_and_abort:
				io::queue::io_task<transaction_task>::complete(true);
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

		virtual void canceling() // transaction_task got canceled before anything executed.
		{
			m_transactionIoQueue->close();
			io::queue::io_task<transaction_task>::canceling();
		}

		virtual const transaction_task& get() const volatile { return *(const transaction_task*)this; }
	};

	rcref<transaction_task> m_transactionTask;

	// This IO task is posted to the end of a transaction's io::queue
	// to trigger the completion of the transaction_task.
	class terminator : public io::queue::io_task<terminator>
	{
	public:
		rcref<transaction_task> m_transactionTask;

		terminator(rc_obj_base& desc, const rcref<transaction_task>& t)
			: io::queue::io_task<terminator>(desc),
			m_transactionTask(t)
		{ }

		virtual void executing()
		{
			rcref<transaction_task> tt = m_transactionTask;
			io::queue::io_task<terminator>::complete();
			tt->complete();
		}

		virtual void canceling()
		{
			// queue was closed, pass along close.
			rcref<transaction_task> tt = m_transactionTask;
			io::queue::io_task<terminator>::canceling();
			tt->abort();
		}

		virtual const terminator& get() const volatile { return *(const terminator*)this; }
	};

	virtual rcref<reader> create_reader(const rcref<datasource>& proxy)
	{
		rcptr<datasource> ds = m_source;
		if (!!ds)
			return ds->create_reader(proxy);
		abort_source();
		return datasource::create_reader(proxy);
	}

	virtual rcref<flusher> create_source_flusher(const rcref<datasource>& proxy)
	{
		rcptr<datasource> ds = m_source;
		if (!!ds)
			return ds->create_source_flusher(proxy);
		abort_source();
		return datasource::create_source_flusher(proxy);
	}

	virtual rcref<closer> create_source_closer(const rcref<datasource>& proxy)
	{
		rcptr<datasource> ds = m_source;
		if (!ds)
			abort_source();
		else if (m_transactionTask->m_closePropagationMode == propagate_close_and_abort)
			return ds->create_source_closer(proxy);
		return datasource::create_source_closer(proxy);
	}

	virtual void source_enqueue(const rcref<io::queue::task_base>& t)
	{
		m_transactionTask->queue_plug();
		datasource::source_enqueue(t);
	}

public:
	friend class datasource;

	/// @brief Constructor.
	/// @param ds Target datasource
	/// @param startImmediately Indicates whether to start the transaction immediately.  If false, start() must be called
	/// at some point to queue the transaction to the datasource.
	/// @param closePropagationMode Indicates what happens to the target datasource when a datasource::transaction closes or aborts.
	transaction(rc_obj_base& desc, const rcref<datasource>& ds, bool startImmediately = true, close_propagation_mode closePropagationMode = propagate_close_and_abort)
		: datasource(desc, ds->m_overflow),
		m_source(ds),
		m_started(startImmediately),
		m_transactionTask(rcnew(transaction_task, m_ioQueue.dereference(), closePropagationMode))
	{
		if (startImmediately)
			ds->source_enqueue(m_transactionTask);
	}

	/// @brief Starts processing of tasks queued to the transaction
	void start()
	{
		rcptr<datasource> ds = m_source;
		if (!!ds && m_started.compare_exchange(true, false))
			ds->source_enqueue(m_transactionTask);
	}

	/// @brief Cancels any pending I/O operations on the datasource and closes it immediately.
	virtual void abort_source()
	{
		if (m_transactionTask->m_transactionAborted.compare_exchange(true, false))
		{
			if (m_transactionTask->m_closePropagationMode == no_close_propagation)
				m_ioQueue->close();
			else
			{
				rcptr<datasource> ds = m_source;
				if (!!ds)
					ds->abort_source();
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

		m_ioQueue.release(); // Prevents datasource destructor from closing it yet.
	}
};


class datasource::default_coupler : public signallable_task_base<void>
{
private:
	friend class datasource;

	const weak_rcptr<datasource> m_source;

	const bool m_closeSinkOnSourceClose;
	const bool m_closeSourceOnSinkClose;

	rcptr<datasource::transaction> m_coupledRead;
	rcptr<datasink::transaction> m_coupledWrite;
	size_t m_bufferBlockSize;

	dynamic_integer m_remaining;
	bool m_hasMaxLength;

	rcptr<datasource::reader> m_currentReader;
	rcptr<datasink::writer> m_currentWriter;
	composite_buffer m_pendingBuffers;
	bool m_reading = false;
	bool m_writing = false;
	bool m_readClosed = false;
	bool m_writeClosed = false;
	volatile boolean m_decoupling;

	rcptr<task<void> > m_onSourceAbortTask;
	rcptr<task<void> > m_onSinkAbortTask;

	typedef bool (default_coupler::*task_f)();

	volatile container_queue<task_f> m_completionSerializer;

	void process(task_f t)
	{
		if (m_completionSerializer.append(t))
		{
			size_t releaseCount = 0;
			for (;;)
			{
				if ((this->*t)())
					++releaseCount;
				bool wasLast;
				m_completionSerializer.remove_first(wasLast);
				if (wasLast)
					break;
				m_completionSerializer.peek_first(t);
			}
			get_desc()->release(strong, releaseCount);
		}
	}

	bool process_read()
	{
		m_reading = false;
		if (m_hasMaxLength)
			m_remaining -= m_currentReader->get_read_size();
		m_pendingBuffers.append(m_currentReader->get_read_buffer());
		if (!m_readClosed && m_currentReader->was_closed())
		{
			m_readClosed = true;
			m_onSinkAbortTask->cancel();
		}
		m_currentReader.release();
		if (!m_writing)
		{
			bool readClosedAndDone = m_readClosed && (m_pendingBuffers.get_length() == 0);
			if (readClosedAndDone && !m_writeClosed && m_closeSinkOnSourceClose)
				m_coupledWrite->close_sink();

			if (readClosedAndDone || m_writeClosed || m_decoupling)
			{
				m_coupledRead->prepend_overflow(m_pendingBuffers);
				m_coupledRead.release();
				m_coupledWrite.release();
				signal();
				return true;
			}

			m_writing = true; // data was read, and no write was in progress.  Issue the write.
			m_currentWriter = m_coupledWrite->write(m_pendingBuffers);
			m_pendingBuffers.clear();
			m_currentWriter->dispatch([this]()
			{
				process(&default_coupler::process_write);
			});
		}

		if (!m_readClosed && !m_decoupling)
		{
			size_t readSize = m_bufferBlockSize - m_pendingBuffers.get_length();
			if (!!readSize)
			{
				if (m_hasMaxLength && (m_remaining < readSize))
					readSize = (size_t)m_remaining.get_int();
				if (!!readSize) // else limit reached, allow write to complete the coupler when it's done.
				{
					m_reading = true;
					m_currentReader = m_coupledRead->read(readSize, datasource::read_some);
					m_currentReader->dispatch([this]()
					{
						process(&default_coupler::process_read);
					});
				}
			}
		}
		return false;
	}

	bool process_write()
	{
		m_writing = false;
		composite_buffer unwrittenBufferList = m_currentWriter->get_unwritten_buffer();
		if (!m_writeClosed && m_currentWriter->was_closed())
		{
			m_writeClosed = true;
			m_onSourceAbortTask->cancel();
		}
		m_currentWriter.release();
		if (!!m_reading)
		{
			if (m_writeClosed)
			{
				m_pendingBuffers.prepend(unwrittenBufferList);
				if (m_closeSourceOnSinkClose)
					m_coupledRead->close_source();
				else
					m_currentReader->abort();
				return false;
			}
		}
		else // if (!m_reading)
		{
			bool readClosedAndDone = m_readClosed && (m_pendingBuffers.get_length() == 0) && (unwrittenBufferList.get_length() == 0);
			if (m_writeClosed)
			{
				if (m_closeSourceOnSinkClose && !m_readClosed)
				{
					m_pendingBuffers.prepend(unwrittenBufferList);
					m_coupledRead->close_source();
				}
			}
			else // if (!m_writeClosed)
			{
				if (readClosedAndDone && m_closeSinkOnSourceClose)
					m_coupledWrite->close_sink();
			}

			if (readClosedAndDone || m_writeClosed || m_decoupling)
			{
				m_pendingBuffers.prepend(unwrittenBufferList);
				m_coupledRead->prepend_overflow(m_pendingBuffers);
				m_coupledRead.release();
				m_coupledWrite.release();
				signal();
				return true;
			}
		}

		if (!m_decoupling)
		{
			if (!!unwrittenBufferList) // Don't combine with m_pendingBuffers.  Functionality may depend on a separation point between message blocks.
				m_writing = true;
			else if (!!m_pendingBuffers)
			{
				m_writing = true;
				unwrittenBufferList = m_pendingBuffers;
				m_pendingBuffers.clear();
			}

			if (!m_reading && !m_readClosed)
			{
				size_t readSize = m_bufferBlockSize - m_pendingBuffers.get_length();
				if (!!readSize)
				{
					if (m_hasMaxLength && (m_remaining < readSize))
						readSize = (size_t)m_remaining.get_int();
					if (!!readSize) // else limit reached, allow write to complete the coupler when it's done.
					{
						m_reading = true;
						m_currentReader = m_coupledRead->read(readSize, datasource::read_some);
						m_currentReader->dispatch([this]()
						{
							process(&default_coupler::process_read);
						});
					}
					else if (!m_writing)
					{
						m_coupledRead.release();
						m_coupledWrite.release();
						signal();
						return true;
					}
				}
			}

			if (!!m_writing)
			{
				m_currentWriter = m_coupledWrite->write(unwrittenBufferList);
				m_currentWriter->dispatch([this]()
				{
					process(&default_coupler::process_write);
				});
			}
		}
		return false;
	}

	bool process_source_aborted()
	{
		m_readClosed = true;
		if (!!m_writing) // Because the source was aborted, it's unnecessary to flush what was read to the writer.  Abort writer.
			m_currentWriter->abort();
		return false;
	}

	bool process_sink_aborted()
	{
		m_writeClosed = true;
		if (!!m_reading) // Nothing to write to, must cancel read regardless of whether we will close the source because it's decoupling.
			m_currentReader->abort();
		return false;
	}

	bool process_decouple()
	{
		m_decoupling = true;
		if (!!m_reading)
			m_currentReader->abort();
		if (!!m_writing)
			m_currentWriter->abort();
		return true;
	}

	void decouple()
	{
		self_acquire(); // Ensure we don't go out of scope if decoupling() arrives at an inopportune time.
		m_onSinkAbortTask->cancel();
		m_onSourceAbortTask->cancel();
		process(&default_coupler::process_decouple);
	}

public:
	virtual rcref<task<bool> > cancel() volatile
	{
		auto t = signallable_task_base<void>::cancel();
		if (t->get())
			((default_coupler*)this)->decouple(); // we manage thread safety, so cast away volatility
		return t;
	}

protected:
	default_coupler(
		rc_obj_base& desc,
		const rcref<datasource>& src,
		const rcref<datasink>& snk,
		bool closeSinkOnSourceClose = false,
		bool closeSourceOnSinkClose = false,
		const dynamic_integer& maxLength = dynamic_integer(),
		size_t bufferBlockSize = COGS_DEFAULT_BLOCK_SIZE)
		: signallable_task_base<void>(desc),
		m_source(src),
		m_closeSinkOnSourceClose(closeSinkOnSourceClose),
		m_closeSourceOnSinkClose(closeSourceOnSinkClose),
		m_coupledRead(rcnew(datasource::transaction, src)),
		m_coupledWrite(rcnew(datasink::transaction, snk)),
		m_bufferBlockSize((bufferBlockSize == 0) ? COGS_DEFAULT_BLOCK_SIZE : bufferBlockSize),
		m_remaining(maxLength),
		m_hasMaxLength(!!maxLength)
	{
	}

	void start_coupler()
	{
		self_acquire();
		m_reading = true;
		m_currentReader = m_coupledRead->read(m_bufferBlockSize, datasource::read_some);
		m_currentReader->dispatch([this]()
		{
			process(&default_coupler::process_read);
		});

		// Since we have transactions, so the source or sink will not close under us, only abort.
		m_onSourceAbortTask = m_coupledRead->get_source_close_event().dispatch([this]()
		{
			process(&default_coupler::process_source_aborted);
		});
		m_onSinkAbortTask = m_coupledWrite->get_sink_close_event().dispatch([this]()
		{
			process(&default_coupler::process_sink_aborted);
		});
	}

};


inline rcref<task<void> > datasource::create_coupler(
	const rcref<datasink>& snk,
	bool closeSinkOnSourceClose,
	bool closeSourceOnSinkClose,
	const dynamic_integer& maxLength,
	size_t bufferBlockSize)
{
	rcref<task<void> > result = rcnew(bypass_constructor_permission<default_coupler>, this_rcref, snk, closeSinkOnSourceClose, closeSourceOnSinkClose, maxLength, bufferBlockSize);
	result.template static_cast_to<default_coupler>()->start_coupler();
	return result;
}


}
}


#endif
