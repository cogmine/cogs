//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_WEAK_RCPTR_LIST
#define COGS_HEADER_COLLECTION_WEAK_RCPTR_LIST

#include <type_traits>


#include "cogs/collections/container_dlist.hpp"
#include "cogs/mem/rcnew.hpp"


namespace cogs {


template <typename T>
class weak_rcptr_list : public object
{
private:
	typedef weak_rcptr_list<T> this_t;

	class node
	{
	public:
		weak_rcptr<T> m_obj;
		rc_obj_base::released_handler_remove_token m_removeToken;
	};

	container_dlist<node> m_list;

	weak_rcptr_list(const this_t& src) = delete;
	this_t& operator=(const this_t& src) = delete;

public:
	class iterator;
	class remove_token;

	class iterator
	{
	private:
		typename container_dlist<node>::volatile_iterator m_itor;

		iterator(const typename container_dlist<node>::volatile_iterator& i) : m_itor(i) { }
		iterator(const typename container_dlist<node>::preallocated_t& i) : m_itor(i) { }

		friend class weak_rcptr_list;

	public:
		void disown() { m_itor.disown(); }
		void disown() volatile { m_itor.disown(); }

		iterator() { }
		iterator(const iterator& i) : m_itor(i.m_itor) { }
		iterator(const remove_token& rt) : m_itor(rt.m_removeToken) { }
		iterator(const volatile iterator& i) : m_itor(i.m_itor) { }
		iterator(const volatile remove_token& rt) : m_itor(rt.m_removeToken) { }

		iterator& operator=(const iterator& i) { m_itor = i.m_itor; return *this; }
		iterator& operator=(const remove_token& rt) { m_itor = rt.m_removeToken; return *this; }
		iterator& operator=(const volatile iterator& i) { m_itor = i.m_itor; return *this; }
		iterator& operator=(const volatile remove_token& rt) { m_itor = rt.m_removeToken; return *this; }
		void operator=(const iterator& i) volatile { m_itor = i.m_itor; }
		void operator=(const remove_token& rt) volatile { m_itor = rt.m_removeToken; }

		bool is_active() const
		{
			if (!m_itor.is_active())
				return false;
			if (!m_itor->m_removeToken) // non rc obj
				return true;
			rc_obj_base* desc = m_itor->m_obj.get_desc();
			if (!desc)
				return true;
			return !desc->is_released();
		}

		bool is_active() const volatile
		{
			iterator itor = *this;
			return itor.is_active();
		}

		bool is_removed() const
		{
			if (!m_itor)
				return false;
			if (m_itor.is_removed())
				return true;
			if (!m_itor->m_removeToken) // non rc obj
				return false;
			rc_obj_base* desc = m_itor->m_obj.get_desc();
			if (!desc)
				return false;
			return desc->is_released();
		}

		bool is_removed() const volatile
		{
			iterator itor = *this;
			return itor.is_removed();
		}

		iterator& operator++()
		{
			if (!!*this)
			{
				for (;;)
				{
					++m_itor;
					if (!is_removed())
						break;
				}
			}
			return *this;
		}

		iterator operator++() volatile
		{
			iterator newValue;
			iterator oldValue(*this);
			for (;;)
			{
				if (!oldValue)
					break;
				newValue = oldValue;
				++newValue;
				if (compare_exchange(newValue, oldValue, oldValue))
					break;
			}
			return newValue;
		}

		iterator& operator--()
		{
			if (!!*this)
			{
				for (;;)
				{
					--m_itor;
					if (!is_removed())
						break;
				}
			}
			return *this;
		}

		iterator operator--() volatile
		{
			iterator newValue;
			iterator oldValue(*this);
			for (;;)
			{
				if (!oldValue)
					break;
				newValue = oldValue;
				--newValue;
				if (compare_exchange(newValue, oldValue, oldValue))
					break;
			}
			return newValue;
		}

		iterator operator++(int) { iterator i(*this); ++*this; return i; }
		iterator operator--(int) { iterator i(*this); --*this; return i; }

		iterator operator++(int) volatile
		{
			iterator oldValue(*this);
			for (;;)
			{
				if (!oldValue)
					break;
				iterator newValue(oldValue);
				++newValue;
				if (compare_exchange(newValue, oldValue, oldValue))
					break;
			}
			return oldValue;
		}

		iterator operator--(int) volatile
		{
			iterator oldValue(*this);
			for (;;)
			{
				if (!oldValue)
					break;
				iterator newValue(oldValue);
				--newValue;
				if (compare_exchange(newValue, oldValue, oldValue))
					break;
			}
			return oldValue;
		}

		bool operator!() const { return !m_itor; }
		bool operator!() const volatile { return !m_itor; }

