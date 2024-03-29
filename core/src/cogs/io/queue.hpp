//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_IO_QUEUE
#define COGS_HEADER_IO_QUEUE


#include "cogs/collections/container_queue.hpp"
#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/function.hpp"
#include "cogs/io/buffer.hpp"
#include "cogs/mem/rcnew.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/dispatcher.hpp"
#include "cogs/sync/count_down_condition.hpp"
#include "cogs/sync/thread_pool.hpp"


namespace cogs {
namespace io {


/// @brief Combines a lock-free queue of tasks to be executed serially, and an open/closed state.
///
/// An io::queue is a helper class used internally by datasource and datasink to serialize I/O operations/transactions
/// on a serial stream (i.e. a tcp/ip channel or a file cursor), and maintain the stream's open/closed state.  It's useful to
/// combine these concepts in a lock-free manner to synchronize cancellation of pending tasks with a close
/// operation.
///
/// An io::queue is initially open, and may potentially become permanently closed.  It is waitable, permanently becoming
/// signaled if closed.  Queued tasks are automatically executed when at the head of the queue and removed as
/// they are completed.  If the queue is closed, any unexecuted tasks are canceled, and any new tasks
/// are immediately canceled instread of queued.  Derived tasks overload executing(), aborting().
/// and canceling().
///
/// Whether or not aborting() or canceling() is called depends on the state of the task when aborted.
/// When a task is aborted by a call to task::abort(), aborting() will be called if the task had
/// already begun executing asynchronously, canceling() is called if it had not.  If the io::queue itself
/// is aborted while tasks are queued, aborting() is called on the currently executing task, canceling() is
/// called on all unstarted tasks.
///
/// tasks are reference-counted objects.  If execute()'ed, the task will not be released until complete()'ed.
/// complete() must be called for each task executed, or a reference will be leaked.
/// complete() must be called for each task executed, or a reference will be leaked.
///
class queue : public object
{
private:
	queue(const queue&) = delete;
	queue& operator=(const queue&) = delete;

	rcptr<count_down_condition> m_closeCondition;

public:
	/// @brief Base class for I/O tasks
	class task_base : public virtual object
	{
	private:
		enum class state
		{
			done_bit       = 0x0001, // Set when complete
			canceled_bit   = 0x0010, // Set when cancel is requested
			active_bit     = 0x0100, // Set when active - no longer in queue, unset when complete (may still be executing)
			executing_bit  = 0x1000, // Set when executing, unset when no longer executing

			queued         = 0x0000,
			executing      = 0x1100, // executing_bit | active_bit
			cancel_pending = 0x1110, // executing_bit | active_bit | canceled_bit
			abort_pending  = 0x0110, //                 active_bit | canceled_bit
			async          = 0x0100, //                 active_bit
			abort_complete = 0x0011, //                              canceled_bit | done_bit
		};

		task_base(task_base&&) = delete;
		task_base(const task_base&) = delete;
		task_base& operator=(task_base&&) = delete;
		task_base& operator=(const task_base&) = delete;

		// 0 = has already completed or been canceled. 1 = cancelling. -1 = was started, aborting.
		int abort_inner(bool closing)
		{
			state oldState;
			atomic::load(m_state, oldState);
			for (;;)
			{
				if (((int)oldState & ((int)state::done_bit | (int)state::canceled_bit)) == 0)
				{
					state newState = (state)((int)oldState | (int)state::canceled_bit | (oldState == state::queued ? (int)state::done_bit : 0));
					if (!atomic::compare_exchange(m_state, newState, oldState, oldState))
						continue;

					if (oldState == state::queued)
					{
						if (closing)
							get_was_closed() = true;
						canceling();
						return 1;
					}
					else if (oldState == state::async)
					{
						if (closing)
							get_was_closed() = true;
						aborting();
					}

					// aborting() will be called after it returns, if it doesn't complete
					return -1;
				}
				break;
			}
			return 0;
		}

