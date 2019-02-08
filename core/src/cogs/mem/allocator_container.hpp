//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_ALLOCATOR_CONTAINER
#define COGS_ALLOCATOR_CONTAINER


#include "cogs/env.hpp"
#include "cogs/math/least_multiple_of.hpp"
#include "cogs/math/const_lcm.hpp"
#include "cogs/mem/allocator.hpp"
#include "cogs/mem/placement.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {


#pragma warning(push)
#pragma warning (disable: 4521)	// multiple copy constructors speecified
#pragma warning (disable: 4522)	// multiple assignment operators specified


template <class allocator_type,
	bool is_static_in = allocator_type::is_static,
	bool is_ptr_based = std::is_same<typename allocator_type::ref_t, ptr<void> >::value>
class allocator_container
{
private:
	ptr<volatile allocator_type> m_allocator;

public:
	static const bool is_static = is_static_in;

	typedef allocator_container<allocator_type, false, false>	this_t;

	typedef typename allocator_type::ref_t ref_t;


	allocator_container();	// not linkable

	allocator_container(const this_t& src)
		:	m_allocator(src.m_allocator)
	{ }

	allocator_container(const volatile this_t& src)
		:	m_allocator(src.m_allocator)
	{ }

	allocator_container(volatile allocator_type& al)
		:	m_allocator(&al)
	{ }

	this_t& operator=(const  this_t& src)				{ m_allocator = src.m_allocator; return *this; }
	void operator=(const this_t& src) volatile		{ m_allocator = src.m_allocator; }
	//volatile this_t& operator=(const this_t& src) volatile		{ m_allocator = src.m_allocator; return *this; }
	this_t& operator=(const volatile this_t& src)				{ m_allocator = src.m_allocator; return *this; }

	ref<volatile allocator_type> get_allocator() const			{ return *m_allocator; }
	ref<volatile allocator_type> get_allocator() const volatile	{ return *m_allocator; }
	
	ref_t allocate(size_t n, size_t align) const										{ return m_allocator->allocate(n, align); }
	ref_t allocate(size_t n, size_t align) const volatile								{ return m_allocator->allocate(n, align); }

	template <typename type>
	typename ref_t::template cast_type<type>::type allocate_type(size_t n = 1) const			{ return allocate(sizeof(type) * n, std::alignment_of<type>::value).template reinterpret_cast_to<type>(); }

	template <typename type>
	typename ref_t::template cast_type<type>::type allocate_type(size_t n = 1) const volatile	{ return allocate(sizeof(type) * n, std::alignment_of<type>::value).template reinterpret_cast_to<type>(); }

	void deallocate(const ref_t& p) const			{ m_allocator->deallocate(p); }
	void deallocate(const ref_t& p) const volatile	{ m_allocator->deallocate(p); }

	template <typename type> void deallocate(const typename ref_t::template cast_type<type>::type& p) const				{ m_allocator->deallocate(p.template reinterpret_cast_to<void>()); }
	template <typename type> void deallocate(const typename ref_t::template cast_type<type>::type& p) const volatile	{ m_allocator->deallocate(p.template reinterpret_cast_to<void>()); }
	
	template <typename type>
	void destruct_deallocate_type(const typename ref_t::template cast_type<type>::type& p, size_t n = 1) const
	{
		if (!!p)
		{
			placement_destruct_multiple(p.get_ptr(), placement_destruct_multiple);
			m_allocator->deallocate(p);
		}
	}

	template <typename type>
	void destruct_deallocate_type(const typename ref_t::template cast_type<type>::type& p, size_t n = 1) const volatile
	{
		if (!!p)
		{
			placement_destruct_multiple(p.get_ptr(), n);
			m_allocator->deallocate(p);
		}
	}

	template <typename type> bool try_reallocate(const typename ref_t::template cast_type<type>::type& p, size_t n) const	
	{ return m_allocator->try_reallocate(p.template reinterpret_cast_to<void>(), n);  }

	template <typename type> bool try_reallocate(const typename ref_t::template cast_type<type>::type& p, size_t n) const volatile	
	{ return m_allocator->try_reallocate(p.template reinterpret_cast_to<void>(), n);  }