		bool operator==(const iterator& i) const { return m_itor == i.m_itor; }
		bool operator==(const volatile iterator& i) const { return m_itor == i.m_itor; }
		bool operator==(const iterator& i) const volatile { return m_itor == i.m_itor; }

		bool operator==(const remove_token& rt) const { return m_itor == rt.m_removeToken; }
		bool operator==(const volatile remove_token& rt) const { return m_itor == rt.m_removeToken; }
		bool operator==(const remove_token& rt) const volatile { return m_itor == rt.m_removeToken; }

		bool operator!=(const iterator& i) const { return !operator==(i); }
		bool operator!=(const volatile iterator& i) const { return !operator==(i); }
		bool operator!=(const iterator& i) const volatile { return !operator==(i); }

		bool operator!=(const remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const volatile remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const remove_token& rt) const volatile { return !operator==(rt); }

		rcptr<T> get() const
		{
			rcptr<T> result;
			if (!!m_itor)
				result = m_itor->m_obj;
			return result;
		}

		T& operator*() const { return *get(); }
		rcptr<T> operator->() const { return get(); }

		void release() { m_itor.release(); }
		void release() volatile { m_itor.release(); }

		iterator next() const { iterator result(*this); ++result; return result; }
		iterator prev() const { iterator result(*this); --result; return result; }

		bool compare_exchange(const iterator& src, const iterator& cmp) volatile
		{
			return m_itor(src.m_itor, cmp.m_itor);
		}

		bool compare_exchange(const iterator& src, const iterator& cmp, iterator& rtn) volatile
		{
			return m_itor(src.m_itor, cmp.m_itor, rtn.m_itor);
		}
	};

	class remove_token
	{
	private:
		typename container_dlist<node>::volatile_remove_token m_removeToken;

		remove_token(const typename container_dlist<node>::volatile_remove_token& rt) : m_removeToken(rt) { }
		remove_token(const typename container_dlist<node>::preallocated_t& i) : m_removeToken(i) { }

		friend class weak_rcptr_list;

	public:
		remove_token() { }
		remove_token(const iterator& i) : m_removeToken(i.m_itor) { }
		remove_token(const remove_token& rt) : m_removeToken(rt.m_removeToken) { }
		remove_token(const volatile iterator& i) : m_removeToken(i.m_itor) { }
		remove_token(const volatile remove_token& rt) : m_removeToken(rt.m_removeToken) { }

		remove_token& operator=(const iterator& i) { m_removeToken = i.m_itor; return *this; }
		remove_token& operator=(const remove_token& rt) { m_removeToken = rt.m_removeToken; return *this; }
		remove_token& operator=(const volatile iterator& i) { m_removeToken = i.m_itor; return *this; }
		remove_token& operator=(const volatile remove_token& rt) { m_removeToken = rt.m_removeToken; return *this; }
		void operator=(const iterator& i) volatile { m_removeToken = i.m_itor; }
		void operator=(const remove_token& rt) volatile { m_removeToken = rt.m_removeToken; }

		bool is_active() const { iterator itor(*this); return itor.is_active(); }
		bool is_active() const volatile { iterator itor(*this); return itor.is_active(); }

		void release() { m_removeToken.release(); }
		void release() volatile { m_removeToken.release(); }

		bool operator!() const { return !m_removeToken; }
		bool operator!() const volatile { return !m_removeToken; }

		bool operator==(const iterator& i) const { return m_removeToken == i.m_itor; }
		bool operator==(const volatile iterator& i) const { return m_removeToken == i.m_itor; }
		bool operator==(const iterator& i) const volatile { return m_removeToken == i.m_itor; }

		bool operator==(const remove_token& rt) const { return m_removeToken == rt.m_removeToken; }
		bool operator==(const volatile remove_token& rt) const { return m_removeToken == rt.m_removeToken; }
		bool operator==(const remove_token& rt) const volatile { return m_removeToken == rt.m_removeToken; }

		bool operator!=(const iterator& i) const { return !operator==(i); }
		bool operator!=(const volatile iterator& i) const { return !operator==(i); }
		bool operator!=(const iterator& i) const volatile { return !operator==(i); }

		bool operator!=(const remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const volatile remove_token& rt) const { return !operator==(rt); }
		bool operator!=(const remove_token& rt) const volatile { return !operator==(rt); }
	};

	weak_rcptr_list(const ptr<rc_obj_base>& desc)
		: object(desc)
	{ }

	weak_rcptr_list(const ptr<rc_obj_base>& desc, this_t&& src)
		: object(desc),
		m_list(std::move(src.m_list))
	{ }

	bool drain() volatile
	{
		bool foundAny = false;
		for (;;)
		{
			auto itor = m_list.remove_last();
			if (!itor)
				break;
			if (!itor->m_removeToken) // non rc obj
				foundAny = true;
			else
			{
				if (itor->m_obj.get_desc()->uninstall_released_handler(itor->m_removeToken))
					foundAny |= !itor->m_obj.is_released();
			}
		}

		return foundAny;
	}

