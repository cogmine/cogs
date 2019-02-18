//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_COLLECTION_VECTOR
#define COGS_HEADER_COLLECTION_VECTOR

#include <type_traits>

#include "cogs/operators.hpp"
#include "cogs/math/const_max_int.hpp"
#include "cogs/mem/default_allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/mem/rc_obj_base.hpp"
#include "cogs/mem/unowned.hpp"
#include "cogs/sync/hazard.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


// Forward declare
namespace io
{
	class buffer;
	class composite_buffer_content;
}

#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors specified
#pragma warning (disable: 4522)	// multiple assignment operators specified


enum split_options
{
	split_includes_empty_segments = 0,
	split_omits_empty_segments = 1
};


template <typename type, typename enable = void>
class vector_descriptor;

template <typename type>
class vector_descriptor<type, std::enable_if_t<!std::is_trivially_destructible_v<type> > > : public rc_obj_base
{
private:
	// non_pod array desc storage layout:
	//
	// true_base               m_start                                                   m_start+desc.m_length
	// |                       |                     m_ptr      m_ptr+content_t.m_length |
	// |                       |                     |          |                        |
	// +-----------------------+---------------------+----------+------------------------+
	// |                 full leading                |          |                        |
	// +-----------------------+---------------------+ contents + constructed trailing   | 
	// | unconstructed leading | constructed leading |          |                        | Possible reallocate?
	// +-----------------------+---------------------+----------+------------------------+-----------------------*

	// These vars do not need to be volatile.  They are read-only.
	// Only an owning thread will ever write to them.

	size_t	m_capacity;		// Full length of allocation.  We know the full buffer is at least this large.
	size_t	m_length;		// Length of constructed portion of the buffer
	type*	m_start;		// Starting position of constructed elements
	//    There may be some number of purged elements at the start of the buffer

	virtual void released()	{ placement_destruct_multiple(m_start, m_length); }

public:
	virtual void dispose()	{ default_allocator::destruct_deallocate_type(this); }

	vector_descriptor(size_t length, size_t capacity)
	{
		m_capacity = capacity;
		m_length = length;
		m_start = get_true_base();
	}

	size_t get_capacity() const					{ return m_capacity; }

	size_t get_capacity_before(type* p) const	{ return p - get_true_base(); }
	size_t get_capacity_after(type* p) const	{ return m_capacity - get_capacity_before(p); }

	void set_constructed_range(type* p, size_t length)	{ m_start = p; m_length = length; }

	type* get_true_base() const						{ return reinterpret_cast<type*>(default_allocator::get_type_block_from_header<vector_descriptor<type>, type>(this)); }
	type* get_ptr() const							{ return m_start; }
	size_t get_constructed_length() const			{ return m_length; }
	size_t get_constructed_length(type* p) const	{ return m_length - get_index_of(p); }
	void set_constructed_length(size_t newLength)	{ m_length = newLength; }

	void advance(size_t n = 1)						{ m_start += n; m_length -= n; }
	void trim_unconstructed_trailing(size_t n)		{ m_length -= n; }
	size_t get_gap() const							{ return m_start - get_true_base(); }

	size_t get_index_of(type* p) const				{ return p - m_start; }

	// Only call with n <= get_gap()
	void add_leading(size_t n)						{ m_start -= n; m_length += n; }

	void destruct_all()
	{
		placement_destruct_multiple(m_start, m_length);
		m_start = get_true_base();
		m_length = 0;
	}

	bool try_reallocate(size_t n)
	{
		if (!m_capacity)
			return false;
		if (m_capacity >= n)
			return true;
		if (!default_allocator::try_reallocate_type_with_header<vector_descriptor<type>, type>(this, n))
			return false;
		m_capacity = default_allocator::get_allocation_size_type_without_header<vector_descriptor<type>, type>(this, n);
		return true;
	}

	static ptr<vector_descriptor<type> > allocate(size_t n)
	{
		ptr<vector_descriptor<type> > desc = default_allocator::allocate_type_with_header<vector_descriptor<type>, type>(n);
		size_t capacity = default_allocator::get_allocation_size_type_without_header<vector_descriptor<type>, type>(desc, n);
		new (desc) vector_descriptor<type>(n, capacity);

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
		desc->set_type_name(typeid(vector_descriptor<type>).name());
		desc->set_debug_str("vector_descriptor");
		desc->set_obj_ptr(desc->get_ptr());
#endif

#if COGS_DEBUG_RC_LOGGING
		unsigned long rcCount = pre_assign_next(g_rcLogCount);
		printf("(%lu) RC_NEW(vector): %p (desc) %p (ptr) %s\n", rcCount, (rc_obj_base*)(desc.get_ptr()), desc->get_ptr(), typeid(vector_descriptor<type>).name());
#endif

		return desc;
	}
};

template <typename type>
class vector_descriptor<type, std::enable_if_t<std::is_trivially_destructible_v<type> > > : public rc_obj_base
{
private:
	// pod array desc storage layout:
	//
	// true_base                                       true_base+desc.m_capacity
	// |           m_ptr      m_ptr+content_t.m_length |
	// |           |          |                        |
	// +-----------+----------+------------------------+
	// |           pod contents                        | Possible reallocate?
	// +-----------------------------------------------+-----------------------*

	// These vars do not need to be volatile.  They are read-only.
	// Only an owning thread will ever write to them.

	size_t	m_capacity;

	virtual void released()	{ }

public:
	virtual void dispose()	{ default_allocator::destruct_deallocate_type(this); }
	
	explicit vector_descriptor(size_t capacity)
	{
		m_capacity = capacity;
	}

	size_t get_capacity() const					{ return m_capacity; }

	size_t get_capacity_before(type* p) const	{ return p - get_true_base(); }
	size_t get_capacity_after(type* p) const	{ return m_capacity - get_capacity_before(p); }

	size_t get_index_of(type* p) const			{ return p - get_true_base(); }

	void set_constructed_range(type* p, size_t length)	{ }

	type* get_true_base() const						{ return reinterpret_cast<type*>(default_allocator::get_type_block_from_header<vector_descriptor<type>, type>(this)); }
	type* get_ptr() const							{ return get_true_base(); }
	size_t get_constructed_length() const			{ return m_capacity; }
	size_t get_constructed_length(type* p) const	{ return get_capacity_after(p); }
	void set_constructed_length(size_t newLength)	{ }

	void advance(size_t n = 1)						{ }
	void trim_unconstructed_trailing(size_t n)		{ }
	size_t get_gap() const							{ return 0; }

	// Only call with n <= get_gap()
	void add_leading(size_t n)						{ }

	void destruct_all()								{ }

	bool try_reallocate(size_t n)
	{
		if (!m_capacity)
			return false;
		if (m_capacity >= n)
			return true;
		if (!default_allocator::try_reallocate_type_with_header<vector_descriptor<type>, type>(this, n))
			return false;
		m_capacity = default_allocator::get_allocation_size_type_without_header<vector_descriptor<type>, type>(this, n);
		return true;
	}

	static ptr<vector_descriptor<type> > allocate(size_t n)
	{
		ptr<vector_descriptor<type> > desc = default_allocator::allocate_type_with_header<vector_descriptor<type>, type>(n);
		size_t capacity = default_allocator::get_allocation_size_type_without_header<vector_descriptor<type>, type>(desc, n);
		new (desc) vector_descriptor<type>(capacity);

#if COGS_DEBUG_LEAKED_REF_DETECTION || COGS_DEBUG_RC_LOGGING
		desc->set_type_name(typeid(vector_descriptor<type>).name());
		desc->set_debug_str("vector_descriptor");
		desc->set_obj_ptr(desc->get_ptr());
#endif

#if COGS_DEBUG_RC_LOGGING
		unsigned long rcCount = pre_assign_next(g_rcLogCount);
		printf("(%lu) RC_NEW(vector): %p (desc) %p (ptr) %s\n", rcCount, (rc_obj_base*)(desc.get_ptr()), desc->get_ptr(), typeid(vector_descriptor<type>).name());
#endif

		return desc;
	}
};

// vector_content is an internal helper class containing the contents of a vector.
template <typename type>
class vector_content
{
public:
	typedef vector_content<type> this_t;
	typedef vector_descriptor<type> desc_t;

	vector_content()
		: m_desc(0), m_ptr(0), m_length(0)
	{ }

	type*	m_ptr;
	desc_t*	m_desc;
	size_t	m_length;

	const type* get_const_ptr() const { return m_ptr; }

	type& operator[](size_t i) { return m_ptr[i]; }
	const type& operator[](size_t i) const { return m_ptr[i]; }

	type* get_ptr() { copy_on_write(); return m_ptr; }

	size_t get_length() const { return m_length; }

	vector_content(this_t&& src)
	{
		m_ptr = src.m_ptr;
		m_desc = src.m_desc;
		m_length = src.m_length;
		src.clear_inner();
	}

	this_t& operator=(this_t&& src)
	{
		clear();
		m_ptr = src.m_ptr;
		m_desc = src.m_desc;
		m_length = src.m_length;
		src.clear_inner();
		return *this;
	}

	vector_content(const this_t& src)	// does not acquire
		: m_ptr(src.m_ptr),
		m_desc(src.m_desc),
		m_length(src.m_length)
	{ }

	vector_content(const this_t& src, size_t i)		// does not acquire
	{
		if (i >= src.m_length)
			clear_inner();
		else
		{
			size_t remainingSpace = src.m_length - i;
			m_desc = src.m_desc;
			m_length = remainingSpace;
			m_ptr = src.m_ptr + i;
		}
	}

	vector_content(const this_t& src, size_t i, size_t n)	// does not acquire
	{
		if (!n || (i >= src.m_length))
			clear_inner();
		else
		{
			size_t remainingSpace = src.m_length - i;
			if (n > remainingSpace)
				n = remainingSpace;
			m_desc = src.m_desc;
			m_length = n;
			m_ptr = src.m_ptr + i;
		}
	}

