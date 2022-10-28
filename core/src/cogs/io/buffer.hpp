//
//  Copyright (C) 2000-2022 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_IO_BUFFER
#define COGS_HEADER_IO_BUFFER


#include "cogs/collections/string.hpp"
#include "cogs/collections/vector.hpp"
#include "cogs/sync/hazard.hpp"


namespace cogs {
namespace io {


class composite_buffer_content;
class composite_buffer;


/// @ingroup IO
/// @brief A reference to dynamically allocated data (opaque bytes) used in I/O.  Copies are shallow.
///
/// A buffer is essentially a vector<char> with routines added that accept void*.
/// Conversions from a buffer to a vector<char> or cstring, are supported.
class buffer
{
public:
	typedef size_t position_t;

protected:
	friend class composite_buffer_content;
	friend class composite_buffer;

	typedef vector_descriptor<char> desc_t;
	typedef vector_content<char> content_t;

	typedef transactable<content_t> transactable_t;
	transactable_t m_contents;

	typedef typename transactable_t::read_token read_token;
	typedef typename transactable_t::write_token write_token;

	// null desc, used for literals or caller owned buffers
	buffer(size_t n, void* p) : m_contents(typename transactable_t::construct_embedded_t(), (char*)p, n) { }
	buffer(size_t n, const void* p) : m_contents(typename transactable_t::construct_embedded_t(), (char*)const_cast<void*>(p), n) { }

	read_token guarded_begin_read() const volatile
	{
		read_token rt;
		guarded_begin_read(rt);
		return rt;
	}

	void guarded_begin_read(read_token& rt) const volatile
	{
		volatile hazard& h = rc_obj_base::get_hazard();
		desc_t* desc;
		hazard::pointer p;
		for (;;)
		{
			m_contents.begin_read(rt);
			desc = rt->m_desc;
			if (!desc)
				break;
			p.bind_unacquired(h, desc);
			if (!m_contents.is_current(rt) || !p.validate())
				continue;
			bool acquired = desc->acquire();
			if (p.release())
				desc->dispose();
			if (!acquired)
				continue;
			break;
		}
	}

	write_token guarded_begin_write() volatile
	{
		write_token wt;
		guarded_begin_write(wt);
		return wt;
	}

	void guarded_begin_write(write_token& wt) volatile
	{
		volatile hazard& h = rc_obj_base::get_hazard();
		desc_t* desc;
		hazard::pointer p;
		for (;;)
		{
			m_contents.begin_write(wt);
			desc = wt->m_desc;
			if (!desc)
				break;
			p.bind_unacquired(h, desc);
			if (!m_contents.is_current(wt) || !p.validate())
				continue;
			bool acquired = desc->acquire();
			if (p.release())
				desc->dispose();
			if (!acquired)
				continue;
			break;
		}
	}

	void set(desc_t* d, void* p, size_t n) { m_contents->set(d, (char*)p, n); }

	void disown() { m_contents->m_desc = nullptr; }
	void disown() volatile
	{
		write_token wt;
		do {
			m_contents.begin_write(wt);
			wt->m_desc = nullptr;
		} while (!m_contents.end_write(wt));
	}

	desc_t* get_desc() const { return m_contents->m_desc; }

public:
	operator const vector<char>() const { return get_vector(); }
	operator const cstring() const { return get_cstring(); }

	const vector<char>& get_vector(unowned_t<vector<char> >& storage = unowned_t<vector<char> >().get_unowned()) const
	{
		storage.set(m_contents->m_desc, m_contents->m_ptr, m_contents->m_length);
		return storage;
	}

	static buffer& from_vector(vector<char>& v, unowned_t<buffer>& storage = unowned_t<buffer>().get_unowned())
	{
		storage.set(v.get_desc(), (void*)v.get_raw_ptr(), v.get_length());
		return storage;
	}

	static const buffer& from_vector(const vector<char>& v, unowned_t<buffer>& storage = unowned_t<buffer>().get_unowned())
	{
		storage.set(v.get_desc(), (void*)v.get_raw_ptr(), v.get_length());
		return storage;
	}

	static buffer& from_cstring(cstring& s, unowned_t<buffer>& storage = unowned_t<buffer>().get_unowned())
	{
		storage.set(s.get_desc(), s.get_raw_ptr(), s.get_length());
		return storage;
	}

	static const buffer& from_cstring(const cstring& s, unowned_t<buffer>& storage = unowned_t<buffer>().get_unowned())
	{
		storage.set(s.get_desc(), s.get_raw_ptr(), s.get_length());
		return storage;
	}

	const cstring& get_cstring(unowned_t<cstring>& storage = unowned_t<cstring>().get_unowned()) const
	{
		storage.set(m_contents->m_desc, m_contents->m_ptr, m_contents->m_length);
		return storage;
	}