		void executing_inner()
		{
			state oldState;
			atomic::load(m_state, oldState);
			COGS_ASSERT(((int)oldState & (int)state::executing_bit) != 0);
			COGS_ASSERT(((int)oldState & (int)state::active_bit) != 0);
			COGS_ASSERT(((int)oldState & (int)state::done_bit) == 0);
			if (((int)oldState & (int)state::canceled_bit) != 0)
			{
				get_queue()->m_closeCondition->signal();
				canceling();
				m_state = state::abort_complete;
				get_queue()->execute_next();
			}
			else
			{
				self_acquire();
				executing();
				atomic::load(m_state, oldState);
				for (;;)
				{
					COGS_ASSERT(((int)oldState & (int)state::executing_bit) != 0);
					state newState = (state)((int)oldState & ~(int)state::executing_bit); // remove executing_bit
					if (!atomic::compare_exchange(m_state, newState, oldState, oldState))
						continue;

					if (oldState == state::cancel_pending) // now state::abort_pending
					{
						aborting();
					}

					break;
				}
			}
		}

		bool execute()
		{
			bool canceled = false;
			state oldState;
			atomic::load(m_state, oldState);
			for (;;)
			{
				COGS_ASSERT(((int)oldState & (int)state::executing_bit) == 0);
				COGS_ASSERT(((int)oldState & (int)state::active_bit) == 0);
				if (((int)oldState & (int)state::canceled_bit) != 0)
				{
					canceled = true;
					break;
				}
				if (!atomic::compare_exchange(m_state, state::executing, oldState, oldState))
					continue;
				get_queue()->m_closeCondition->count_up();

				thread_pool::get_default_or_immediate()->dispatch([r{ this_rcref }]()
				{
					r->executing_inner();
				});
				break;
			}
			return !canceled;
		}

		friend class queue;

		rcptr<queue> m_ioQueue;
		state m_state alignas(atomic::get_alignment_v<state>);
		bool m_wasClosed;

		// Use is thread safe, so don't need volatility
		const rcptr<queue>& get_queue() const volatile { return ((const task_base*)this)->m_ioQueue; }

		bool& get_was_closed() volatile { return ((task_base*)this)->m_wasClosed; }
		const bool& get_was_closed() const volatile { return ((const task_base*)this)->m_wasClosed; }

		virtual void finish() = 0;

	protected:
		/// @brief Constructor
		task_base()
			: m_state(state::queued),
			m_wasClosed(false)
		{
		}

		virtual void executing() = 0;

		virtual void canceling()
		{
			finish();
		}

		virtual void aborting() { }

		void complete(bool closeQueue = false)
		{
			get_queue()->m_closeCondition->signal(); // Releases a ref we used to hold it temporarily open
			get_was_closed() = closeQueue;
			state oldState;
			atomic::load(m_state, oldState);
			for (;;)
			{
				COGS_ASSERT(((int)oldState & (int)state::done_bit) == 0);
				COGS_ASSERT(((int)oldState & (int)state::active_bit) != 0);
				state newState = (state)(((int)oldState | (int)state::done_bit) & ~(int)state::active_bit); // add done_bit, remove active_bit
				if (atomic::compare_exchange(m_state, newState, oldState, oldState))
				{
					if (closeQueue)
						get_queue()->close();

					finish();

					if (!closeQueue)
						get_queue()->execute_next();
					break;
				}
			}

			self_release();
		}

	public:
		/// @brief Tests is this task has been completed, aborted or canceled.
		/// @return True if this task has been completed, aborted or canceled
		bool is_complete() const volatile
		{
			state oldState;
			atomic::load(m_state, oldState);
			return ((int)oldState & (int)state::done_bit) != 0;
		}

		/// @brief Tests if the queue was closed before or as a result of this task.  May only be called on a complete task.
		/// @return True if the queue was closed before or as a result of this task.
		bool was_closed() const { return get_was_closed(); }