	template <typename type2 = type>
	vector_content(size_t n, const type2& src)
		: m_desc(0), m_ptr(0), m_length(0)
	{
		allocate(n, src);
	}

	template <typename type2 = type>
	vector_content(type2* src, size_t n)
		: m_desc(0), m_ptr(0), m_length(0)
	{
		allocate(src, n);
	}

	this_t& operator=(const this_t& src)	// does not acquire
	{
		m_ptr = src.m_ptr;
		m_desc = src.m_desc;
		m_length = src.m_length;
		return *this;
	}
	
	size_t get_capacity() const
	{
		if (!m_desc || (!m_desc->is_owned()))
			return 0;
		return m_desc->get_capacity_after(m_ptr);
	}

	size_t get_reverse_capacity() const
	{
		if (!m_desc || (!m_desc->is_owned()))
			return 0;
		return m_desc->get_capacity_before(m_ptr);
	}

	void set(desc_t* desc, type* p, size_t n)
	{
		m_desc = desc;
		m_ptr = p;
		m_length = n;
	}

	void set(const this_t& src)
	{
		m_desc = src.m_desc;
		m_ptr = src.m_ptr;
		m_length = src.m_length;
	}

	void swap(this_t& wth)
	{
		type* p = m_ptr;
		m_ptr = wth.m_ptr;
		wth.m_ptr = p;

		desc_t* d = m_desc;
		m_desc = wth.m_desc;
		wth.m_desc = d;

		size_t n = m_length;
		m_length = wth.m_length;
		wth.m_length = n;
	}

	void exchange(const this_t& src, this_t& rtn)
	{
		type* p = m_ptr;
		m_ptr = src.m_ptr;
		rtn.m_ptr = p;

		desc_t* d = m_desc;
		m_desc = src.m_desc;
		rtn.m_desc = d;

		size_t n = m_length;
		m_length = src.m_length;
		rtn.m_length = n;
	}

	void acquire()
	{
		if (!!m_desc)
			m_desc->acquire();
	}

	void acquire(const this_t& src)
	{
		desc_t* oldDesc = m_desc;
		m_desc = src.m_desc;
		m_ptr = src.m_ptr;
		m_length = src.m_length;
		if (m_desc != oldDesc)
		{
			acquire();
			if (!!oldDesc)
				oldDesc->release();
		}
	}

	void acquire(const this_t& src, size_t i)
	{
		desc_t* oldDesc = m_desc;
		for (;;)
		{
			if (i >= src.m_length)
				clear_inner();
			else
			{
				size_t remainingSpace = src.m_length - i;
				m_desc = src.m_desc;
				m_length = remainingSpace;
				m_ptr = src.m_ptr + i;
				if (m_desc == oldDesc)
					break;
				acquire();
			}
			if (!!oldDesc)
				oldDesc->release();
			break;
		}
	}

	void acquire(const this_t& src, size_t i, size_t n)
	{
		desc_t* oldDesc = m_desc;
		for (;;)
		{
			if (!n || (i >= src.m_length))
				clear_inner();
			else
			{
				size_t remainingSpace = src.m_length - i;
				if (n > remainingSpace)
					n = remainingSpace;
				m_desc = src.m_desc;
				m_length = n;
				m_ptr = src.m_ptr + i;
				if (m_desc == oldDesc)
					break;
				acquire();
			}
			if (!!oldDesc)
				oldDesc->release();
			break;
		}
	}

	void release() const
	{
		if (!!m_desc)
			m_desc->release();
	}

	void clear_inner()
	{
		m_ptr = 0;
		m_length = 0;
		m_desc = 0;
	}

	void clear()
	{
		if (!m_desc)
			m_ptr = 0;
		else if (m_desc->is_owned())	// If we own the buffer, try to reuse it to preserve reserved space
		{
			m_desc->destruct_all();
			m_ptr = m_desc->get_true_base();
		}
		else
		{
			m_desc->release();
			m_desc = 0;
		}
		m_length = 0;
	}

	// Low-level allocate.
	// Does not account for or release existing contents. 
	// Does not construct.
	void allocate_inner(size_t n)
	{
		desc_t* newDesc = desc_t::allocate(n).get_ptr();
		m_desc = newDesc;
		m_ptr = newDesc->get_ptr();
	}

	void allocate(size_t n)
	{
		allocate_inner(n);
		m_length = n;
		placement_construct_multiple(m_ptr, n);
	}

	template <typename type2 = type>
	void allocate(size_t n, const type2& src)
	{
		allocate_inner(n);
		m_length = n;
		placement_construct_multiple(m_ptr, n, src);
	}

	template <typename type2 = type>
	void allocate(const type2* src, size_t n)
	{
		allocate_inner(n);
		m_length = n;
		placement_copy_construct_array(m_ptr, src, n);
	}

	void reserve(size_t n)
	{
		if (!!n)
		{
			desc_t* oldDesc = m_desc;
			if (!!oldDesc && oldDesc->is_owned() && oldDesc->try_reallocate(n))
			{
				trim_lingering();
				type* trueBase = oldDesc->get_true_base();
				size_t gap = m_ptr - trueBase;
				size_t stationaryCapacity = oldDesc->get_capacity() - gap;
				if (n > stationaryCapacity)
				{
					placement_move(trueBase, m_ptr, m_length);
					m_ptr = trueBase;
					oldDesc->set_constructed_range(m_ptr, m_length);
				}
			}
			else
			{
				type* oldPtr = m_ptr;
				size_t oldLength = m_length;
				allocate_inner(n);
				placement_copy_construct_array(m_ptr, oldPtr, oldLength);
				if (!!oldDesc)
					oldDesc->release();
			}
		}
	}

	// Only call if is_owned()
	void trim_leading_lingering()
	{
		if (!std::is_trivially_destructible_v<type>)
		{
			// If there are any lingering constructed leading elements, dispose of them.
			size_t constructedLeading = m_desc->get_index_of(m_ptr);
			if (!!constructedLeading)
			{
				placement_destruct_multiple(m_desc->get_ptr(), constructedLeading);
				m_desc->advance(constructedLeading);
			}
		}
	}

	// Only call if is_owned()
	void trim_trailing_lingering()
	{
		if (!std::is_trivially_destructible_v<type>)
		{
			// At this point we know that m_ptr == m_start
			// If there are any lingering constructed trailing elements, dispose of them.
			size_t constructedLength = m_desc->get_constructed_length();
			if (constructedLength != m_length)
			{
				size_t constructedTrailing = constructedLength - m_length;
				placement_destruct_multiple(m_desc->get_ptr() + m_length, constructedTrailing);
				m_desc->set_constructed_length(m_length);
			}
		}
	}

	// Trims lingering leading and trailing constructed elements, if !is_pod
	// Only call if is_owned()
	// On exit, we know that: ((m_ptr == m_desc->m_start) && (m_length == m_desc->m_length))
	void trim_lingering()
	{
		if (!std::is_trivially_destructible_v<type>)
		{
			trim_leading_lingering();
			trim_trailing_lingering();
		}
	}

	void advance(size_t n = 1)
	{
		if (!!n)
		{
			if (n >= m_length)
				clear();
			else // if (n < m_length)
			{
				m_ptr += n;
				m_length -= n;
				if ((!std::is_trivially_destructible_v<type>) && !!m_desc && m_desc->is_owned())		// If non-pod data, and some are constructed before the current start, destruct them.
					trim_leading_lingering();
			}
		}
	}

	void truncate_to(size_t n)
	{
		if (!n)
			clear();
		else if (n < m_length)
		{
			m_length = n;
			if ((!std::is_trivially_destructible_v<type>) && !!m_desc && m_desc->is_owned())
				trim_trailing_lingering();
		}
	}

	void truncate(size_t n)
	{
		if (m_length <= n)
			clear();
		else
			truncate_to(m_length - n);
	}

	void truncate_to_right(size_t n)
	{
		if (!n)
			clear();
		else if (n < m_length)
			advance(m_length - n);
	}

	void prepare_append(size_t n)
	{
		size_t oldLength = m_length;
		size_t newLength = n + oldLength;
		desc_t* oldDesc = m_desc;
		if (!!oldDesc && oldDesc->is_owned() && oldDesc->try_reallocate(newLength))
		{
			trim_lingering();
			type* trueBase = oldDesc->get_true_base();
			size_t gap = m_ptr - trueBase;
			size_t stationaryCapacity = oldDesc->get_capacity() - gap;
			if (n > stationaryCapacity)
			{
				placement_move(trueBase, m_ptr, oldLength);
				m_ptr = trueBase;
			}
			oldDesc->set_constructed_range(m_ptr, newLength);
		}
		else
		{
			type* oldPtr = m_ptr;
			allocate_inner(newLength);			// a reallocation was necessary.
			placement_copy_construct_array(m_ptr, oldPtr, oldLength);
			if (!!oldDesc)
				oldDesc->release();
		}
		m_length = newLength;
	}

	void append(size_t n)
	{
		size_t oldLength = m_length;
		prepare_append(n);
		placement_construct_multiple(m_ptr + oldLength, n);
	}

	void append(size_t n, const type& src)
	{
		if (n == 1)
			append(&src, 1);	// in case src is a ptr to something already in the vector, avoid duplicating the buffer.
		else
		{
			size_t oldLength = m_length;
			prepare_append(n);
			placement_construct_multiple(m_ptr + oldLength, n, src);
		}
	}

	template <typename type2>
	void append(size_t n, const type2& src)
	{
		size_t oldLength = m_length;
		prepare_append(n);
		placement_construct_multiple(m_ptr + oldLength, n, src);
	}

	void append(const type* src, size_t n)
	{
		size_t oldLength = m_length;
		if (src == m_ptr + oldLength)
			m_length += n;	// in case src is already in the vector, avoid duplicating the buffer.
		else
		{
			prepare_append(n);
			placement_copy_construct_array(m_ptr + oldLength, src, n);
		}
	}