	const cstring get_cstring() const volatile
	{
		cstring s;
		read_token rt;
		guarded_begin_read(rt);
		s.set(rt->m_desc, rt->m_ptr, rt->m_length);
		return s;
	}

	/// @brief A buffer data iterator
	class iterator
	{
	protected:
		buffer* m_buffer;
		size_t m_index;

		iterator(buffer* v, size_t i)
			: m_buffer(v),
			m_index(i)
		{ }

		friend class buffer;

	public:
		iterator() { }
		iterator(const iterator& i) : m_buffer(i.m_buffer), m_index(i.m_index) { }

		void release() { m_buffer = 0; }

		iterator& operator++()
		{
			if (m_buffer)
			{
				m_index++;
				if (m_index >= m_buffer->get_length())
					m_buffer = 0;
			}

			return *this;
		}

		iterator& operator--()
		{
			if (m_buffer)
			{
				if (m_index == 0)
					m_buffer = 0;
				else
					--m_index;
			}

			return *this;
		}

		iterator operator++(int) { iterator i(*this); ++*this; return i; }
		iterator operator--(int) { iterator i(*this); --*this; return i; }

		bool operator!() const { return !m_buffer || (m_index >= m_buffer->get_length()); }

		bool operator==(const iterator& i) const { return (m_buffer == i.m_buffer) && (!m_buffer || (m_index == i.m_index)); }
		bool operator!=(const iterator& i) const { return !operator==(i); }
		iterator& operator=(const iterator& i) { m_buffer = i.m_buffer; m_index = i.m_index; return *this; }

		char* get() const { return m_buffer->get_ptr() + m_index; }
		char& operator*() const { return *get(); }
		char* operator->() const { return get(); }

		size_t get_position() const { return m_index; }

		iterator next() const { iterator result(*this); ++result; return result; }
		iterator prev() const { iterator result(*this); --result; return result; }
	};

	iterator get_first_iterator() { size_t sz = get_length(); return iterator(!!sz ? this : 0, 0); }
	iterator get_last_iterator() { size_t sz = get_length(); return iterator(!!sz ? this : 0, sz - 1); }

	/// @brief A buffer const data iterator
	class const_iterator
	{
	protected:
		const buffer* m_buffer;
		size_t m_index;

		const_iterator(const buffer* v, size_t i)
			: m_buffer(v),
			m_index(i)
		{ }

		friend class buffer;

	public:
		const_iterator() { }
		const_iterator(const const_iterator& i) : m_buffer(i.m_buffer), m_index(i.m_index) { }
		const_iterator(const iterator& i) : m_buffer(i.m_buffer), m_index(i.m_index) { }

		void release() { m_buffer = 0; }

		const_iterator& operator++()
		{
			if (m_buffer)
			{
				m_index++;
				if (m_index >= m_buffer->get_length())
					m_buffer = 0;
			}

			return *this;
		}

		const_iterator& operator--()
		{
			if (m_buffer)
			{
				if (m_index == 0)
					m_buffer = 0;
				else
					--m_index;
			}

			return *this;
		}

		const_iterator operator++(int) { const_iterator i(*this); ++*this; return i; }
		const_iterator operator--(int) { const_iterator i(*this); --*this; return i; }

		bool operator!() const { return !m_buffer || (m_index >= m_buffer->get_length()); }

		bool operator==(const const_iterator& i) const { return (m_buffer == i.m_buffer) && (!m_buffer || (m_index == i.m_index)); }
		bool operator==(const iterator& i) const { return (m_buffer == i.m_buffer) && (!m_buffer || (m_index == i.m_index)); }
		bool operator!=(const const_iterator& i) const { return !operator==(i); }
		bool operator!=(const iterator& i) const { return !operator==(i); }
		const_iterator& operator=(const const_iterator& i) { m_buffer = i.m_buffer; m_index = i.m_index; return *this; }
		const_iterator& operator=(const iterator& i) { m_buffer = i.m_buffer; m_index = i.m_index; return *this; }

		const char* get() const { return (m_buffer->get_const_ptr() + m_index); }
		const char& operator*() const { return *get(); }
		const char* operator->() const { return get(); }

		size_t get_position() const { return m_index; }

		const_iterator next() const { const_iterator result(*this); ++result; return result; }
		const_iterator prev() const { const_iterator result(*this); --result; return result; }
	};

	const_iterator get_first_const_iterator() const { size_t sz = get_length(); return const_iterator(!!sz ? this : 0, 0); }
	const_iterator get_last_const_iterator() const { size_t sz = get_length(); return const_iterator(!!sz ? this : 0, sz - 1); }

	// contains specified buffer (allocates if ptr is NULL)
	static buffer contain(const void* ptr, size_t sz) { return buffer(sz, ptr); }

	buffer()
	{ }

	buffer(const buffer& src)
	{
		m_contents->acquire(*(src.m_contents));
	}

	buffer(const volatile buffer& src)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.guarded_begin_read())) // takes ownership of guarded desc reference
	{ }

