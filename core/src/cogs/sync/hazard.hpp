//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_SYNC_HAZARD
#define COGS_HEADER_SYNC_HAZARD


#include "cogs/debug.hpp"
#include "cogs/env.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/env/mem/memory_manager.hpp"
#include "cogs/mem/freelist.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/operators.hpp"
#include "cogs/sync/atomic_load.hpp"
#include "cogs/sync/versioned_ptr.hpp"


namespace cogs {


template <typename signature, size_t n = sizeof(void*) * 6> // Enough space for 2x rcptr's, and a vtable ptr, plus padding
class function;


/// @ingroup Synchronization
/// @brief Prevents a resource from being disposed while it may still be in use.
///
/// <a href='https://en.wikipedia.org/wiki/Hazard_pointer'>Hazard pointers</a> ensure
/// data structures are not disposed out from under lock-free algorithms.
///
/// cogs::hazard is inspired by a paper published by Maged M Michael titled,
/// "Hazard Pointers: Safe Memory Reclamation for Lock-Free Objects", but diverges significantly.
/// This implementation is independent of the deallocation mechanism, releases resources immediately when
/// no longer referenced, and is contextual (opt-in).
///
/// One example of hazard pointer use would be a lock-free stack.  In order to remove an element,
/// it's necessary to dereference the node at the head of the list, to read it's 'next' pointer.
/// On a platform with paged virtual memory, reading from released memory could cause a fault.
/// A hazard pointer can be used to prevent a node from being released
/// while it's being dereferenced, by temporarily placing it on a "do not delete" list.
///
/// 'Acquiring' a hazard pointer requires some atomic double-checking.  After a pointer is
/// added to the "do not delete" list, it's necessary to double-check that the pointer is still in use.
///
/// In the simple use case in which the pointer to acquire is a volatile
/// pointer variable, hazard::pointer::acquire() can be used and does the lock-free double-checking internally.
///
/// In more complex scenarios, the pointer may be a member of a data structure, computed, or otherwise not
/// accessible directly using a volatile pointer variable.
/// If the process of double-checking the pointer value can be accomplished with a lambda,
/// the lambda version of hazard::pointer::acquire() can be used.  Otherwise, the caller must perform the double-checking logic:
///
/// - Call hazard::bind_unacquired() with the pointer value.
/// - Re-reading the original pointer value.
/// - If the value has changed, call hazard::bind_unacquired() with the new value, and re-read again.
///   Repeat until the original value is unchanged.
/// - Call hazard::pointer::validate().  If false is returned, start over.
///
/// The call to validate() promotes the hazard to the 'acquired' state.  This is because the
/// hazard does not assume ownership of the pointer until after it is confirmed to still
/// point to the data type we expect.  validate() may fail if release of the pointer has already
/// been detected.
///
/// To release a resource that may be tracked by a hazard, call hazard::release().
/// If there are no associated hazard pointers, hazard::release() will return true,
/// and the resource can be released by the caller.  If there are associated hazard::pointer's,
/// hazard::release() will return false and release of the resource is deferred.
///
/// When done using a hazard::pointer, it must be released by calling hazard::pointer::release().
/// If true is returned, it's the responsibility of the caller to release the resource.
class hazard
{
private:
	class token
	{
	public:
		enum class state : bytes_to_uint_t<sizeof(void*)>
		{
			empty = 0,    // 00 - Either null, or a pointer that has been bound and not yet confirmed
			disposed = 1, // 01 - A pointer that has been bound, and disposed before confirmed
			acquired = 2, // 10 - A pointer that has been acquired, and is not owned by this hazard::pointer
			owned = 3,    // 11 - A pointer that has been acquired, has been disposed, and is now owned by this hazard::pointer
		};

		class content_t
		{
		public:
			void* m_value;
			state m_state alignas(atomic::get_alignment_v<state>);
		};

		volatile content_t m_contents alignas(atomic::get_alignment_v<content_t>);