	template <typename type2>
	void append(const type2* src, size_t n)
	{
		size_t oldLength = m_length;
		prepare_append(n);
		placement_copy_construct_array(m_ptr + oldLength, src, n);
	}

	void prepare_prepend(size_t n)	// >0
	{
		size_t oldLength = m_length;
		size_t newLength = n + oldLength;
		desc_t* oldDesc = m_desc;
		if (!!oldDesc && oldDesc->is_owned() && oldDesc->try_reallocate(newLength))
		{
			trim_lingering();
			type* trueBase = oldDesc->get_true_base();
			size_t gap = m_ptr - trueBase;
			if (n <= gap)
				m_ptr -= n;
			else		// Implies the contents will be moving forward
			{
				placement_move(trueBase + n, m_ptr, oldLength);
				m_ptr = trueBase;
			}
			oldDesc->set_constructed_range(m_ptr, newLength);
		}
		else
		{
			type* oldPtr = m_ptr;
			allocate_inner(newLength);			// a reallocation was necessary.
			placement_copy_construct_array(m_ptr + n, oldPtr, oldLength);
			if (!!oldDesc)
				oldDesc->release();
		}
		m_length = newLength;
	}

	void prepend(size_t n)
	{
		prepare_prepend(n);
		placement_construct_multiple(m_ptr, n);
	}

	void prepend(size_t n, const type& src)
	{
		if (n == 1)
			prepend(&src, 1);	// in case src is a ptr to something already in the vector, avoid duplicating the buffer.
		else
		{
			prepare_prepend(n);
			placement_construct_multiple(m_ptr, n, src);
		}
	}

	template <typename type2>
	void prepend(size_t n, const type2& src)
	{
		prepare_prepend(n);
		placement_construct_multiple(m_ptr, n, src);
	}

	void prepend(const type* src, size_t n)
	{
		if (m_ptr == src + n)	// in case src is already in the vector, avoid duplicating the buffer.
		{
			m_ptr -= n;
			m_length += n;
		}
		else
		{
			prepare_prepend(n);
			placement_copy_construct_array(m_ptr, src, n);
		}
	}

	template <typename type2>
	void prepend(const type2* src, size_t n)
	{
		prepare_prepend(n);
		placement_copy_construct_array(m_ptr, src, n);
	}

	template <typename type2 = type>
	void assign(size_t n, const type2& src)
	{
		clear();
		if (!!n)
		{
			reserve(n);
			placement_construct_multiple(m_ptr, n, src);
			m_desc->set_constructed_range(m_ptr, n);
		}
		m_length = n;
	}

	template <typename type2 = type>
	void assign(const type2* src, size_t n)
	{
		clear();
		if (!!n)
		{
			reserve(n);
			placement_copy_construct_array(m_ptr, src, n);
			m_desc->set_constructed_range(m_ptr, n);
		}
		m_length = n;
	}

	void copy_on_write()
	{
		if (!!m_length && (!m_desc || !m_desc->is_owned()))
		{
			desc_t* oldDesc = m_desc;
			type* oldPtr = m_ptr;
			allocate_inner(m_length);
			placement_copy_construct_array(m_ptr, oldPtr, m_length);
			if (oldDesc)
				oldDesc->release();
		}
	}

	// i <= m_length
	// n > 0
	void prepare_insert_replace(size_t i, size_t n, size_t replaceLength = 0)
	{
		size_t forwardLength = m_length - i;
		if (replaceLength > forwardLength)
			replaceLength = forwardLength;
		size_t secondSegmentLength = forwardLength - replaceLength;
		size_t newLength = (m_length - replaceLength) + n;
		desc_t* oldDesc = m_desc;
		if (!!oldDesc && oldDesc->is_owned() && oldDesc->try_reallocate(newLength))
		{
			trim_lingering();
			type* pastFirstSegment = m_ptr + i;
			placement_destruct_multiple(pastFirstSegment, replaceLength);
			if (n < replaceLength)
				placement_move(pastFirstSegment + n, pastFirstSegment + replaceLength, secondSegmentLength);
			else if (n > replaceLength)
			{
				type* trueBase = oldDesc->get_true_base();
				size_t startGap = m_ptr - trueBase;

				size_t growingBy = n - replaceLength;
				bool leadingLargeEnough = growingBy <= startGap;

				size_t stationaryCapacity = oldDesc->get_capacity() - startGap;
				bool trailingLargeEnough = newLength <= stationaryCapacity;

				type* src = m_ptr;
				bool shiftBack;
				bool shiftForward;
				if (leadingLargeEnough)
				{
					if (!!trailingLargeEnough)
					{
						shiftBack = i <= secondSegmentLength;	// Shrink in the direction requiring the least transplanting
						shiftForward = !shiftBack;
						if (shiftBack)
							m_ptr -= growingBy;
					}
					else
					{
						shiftBack = true;
						shiftForward = false;
						m_ptr -= growingBy;
					}
				}
				else if (trailingLargeEnough)
				{
					shiftBack = false;
					shiftForward = true;
				}
				else
				{
					shiftBack = true;
					shiftForward = true;
					m_ptr = trueBase;
				}

				if (shiftForward)				// Just need to scoot secondSegment forward a bit.
					placement_move(pastFirstSegment + n, pastFirstSegment + replaceLength, secondSegmentLength);
				if (shiftBack)
					placement_move(m_ptr, src, i);
			}
			oldDesc->set_constructed_range(m_ptr, newLength);
		}
		else
		{
			type* oldPtr = m_ptr;
			allocate_inner(newLength);			// a reallocation was necessary.
			placement_copy_construct_array(m_ptr, oldPtr, i);
			placement_copy_construct_array(m_ptr + n, oldPtr + replaceLength, secondSegmentLength);
			if (!!oldDesc)
				oldDesc->release();
		}
		m_length = newLength;
	}

	void insert(size_t i, size_t insertLength)	// insertLength > 0
	{
		if (i > m_length)
			i = m_length;
		prepare_insert_replace(i, insertLength);
		placement_construct_multiple(m_ptr + i, insertLength);
	}

	template <typename type2 = type>
	void insert(size_t i, size_t insertLength, const type2& src)	// insertLength > 0
	{
		if (i > m_length)
			i = m_length;
		prepare_insert_replace(i, insertLength);
		placement_construct_multiple(m_ptr + i, insertLength, src);
	}

	template <typename type2 = type>
	void insert(size_t i, const type2* src, size_t insertLength)	// insertLength > 0
	{
		if (i > m_length)
			i = m_length;
		prepare_insert_replace(i, insertLength);
		placement_copy_construct_array(m_ptr + i, src, insertLength);
	}

	template <typename type2 = type>
	void replace(size_t i, size_t replaceLength, const type2& src)
	{
		if (!!replaceLength && (i < m_length))
		{
			size_t remainingLength = m_length - i;
			if (replaceLength > remainingLength)
				replaceLength = remainingLength;
			prepare_insert_replace(i, replaceLength, replaceLength);
			placement_construct_multiple(m_ptr + i, replaceLength, src);
		}
	}

	template <typename type2 = type>
	void replace(size_t i, const type2* src, size_t replaceLength)
	{
		if (!!replaceLength && (i < m_length))
		{
			size_t remainingLength = m_length - i;
			if (replaceLength > remainingLength)
				replaceLength = remainingLength;
			prepare_insert_replace(i, replaceLength, replaceLength);
			placement_copy_construct_array(m_ptr + i, src, replaceLength);
		}
	}

	template <typename type2 = type>
	void insert_replace(size_t i, size_t replaceLength, size_t insertLength, const type2& src)	// insertLength > 0
	{
		if (i > m_length)
			i = m_length;
		prepare_insert_replace(i, insertLength, replaceLength);
		placement_construct_multiple(m_ptr + i, insertLength, src);
	}

	template <typename type2 = type>
	void insert_replace(size_t i, size_t replaceLength, const type2* src, size_t insertLength)	// insertLength > 0
	{
		if (i > m_length)
			i = m_length;
		prepare_insert_replace(i, insertLength, replaceLength);
		placement_copy_construct_array(m_ptr + i, src, insertLength);
	}

	void erase(size_t i)
	{
		if (i < m_length)
			prepare_insert_replace(i, 0, 1);
	}

	void erase(size_t i, size_t n)
	{
		if (!!n && i < m_length)
			prepare_insert_replace(i, 0, n);
	}

	void resize(size_t n)
	{
		if (n > m_length)
			append(n - m_length);
		else
			truncate_to(n);
	}

	template <typename type2 = type>
	void resize(size_t n, const type2& src)
	{
		if (n > m_length)
			append(n, &src);
		else
			truncate_to(n);
	}

	template <typename type2, class comparator_t>
	bool equals(size_t n, const type2& cmp) const
	{
		if (n != m_length)
			return false;

		for (size_t i = 0; i < n; i++)
		{
			if (!comparator_t::equals(m_ptr[i], cmp))
				return false;
		}
		return true;
	}

	template <typename type2, class comparator_t>
	bool equals(const type2* cmp, size_t n) const
	{
		if (std::is_same_v<type, type2> && std::is_same_v<comparator_t, default_comparator> && (std::is_same_v<type, char> || std::is_same_v<type, unsigned char> || std::is_same_v<type, signed char>))
			return (n == m_length) && (memcmp(m_ptr, cmp, n) == 0);

		if (n != m_length)
			return false;

		if (!std::is_same_v<type, type2> || ((void*)cmp != (void*)m_ptr))
		{
			for (size_t i = 0; i < n; i++)
			{
				if (!comparator_t::equals(m_ptr[i], cmp[i]))
					return false;
			}
		}

		return true;
	}

	template <typename type2, class comparator_t>
	bool starts_with(size_t n, const type2& cmp) const
	{
		if (!n || n > m_length)
			return false;

		for (size_t i = 0; i < n; i++)
		{
			if (!comparator_t::equals(m_ptr[i], cmp))
				return false;
		}
		return true;
	}