	template <typename type> bool try_reallocate_type(const typename ref_t::template cast_type<type>::type& p, size_t n) const
	{ return m_allocator->try_reallocate(p.template reinterpret_cast_to<void>(), n * sizeof(type)); }

	template <typename type> bool try_reallocate_type(const typename ref_t::template cast_type<type>::type& p, size_t n) const volatile
	{ return m_allocator->try_reallocate(p.template reinterpret_cast_to<void>(), n * sizeof(type)); }
	
	template <typename type> size_t get_allocation_size(const typename ref_t::template cast_type<type>::type& p, size_t align, size_t knownSize) const
	{ return m_allocator->get_allocation_size(p.template reinterpret_cast_to<void>(), align, knownSize);  }

	template <typename type> size_t get_allocation_size(const typename ref_t::template cast_type<type>::type& p, size_t align, size_t knownSize) const volatile
	{ return m_allocator->get_allocation_size(p.template reinterpret_cast_to<void>(), align, knownSize);  }

	template <typename type> size_t get_allocation_size_type(const ptr<type>& p, size_t knownSize) const	
	{ return m_allocator->get_allocation_size(p.template reinterpret_cast_to<void>(), std::alignment_of<type>::value, knownSize * sizeof(type)) / sizeof(type); }

	template <typename type> size_t get_allocation_size_type(const ptr<type>& p, size_t knownSize) const volatile	
	{ return m_allocator->get_allocation_size(p.template reinterpret_cast_to<void>(), std::alignment_of<type>::value, knownSize * sizeof(type)) / sizeof(type); }
};


template <class allocator_type>
class allocator_container<allocator_type, false, true>
{
private:
	ptr<volatile allocator_type> m_allocator;

public:
	typedef allocator_container<allocator_type, false, true>	this_t;

	static const bool is_static = false;

	typedef ptr<void> ref_t;

	allocator_container();	// not linkable

	allocator_container(const this_t& src)
		:	m_allocator(src.m_allocator)
	{ }

	allocator_container(const volatile this_t& src)
		:	m_allocator(src.m_allocator)
	{ }

	allocator_container(volatile allocator_type& al)
		:	m_allocator(&al)
	{ }

	         this_t& operator=(const          this_t& src)				{ m_allocator = src.m_allocator; return *this; }
	volatile this_t& operator=(const          this_t& src) volatile		{ m_allocator = src.m_allocator; return *this; }
	         this_t& operator=(const volatile this_t& src)				{ m_allocator = src.m_allocator; return *this; }

	ref<volatile allocator_type> get_allocator() const			{ return *m_allocator; }
	ref<volatile allocator_type> get_allocator() const volatile	{ return *m_allocator; }
	
	ref_t allocate(size_t n, size_t align) const										{ return m_allocator->allocate(n, align); }
	ref_t allocate(size_t n, size_t align) const volatile								{ return m_allocator->allocate(n, align); }

	template <typename type>
	type* allocate_type(size_t n = 1) const			{ return allocate(sizeof(type) * n, std::alignment_of<type>::value).template reinterpret_cast_to<type>(); }

	template <typename type>
	type* allocate_type(size_t n = 1) const volatile	{ return allocate(sizeof(type) * n, std::alignment_of<type>::value).template reinterpret_cast_to<type>(); }

	void deallocate(const ref_t& p) const			{ m_allocator->deallocate(p); }
	void deallocate(const ref_t& p) const volatile	{ m_allocator->deallocate(p); }

	template <typename type> void deallocate(const ptr<type>& p) const			{ m_allocator->deallocate(p.template reinterpret_cast_to<void>()); }
	template <typename type> void deallocate(const ptr<type>& p) const volatile	{ m_allocator->deallocate(p.template reinterpret_cast_to<void>()); }
	
	template <typename type>
	void destruct_deallocate_type(const ptr<type>& p, size_t n = 1) const
	{
		if (!!p)
		{
			placement_destruct_multiple(p.get_ptr(), n);
			m_allocator->deallocate(p);
		}
	}

	template <typename type>
	void destruct_deallocate_type(const ptr<type>& p, size_t n = 1) const volatile
	{
		if (!!p)
		{
			placement_destruct_multiple(p.get_ptr(), n);
			m_allocator->deallocate(p);
		}
	}