	explicit buffer(size_t n)
	{
		if (n)
			m_contents->allocate(n);
	}

	buffer(size_t n, const char& src)
	{
		if (n)
			m_contents->allocate(n, src);
	}

	buffer(const char* src, size_t n)
	{
		if (n)
			m_contents->allocate(src, n);
	}

	buffer(const buffer& src, size_t i)
	{
		m_contents->acquire(*(src.m_contents), i);
	}

	buffer(const volatile buffer& src, size_t i)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.guarded_begin_read())) // takes ownership of guarded desc reference
	{
		set_to_subrange(i);
	}


	buffer(const buffer& src, size_t i, size_t n)
	{
		m_contents->acquire(*(src.m_contents), i, n);
	}

	buffer(const volatile buffer& src, size_t i, size_t n)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.guarded_begin_read())) // takes ownership of guarded desc reference
	{
		set_to_subrange(i, n);
	}

	~buffer() { m_contents->release(); }

	size_t get_length() const { return m_contents->m_length; }
	size_t get_length() const volatile { return m_contents.begin_read()->m_length; }

	size_t get_capacity() const { return m_contents->get_capacity(); }
	size_t get_reverse_capacity() const { return m_contents->get_reverse_capacity(); }

	bool is_empty() const { return m_contents->m_length == 0; }
	bool is_empty() const volatile { return m_contents.begin_read()->m_length == 0; }

	bool operator!() const { return m_contents->m_length == 0; }
	bool operator!() const volatile { return m_contents.begin_read()->m_length == 0; }

	const char* get_const_ptr() const { return m_contents->m_ptr; }

	// caller error to pass index >= length
	const char& operator[](size_t i) const { return get_const_ptr()[i]; }

	const char& get_first_const() const { return get_const_ptr()[0]; }
	const char& get_last_const() const { return get_const_ptr()[get_length() - 1]; }


	const buffer& subrange(size_t i, size_t n = const_max_int_v<size_t>, unowned_t<buffer>& storage = unowned_t<buffer>().get_unowned()) const
	{
		size_t length = get_length();
		if (i <= length)
		{
			size_t adjustedLength = length - i;
			if (adjustedLength > n)
				adjustedLength = n;
			storage.set(m_contents->m_desc, m_contents->m_ptr + i, adjustedLength);
		}
		return storage;
	}

	buffer subrange(size_t i, size_t n = const_max_int_v<size_t>) const volatile
	{
		buffer result(*this);
		result.set_to_subrange(i, n);
		return result;
	}


	int compare(size_t n, const char& cmp) const { return m_contents->template compare<char, default_comparator>(n, cmp); }

	int compare(size_t n, const char& cmp) const volatile { buffer tmp(*this); return tmp.compare(n, cmp); }

	int compare(const char* cmp, size_t n) const { return m_contents->template compare<char, default_comparator>(cmp, n); }

	int compare(const char* cmp, size_t n) const volatile { buffer tmp(*this); return tmp.compare(cmp, n); }

	int compare(const buffer& cmp) const { return compare(cmp.get_const_ptr(), cmp.get_length()); }

	int compare(const buffer& cmp) const volatile { return compare(cmp.get_const_ptr(), cmp.get_length()); }

	int compare(const volatile buffer& cmp) const { buffer tmp(cmp); return compare(tmp); }


	bool equals(size_t n, const char& cmp) const { return m_contents->template equals<char, default_comparator>(n, cmp); }

	bool equals(size_t n, const char& cmp) const volatile { buffer tmp(*this); return tmp.equals(n, cmp); }

	bool equals(const char* cmp, size_t n) const { return m_contents->template equals<char, default_comparator>(cmp, n); }

	bool equals(const char* cmp, size_t n) const volatile { buffer tmp(*this); return tmp.equals(cmp, n); }

	bool equals(const buffer& cmp) const { return equals(cmp.get_const_ptr(), cmp.get_length()); }

	bool equals(const buffer& cmp) const volatile { return equals(cmp.get_const_ptr(), cmp.get_length()); }

	bool equals(const volatile buffer& cmp) const { buffer tmp(cmp); return equals(tmp); }


	bool operator==(const buffer& cmp) const { return equals(cmp); }

	bool operator==(const buffer& cmp) const volatile { return equals(cmp); }

	bool operator==(const volatile buffer& cmp) const { return equals(cmp); }

	bool operator!=(const buffer& cmp) const { return !equals(cmp); }

	bool operator!=(const buffer& cmp) const volatile { return !equals(cmp); }

	bool operator!=(const volatile buffer& cmp) const { return !equals(cmp); }


	bool starts_with(size_t n, const char& cmp) const { return m_contents->template starts_with<char, default_comparator>(n, cmp); }

	bool starts_with(size_t n, const char& cmp) const volatile { buffer tmp(*this); return tmp.starts_with(n, cmp); }

	bool starts_with(const char* cmp, size_t n) const { return m_contents->template starts_with<char, default_comparator>(cmp, n); }

	bool starts_with(const char* cmp, size_t n) const volatile { buffer tmp(*this); return tmp.starts_with(cmp, n); }

	bool starts_with(const buffer& cmp) const { return starts_with(cmp.get_const_ptr(), cmp.get_length()); }

	bool starts_with(const buffer& cmp) const volatile { return starts_with(cmp.get_const_ptr(), cmp.get_length()); }

	bool starts_with(const volatile buffer& cmp) const { buffer tmp(cmp); return starts_with(tmp); }


	bool ends_with(size_t n, const char& cmp) const { return m_contents->template ends_with<char, default_comparator>(n, cmp); }

	bool ends_with(size_t n, const char& cmp) const volatile { buffer tmp(*this); return tmp.ends_with(n, cmp); }

	bool ends_with(const char* cmp, size_t n) const { return m_contents->template ends_with<char, default_comparator>(cmp, n); }

	bool ends_with(const char* cmp, size_t n) const volatile { buffer tmp(*this); return tmp.ends_with(cmp, n); }

	bool ends_with(const buffer& cmp) const { return ends_with(cmp.get_const_ptr(), cmp.get_length()); }

	bool ends_with(const buffer& cmp) const volatile { return ends_with(cmp.get_const_ptr(), cmp.get_length()); }

	bool ends_with(const volatile buffer& cmp) const { buffer tmp(cmp); return ends_with(tmp); }


	bool is_less_than(const char* cmp, size_t n) const { return m_contents->template is_less_than<char, default_comparator>(cmp, n); }

	bool is_less_than(const char* cmp, size_t n) const volatile { buffer tmp(*this); return tmp.is_less_than(cmp, n); }

	bool is_less_than(const buffer& cmp) const { return is_less_than(cmp.get_const_ptr(), cmp.get_length()); }

	bool is_less_than(const buffer& cmp) const volatile { return is_less_than(cmp.get_const_ptr(), cmp.get_length()); }

	bool is_less_than(const volatile buffer& cmp) const { buffer tmp(cmp); return is_less_than(tmp); }


	bool is_greater_than(const char* cmp, size_t n) const { return m_contents->template is_greater_than<char, default_comparator>(cmp, n); }

	bool is_greater_than(const char* cmp, size_t n) const volatile { buffer tmp(*this); return tmp.is_greater_than(cmp, n); }

	bool is_greater_than(const buffer& cmp) const { return is_greater_than(cmp.get_const_ptr(), cmp.get_length()); }

	bool is_greater_than(const buffer& cmp) const volatile { return is_greater_than(cmp.get_const_ptr(), cmp.get_length()); }

	bool is_greater_than(const volatile buffer& cmp) const { buffer tmp(cmp); return is_greater_than(tmp); }


	bool operator<(const buffer& cmp) const { return is_less_than(cmp); }

	bool operator<(const buffer& cmp) const volatile { return is_less_than(cmp); }

	bool operator<(const volatile buffer& cmp) const { return is_less_than(cmp); }

	bool operator>=(const buffer& cmp) const { return !is_less_than(cmp); }

	bool operator>=(const buffer& cmp) const volatile { return !is_less_than(cmp); }

	bool operator>=(const volatile buffer& cmp) const { return !is_less_than(cmp); }

	bool operator>(const buffer& cmp) const { return is_greater_than(cmp); }

	bool operator>(const buffer& cmp) const volatile { return is_greater_than(cmp); }

	bool operator>(const volatile buffer& cmp) const { return is_greater_than(cmp); }

	bool operator<=(const buffer& cmp) const { return !is_greater_than(cmp); }

	bool operator<=(const buffer& cmp) const volatile { return !is_greater_than(cmp); }

	bool operator<=(const volatile buffer& cmp) const { return !is_greater_than(cmp); }


	size_t index_of(const char& cmp) const { return m_contents->template index_of<char, default_comparator>(0, cmp); }

	size_t index_of(const char& cmp) const volatile { buffer tmp(*this); return tmp.index_of(cmp); }

	size_t index_of(size_t i, const char& cmp) const { return m_contents->template index_of<char, default_comparator>(i, cmp); }

	size_t index_of(size_t i, const char& cmp) const volatile { buffer tmp(*this); return tmp.index_of(i, cmp); }


	size_t index_of_any(const char* cmp, size_t n) const { return m_contents->template index_of_any<char, default_comparator>(0, cmp, n); }

	size_t index_of_any(const char* cmp, size_t n) const volatile { buffer tmp(*this); return tmp.index_of_any(cmp, n); }

	size_t index_of_any(size_t i, const char* cmp, size_t n) const { return m_contents->template index_of_any<char, default_comparator>(i, cmp, n); }

	size_t index_of_any(size_t i, const char* cmp, size_t n) const volatile { buffer tmp(*this); return tmp.index_of_any(i, cmp, n); }


	size_t index_of_segment(const char* cmp, size_t n) const { return m_contents->template index_of_segment<char, default_comparator>(0, cmp, n); }

	size_t index_of_segment(const char* cmp, size_t n) const volatile { buffer tmp(*this); return tmp.index_of_segment(cmp, n); }

	size_t index_of_segment(const buffer& cmp) const { return index_of_segment(cmp.get_const_ptr(), cmp.get_length()); }

	size_t index_of_segment(const buffer& cmp) const volatile { return index_of_segment(cmp.get_const_ptr(), cmp.get_length()); }

	size_t index_of_segment(const volatile buffer& cmp) const { buffer tmp(cmp); return index_of_segment(tmp); }


	size_t index_of_segment(size_t i, const char* cmp, size_t n) const { return m_contents->template index_of_segment<char, default_comparator>(i, cmp, n); }

	size_t index_of_segment(size_t i, const char* cmp, size_t n) const volatile { buffer tmp(*this); return tmp.index_of_segment(i, cmp, n); }

	size_t index_of_segment(size_t i, const buffer& cmp) const { return index_of_segment(i, cmp.get_const_ptr(), cmp.get_length()); }

	size_t index_of_segment(size_t i, const buffer& cmp) const volatile { return index_of_segment(i, cmp.get_const_ptr(), cmp.get_length()); }

	size_t index_of_segment(size_t i, const volatile buffer& cmp) const { buffer tmp(cmp); return index_of_segment(i, tmp); }


	bool contains(const char& cmp) const { return index_of(cmp) != const_max_int_v<size_t>; }

	bool contains(const char& cmp) const volatile { return index_of(cmp) != const_max_int_v<size_t>; }

	//bool contains(size_t i, const char& cmp) const { return index_of(i, cmp) != const_max_int_v<size_t>; }

	//bool contains(size_t i, const char& cmp) const volatile { return index_of(i, cmp) != const_max_int_v<size_t>; }


	bool contains_any(const char* cmp, size_t n) const { return index_of_any(cmp, n) != const_max_int_v<size_t>; }

	bool contains_any(const char* cmp, size_t n) const volatile { return index_of_any(cmp, n) != const_max_int_v<size_t>; }

	//bool contains_any(size_t i, const char* cmp, size_t n) const { return index_of_any(i, cmp, n) != const_max_int_v<size_t>; }

	//bool contains_any(size_t i, const char* cmp, size_t n) const volatile { return index_of_any(i, cmp, n) != const_max_int_v<size_t>; }


	bool contains_segment(const char* cmp, size_t n) const { return index_of_segment(cmp, n) != const_max_int_v<size_t>; }

	bool contains_segment(const char* cmp, size_t n) const volatile { return index_of_segment(cmp, n) != const_max_int_v<size_t>; }

	bool contains_segment(const buffer& cmp) const { return index_of_segment(cmp) != const_max_int_v<size_t>; }

	bool contains_segment(const buffer& cmp) const volatile { return index_of_segment(cmp) != const_max_int_v<size_t>; }

	bool contains_segment(const volatile buffer& cmp) const { return index_of_segment(cmp) != const_max_int_v<size_t>; }


	char* get_ptr() { return m_contents->m_ptr; }
	char& get_first() { return m_contents->m_ptr[0]; }
	char& get_last() { return m_contents->m_ptr[m_contents->m_length - 1]; }

	void set_index(size_t i, const char& src) { get_ptr()[i] = src; }

	void reverse() { m_contents->reverse(); }

	void set_to_subrange(size_t i) { m_contents->advance(i); }
	void set_to_subrange(size_t i) volatile
	{
		write_token wt;
		do {
			m_contents.begin_write(wt);
			if (i >= wt->m_length)
				wt->m_length = 0;
			else
			{
				wt->m_ptr += i;
				wt->m_length -= i;
			}
		} while (!m_contents.end_write(wt));
	}

	void set_to_subrange(size_t i, size_t n) { m_contents->set_to_subrange(i, n); }

	void set_to_subrange(size_t i, size_t n) volatile
	{
		write_token wt;
		do {
			m_contents.begin_write(wt);
			size_t length = wt->m_length;
			if (i >= length)
				wt->m_length = 0;
			else
			{
				size_t remainingLength = length - i;
				if (n > remainingLength)
					n = remainingLength;
				wt->m_ptr += i;
				wt->m_length = n;
			}
		} while (!m_contents.end_write(wt));
	}

	// clears reserved space as well
	void release()
	{
		m_contents->release();
		m_contents->clear_inner();
	}

	// clears reserved space as well
	void release() volatile
	{
		content_t tmp;
		m_contents.swap_contents(tmp);
		tmp.release();
	}

	void copy(size_t n, const char& src)
	{
		size_t n2 = m_contents->m_length;
		if (n > n2)
			n = n2;
		memset(m_contents->m_ptr, src, n);
	}

	void copy(const char* src, size_t n)
	{
		size_t n2 = m_contents->m_length;
		if (n > n2)
			n = n2;
		memmove(m_contents->m_ptr, src, n);
	}

	void copy(size_t i, const char* src, size_t n)
	{
		size_t n2 = m_contents->m_length;
		if (i < n2)
		{
			n2 -= i;
			if (n > n2)
				n = n2;
			memmove(m_contents->m_ptr + i, src, n);
		}
	}


	void assign(const buffer& src)
	{
		if (this != &src)
			m_contents->acquire(*(src.m_contents));
	}

	void assign(const buffer& src) volatile
	{
		buffer tmp(src);
		swap(tmp);
	}

	void assign(const volatile buffer& src)
	{
		buffer tmp(src);
		assign(tmp);
	}


	buffer& operator=(const buffer& src)
	{
		assign(src);
		return *this;
	}

	buffer operator=(const buffer& src) volatile
	{
		assign(src);
		return src;
	}

	buffer& operator=(const volatile buffer& src)
	{
		assign(src);
		return *this;
	}

	void advance(size_t n = 1) { m_contents->advance(n); }

	void truncate_to(size_t n) { m_contents->truncate_to(n); }

	void truncate(size_t n) { m_contents->truncate(n); }

	void truncate_to_right(size_t n) { m_contents->truncate_to_right(n); }


	void replace(size_t i, size_t replaceLength, const char& src)
	{
		m_contents->replace(i, replaceLength, src);
	}

	void replace(size_t i, const char* src, size_t replaceLength)
	{
		m_contents->replace(i, src, replaceLength);
	}

	void replace(size_t i, const buffer& src)
	{
		replace(i, src.m_contents->m_ptr, src.get_length());
	}

	void replace(size_t i, const volatile buffer& src)
	{
		buffer tmp(src);
		replace(i, tmp);
	}


	//void replace(size_t i, const volatile buffer& src, size_t srcIndex, size_t n)
	//{
	//	if (!!n)
	//	{
	//		buffer tmp(src, srcIndex, n);
	//		replace(i, src);
	//	}
	//}


	void swap(buffer& wth) { m_contents.swap(wth.m_contents); }

	void swap(buffer& wth) volatile { m_contents.swap(wth.m_contents); }

	void swap(volatile buffer& wth) { wth.swap(*this); }

	buffer split_off_before(size_t i)
	{
		buffer result;
		if (!!i)
		{
			if (i >= m_contents->m_length)
			{
				result = *this;
				release();
			}
			else
			{
				result.m_contents->acquire(*m_contents, 0, i);
				m_contents->m_length -= i;
				m_contents->m_ptr += i;
			}
		}
		return result;
	}

	buffer split_off_before(size_t i) volatile
	{
		buffer result;
		if (!!i)
		{
			write_token wt;
			desc_t* desc;
			for (;;)
			{
				guarded_begin_write(wt);
				desc = wt->m_desc; // acquired, regardless of whether the commit succeeds.
				result.m_contents->m_desc = desc;
				result.m_contents->m_ptr = wt->m_ptr;
				size_t length = wt->m_length;
				if (i >= length)
				{
					result.m_contents->m_length = length;
					wt->clear_inner();
					if (desc) // Ok to release now.  If commit succeeds, we know it remained valid up until then.
						desc->release();
					desc = 0;
				}
				else
				{
					result.m_contents->m_length = i;
					wt->m_ptr += i;
					wt->m_length -= i;
				}
				if (!!m_contents.end_write(wt))
					break;
				if (desc) // if commit unsuccessful, and i < length, we need to release 1 reference
					desc->release();
				result.m_contents->m_desc = 0;
			}
		}
		return result;
	}

	buffer split_off_after(size_t i)
	{
		buffer result;
		size_t length = m_contents->m_length;
		if (i < length)
		{
			result.m_contents->acquire(*m_contents, i, length - i);
			m_contents->m_length = i;
		}
		return result;
	}

	buffer split_off_after(size_t i) volatile
	{
		buffer result;
		if (!i)
			swap(result);
		else
		{
			write_token wt;
			for (;;)
			{
				guarded_begin_write(wt);
				desc_t* desc = wt->m_desc; // acquired, regardless of whether the commit succeeds.
				size_t length = wt->m_length;
				if (i >= length) // nop.  If nothing to split off back, we don't need to write at all.
				{
					result.set(0, 0, 0);
					if (desc) // Release our copy from guard
						desc->release();
					break;
				}
				size_t remainingLength = length - i;
				result.set(desc, wt->m_ptr + i, remainingLength);
				wt->m_length = i;
				if (!!m_contents.end_write(wt))
					break;
				if (desc)
					desc->release();
				result.m_contents->m_desc = 0;
			}
		}
		return result;
	}

	buffer split_off_front(size_t n) { return split_off_before(n); }

	buffer split_off_front(size_t n) volatile { return split_off_before(n); }

	buffer split_off_back(size_t n)
	{
		buffer result;
		if (!!n)
		{
			if (n >= m_contents->m_length)
			{
				result = *this;
				release();
			}
			else
			{
				m_contents->m_length -= n;
				result.m_contents->acquire(*m_contents, m_contents->m_length, n);
			}
		}
		return result;
	}

	buffer split_off_back(size_t n) volatile
	{
		buffer result;
		if (!!n)
		{
			write_token wt;
			desc_t* desc;
			for (;;)
			{
				guarded_begin_write(wt);
				desc = wt->m_desc; // acquired, regardless of whether the commit succeeds.
				result.m_contents->m_ptr = wt->m_ptr;
				result.m_contents->m_desc = desc;
				size_t length = wt->m_length;
				if (n >= length)
				{
					result.m_contents->m_length = length;
					wt->clear_inner();
					if (desc) // Ok to release now.  If commit succeeds, we know it remained valid up until then.
						desc->release();
					desc = 0;
				}
				else
				{
					size_t newLength = length - n;
					result.m_contents->m_length = n;
					result.m_contents->m_ptr += newLength;
					wt->m_length = newLength;
				}
				if (!!m_contents.end_write(wt))
					break;
				if (desc) // if commit unsuccessful, and i < length, we need to release 1 reference
					desc->release();
				result.m_contents->m_desc = 0;
			}
		}
		return result;
	}


	vector<buffer> split_on(const char& splitOn, include_empty_segments opt = include_empty_segments::yes) const
	{
		return split_on_any_inner(&splitOn, 1, opt);
	}

	vector<buffer> split_on(const char& splitOn, include_empty_segments opt = include_empty_segments::yes) const volatile
	{
		buffer tmp(*this);
		return tmp.split_on_any_inner(&splitOn, 1, opt);
	}


	vector<buffer> split_on_any(const char* splitOn, size_t n, include_empty_segments opt = include_empty_segments::yes) const
	{
		return split_on_any_inner(splitOn, n, opt);
	}

	vector<buffer> split_on_any(const char* splitOn, size_t n, include_empty_segments opt = include_empty_segments::yes) const volatile
	{
		buffer tmp(*this);
		return tmp.split_on_any_inner(splitOn, n, opt);
	}

	vector<buffer> split_on_any(const vector<char>& splitOn, include_empty_segments opt = include_empty_segments::yes) const
	{
		return split_on_any_inner(splitOn.get_const_ptr(), splitOn.get_length(), opt);
	}

	vector<buffer> split_on_any(const vector<char>& splitOn, include_empty_segments opt = include_empty_segments::yes) const volatile
	{
		buffer tmp(*this);
		return tmp.split_on_any_inner(splitOn.get_const_ptr(), splitOn.get_length(), opt);
	}

	vector<buffer> split_on_any(const volatile vector<char>& splitOn, include_empty_segments opt = include_empty_segments::yes) const
	{
		vector<char> tmp(splitOn);
		return split_on_any_inner(tmp.get_const_ptr(), tmp.get_length(), opt);
	}


	vector<buffer> split_on_segment(const char* splitOn, size_t n, include_empty_segments opt = include_empty_segments::yes) const
	{
		return split_on_segment_inner(splitOn, n, opt);
	}

	vector<buffer> split_on_segment(const char* splitOn, size_t n, include_empty_segments opt = include_empty_segments::yes) const volatile
	{
		buffer tmp(*this);
		return tmp.split_on_segment_inner(splitOn, n, opt);
	}

	vector<buffer> split_on_segment(const vector<char>& splitOn, include_empty_segments opt = include_empty_segments::yes) const
	{
		return split_on_segment_inner(splitOn.get_const_ptr(), splitOn.get_length(), opt);
	}

	vector<buffer> split_on_segment(const vector<char>& splitOn, include_empty_segments opt = include_empty_segments::yes) const volatile
	{
		buffer tmp(*this);
		return tmp.split_on_segment_inner(splitOn.get_const_ptr(), splitOn.get_length(), opt);
	}

	vector<buffer> split_on_segment(const volatile vector<char>& splitOn, include_empty_segments opt = include_empty_segments::yes) const
	{
		vector<char> tmp(splitOn);
		return split_on_segment_inner(tmp.get_const_ptr(), tmp.get_length(), opt);
	}