	template <typename type2, class comparator_t>
	bool starts_with(const type2* cmp, size_t n) const
	{
		if (std::is_same_v<type, type2> && std::is_same_v<comparator_t, default_comparator> && (std::is_same_v<type, char> || std::is_same_v<type, unsigned char> || std::is_same_v<type, signed char>))
			return (n <= m_length) && (memcmp(m_ptr, cmp, n) == 0);

		if (!n || n > m_length)
			return false;

		if (!std::is_same_v<type, type2> || ((void*)cmp != (void*)m_ptr))
		{
			for (size_t i = 0; i < n; i++)
			{
				if (!comparator_t::equals(m_ptr[i], cmp[i]))
					return false;
			}
		}

		return true;
	}


	template <typename type2, class comparator_t>
	bool ends_with(size_t n, const type2& cmp) const
	{
		size_t length = m_length;
		if (!n || n > length)
			return false;

		for (size_t i = length - n; i < length; i++)
		{
			if (!comparator_t::equals(m_ptr[i], cmp))
				return false;
		}
		return true;
	}

	template <typename type2, class comparator_t>
	bool ends_with(const type2* cmp, size_t n) const
	{
		if (std::is_same_v<type, type2> && std::is_same_v<comparator_t, default_comparator> && (std::is_same_v<type, char> || std::is_same_v<type, unsigned char> || std::is_same_v<type, signed char>))
			return(n <= m_length) && (memcmp(m_ptr + (m_length - n), cmp, n * sizeof(type)) == 0);

		size_t length = m_length;
		if (!n || n > length)
			return false;

		if (!std::is_same_v<type, type2> || ((void*)(m_ptr + length) != (void*)(cmp + n)))
		{
			for (size_t i = length - n; i < length; i++)
			{
				if (!comparator_t::equals(m_ptr[i], cmp[i]))
					return false;
			}
		}

		return true;
	}

	template <typename type2, class comparator_t>
	int compare(size_t n, const type2& cmp) const
	{
		if (!m_length)
			return !!n ? -1 : 0;
		bool cmpIsLonger = (n > m_length);
		size_t shorterLength = cmpIsLonger ? m_length : n;
		size_t i = 0;
		do {
			const type& value = m_ptr[i];
			if (comparator_t::is_less_than(value, cmp))
				return -1;
			if (comparator_t::is_greater_than(value, cmp))
				return 1;
		} while (++i != shorterLength);
		return !!cmpIsLonger ? -1 : ((n == m_length) ? 0 : 1);
	}

	template <typename type2, class comparator_t>
	int compare(const type2* cmp, size_t n) const
	{
		if (!m_length)
			return !!n ? -1 : 0;
		if (!n)
			return 1;
		bool cmpIsLonger = (n > m_length);
		if (!std::is_same_v<type, type2> || ((void*)cmp != (void*)m_ptr))
		{
			size_t shorterLength = cmpIsLonger ? m_length : n;
			if (std::is_same_v<type, type2> && std::is_same_v<comparator_t, default_comparator> && (std::is_same_v<type, char> || std::is_same_v<type, unsigned char> || std::is_same_v<type, signed char>))
			{
				int i = memcmp(m_ptr, cmp, shorterLength);
				if (i != 0)
					return (i < 0) ? -1 : 1;
			}
			else
			{
				size_t i = 0;
				do {
					const type& value = m_ptr[i];
					const type2& cmpValue = cmp[i];
					if (comparator_t::is_less_than(value, cmpValue))
						return -1;
					if (comparator_t::is_greater_than(value, cmpValue))
						return 1;
				} while (++i != shorterLength);
			}
		}
		return !!cmpIsLonger ? -1 : ((n == m_length) ? 0 : 1);
	}


	template <typename type2, class comparator_t>
	bool is_less_than(const type2* cmp, size_t n) const
	{
		bool result = false;
		if (!m_length)
			result = !!n;
		else if (!!n)
		{
			bool cmpIsLonger = (n > m_length);
			if (std::is_same_v<type, type2> && ((void*)cmp == (void*)m_ptr))
				result = cmpIsLonger;
			else
			{
				size_t shorterLength = cmpIsLonger ? m_length : n;
				if (std::is_same_v<type, type2> && std::is_same_v<comparator_t, default_comparator> && (std::is_same_v<type, char> || std::is_same_v<type, unsigned char> || std::is_same_v<type, signed char>))
				{
					int i = memcmp(m_ptr, cmp, shorterLength);
					result = (i == 0) ? cmpIsLonger : (i < 0);
				}
				else
				{
					size_t i = 0;
					for (;;)
					{
						const type& value = m_ptr[i];
						const type2& cmpValue = cmp[i];
						if (comparator_t::is_less_than(value, cmpValue))
						{
							result = true;
							break;
						}
						if (comparator_t::is_greater_than(value, cmpValue))
							break;
						if (++i == shorterLength)
						{
							result = cmpIsLonger;
							break;
						}
					}
				}
			}
		}
		return result;
	}


	template <typename type2, class comparator_t>
	bool is_greater_than(const type2* cmp, size_t n) const
	{
		bool result = true;
		if (!m_length)
			result = !n;
		else if (!!n)
		{
			bool cmpIsShorter = (n < m_length);
			if (std::is_same_v<type, type2> && ((void*)cmp == (void*)m_ptr))
				result = cmpIsShorter;
			else
			{
				size_t shorterLength = cmpIsShorter ? n : m_length;
				if (std::is_same_v<type, type2> && std::is_same_v<comparator_t, default_comparator> && (std::is_same_v<type, char> || std::is_same_v<type, unsigned char> || std::is_same_v<type, signed char>))
				{
					int i = memcmp(m_ptr, cmp, shorterLength);
					result = (i == 0) ? cmpIsShorter : (i > 0);
				}
				else
				{
					size_t i = 0;
					for (;;)
					{
						const type& value = m_ptr[i];
						const type2& cmpValue = cmp[i];
						if (comparator_t::is_less_than(value, cmpValue))
						{
							result = false;
							break;
						}
						if (comparator_t::is_greater_than(value, cmpValue))
							break;
						if (++i == shorterLength)
						{
							result = cmpIsShorter;
							break;
						}
					}
				}
			}
		}
		return result;
	}


	template <typename type2, class comparator_t>
	size_t index_of(size_t i, const type2& cmp) const
	{
		size_t length = m_length;
		for (size_t i2 = i; i2 < length; i2++)
			if (comparator_t::equals(m_ptr[i2], cmp))
				return i2;
		return const_max_int_v<size_t>;
	}

	template <typename type2, class comparator_t>
	size_t index_of_any(size_t i, const type2* cmp, size_t cmpLength) const
	{
		if (cmpLength == 1)
			return index_of<type2, comparator_t>(i, *cmp);
		if (!!cmpLength)
		{
			size_t length = m_length;
			for (size_t i2 = i; i2 < length; i2++)
			{
				type& t = m_ptr[i2];
				for (size_t i3 = 0; i3 < cmpLength; i3++)
				{
					if (comparator_t::equals(t, cmp[i3]))
						return i2;
				}
			}
		}
		return const_max_int_v<size_t>;
	}

	template <typename type2, class comparator_t>
	size_t index_of_segment(size_t i, const type2* cmp, size_t cmpLength) const
	{
		if (!!cmpLength && (i < m_length))
		{
			size_t remainingLength = m_length - i;
			if (cmpLength <= remainingLength)
			{
				size_t endPos = m_length - cmpLength;
				while (i < endPos)
				{
					if (m_ptr[i] == cmp[0])
					{
						bool isMatch = true;
						for (size_t i2 = 1; i2 < cmpLength; i2++)
						{
							if (!comparator_t::equals(m_ptr[i + i2], cmp[i2]))
							{
								isMatch = false;
								break;
							}
						}
						if (isMatch)
							return i;
					}
					++i;
				}
			}
		}
		return const_max_int_v<size_t>;
	}

	void reverse()
	{
		if (m_length > 1)
		{
			type* p = get_ptr();
			size_t lengthMinusOne = m_length - 1;
			for (size_t i = 0; i < m_length / 2; i++)
			{
				type tmp = p[i];
				size_t alternatePosition = lengthMinusOne - i;
				p[i] = p[alternatePosition];
				p[alternatePosition] = tmp;
			}
		}
	}

	void set_to_subrange(size_t i, size_t n)
	{
		if (i >= m_length)
			clear();
		else
		{
			size_t remainingLength = m_length - i;
			if (n > remainingLength)
				n = remainingLength;
			m_ptr += i;
			m_length = n;
			if (!!m_desc && !m_desc->is_owned())
				trim_lingering();
		}
	}


	//	template <typename type2>
	//	bool is_same_set(const type2* cmp, size_t n) const
	//
	//	template <typename type2>
	//	bool is_subset(const type2* cmp, size_t n) const
	//
	//	template <typename type2>
	//	bool is_same_or_subset(const type2* cmp, size_t n) const
	//
	//	template <typename type2>
	//	bool is_superset(const type2* cmp, size_t n) const
	//		
	//	template <typename type2>
	//	bool is_same_or_superset(const type2* cmp, size_t n) const
};



/// @ingroup LockFreeCollections
/// @brief A resizable dynamically allocated array, which uses shared-read, copy-on-write buffers.  Copies are shallow.
///
/// A vector<> is a resizable dynamically allocated array.
///
/// vector<> differs from simple::vector<> in that multiple instances may reference the same shared
/// read-only buffer, which is copied on write.  This may be more efficient as it removes
/// the overhead of frequent copying and may use less memory.  Multiple instances may refer
/// to different subranges in the same buffer.
///
/// A const vector extends const-ness to its elements.
///
/// const and/or volatile element types are not supported.
///
/// One non-const operation that does not trigger a copy on write, is truncation (from either side).
/// This is equivalent to acquiring a subset.  If the buffer is owned (not shared), removed elements
/// will be destructed.  If the buffer is shared, removed elements will not be destructed because they
/// may be referenced by another vector<>.  It's possible for unreachable elements to linger until
/// its buffer is no longer referenced.
/// @tparam T type to contain
template <typename T>
class vector
{
private:
	static_assert(!std::is_const_v<T>);
	static_assert(!std::is_volatile_v<T>);
	static_assert(!std::is_void_v<T>);

public:
	typedef T type;
	typedef vector<type>	this_t;

protected:
	template <typename>
	friend class vector;