		ptr<token> m_next;
		ptr<token> m_nextFreeToken;

		void bind_unacquired(void* value) volatile { atomic::store(m_contents, { value, state::empty }); }
		void bind_acquired(void* value) volatile { atomic::store(m_contents, { value, state::acquired }); }

		bool is_acquired() volatile
		{
			state oldState;
			atomic::load(m_contents.m_state, oldState);
			return (int)oldState >= 2;
		}

		bool is_owner() volatile
		{
			state oldState;
			atomic::load(m_contents.m_state, oldState);
			return oldState == state::owned;
		}

		bool validate() volatile
		{
			bool result = false;
			state oldState;
			atomic::load(m_contents.m_state, oldState);
			while (oldState != state::disposed)
			{
				if (!atomic::compare_exchange(m_contents.m_state, state::acquired, oldState, oldState))
					continue;
				result = true;
				break;
			}
			return result;
		}

		bool transfer_ownership(void* value) volatile
		{
			bool owned = true;
			ptr<volatile token> curToken = this;
			content_t oldContents;
			content_t newContents;

			for (;;)
			{
				atomic::load(curToken->m_contents, oldContents);
				if (oldContents.m_value == value)
				{
					COGS_ASSERT((oldContents.m_state == state::empty) || (oldContents.m_state == state::acquired));
					newContents.m_value = (oldContents.m_state == state::empty) ? 0 : value;
					newContents.m_state = (state)((int)oldContents.m_state + 1);
					if (!atomic::compare_exchange(curToken->m_contents, newContents, oldContents, oldContents))
						continue; // Using continue to jump to the top, so cannot use a do-while loop, which would jump to the condition
					if (newContents.m_state == state::owned)
					{
						owned = false;
						break;
					}
				}
				curToken = curToken->m_next;
				if (!curToken)
					break;
			}
			return owned;
		}

		bool release() volatile
		{
			content_t oldContents;
			content_t newContents = { 0, state::empty };
			atomic::exchange(m_contents, newContents, oldContents);
			bool owned = (oldContents.m_state == state::owned);
			if (owned)
			{
				ptr<token> nextToken = m_next;
				if (!!nextToken)
					owned = nextToken->transfer_ownership(oldContents.m_value);
			}
			return owned;
		}

		static token* create()
		{
			volatile token_freelist_t& tokenFreeList = s_tokenFreeList.get();
			token* t = tokenFreeList.get();
			t->m_contents.m_value = 0;
			t->m_contents.m_state = state::empty;
			return t;
		}

		static void destroy(token& t)
		{
			volatile token_freelist_t& tokenFreeList = s_tokenFreeList.get();
			return tokenFreeList.release(t);
		}
	};

	class list
	{
	private:
		ptr<token> m_tokens;
		versioned_ptr<token> m_freeTokens;

		typedef versioned_ptr<token>::version_t version_t;

	public:
		ptr<token> get_head_token() volatile { return m_tokens; }

		token* get_token() volatile
		{
			ptr<token> newHead;
			ptr<token> result;
			version_t v;
			m_freeTokens.get(result, v);
			do {
				if (!result)
					break;
				newHead = result->m_nextFreeToken;
			} while (!m_freeTokens.versioned_exchange(newHead, v, result));

			if (!result)
			{
				result = token::create();
				ptr<token> oldTokenHead = m_tokens;
				do {
					result->m_next = oldTokenHead;
				} while (!m_tokens.compare_exchange(result, oldTokenHead, oldTokenHead));
			}
			return result.get_ptr();
		}

		void release_token(token& t) volatile
		{
			ptr<token> oldHead;
			version_t v;
			m_freeTokens.get(oldHead, v);
			do {
				t.m_nextFreeToken = oldHead;
			} while (!m_freeTokens.versioned_exchange(&t, v, oldHead));
		}