protected:
	vector<buffer> split_on_any_inner(const char* splitOn, size_t n, include_empty_segments opt = include_empty_segments::yes) const
	{
		vector<buffer> result;
		size_t lastStart = 0;
		size_t i = 0;
		for (;;)
		{
			i = index_of_any(i, splitOn, n);
			size_t segmentLength = (i != const_max_int_v<size_t>) ? i : get_length();
			segmentLength -= lastStart;
			if (!!segmentLength || (opt == include_empty_segments::yes))
				result.append(1, subrange(lastStart, segmentLength));
			if (i == const_max_int_v<size_t>)
				break;
			lastStart = ++i;
		}
		return result;
	}

	vector<buffer> split_on_segment_inner(const char* splitOn, size_t n, include_empty_segments opt = include_empty_segments::yes) const
	{
		vector<buffer> result;
		size_t lastStart = 0;
		size_t i = 0;
		for (;;)
		{
			i = index_of_segment(i, splitOn, n);
			size_t segmentLength = (i != const_max_int_v<size_t>) ? i : get_length();
			segmentLength -= lastStart;
			if (!!segmentLength || (opt == include_empty_segments::yes))
				result.append(1, subrange(lastStart, segmentLength));
			if (i == const_max_int_v<size_t>)
				break;
			i += n;
			lastStart = i;
		}
		return result;
	}