		/// @brief Aborts a task.  If queued, dequeues and cancels it.  If executing, aborts it.
		int abort() volatile { return ((task_base*)this)->abort_inner(false); }
		// 0 = has already completed or been canceled. 1 = cancelling. -1 = was started, aborting.
	};

	template <typename result_t>
	class io_task : public task_base, public signallable_task_base<result_t>
	{
	private:
		using signallable_task_base<result_t>::signal;

		virtual void finish() { signal(); }

	public:
		virtual rcref<task<bool> > cancel() volatile
		{
			abort();

			// IO operations will always return false from cancel, as they support partial completion,
			// and because a task can become cancelled due to the queue being closes, not by direct task cancallation.
			return signaled(false);
		}

	protected:
		/// @brief Derived class implements executing() to executing the task
		virtual void executing() = 0;

		/// @brief Derived class implements canceling() to cancel a task that has not yet been executed.
		virtual void canceling() { task_base::canceling(); }

		/// @brief Derived class implements aborting() to cancel a task that has started executing.
		///
		/// aborting() is only called if execute() was called and returned without having completed synchronously.
		/// If the derived task executes synchronously, it does not need to override aborting().
		virtual void aborting() { }

		/// @brief Completes the task, and starts the next task, if any.  Called by a derived task.
		/// @param closeQueue If true the io::queue is also closed and all subsequent tasks are canceled.  Default: false
		void complete(bool closeQueue = false) { task_base::complete(closeQueue); }
	};

protected:
	class content_t
	{
	private:
		content_t(const content_t&) = delete;
		content_t operator=(const content_t&) = delete;

		volatile container_queue<rcref<task_base> > m_queue;

		rcref<count_down_condition> m_closeCondition;

		friend class queue;

		void enqueue(const rcref<task_base>& t)
		{
			if (m_queue.append(t) && !t->execute())
				execute_next();
		}

		void execute_next()
		{
			for (;;)
			{
				rcptr<task_base> t;
				bool wasEmptied = m_queue.pop(t.dereference()).wasEmptied;
				COGS_ASSERT(!!t);

				t.release();

				if (wasEmptied)
					break;

				m_queue.peek(t.dereference());
				if (!!t->execute())
					break;
				continue;
			}
		}

	public:
		content_t()
			: m_closeCondition(rcnew(count_down_condition)(1))
		{ }

		~content_t()
		{
			for (;;)
			{
				rcptr<task_base> t;
				m_queue.pop(t.dereference());
				if (!t)
					break;
				t->abort_inner(true);

				t.release();
			}
			m_closeCondition->signal();
		}
	};

	volatile rcptr<content_t> m_contents;

	void execute_next()
	{
		rcptr<content_t> c = m_contents;
		if (!!c)
			c->execute_next();
	}

public:
	queue()
		: m_contents(rcnew(content_t))
	{
		m_closeCondition = m_contents->m_closeCondition;
	}

	~queue() { close(); }

	void close()
	{
		if (!!m_contents)
		{
			rcptr<content_t> tmp;
			m_contents.swap(tmp);
		}
	}

	const waitable& get_close_condition() const { return *m_closeCondition.dereference().template static_cast_to<waitable>(); }

	bool is_closed() const { return !m_contents; }
	bool operator!() const { return is_closed(); }

	bool enqueue(const rcref<task_base>& t)
	{
		rcptr<content_t> c = m_contents;
		if (!c)
		{
			t->canceling();
			return false;
		}

		t->m_ioQueue = this_rcref;
		c->enqueue(t);
		return true;
	}

	bool try_enqueue(const rcref<task_base>& t)
	{
		rcptr<content_t> c = m_contents;
		if (!c)
			return false;

		t->m_ioQueue = this_rcref;
		c->enqueue(t);
		return true;
	}
};


}
}


#endif