	template <typename type> bool try_reallocate(const ptr<type>& p, size_t n) const	
	{ return m_allocator->try_reallocate(p.template reinterpret_cast_to<void>(), n);  }

	template <typename type> bool try_reallocate(const ptr<type>& p, size_t n) const volatile	
	{ return m_allocator->try_reallocate(p.template reinterpret_cast_to<void>(), n);  }

	template <typename type> bool try_reallocate_type(const ptr<type>& p, size_t n) const
	{ return m_allocator->try_reallocate(p.template reinterpret_cast_to<void>(), n * sizeof(type)); }

	template <typename type> bool try_reallocate_type(const ptr<type>& p, size_t n) const volatile
	{ return m_allocator->try_reallocate(p.template reinterpret_cast_to<void>(), n * sizeof(type)); }
	
	template <typename type> size_t get_allocation_size(const ptr<type>& p, size_t align, size_t knownSize) const
	{ return m_allocator->get_allocation_size(p.template reinterpret_cast_to<void>(), align, knownSize);  }

	template <typename type> size_t get_allocation_size(const ptr<type>& p, size_t align, size_t knownSize) const volatile
	{ return m_allocator->get_allocation_size(p.template reinterpret_cast_to<void>(), align, knownSize);  }

	template <typename type> size_t get_allocation_size_type(const ptr<type>& p, size_t knownSize) const	
	{ return m_allocator->get_allocation_size(p.template reinterpret_cast_to<void>(), std::alignment_of<type>::value, knownSize * sizeof(type)) / sizeof(type); }

	template <typename type> size_t get_allocation_size_type(const ptr<type>& p, size_t knownSize) const volatile	
	{ return m_allocator->get_allocation_size(p.template reinterpret_cast_to<void>(), std::alignment_of<type>::value, knownSize * sizeof(type)) / sizeof(type); }

	// helpers with raw pointers
	
	void deallocate(void* p) const					{ m_allocator->deallocate(p); }
	void deallocate(void* p) const volatile			{ m_allocator->deallocate(p); }

	template <typename type> void deallocate(type* p) const					{ m_allocator->deallocate(ptr<type>(p).template reinterpret_cast_to<void>()); }
	template <typename type> void deallocate(type* p) const volatile			{ m_allocator->deallocate(ptr<type>(p).template reinterpret_cast_to<void>()); }
		
	template <typename type>
	void destruct_deallocate_type(type* p, size_t n = 1) const
	{
		if (!!p)
		{
			placement_destruct_multiple(p, n);
			m_allocator->deallocate(p);
		}
	}

	template <typename type>
	void destruct_deallocate_type(type* p, size_t n = 1) const volatile
	{
		if (!!p)
		{
			placement_destruct_multiple(p, n);
			m_allocator->deallocate(p);
		}
	}

	template <typename type> bool try_reallocate(type* p, size_t n) const	
	{ return m_allocator->try_reallocate(ptr<type>(p).template reinterpret_cast_to<void>(), n);  }

	template <typename type> bool try_reallocate(type* p, size_t n) const volatile	
	{ return m_allocator->try_reallocate(ptr<type>(p).template reinterpret_cast_to<void>(), n);  }
	
	template <typename type> bool try_reallocate_type(type* p, size_t n) const
	{ return m_allocator->try_reallocate(ptr<type>(p).template reinterpret_cast_to<void>(), n * sizeof(type)); }

	template <typename type> bool try_reallocate_type(type* p, size_t n) const volatile
	{ return m_allocator->try_reallocate(ptr<type>(p).template reinterpret_cast_to<void>(), n * sizeof(type)); }
	
	template <typename type> size_t get_allocation_size(type* p, size_t align, size_t knownSize) const
	{ return m_allocator->get_allocation_size(ptr<type>(p).template reinterpret_cast_to<void>(), align, knownSize);  }

	template <typename type> size_t get_allocation_size(type* p, size_t align, size_t knownSize) const volatile
	{ return m_allocator->get_allocation_size(ptr<type>(p).template reinterpret_cast_to<void>(), align, knownSize);  }

	template <typename type> size_t get_allocation_size_type(type* p, size_t knownSize) const	
	{ return m_allocator->get_allocation_size(p.template reinterpret_cast_to<void>(), std::alignment_of<type>::value, knownSize * sizeof(type)) / sizeof(type); }