		static list* create()
		{
			volatile list_freelist_t& listFreeList = s_listFreeList.get();
			list* l = listFreeList.get();
			l->m_tokens.release();
			l->m_freeTokens.release();
			return l;
		}

		static void destory(list& l)
		{
			ptr<token> curToken = l.m_tokens;
			while (!!curToken)
			{
				ptr<token> nextToken = curToken->m_next;
				token::destroy(*curToken);
				curToken = nextToken;
			}
			volatile list_freelist_t& listFreeList = s_listFreeList.get();
			listFreeList.release(l);
		}
	};

	typedef freelist<list, 10, default_allocator<freelist_node<list>, env::memory_manager>> list_freelist_t;
	typedef freelist<token, 10, default_allocator<freelist_node<token>, env::memory_manager>> token_freelist_t;

	inline static placement<list_freelist_t> s_listFreeList; // zero-initialize as bss, leaked
	inline static placement<token_freelist_t> s_tokenFreeList; // zero-initialize as bss, leaked

	class content_t
	{
	public:
		size_t m_useCount;
		list* m_list;
	};

	volatile content_t m_contents alignas(atomic::get_alignment_v<content_t>);

	token* get_token() volatile
	{
		list* newList = 0;
		content_t newContents;
		content_t oldContents;
		atomic::load(m_contents, oldContents);
		for (;;)
		{
			COGS_ASSERT((!!oldContents.m_list || !oldContents.m_useCount));
			if (oldContents.m_useCount != 0)
			{
				newContents.m_list = oldContents.m_list;
				newContents.m_useCount = oldContents.m_useCount + 1;
				COGS_ASSERT((!!newContents.m_list || !newContents.m_useCount));
				if (!atomic::compare_exchange(m_contents, newContents, oldContents, oldContents))
					continue;
				break;
			}
			newContents.m_useCount = 1;
			if (!newList)
				newList = list::create();
			newContents.m_list = newList;
			COGS_ASSERT((!!newContents.m_list || !newContents.m_useCount));
			if (!atomic::compare_exchange(m_contents, newContents, oldContents, oldContents))
				continue;
			newList = 0;
			break;
		}
		if (!!newList)
			list::destory(*newList);
		return newContents.m_list->get_token();
	}

	void release_token(token& t) volatile
	{
		content_t oldContents;
		atomic::load(m_contents, oldContents);
		COGS_ASSERT((!!oldContents.m_list || !oldContents.m_useCount));
		oldContents.m_list->release_token(t);
		for (;;)
		{
			content_t newContents;
			newContents.m_useCount = oldContents.m_useCount - 1;
			if (newContents.m_useCount == 0)
				newContents.m_list = 0;
			else
				newContents.m_list = oldContents.m_list;
			COGS_ASSERT((!!newContents.m_list || !newContents.m_useCount));
			if (!atomic::compare_exchange(m_contents, newContents, oldContents, oldContents))
				continue;
			if (newContents.m_useCount == 0)
				list::destory(*oldContents.m_list);
			break;
		}
	}

	hazard(const hazard&) = delete;
	hazard* operator=(const hazard&) = delete;

	bool release_inner(void* value) volatile
	{
		bool result = true;
		content_t newContents;
		content_t oldContents;
		atomic::load(m_contents, oldContents);
		while (oldContents.m_useCount != 0) // If no tokens, caller is free and clear
		{
			COGS_ASSERT((!!oldContents.m_list || !oldContents.m_useCount));
			newContents.m_list = oldContents.m_list;
			newContents.m_useCount = oldContents.m_useCount + 1;
			COGS_ASSERT((!!newContents.m_list || !newContents.m_useCount));
			if (!atomic::compare_exchange(m_contents, newContents, oldContents, oldContents))
				continue;

			ptr<token> t = oldContents.m_list->get_head_token();
			result = (!t) ? true : t->transfer_ownership(value);

			oldContents.m_useCount = newContents.m_useCount;
			for (;;)
			{
				newContents.m_useCount = oldContents.m_useCount - 1;
				if (newContents.m_useCount == 0)
					newContents.m_list = 0;
				else
					newContents.m_list = oldContents.m_list;
				COGS_ASSERT((!!newContents.m_list || !newContents.m_useCount));
				if (!atomic::compare_exchange(m_contents, newContents, oldContents, oldContents))
					continue;
				if (newContents.m_useCount == 0)
					list::destory(*(oldContents.m_list));
				break;
			}
			break;
		}
		return result;
	}

public:
	hazard()
		: m_contents { 0, 0 }
	{ }