public:
	//template <typename char_t>
	//string_t<char_t> to_string_t() const
	//{
	//	string_t<char_t> result;
	//	size_t n = get_length();
	//	if (n > 0)
	//	{
	//		result.resize(n * 2);
	//		const char* src = get_const_ptr();
	//		char_t* dst = result.get_ptr();
	//		for (size_t i = 0; i < n; i++)
	//		{
	//			const char c = src[i];
	//			const char c1 = c >> 4;
	//			size_t pos = i * 2;
	//			dst[pos] = c1 + ((c1 < 10) ? '0' : ('A' - 10));
	//			const char c2 = c & 0x0F;
	//			dst[pos + 1] = (char_t)(c2 + ((c2 < 10) ? '0' : ('A' - 10)));
	//		}
	//	}
	//	return result;
	//}

	template <typename char_t>
	string_t<char_t> to_string_t() const
	{
		static constexpr char hex_table[] =
			"000102030405060708090A0B0C0D0E0F"
			"101112131415161718191A1B1C1D1E1F"
			"202122232425262728292A2B2C2D2E2F"
			"303132333435363738393A3B3C3D3E3F"
			"404142434445464748494A4B4C4D4E4F"
			"505152535455565758595A5B5C5D5E5F"
			"606162636465666768696A6B6C6D6E6F"
			"707172737475767778797A7B7C7D7E7F"
			"808182838485868788898A8B8C8D8E8F"
			"909192939495969798999A9B9C9D9E9F"
			"A0A1A2A3A4A5A6A7A8A9AAABACADAEAF"
			"B0B1B2B3B4B5B6B7B8B9BABBBCBDBEBF"
			"C0C1C2C3C4C5C6C7C8C9CACBCCCDCECF"
			"D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF"
			"E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF"
			"F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF";

		string_t<char_t> result;
		size_t n = get_length();
		if (n > 0)
		{
			result.resize(n * 2);
			const char* src = get_const_ptr();
			char_t* dst = result.get_ptr();
			for (size_t i = 0; i < n; i++)
			{
				const char c = src[i];
				size_t pos = i * 2;
				const char* tablePos = hex_table + ((unsigned char)c * 2);
				dst[pos] = (char_t)tablePos[0];
				dst[pos + 1] = (char_t)tablePos[1];
			}
		}
		return result;
	}

	string to_string() const { return to_string_t<wchar_t>(); }
	cstring to_cstring() const { return to_string_t<char>(); }

	iterator begin() { return get_first_iterator(); }
	const_iterator begin() const { return get_first_const_iterator(); }

	iterator rbegin() { return get_last_iterator(); }
	const_iterator rbegin() const { return get_last_const_iterator(); }

	iterator end() { iterator i; return i; }
	const_iterator end() const { const_iterator i; return i; }

	iterator rend() { iterator i; return i; }
	const_iterator rend() const { const_iterator i; return i; }
};


}


template <bool has_sign, size_t n_bits>
template <endian_t e>
inline io::buffer fixed_integer_native<has_sign, n_bits>::to_buffer() const
{
	static constexpr size_t width_bytes = bits_to_bytes_v<bits>;
	io::buffer result(width_bytes);
	uint8_t* resultPtr = (uint8_t*)result.get_ptr();
	unsigned_int_t src = m_int;
	for (size_t i = 0; i < width_bytes; i++)
	{
		if constexpr (e == endian_t::little)
			resultPtr[i] = (uint8_t)(src >> (i * 8));
		else
			resultPtr[i] = (uint8_t)(src >> (((width_bytes - 1) - i) * 8));
	}
	return result;
}

template <bool has_sign, size_t n_bits>
template <endian_t e>
inline io::buffer fixed_integer_native<has_sign, n_bits>::to_buffer() const volatile
{
	this_t tmp(*this);
	return to_buffer(tmp);
}


template <bool has_sign, size_t bits, bits_to_int_t<bits, has_sign> value>
template <endian_t e>
inline io::buffer fixed_integer_native_const<has_sign, bits, value>::to_buffer() const volatile
{
	non_const_t tmp(*this);
	return tmp.to_buffer();
}


}


#endif