	template <typename type> size_t get_allocation_size_type(type* p, size_t knownSize) const volatile	
	{ return m_allocator->get_allocation_size(p.template reinterpret_cast_to<void>(), std::alignment_of<type>::value, knownSize * sizeof(type)) / sizeof(type); }

	template <typename header_t, size_t align>
	ptr<header_t> allocate_with_header(size_t n) const
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return allocate(n + headerSize, commonAlignment).template reinterpret_cast_to<header_t>();
	}

	template <typename header_t, size_t align>
	ptr<header_t> allocate_with_header(size_t n) const volatile
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return allocate(n + headerSize, commonAlignment).template reinterpret_cast_to<header_t>();
	}

	template <typename header_t, typename type>
	ptr<header_t> allocate_type_with_header(size_t n = 1) const					{ return allocate_with_header<header_t, std::alignment_of<type>::value>(n * sizeof(type)); }

	template <typename header_t, typename type>
	ptr<header_t> allocate_type_with_header(size_t n = 1) const volatile		{ return allocate_with_header<header_t, std::alignment_of<type>::value>(n * sizeof(type)); }


	template <typename header_t, size_t align>
	bool try_reallocate_with_header(const ptr<header_t>& p, size_t n) const
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return try_reallocate(p, n + headerSize);
	}

	template <typename header_t, size_t align>
	bool try_reallocate_with_header(const ptr<header_t>& p, size_t n) const volatile
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return try_reallocate(p, n + headerSize);
	}

	template <typename header_t, typename type>
	bool try_reallocate_type_with_header(const ptr<header_t>& p, size_t n) const			{ return try_reallocate_with_header<header_t, std::alignment_of<type>::value>(p, n * sizeof(type)); }
	
	template <typename header_t, typename type>
	bool try_reallocate_type_with_header(const ptr<header_t>& p, size_t n) const volatile	{ return try_reallocate_with_header<header_t, std::alignment_of<type>::value>(p, n * sizeof(type)); }
	
	template <typename header_t, size_t align>
	size_t get_allocation_size_without_header(const ptr<header_t>& p, size_t knownSize) const
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return get_allocation_size(p, commonAlignment, knownSize + headerSize) - headerSize;
	}
	
	template <typename header_t, size_t align>
	size_t get_allocation_size_without_header(const ptr<header_t>& p, size_t knownSize) const volatile
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return get_allocation_size(p, commonAlignment, knownSize + headerSize) - headerSize;
	}
	
	template <typename header_t, typename type>
	size_t get_allocation_size_type_without_header(const ptr<header_t>& p, size_t knownSize) const
	{
		return get_allocation_size_without_header<header_t, std::alignment_of<type>::value>(p, knownSize * sizeof(type)) / sizeof(type);
	}

	template <typename header_t, typename type>
	size_t get_allocation_size_type_without_header(const ptr<header_t>& p, size_t knownSize) const volatile
	{
		return get_allocation_size_without_header<header_t, std::alignment_of<type>::value>(p, knownSize * sizeof(type)) / sizeof(type);
	}
	
	template <typename header_t, size_t align>
	static void* get_block_from_header(const ptr<const header_t>& p)
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return (void*)(((unsigned char*)p.get_ptr()) + headerSize);
	}

	template <typename header_t, typename type>
	static type* get_type_block_from_header(const ptr<const header_t>& p)	{ return (type*)get_block_from_header<header_t, std::alignment_of<type>::value>(p); }


	template <typename header_t, size_t align>
	static header_t* get_header_from_block(const ptr<void>& p)
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return reinterpret_cast<header_t*>(((unsigned char*)p.get_ptr()) - headerSize);
	}

	template <typename header_t, typename type>
	static header_t* get_header_from_type_block(const ptr<type>& p)	{ return get_header_from_block<header_t, std::alignment_of<type>::value>(p); }
};


template <class allocator_type>
class allocator_container<allocator_type, true, false>
{
public:
	typedef allocator_container<allocator_type, true, false>	this_t;

	static const bool is_static = true;

	typedef typename allocator_type::ref_t ref_t;