	bool is_empty() const volatile { return m_list.is_empty(); }
	bool operator!() const volatile { return m_list.is_empty(); }

	iterator get_first() const volatile
	{
		iterator result;
		for (;;)
		{
			result = m_list.get_first();
			if (!result.is_removed())
				break;
			const_cast<this_t*>(this)->remove(result);
		}
		return result;
	}

	iterator get_last() const volatile
	{
		iterator result;
		for (;;)
		{
			result = m_list.get_last();
			if (!result.is_removed())
				break;
			const_cast<this_t*>(this)->remove(result);
		}
		return result;
	}

	iterator prepend(const rcref<T>& t, insert_mode insertMode = insert_mode::normal) volatile
	{
		typename container_dlist<node>::preallocated_t p = m_list.preallocate();
		p->m_obj = t;
		rc_obj_base* desc = t.get_desc();
		if (!!desc)
		{
			p->m_removeToken = desc->on_released([r{ this_weak_rcptr }, rt{ typename container_dlist<node>::volatile_remove_token(p) }](rc_obj_base&)
			{
				rcptr<volatile this_t> r2 = r;
				if (!!r2)
					r2->m_list.remove(rt);
			});
		}
		iterator result = m_list.prepend_preallocated(p, insertMode);
		if (!result && !!desc)
			desc->uninstall_released_handler(p->m_removeToken);
		return result;
	}

	iterator append(const rcref<T>& t, insert_mode insertMode = insert_mode::normal) volatile
	{
		typename container_dlist<node>::preallocated_t p = m_list.preallocate();
		p->m_obj = t;
		rc_obj_base* desc = t.get_desc();
		if (!!desc)
		{
			p->m_removeToken = desc->on_released([r{ this_weak_rcptr }, rt{ typename container_dlist<node>::volatile_remove_token(p) }](rc_obj_base&)
			{
				rcptr<volatile this_t> r2 = r;
				if (!!r2)
					r2->m_list.remove(rt);
			});
		}
		iterator result = m_list.append_preallocated(p, insertMode);
		if (!result && !!desc)
			desc->uninstall_released_handler(p->m_removeToken);
		return result;
	}

	iterator insert_before(const rcref<T>& t, const iterator& insertBefore) volatile
	{
		typename container_dlist<node>::preallocated_t p = m_list.preallocate();
		p->m_obj = t;
		rc_obj_base* desc = t.get_desc();
		if (!!desc)
		{
			p->m_removeToken = desc->on_released([r{ this_weak_rcptr }, rt{ typename container_dlist<node>::volatile_remove_token(p) }](rc_obj_base&)
			{
				rcptr<volatile this_t> r2 = r;
				if (!!r2)
					r2->m_list.remove(rt);
			});
		}
		iterator result = m_list.insert_preallocated_before(p, insertBefore);
		if (!result && !!desc)
			desc->uninstall_released_handler(p->m_removeToken);
		return result;
	}

	iterator insert_after(const rcref<T>& t, const iterator& insertAfter) volatile
	{
		typename container_dlist<node>::preallocated_t p = m_list.preallocate();
		p->m_obj = t;
		rc_obj_base* desc = t.get_desc();
		if (!!desc)
		{
			p->m_removeToken = desc->on_released([r{ this_weak_rcptr }, rt{ typename container_dlist<node>::volatile_remove_token(p) }](rc_obj_base&)
			{
				rcptr<volatile this_t> r2 = r;
				if (!!r2)
					r2->m_list.remove(rt);
			});
		}
		iterator result = m_list.insert_preallocated_after(p, insertAfter);
		if (!result && !!desc)
			desc->uninstall_released_handler(p->m_removeToken);
		return result;
	}

	bool remove(const iterator& i) volatile
	{
		if (!i)
			return false;
		if (!m_list.remove(i.m_itor))
			return false;

		rc_obj_base* desc = i.m_itor->m_obj.get_desc();
		if (!!desc)
			desc->uninstall_released_handler(i.m_itor->m_removeToken);
		return true;
	}

	bool remove(const remove_token& rt) volatile
	{
		iterator i(rt);
		return remove(i);
	}


	//bool peek_first(type& t) volatile;
	//bool peek_last(type& t) volatile;

	//bool pop_first(type& t, bool& wasLast) volatile;
	//bool pop_first(type& t) volatile;

	//bool pop_last(type& t, bool& wasLast) volatile;
	//bool pop_last(type& t) volatile;

	//bool remove_first(bool& wasLast) volatile;
	//bool remove_first() volatile;

	//bool remove_last(bool& wasLast) volatile;
	//bool remove_last() volatile;
};


}


#endif