	friend class io::composite_buffer_content;
	friend class io::buffer;

	typedef vector_descriptor<type>	desc_t;
	typedef vector_content<type>	content_t;
	typedef transactable<content_t> transactable_t;
	transactable_t	m_contents;

	typedef typename transactable_t::read_token		read_token;
	typedef typename transactable_t::write_token	write_token;

	// null desc, used for literals or caller owned buffers
	vector(size_t n, type* p) : m_contents(typename transactable_t::construct_embedded_t(), p, n) { }
	vector(size_t n, const type* p) : m_contents(typename transactable_t::construct_embedded_t(), const_cast<type*>(p), n) { }

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
			p.bind(h, desc);
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
			p.bind(h, desc);
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

	void set(desc_t* d, type* p, size_t n) { m_contents->set(d, p, n); }

	void disown() { m_contents->m_desc = 0; }

	desc_t* get_desc() const { return m_contents->m_desc; }
	type* get_raw_ptr() const { return m_contents->m_ptr; }

public:
	/// @brief A vector element iterator
	class iterator
	{
	protected:
		this_t*	m_vector;
		size_t	m_index;

		iterator(this_t* v, size_t i)
			: m_vector(v),
			m_index(i)
		{ }

		template <typename>
		friend class vector;

	public:
		iterator() { }
		iterator(const iterator& i) : m_vector(i.m_vector), m_index(i.m_index) { }

		void release() { m_vector = 0; }

		iterator& operator++()
		{
			if (m_vector)
			{
				m_index++;
				if (m_index >= m_vector->get_length())
					m_vector = 0;
			}

			return *this;
		}

		iterator& operator--()
		{
			if (m_vector)
			{
				if (m_index == 0)
					m_vector = 0;
				else
					--m_index;
			}

			return *this;
		}

		iterator operator++(int) { iterator i(*this); ++*this; return i; }
		iterator operator--(int) { iterator i(*this); --*this; return i; }

		bool operator!() const { return !m_vector || (m_index >= m_vector->get_length()); }

		bool operator==(const iterator& i) const { return (m_vector == i.m_vector) && (!m_vector || (m_index == i.m_index)); }
		bool operator!=(const iterator& i) const { return !operator==(i); }
		iterator& operator=(const iterator& i) { m_vector = i.m_vector; m_index = i.m_index; return *this; }

		type* get() const { return m_vector->get_ptr() + m_index; }
		type& operator*() const { return *get(); }
		type* operator->() const { return get(); }

		size_t get_position() const { return m_index; }

		iterator next() const { iterator result(*this); ++result; return result; }
		iterator prev() const { iterator result(*this); --result; return result; }
	};

	iterator get_first_iterator() { size_t sz = get_length(); return iterator(!!sz ? this : 0, 0); }
	iterator get_last_iterator() { size_t sz = get_length(); return iterator(!!sz ? this : 0, sz - 1); }

	/// @brief A vector const element iterator
	class const_iterator
	{
	protected:
		const this_t*	m_vector;
		size_t			m_index;

		const_iterator(const this_t* v, size_t i)
			: m_vector(v),
			m_index(i)
		{ }

		template <typename>
		friend class vector;

	public:
		const_iterator() { }
		const_iterator(const const_iterator& i) : m_vector(i.m_vector), m_index(i.m_index) { }
		const_iterator(const iterator& i) : m_vector(i.m_vector), m_index(i.m_index) { }

		void release() { m_vector = 0; }

		const_iterator& operator++()
		{
			if (m_vector)
			{
				m_index++;
				if (m_index >= m_vector->get_length())
					m_vector = 0;
			}

			return *this;
		}

		const_iterator& operator--()
		{
			if (m_vector)
			{
				if (m_index == 0)
					m_vector = 0;
				else
					--m_index;
			}

			return *this;
		}

		const_iterator operator++(int) { const_iterator i(*this); ++*this; return i; }
		const_iterator operator--(int) { const_iterator i(*this); --*this; return i; }

		bool operator!() const { return !m_vector || (m_index >= m_vector->get_length()); }

		bool operator==(const const_iterator& i) const { return (m_vector == i.m_vector) && (!m_vector || (m_index == i.m_index)); }
		bool operator==(const iterator& i) const { return (m_vector == i.m_vector) && (!m_vector || (m_index == i.m_index)); }
		bool operator!=(const const_iterator& i) const { return !operator==(i); }
		bool operator!=(const iterator& i) const { return !operator==(i); }
		const_iterator& operator=(const const_iterator& i) { m_vector = i.m_vector; m_index = i.m_index; return *this; }
		const_iterator& operator=(const iterator& i) { m_vector = i.m_vector; m_index = i.m_index; return *this; }

		const type* get() const { return (m_vector->get_const_ptr() + m_index); }
		const type& operator*() const { return *get(); }
		const type* operator->() const { return get(); }

		size_t get_position() const { return m_index; }

		const_iterator next() const { const_iterator result(*this); ++result; return result; }
		const_iterator prev() const { const_iterator result(*this); --result; return result; }
	};

	const_iterator get_first_const_iterator() const { size_t sz = get_length(); return const_iterator(!!sz ? this : 0, 0); }
	const_iterator get_last_const_iterator() const { size_t sz = get_length(); return const_iterator(!!sz ? this : 0, sz - 1); }

	// contains specified buffer (allocates if ptr is NULL)
	static this_t contain(const type* ptr, size_t sz) { return this_t(sz, ptr); }

	vector(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	this_t& operator=(this_t&& src)
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}


	vector() { }

	explicit vector(size_t n) { if (n) m_contents->allocate(n); }

	template <typename type2 = type>
	vector(size_t n, const type2& src) { if (n) m_contents->allocate(n, src); }

	template <typename type2 = type>
	vector(const type2* src, size_t n) { if (n) m_contents->allocate(src, n); }