	allocator_container()								{ }
	allocator_container(const          this_t& src)	{ }
	allocator_container(const volatile this_t& src)	{ }
	allocator_container(volatile allocator_type&);				// not linkable

	         this_t& operator=(const          this_t& src)				{ return *this; }
	volatile this_t& operator=(const          this_t& src) volatile		{ return *this; }
	         this_t& operator=(const volatile this_t& src)				{ return *this; }

	volatile allocator_type* get_allocator() const volatile { return 0; }
	
	static ref_t allocate(size_t n, size_t align)								{ return allocator_type::allocate(n, align); }

	template <typename type>
	static typename ref_t::template cast_type<type>::type allocate_type(size_t n = 1)
	{ return allocate(sizeof(type) * n, std::alignment_of<type>::value).template reinterpret_cast_to<type>(); }

	static void deallocate(const ref_t& p)	{ allocator_type::deallocate(p); }

	template <typename type> static void deallocate(const typename ref_t::template cast_type<type>::type& p)	{ allocator_type::deallocate(p.template reinterpret_cast_to<void>()); }
	
	template <typename type>
	static void destruct_deallocate_type(const typename ref_t::template cast_type<type>::type& p, size_t n = 1)
	{
		if (!!p)
		{
			placement_destruct_multiple(p.get_ptr(), n);
			allocator_type::deallocate(p);
		}
	}

	template <typename type> static bool try_reallocate(const typename ref_t::template cast_type<type>::type& p, size_t n)	
	{ return allocator_type::try_reallocate(p.template reinterpret_cast_to<void>(), n);  }

	template <typename type> static bool try_reallocate_type(const typename ref_t::template cast_type<type>::type& p, size_t n)
	{ return allocator_type::try_reallocate(p.template reinterpret_cast_to<void>(), n * sizeof(type)); }

	template <typename type> static size_t get_allocation_size(const typename ref_t::template cast_type<type>::type& p, size_t align, size_t knownSize)
	{ return allocator_type::get_allocation_size(p.template reinterpret_cast_to<void>(), align, knownSize);  }

	template <typename type> static size_t get_allocation_size_type(const ptr<type>& p, size_t knownSize)	
	{ return allocator_type::get_allocation_size(p.template reinterpret_cast_to<void>(), std::alignment_of<type>::value, knownSize * sizeof(type)) / sizeof(type); }
};


template <class allocator_type>
class allocator_container<allocator_type, true, true>
{
public:
	typedef allocator_container<allocator_type, true, true>	this_t;

	static const bool is_static = true;

	typedef ptr<void> ref_t;

	allocator_container()							{ }
	allocator_container(const          this_t& src)	{ }
	allocator_container(const volatile this_t& src)	{ }
	allocator_container(volatile allocator_type&);				// not linkable

	         this_t& operator=(const          this_t& src)				{ return *this; }
	volatile this_t& operator=(const          this_t& src) volatile		{ return *this; }
	         this_t& operator=(const volatile this_t& src)				{ return *this; }

	ptr<volatile allocator_type> get_allocator() const volatile { return 0; }

	static ref_t allocate(size_t n, size_t align)								{ return allocator_type::allocate(n, align); }

	template <typename type>
	static type* allocate_type(size_t n = 1)	{ return allocate(sizeof(type) * n, std::alignment_of<type>::value).template reinterpret_cast_to<type>().get_ptr(); }

	static void deallocate(const ref_t& p)	{ allocator_type::deallocate(p); }

	template <typename type> static void deallocate(const ptr<type>& p)	{ allocator_type::deallocate(p.template reinterpret_cast_to<void>()); }

	template <typename type>
	static void destruct_deallocate_type(const ptr<type>& p, size_t n = 1)
	{
		if (!!p)
		{
			placement_destruct_multiple(p.get_ptr(), n);
			allocator_type::deallocate(p);
		}
	}

	template <typename type> static bool try_reallocate(const ptr<type>& p, size_t n)	
	{ return allocator_type::try_reallocate(p.template reinterpret_cast_to<void>(), n);  }

	template <typename type> static bool try_reallocate_type(const ptr<type>& p, size_t n)
	{ return allocator_type::try_reallocate(p.template reinterpret_cast_to<void>(), n * sizeof(type)); }