	/// @brief Prevents a resource from being disposed while it may still be in use.
	///
	/// See cogs::hazard
	class pointer
	{
	private:
		volatile hazard* m_hazard;
		token* m_token;

		void set_token(volatile hazard& h)
		{
			for (;;)
			{
				if (!!m_token)
				{
					if (m_hazard == &h)
						break;
					m_hazard->release_token(*m_token);
				}
				m_token = h.get_token();
				m_hazard = &h;
				break;
			}
		}

	public:
		/// @{
		/// @brief Constructor.  Initializes a hazard::pointer to NULL
		pointer()
			: m_token(0)
		{ }

		/// @}

		/// @{
		/// @brief Move constructor.
		///
		/// Ownership of an acquired hazard::pointer is transfered on assignment/copy-construct
		/// @param p hazard::pointer to copy
		pointer(pointer&& p)
			: m_hazard(p.m_hazard),
			m_token(p.m_token)
		{
			p.m_hazard = nullptr;
			p.m_token = nullptr;
		}

		/// @brief Initializes a hazard::pointer and binds it to the specified hazard and pointer value without fully acquiring it.
		/// @param h hazard pool to bind value within
		/// @param value value to bind in hazard pool
		/// @tparam type type pointed to
		template <typename type>
		pointer(volatile hazard& h, type* value)
			: m_hazard(&h),
			m_token(h.get_token())
		{
			m_token->bind_unacquired((void*)value);
		}
		/// @}

		template <typename F>
		pointer(std::enable_if_t<std::is_invocable_v<F> && std::is_pointer_v<std::remove_reference_t<std::invoke_result_t<F>>>, volatile hazard&> h, F&& getter)
			: m_token(0)
		{
			acquire(h, std::forward<F>(getter));
		}

		template <typename type, typename F>
		pointer(std::enable_if_t<std::is_invocable_v<F> && std::is_pointer_v<std::remove_reference_t<std::invoke_result_t<F>>>, volatile hazard&> h, type* value, F&& getter)
			: m_token(0)
		{
			acquire(h, value, std::forward<F>(getter));
		}

		~pointer()
		{
			if (!!m_token)
			{
				COGS_ASSERT(m_token->is_acquired() == false);
				m_hazard->release_token(*m_token);
			}
		}

		bool is_empty() const { return !m_token; }
		bool operator!() const { return !m_token; }

		/// @{
		/// @brief Move-assignment
		///
		/// Ownership of an acquired hazard::pointer is transfered on assignment/copy-construct
		/// @param p hazard::pointer to copy
		/// @return A reference to this object
		pointer& operator=(pointer&& p)
		{
			if (!!m_token)
			{
				COGS_ASSERT(m_token->is_acquired() == false);
				m_hazard->release_token(*m_token);
			}
			m_hazard = p.m_hazard;
			m_token = p.m_token;
			p.m_hazard = 0;
			p.m_token = 0;
			return *this;
		}
		/// @}

		/// @{
		/// @brief Bind a pointer value and hazard pool
		///
		/// A hazard::pointer can be bound to a new pointer value when in the empty, unconfirmed, or released states.
		/// It is caller error to call bind() on an acquired hazard::pointer.
		/// @param h hazard pool to bind value within
		/// @param value value to bind
		template <typename type>
		void bind_unacquired(volatile hazard& h, type* value)
		{
			set_token(h);
			m_token->bind_unacquired((void*)value);
		}
		/// @}