	vector(const this_t& src) : m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents)) { m_contents->acquire(); }
	vector(const this_t& src, size_t i) : m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents), i) { m_contents->acquire(); }
	vector(const this_t& src, size_t i, size_t n) : m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents), i, n) { m_contents->acquire(); }

	vector(const volatile this_t& src) : m_contents(typename transactable_t::construct_embedded_t(), *(src.guarded_begin_read())) { }	// takes ownership of guarded desc reference
	vector(const volatile this_t& src, size_t i) : m_contents(typename transactable_t::construct_embedded_t(), *(src.guarded_begin_read())) { set_to_subrange(i); }	// takes ownership of guarded desc reference
	vector(const volatile this_t& src, size_t i, size_t n) : m_contents(typename transactable_t::construct_embedded_t(), *(src.guarded_begin_read())) { set_to_subrange(i, n); }	// takes ownership of guarded desc reference

	template <typename type2 = type>
	vector(const vector<type2>& src) { m_contents->allocate(src.get_const_ptr(), src.get_length()); }

	template <typename type2 = type>
	vector(const vector<type2>& src, size_t i)
	{
		size_t length = src.get_length();
		if (i < length)
			m_contents->allocate(src.get_const_ptr() + i, length - i);
	}

	template <typename type2 = type>
	vector(const vector<type2>& src, size_t i, size_t n)
	{
		if (!!n)
		{
			size_t length = src.get_length();
			if (i < length)
			{
				size_t remainingLength = length - i;
				if (n > remainingLength)
					n = remainingLength;
				m_contents->allocate(src.get_const_ptr() + i, n);
			}
		}
	}

	template <typename type2 = type>
	vector(const volatile vector<type2>& src)
	{
		typename vector<type2>::read_token rt;
		src.m_contents.guarded_begin_read(rt);
		m_contents->allocate(rt->m_ptr, rt->m_length);
		rt->release();
	}

	template <typename type2 = type>
	vector(const volatile vector<type2>& src, size_t i)
	{
		typename vector<type2>::read_token rt;
		src.m_contents.guarded_begin_read(rt);
		size_t length = rt->m_length;
		if (i < length)
			m_contents->allocate(rt->m_ptr + i, length - i);
		rt->release();
	}

	template <typename type2 = type>
	vector(const volatile vector<type2>& src, size_t i, size_t n)
	{
		if (!!n)
		{
			typename vector<type2>::read_token rt;
			src.m_contents.guarded_begin_read(rt);
			size_t length = rt->m_length;
			if (i < length)
			{
				size_t remainingLength = length - i;
				if (n > remainingLength)
					n = remainingLength;
				m_contents->allocate(rt->m_ptr + i, n);
			}
			rt->release();
		}
	}

	~vector() { m_contents->release(); }

	bool is_unowned() const { return !!m_contents->m_length && !m_contents->m_desc; }
	bool is_unowned() const volatile { read_token rt; m_contents.begin_read(rt); return !!rt->m_length && !rt->m_desc; }

	bool is_owned() const { return !!m_contents->m_length && !!m_contents->m_desc && m_contents->m_desc->is_owned(); }
	bool is_owned() const volatile
	{
		read_token rt;
		m_contents.guarded_begin_read(rt);
		bool result = !!rt->m_length && !!rt->m_desc && rt->m_desc->is_owned();
		rt->release();
		return result;
	}

	bool is_shared() const { return !!m_contents->m_length && !!m_contents->m_desc && !m_contents->m_desc->is_owned(); }
	bool is_shared() const volatile
	{
		read_token rt;
		m_contents.guarded_begin_read(rt);
		bool result = !!rt->m_length && !!rt->m_desc && !rt->m_desc->is_owned();
		rt->release();
		return result;
	}

	size_t get_length() const { return m_contents->m_length; }
	size_t get_length() const volatile { return m_contents.begin_read()->m_length; }

	size_t get_capacity() const { return m_contents->get_capacity(); }

	size_t get_reverse_capacity() const { return m_contents->get_reverse_capacity(); }

	bool is_empty() const { return m_contents->m_length == 0; }
	bool is_empty() const volatile { return m_contents.begin_read()->m_length == 0; }

	bool operator!() const { return m_contents->m_length == 0; }
	bool operator!() const volatile { return m_contents.begin_read()->m_length == 0; }

	const type* get_const_ptr() const { return m_contents->m_ptr; }

	// caller error to pass index >= length
	const type& operator[](size_t i) const { return get_const_ptr()[i]; }

	const type& get_first_const() const { return get_const_ptr()[0]; }
	const type& get_last_const() const { return get_const_ptr()[get_length() - 1]; }


	const this_t& subrange(size_t i, size_t n = const_max_int_v<size_t>, unowned_t<this_t>& storage = unowned_t<this_t>().get_unowned()) const
	{
		size_t length = get_length();
		if (i < length)
		{
			size_t adjustedLength = length - i;
			if (adjustedLength > n)
				adjustedLength = n;
			storage.set(m_contents->m_desc, m_contents->m_ptr + i, adjustedLength);
		}
		return storage;
	}

	this_t subrange(size_t i, size_t n = const_max_int_v<size_t>) const volatile
	{
		this_t result(*this);
		result.set_to_subrange(i, n);
		return result;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t n, const type2& cmp) const { return m_contents->template compare<type2, comparator_t>(n, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t n, const type2& cmp) const volatile { this_t tmp(*this); return tmp.template compare<type2, comparator_t>(n, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const type2* cmp, size_t n) const { return m_contents->template compare<type2, comparator_t>(cmp, n); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const type2* cmp, size_t n) const volatile { this_t tmp(*this); return tmp.template compare<type2, comparator_t>(cmp, n); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const vector<type2>& cmp) const { return compare<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const vector<type2>& cmp) const volatile { return compare<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const volatile vector<type2>& cmp) const { vector<type2> tmp(cmp); return compare<type2, comparator_t>(tmp); }

	
	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t n, const type2& cmp) const { return m_contents->template equals<type2, comparator_t>(n, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t n, const type2& cmp) const volatile { this_t tmp(*this); return tmp.template equals<type2, comparator_t>(n, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const type2* cmp, size_t n) const { return m_contents->template equals<type2, comparator_t>(cmp, n); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const type2* cmp, size_t n) const volatile { this_t tmp(*this); return tmp.template equals<type2, comparator_t>(cmp, n); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const vector<type2>& cmp) const { return equals<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const vector<type2>& cmp) const volatile { return equals<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const volatile vector<type2>& cmp) const { vector<type2> tmp(cmp); return equals<type2, comparator_t>(tmp); }


	template <typename type2 = type>
	bool operator==(const vector<type2>& cmp) const { return equals(cmp); }

	template <typename type2 = type>
	bool operator==(const vector<type2>& cmp) const volatile { return equals(cmp); }

	template <typename type2 = type>
	bool operator==(const volatile vector<type2>& cmp) const { return equals(cmp); }

	template <typename type2 = type>
	bool operator!=(const vector<type2>& cmp) const { return !equals(cmp); }

	template <typename type2 = type>
	bool operator!=(const vector<type2>& cmp) const volatile { return !equals(cmp); }

	template <typename type2 = type>
	bool operator!=(const volatile vector<type2>& cmp) const { return !equals(cmp); }


	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(size_t n, const type2& cmp) const { return m_contents->template starts_with<type2, comparator_t>(n, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(size_t n, const type2& cmp) const volatile { this_t tmp(*this); return tmp.template starts_with<type2, comparator_t>(n, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const type2* cmp, size_t n) const { return m_contents->template starts_with<type2, comparator_t>(cmp, n); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const type2* cmp, size_t n) const volatile { this_t tmp(*this); return tmp.template starts_with<type2, comparator_t>(cmp, n); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const vector<type2>& cmp) const { return starts_with<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const vector<type2>& cmp) const volatile { return starts_with<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const volatile vector<type2>& cmp) const { vector<type2> tmp(cmp); return starts_with<type2, comparator_t>(tmp); }




	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(size_t n, const type2& cmp) const { return m_contents->template ends_with<type2, comparator_t>(n, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(size_t n, const type2& cmp) const volatile { this_t tmp(*this); return tmp.template ends_with<type2, comparator_t>(n, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const type2* cmp, size_t n) const { return m_contents->template ends_with<type2, comparator_t>(cmp, n); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const type2* cmp, size_t n) const volatile { this_t tmp(*this); return tmp.template ends_with<type2, comparator_t>(cmp, n); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const vector<type2>& cmp) const { return ends_with<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const vector<type2>& cmp) const volatile { return ends_with<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const volatile vector<type2>& cmp) const { vector<type2> tmp(cmp); return ends_with<type2, comparator_t>(tmp); }
	




	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const type2* cmp, size_t n) const { return m_contents->template is_less_than<type2, comparator_t>(cmp, n); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const type2* cmp, size_t n) const volatile { this_t tmp(*this); return tmp.template is_less_than<type2, comparator_t>(cmp, n); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const vector<type2>& cmp) const { return is_less_than<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const vector<type2>& cmp) const volatile { return is_less_than<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const volatile vector<type2>& cmp) const { vector<type2> tmp(cmp); return is_less_than<type2, comparator_t>(tmp); }






	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const type2* cmp, size_t n) const { return m_contents->template is_greater_than<type2, comparator_t>(cmp, n); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const type2* cmp, size_t n) const volatile { this_t tmp(*this); return tmp.template is_greater_than<type2, comparator_t>(cmp, n); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const vector<type2>& cmp) const { return is_greater_than<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const vector<type2>& cmp) const volatile { return is_greater_than<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const volatile vector<type2>& cmp) const { vector<type2> tmp(cmp); return is_greater_than<type2, comparator_t>(tmp); }
	


	template <typename type2>
	bool operator<(const vector<type2>& cmp) const { return is_less_than(cmp); }

	template <typename type2>
	bool operator<(const vector<type2>& cmp) const volatile { return is_less_than(cmp); }

	template <typename type2>
	bool operator<(const volatile vector<type2>& cmp) const { return is_less_than(cmp); }

	template <typename type2>
	bool operator>=(const vector<type2>& cmp) const { return !is_less_than(cmp); }

	template <typename type2>
	bool operator>=(const vector<type2>& cmp) const volatile { return !is_less_than(cmp); }

	template <typename type2>
	bool operator>=(const volatile vector<type2>& cmp) const { return !is_less_than(cmp); }

	template <typename type2>
	bool operator>(const vector<type2>& cmp) const { return is_greater_than(cmp); }

	template <typename type2>
	bool operator>(const vector<type2>& cmp) const volatile { return is_greater_than(cmp); }

	template <typename type2>
	bool operator>(const volatile vector<type2>& cmp) const { return is_greater_than(cmp); }

	template <typename type2>
	bool operator<=(const vector<type2>& cmp) const { return !is_greater_than(cmp); }

	template <typename type2>
	bool operator<=(const vector<type2>& cmp) const volatile { return !is_greater_than(cmp); }

	template <typename type2>
	bool operator<=(const volatile vector<type2>& cmp) const { return !is_greater_than(cmp); }


	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of(const type2& cmp) const { return m_contents->template index_of<type2, comparator_t>(0, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of(const type2& cmp) const volatile { this_t tmp(*this); return tmp.template index_of<type2, comparator_t>(cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of(size_t i, const type2& cmp) const { return m_contents->template index_of<type2, comparator_t>(i, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of(size_t i, const type2& cmp) const volatile { this_t tmp(*this); return tmp.template index_of<type2, comparator_t>(i, cmp); }
	



	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_any(const type2* cmp, size_t cmpLength) const { return m_contents->template index_of_any<type2, comparator_t>(0, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_any(const type2* cmp, size_t cmpLength) const volatile { this_t tmp(*this); return tmp.template index_of_any<type2, comparator_t>(cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_any(size_t i, const type2* cmp, size_t cmpLength) const { return m_contents->template index_of_any<type2, comparator_t>(i, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_any(size_t i, const type2* cmp, size_t cmpLength) const volatile { this_t tmp(*this); return tmp.template index_of_any<type2, comparator_t>(i, cmp, cmpLength); }




	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const type2* cmp, size_t cmpLength) const { return m_contents->template index_of_segment<type2, comparator_t>(0, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const type2* cmp, size_t cmpLength) const volatile { this_t tmp(*this); return tmp.template index_of_segment<type2, comparator_t>(cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const vector<type2>& cmp) const { return index_of_segment<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const vector<type2>& cmp) const volatile { return index_of_segment<type2, comparator_t>(cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const volatile vector<type2>& cmp) const { vector<type2> tmp; return index_of_segment<type2, comparator_t>(tmp); }


	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const type2* cmp, size_t cmpLength) const { return m_contents->template index_of_segment<type2, comparator_t>(i, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const type2* cmp, size_t cmpLength) const volatile { this_t tmp(*this); return tmp.template index_of_segment<type2, comparator_t>(i, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const vector<type2>& cmp) const { return index_of_segment<type2, comparator_t>(i, cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const vector<type2>& cmp) const volatile { return index_of_segment<type2, comparator_t>(i, cmp.get_const_ptr(), cmp.get_length()); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const volatile vector<type2>& cmp) const { vector<type2> tmp; return index_of_segment<type2, comparator_t>(i, tmp); }




	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains(const type2& cmp) const { return index_of<type2, comparator_t>(cmp) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains(const type2& cmp) const volatile { return index_of<type2, comparator_t>(cmp) != const_max_int_v<size_t>; }




	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_any(const type2* cmp, size_t cmpLength) const { return index_of_any<type2, comparator_t>(cmp, cmpLength) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_any(const type2* cmp, size_t cmpLength) const volatile { return index_of_any<type2, comparator_t>(cmp, cmpLength) != const_max_int_v<size_t>; }



	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const type2* cmp, size_t cmpLength) const { return index_of_segment<type2, comparator_t>(cmp, cmpLength) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const type2* cmp, size_t cmpLength) const volatile { return index_of_segment<type2, comparator_t>(cmp, cmpLength) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const vector<type2>& cmp) const { return index_of_segment<type2, comparator_t>(cmp) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const vector<type2>& cmp) const volatile { return index_of_segment<type2, comparator_t>(cmp) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const volatile vector<type2>& cmp) const { return index_of_segment<type2, comparator_t>(cmp) != const_max_int_v<size_t>; }



	type* get_ptr() { return m_contents->get_ptr(); }
	type& get_first() { return get_ptr()[0]; }
	type& get_last() { return get_ptr()[m_contents->m_length - 1]; }

	template <typename type2>
	void set_index(size_t i, const type2& src) { get_ptr()[i] = src; }

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

	void clear() { m_contents->clear(); }
	void clear() volatile
	{
		write_token wt;
		do {
			m_contents.begin_write(wt);
			wt->m_length = 0;
		} while (!m_contents.end_write(wt));
	}

	// clears reserved space as well
	void reset()
	{
		m_contents->release();
		m_contents->clear_inner();
	}

	// clears reserved space as well
	void reset() volatile
	{
		content_t tmp;
		m_contents.swap_contents(tmp);
		tmp.release();
	}

	void reserve(size_t n) { m_contents->reserve(n); }

	void assign(size_t n, const type& src)
	{
		m_contents->assign(n, src);
	}

	void assign(size_t n, const type& src) volatile
	{
		m_contents.set(content_t(n, src));
	}

	template <typename type2>
	void assign(size_t n, const type2& src)
	{
		m_contents->assign(n, src);
	}

	template <typename type2>
	void assign(size_t n, const type2& src) volatile
	{
		m_contents.set(content_t(n, src));
	}

	template <typename type2>
	void assign(const type2* src, size_t n)
	{
		m_contents->assign(src, n);
	}

	template <typename type2>
	void assign(const type2* src, size_t n) volatile
	{
		m_contents.set(content_t(src, n));
	}

	void assign(const vector<type>& src)
	{
		if (this != &src)
			m_contents->acquire(*(src.m_contents));
	}

	template <typename type2>
	void assign(const vector<type2>& src)
	{
		m_contents->assign(src.get_const_ptr(), src.get_length());
	}

	template <typename type2>
	void assign(const vector<type2>& src) volatile
	{
		content_t tmp(src.get_const_ptr(), src.get_length());
		m_contents.swap_contents(tmp);
		tmp.release();
	}

	template <typename type2>
	void assign(const volatile vector<type2>& src)
	{
		vector<std::remove_const_t<type2> > tmp(src);
		assign(tmp);
	}

	this_t& operator=(const this_t& src)
	{
		assign(src);
		return *this;
	}

	template <typename type2>
	this_t& operator=(const vector<type2>& src)
	{
		assign(src);
		return *this;
	}

	this_t operator=(const this_t& src) volatile
	{
		assign(src);
		return src;
	}

	template <typename type2>
	this_t operator=(const vector<type2>& src) volatile
	{
		assign(src);
		return tmp;
	}

	template <typename type2>
	this_t& operator=(const volatile vector<type2>& src)
	{
		assign(src);
		return *this;
	}

	void append(size_t n = 1)
	{
		if (!!n)
			m_contents->append(n);
	}

	void append(size_t n, const type& src)
	{
		if (!!n)
			m_contents->append(n, src);
	}

	template <typename type2>
	void append(size_t n, const type2& src)
	{
		if (!!n)
			m_contents->append(n, src);
	}

	template <typename type2>
	void append(const type2* src, size_t n)
	{
		if (!!n)
			m_contents->append(src, n);
	}

	template <typename type2>
	void append(const vector<type2>& src)
	{
		size_t n = src.get_length();
		if (!!n)
			m_contents->append(src.get_const_ptr(), n);
	}

	template <typename type2>
	void append(const volatile vector<type2>& src)
	{
		vector<type2> tmp(src);
		append(tmp);
	}

	this_t& operator+=(const type& src)
	{
		append(1, src);
		return *this;
	}

	template <typename type2>
	this_t& operator+=(const vector<type2>& src)
	{
		append(src);
		return *this;
	}

	template <typename type2>
	this_t& operator+=(const volatile vector<type2>& src)
	{
		vector<type2> tmp(src);
		append(tmp);
		return *this;
	}

	void prepend(size_t n = 1)
	{
		if (!!n)
			m_contents->prepend(n);
	}

	void prepend(size_t n, const type& src)
	{
		if (!!n)
			m_contents->prepend(n, src);
	}

	template <typename type2>
	void prepend(size_t n, const type2& src)
	{
		if (!!n)
			m_contents->prepend(n, src);
	}

	template <typename type2>
	void prepend(const type2* src, size_t n)
	{
		if (!!n)
			m_contents->prepend(src, n);
	}

	template <typename type2>
	void prepend(const vector<type2>& src)
	{
		size_t n = src.get_length();
		if (!!n)
			m_contents->prepend(src.get_const_ptr(), n);
	}

	template <typename type2>
	void prepend(const volatile vector<type2>& src)
	{
		vector<type2> tmp(src);
		prepend(tmp);
	}


	void resize(size_t n) { m_contents->resize(n); }

	void resize(size_t n, const type& src) { m_contents->resize(n, src); }

	void erase(size_t i) { erase(i, 1); }

	void erase(size_t i, size_t n)
	{
		if (!i)
			m_contents->advance(n);
		else
			m_contents->erase(i, n);
	}

	void advance(size_t n = 1) { m_contents->advance(n); }

	void truncate_to(size_t n) { m_contents->truncate_to(n); }

	void truncate(size_t n) { m_contents->truncate(n); }

	void truncate_to_right(size_t n) { m_contents->truncate_to_right(n); }

	void insert(size_t i, size_t n)
	{
		if (!i)
			prepend(n);
		if (i >= m_contents->m_length)
			append(n);
		else if (!!n)
			m_contents->insert(i, n);
	}

	void insert(size_t i, size_t n, const type& src)
	{
		if (!i)
			prepend(n, src);
		if (i >= m_contents->m_length)
			append(n, src);
		else if (!!n)
			m_contents->insert(i, n, src);
	}

	template <typename type2>
	void insert(size_t i, size_t n, const type2& src)
	{
		if (!i)
			prepend(n, src);
		if (i >= m_contents->m_length)
			append(n, src);
		else if (!!n)
			m_contents->insert(i, n, src);
	}

	template <typename type2>
	void insert(size_t i, const type2* src, size_t n)
	{
		if (!i)
			prepend(src, n);
		if (i >= m_contents->m_length)
			append(src, n);
		else if (!!n)
			m_contents->insert(i, src, n);
	}

	template <typename type2>
	void insert(size_t i, const vector<type2>& src)
	{
		if (!m_contents->m_length)
			assign(src);
		else
			insert(i, src.get_const_ptr(), src.get_length());
	}

	template <typename type2>
	void insert(size_t i, const volatile vector<type2>& src)
	{
		if (!i)
			prepend(src);
		if (i >= m_contents->m_length)
			append(src);
		else if (!m_contents->m_length)
			assign(src);
		else
		{
			vector<type2> tmp(src);
			m_contents->insert(i, tmp.get_const_ptr(), tmp.get_length());
		}
	}

	template <typename type2>
	void insert(size_t i, const vector<type2>& src, size_t srcIndex, size_t n = const_max_int_v<size_t>)
	{
		insert(i, src.subrange(srcIndex, n))
	}

	template <typename type2>
	void insert(size_t i, const volatile vector<type2> & src, size_t srcIndex, size_t n = const_max_int_v<size_t>)
	{
		insert(i, src.subrange(srcIndex, n))
	}



	template <typename type2>
	void replace(size_t i, size_t replaceLength, const type2& src)
	{
		m_contents->replace(i, replaceLength, src);
	}

	template <typename type2>
	void replace(size_t i, const type2* src, size_t replaceLength)
	{
		m_contents->replace(i, src, replaceLength);
	}

	template <typename type2>
	void replace(size_t i, const vector<type2>& src)
	{
		replace(i, src.m_contents->get_const_ptr(), src.get_length());
	}

	template <typename type2>
	void replace(size_t i, const volatile vector<type2>& src)
	{
		vector<type2> tmp(src);
		replace(i, src);
	}

	template <typename type2>
	void replace(size_t i, const vector<type2>& src, size_t srcIndex, size_t n = const_max_int_v<size_t>)
	{
		replace(i, src.subrange(srcIndex, n))
	}

	template <typename type2>
	void replace(size_t i, const volatile vector<type2>& src, size_t srcIndex, size_t n = const_max_int_v<size_t>)
	{
		replace(i, src.subrange(srcIndex, n))
	}



	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, size_t insertLength, const type2& src)
	{
		m_contents->insert_replace(i, replaceLength, insertLength, src);
	}

	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, const type2* src, size_t insertLength)
	{
		m_contents->insert_replace(i, replaceLength, src, insertLength);
	}

	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, const vector<type2>& src)
	{
		insert_replace(i, replaceLength, src.get_const_ptr(), src.get_length());
	}

	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, const volatile vector<type2>& src)
	{
		vector<type2> tmp(src);
		insert_replace(i, replaceLength, tmp);
	}

	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, const vector<type2>& src, size_t srcIndex, size_t n = const_max_int_v<size_t>)
	{
		insert_replace(i, replaceLength, src.subrange(srcIndex, n))
	}

	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, const volatile vector<type2>& src, size_t srcIndex, size_t n = const_max_int_v<size_t>)
	{
		insert_replace(i, replaceLength, src.subrange(srcIndex, n))
	}



	void swap(this_t& wth) { m_contents.swap(wth.m_contents); }
	void swap(this_t& wth) volatile { m_contents.swap(wth.m_contents); }
	void swap(volatile this_t& wth) { wth.swap(*this); }

	this_t exchange(this_t&& src) { return cogs::exchange(std::move(src)); }
	this_t exchange(this_t&& src) volatile { return cogs::exchange(std::move(src)); }

	void exchange(const this_t& src, this_t& rtn) { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const volatile this_t& src, this_t& rtn) { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const this_t& src, volatile this_t& rtn) { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const volatile this_t& src, volatile this_t& rtn) { m_contents.exchange(src.m_contents, rtn.m_contents); }

	void exchange(this_t&& src, this_t& rtn) { m_contents.exchange(std::move(src.m_contents), rtn.m_contents); }
	void exchange(this_t&& src, volatile this_t& rtn) { m_contents.exchange(std::move(src.m_contents), rtn.m_contents); }

	void exchange(const this_t& src, this_t& rtn) volatile { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const volatile this_t& src, this_t& rtn) volatile { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const this_t& src, volatile this_t& rtn) volatile { m_contents.exchange(src.m_contents, rtn.m_contents); }
	void exchange(const volatile this_t& src, volatile this_t& rtn) volatile { m_contents.exchange(src.m_contents, rtn.m_contents); }

	void exchange(this_t&& src, this_t& rtn) volatile { m_contents.exchange(std::move(src.m_contents), rtn.m_contents); }
	void exchange(this_t&& src, volatile this_t& rtn) volatile { m_contents.exchange(std::move(src.m_contents), rtn.m_contents); }


	template <typename char_t>
	string_t<char_t> to_string_t(const string_t<char_t>& prefix, const string_t<char_t>& delimiter, const string_t<char_t>& postfix) const;

	string to_string(const string& prefix, const string& delimiter, const string& postfix) const;
	cstring to_cstring(const cstring& prefix, const cstring& delimiter, const cstring& postfix) const;

	template <typename char_t>
	string_t<char_t> to_string_t() const;
	string to_string() const;
	cstring to_cstring() const;

	template <typename char_t>
	string_t<char_t> to_string_t(const string_t<char_t>& prefix, const string_t<char_t>& delimiter, const string_t<char_t>& postfix) const volatile;
	string to_string(const string& prefix, const string& delimiter, const string& postfix) const volatile;
	cstring to_cstring(const cstring& prefix, const cstring& delimiter, const cstring& postfix) const volatile;

	template <typename char_t>
	string_t<char_t> to_string_t() const volatile;
	string to_string() const volatile;
	cstring to_cstring() const volatile;


	this_t split_off_before(size_t i)
	{
		this_t result;
		if (!!i)
		{
			if (i >= m_contents->m_length)
			{
				result = *this;
				clear();
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

	this_t split_off_before(size_t i) volatile
	{
		this_t result;
		if (!!i)
		{
			write_token wt;
			desc_t* desc;
			for (;;)
			{
				guarded_begin_write(wt);
				desc = wt->m_desc;	// acquired, regardless of whether the commit succeeds.
				result.m_contents->m_desc = desc;
				result.m_contents->m_ptr = wt->m_ptr;
				size_t length = wt->m_length;
				if (i >= length)
				{
					result.m_contents->m_length = length;
					wt->clear_inner();
					if (desc)			// Ok to release now.  If commit succeeds, we know it remained valid up until then.
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
				if (desc)							// if commit unsuccessful, and i < length, we need to release 1 reference
					desc->release();
				result.m_contents->m_desc = 0;
			}
		}
		return result;
	}

	this_t split_off_after(size_t i)
	{
		this_t result;
		size_t length = m_contents->m_length;
		if (i < length)
		{
			result.m_contents->acquire(*m_contents, i, length - i);
			m_contents->m_length = i;
		}
		return result;
	}

	this_t split_off_after(size_t i) volatile
	{
		this_t result;
		if (!i)
			swap(result);
		else
		{
			write_token wt;
			for (;;)
			{
				guarded_begin_write(wt);
				desc_t* desc = wt->m_desc;	// acquired, regardless of whether the commit succeeds.
				size_t length = wt->m_length;
				if (i >= length)	// nop.  If nothing to split off back, we don't need to write at all.
				{
					result.m_ptr = 0;
					result.m_length = 0;
					if (desc)			// Release our copy from guard
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
			return result;
		}
	}

	this_t split_off_front(size_t n) { return split_off_before(n); }

	this_t split_off_front(size_t n) volatile { return split_off_before(n); }

	this_t split_off_back(size_t n)
	{
		this_t result;
		if (!!n)
		{
			if (n >= m_contents->m_length)
			{
				result = *this;
				clear();
			}
			else
			{
				m_contents->m_length -= n;
				result.m_contents->acquire(*m_contents, m_contents->m_length, n);
			}
		}
		return result;
	}

	this_t split_off_back(size_t n) volatile
	{
		this_t result;
		if (!!n)
		{
			write_token wt;
			desc_t* desc;
			for (;;)
			{
				guarded_begin_write(wt);
				desc = wt->m_desc;	// acquired, regardless of whether the commit succeeds.
				result.m_contents->m_ptr = wt->m_ptr;
				result.m_contents->m_desc = desc;
				size_t length = wt->m_length;
				if (n >= length)
				{
					result.m_contents->m_length = length;
					wt->clear_inner();
					if (desc)			// Ok to release now.  If commit succeeds, we know it remained valid up until then.
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
				if (desc)							// if commit unsuccessful, and i < length, we need to release 1 reference
					desc->release();
				result.m_contents->m_desc = 0;
			}
		}
		return result;
	}



	template <typename type2>
	vector<this_t> split_on(const type2& splitOn, split_options opt = split_includes_empty_segments) const
	{
		return split_on_any_inner<this_t>(*this, &splitOn, 1, opt);
	}

	template <typename type2>
	vector<this_t> split_on(const type2& splitOn, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return tmp.template split_on_any_inner<this_t>(*this, &splitOn, 1, opt);
	}


	template <typename type2>
	vector<this_t> split_on_any(const type2* splitOn, size_t n, split_options opt = split_includes_empty_segments) const
	{
		return split_on_any_inner<this_t>(*this, splitOn, n, opt);
	}

	template <typename type2>
	vector<this_t> split_on_any(const type2* splitOn, size_t n, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return tmp.template split_on_any_inner<this_t>(*this, splitOn, n, opt);
	}

	template <typename type2>
	vector<this_t> split_on_any(const vector<type2>& splitOn, split_options opt = split_includes_empty_segments) const
	{
		return split_on_any_inner<this_t>(*this, splitOn.get_const_ptr(), splitOn.get_length(), opt);
	}

	template <typename type2>
	vector<this_t> split_on_any(const vector<type2>& splitOn, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return tmp.template split_on_any_inner<this_t>(*this, splitOn.get_const_ptr(), splitOn.get_length(), opt);
	}

	template <typename type2>
	vector<this_t> split_on_any(const volatile vector<type2>& splitOn, split_options opt = split_includes_empty_segments) const
	{
		vector<type2> tmp(splitOn);
		return split_on_any_inner<this_t>(*this, tmp.get_const_ptr(), tmp.get_length(), opt);
	}


	template <typename type2>
	vector<this_t> split_on_segment(const type2* splitOn, size_t n, split_options opt = split_includes_empty_segments) const
	{
		return split_on_segment_inner<this_t>(*this, splitOn, n, opt);
	}

	template <typename type2>
	vector<this_t> split_on_segment(const type2* splitOn, size_t n, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return tmp.template split_on_segment_inner<this_t>(*this, splitOn, n, opt);
	}

	template <typename type2>
	vector<this_t> split_on_segment(const vector<type2>& splitOn, split_options opt = split_includes_empty_segments) const
	{
		return split_on_segment_inner<this_t>(*this, splitOn.get_const_ptr(), splitOn.get_length(), opt);
	}

	template <typename type2>
	vector<this_t> split_on_segment(const vector<type2>& splitOn, split_options opt = split_includes_empty_segments) const volatile
	{
		this_t tmp(*this);
		return tmp.template split_on_segment_inner<this_t>(*this, splitOn.get_const_ptr(), splitOn.get_length(), opt);
	}

	template <typename type2>
	vector<this_t> split_on_segment(const volatile vector<type2>& splitOn, split_options opt = split_includes_empty_segments) const
	{
		vector<type2> tmp(splitOn);
		return split_on_segment_inner<this_t>(*this, tmp.get_const_ptr(), tmp.get_length(), opt);
	}


protected:
	template <typename>
	friend class string_t;

	template <class array_t, typename type2>
	static vector<array_t> split_on_any_inner(const array_t& src, const type2* splitOn, size_t n, split_options opt = split_includes_empty_segments)
	{
		vector<array_t> result;
		size_t lastStart = 0;
		size_t i = 0;
		for (;;)
		{
			i = src.index_of_any(i, splitOn, n);
			size_t segmentLength = (i != const_max_int_v<size_t>) ? i : src.get_length();
			segmentLength -= lastStart;
			if (!!segmentLength || (opt == split_includes_empty_segments))
				result.append(1, src.subrange(lastStart, segmentLength));
			if (i == const_max_int_v<size_t>)
				break;
			lastStart = ++i;
		}
		return result;
	}

	template <class array_t, typename type2>
	static vector<array_t> split_on_segment_inner(const array_t& src, const type2* splitOn, size_t n, split_options opt = split_includes_empty_segments)
	{
		vector<array_t> result;
		size_t lastStart = 0;
		size_t i = 0;
		for (;;)
		{
			i = src.index_of(i, splitOn, n);
			size_t segmentLength = (i != const_max_int_v<size_t>) ? i : src.get_length();
			segmentLength -= lastStart;
			if (!!segmentLength || (opt == split_includes_empty_segments))
				result.append(1, src.subrange(lastStart, segmentLength));
			if (i == const_max_int_v<size_t>)
				break;
			i += n;
			lastStart = i;
		}
		return result;
	}
};






#pragma warning(pop)


}


#endif