	template <typename type> static size_t get_allocation_size(const ptr<type>& p, size_t align, size_t knownSize)
	{ return allocator_type::get_allocation_size(p.template reinterpret_cast_to<void>(), align, knownSize);  }

	template <typename type> static size_t get_allocation_size_type(const ptr<type>& p, size_t knownSize)	
	{ return allocator_type::get_allocation_size(p.template reinterpret_cast_to<void>(), std::alignment_of<type>::value, knownSize * sizeof(type)) / sizeof(type); }

	// helpers with raw pointers

	static void deallocate(void* p)			{ allocator_type::deallocate(p); }

	template <typename type> static void deallocate(type* p)			{ allocator_type::deallocate(ptr<type>(p).template reinterpret_cast_to<void>()); }

	template <typename type>
	static void destruct_deallocate_type(type* p, size_t n = 1)
	{
		if (!!p)
		{
			placement_destruct_multiple(p, n);
			allocator_type::deallocate(p);
		}
	}

	template <typename type> static bool try_reallocate(type* p, size_t n)	
	{ return allocator_type::try_reallocate(ptr<type>(p).template reinterpret_cast_to<void>(), n);  }

	template <typename type> static bool try_reallocate_type(type* p, size_t n)
	{ return allocator_type::try_reallocate(ptr<type>(p).template reinterpret_cast_to<void>(), n * sizeof(type)); }

	template <typename type> static size_t get_allocation_size(type* p, size_t align, size_t knownSize)
	{ return allocator_type::get_allocation_size(ptr<type>(p).template reinterpret_cast_to<void>(), align, knownSize);  }

	template <typename type> static size_t get_allocation_size_type(type* p, size_t knownSize)	
	{ return allocator_type::get_allocation_size(p.template reinterpret_cast_to<void>(), std::alignment_of<type>::value, knownSize * sizeof(type)) / sizeof(type); }

	template <typename header_t, size_t align>
	static ptr<header_t> allocate_with_header(size_t n)
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return allocate(n + headerSize, commonAlignment).template reinterpret_cast_to<header_t>();
	}

	template <typename header_t, typename type>
	static ptr<header_t> allocate_type_with_header(size_t n = 1)		{ return allocate_with_header<header_t, std::alignment_of<type>::value>(n * sizeof(type)); }

	template <typename header_t, size_t align>
	static bool try_reallocate_with_header(const ptr<header_t>& p, size_t n)
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return try_reallocate(p, n + headerSize);
	}

	template <typename header_t, typename type>
	static bool try_reallocate_type_with_header(const ptr<header_t>& p, size_t n)	{ return try_reallocate_with_header<header_t, std::alignment_of<type>::value>(p, n * sizeof(type)); }
	
	template <typename header_t, size_t align>
	static size_t get_allocation_size_without_header(const ptr<header_t>& p, size_t knownSize)
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return get_allocation_size(p, commonAlignment, knownSize + headerSize) - headerSize;
	}

	template <typename header_t, typename type>
	static size_t get_allocation_size_type_without_header(const ptr<header_t>& p, size_t knownSize)
	{
		return get_allocation_size_without_header<header_t, std::alignment_of<type>::value>(p, knownSize * sizeof(type)) / sizeof(type);
	}
	
	template <typename header_t, size_t align>
	static void* get_block_from_header(const ptr<const header_t>& p)
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return (void*)(((unsigned char*)p.get_ptr()) + headerSize);
	}

	template <typename header_t, typename type>
	static type* get_type_block_from_header(const ptr<const header_t>& p)	{ return (type*)get_block_from_header<header_t, std::alignment_of<type>::value>(p); }


	template <typename header_t, size_t align>
	static header_t* get_header_from_block(const ptr<void>& p)
	{
		static const size_t commonAlignment = const_lcm<std::alignment_of<header_t>::value, align>::value;
		static const size_t headerSize = least_multiple_of<sizeof(header_t), commonAlignment>::value;	// header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		return reinterpret_cast<header_t*>(((unsigned char*)p.get_ptr()) - headerSize);
	}

	template <typename header_t, typename type>
	static header_t* get_header_from_type_block(const ptr<type>& p)	{ return get_header_from_block<header_t, std::alignment_of<type>::value>(p); }
};


#pragma warning(pop)


}


#endif