		template <typename type>
		void bind_acquired(volatile hazard& h, type* value)
		{
			set_token(h);
			m_token->bind_acquired((void*)value);
		}

		/// @{
		/// @brief Called to confirm that the pointer value bound to the hazard pointer was not stale.
		/// @return True if the hazard::pointer was successfully acquired
		bool validate()
		{
			return (!!m_token) && m_token->validate();
		}
		/// @}

		/// @{
		/// @brief Acquires a hazardo pointer.
		/// @param h hazard pool to bind pointer value in
		/// @param src Volatile reference to pointer to acquire.
		/// @return The value of the pointer acquired, or 0.
		template <typename type>
		type* acquire(volatile hazard& h, type* const volatile& src)
		{
			type* oldValue = atomic::load(src);
			return acquire(h, oldValue, src);
		}

		template <typename type>
		type* acquire(volatile hazard& h, type* value, type* const volatile& src)
		{
			return acquire(h, value, [src{ &src }]{ return atomic::load(*src); });
		}

		/// @brief Acquires a hazard pointer.
		/// @param h hazard pool to bind pointer value in
		/// @param getter A lambda returning the pointer to acquire.
		/// @return The value of the pointer acquired, or 0.
		template <typename F>
		std::enable_if_t<
			std::is_invocable_v<F> && std::is_pointer_v<std::remove_reference_t<std::invoke_result_t<F>>>,
			std::remove_reference_t<std::invoke_result_t<F>>>
		acquire(volatile hazard& h, F&& getter)
		{
			std::remove_reference_t<std::invoke_result_t<F>> oldValue = getter();
			return acquire(h, oldValue, std::forward<F>(getter));
		}

		/// @}
		/// @brief Acquires a hazard pointer.
		/// @param h hazard pool to bind pointer value in
		/// @param initial The initially read value of the pointer to bind
		/// @param getter A lambda returning the pointer to acquire.
		/// @return The value of the pointer acquired, or 0.
		template <typename type, typename F>
		std::enable_if_t<
			std::is_invocable_v<F> && std::is_same_v<type*, std::remove_reference_t<std::invoke_result_t<F>>>,
			type*>
		acquire(volatile hazard& h, type* value, F&& getter)
		{
			type* oldValue = value;
			if (!!oldValue)
			{
				set_token(h);
				do {
					m_token->bind_unacquired(oldValue);
					type* oldValue2 = getter();
					if (oldValue != oldValue2)
						oldValue = oldValue2;
					else
					{
						if (validate())
							break;
						oldValue = getter();
					}
					if (!oldValue)
						break;
				} while (!!oldValue);
			}
			return oldValue;
		}
		/// @}

		/// @{
		/// @brief Releases the pointer value associated with this hazard pointer.
		/// @return If true, the caller is responsible for releasing the resource associated with the pointer.
		bool release()
		{
			if (!!m_token && m_token->release())
			{
				m_hazard->release_token(*m_token);
				m_hazard = 0;
				m_token = 0;
				return true;
			}
			return false;
		}
		/// @}
	};

	bool is_empty() const volatile
	{
		content_t oldContents;
		atomic::load(m_contents, oldContents);
		return !oldContents.m_list;
	}

	/// @{
	/// @brief Gates release of resources protected by this hazard pool
	///
	/// Call hazard::release() when a resource associated with a hazard pool is ready to be released.
	/// If there are no associated hazard pointers, hazard::release() will return true,
	/// and the resource can actually be released.  If there are associated hazard pointers,
	/// hazard::release() will return false, and actual release is deferred until there are no longer associated
	/// hazard pointers.
	/// @param value Pointer value of object to release
	/// @return If true, the caller has ownership and should actually release the object.  If false, actual release is deferred.
	template <typename type>
	bool release(type* value) volatile { return release_inner((void*)value); }
	/// @}
};


}


#endif
