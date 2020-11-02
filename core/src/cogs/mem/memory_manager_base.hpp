//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_MEMORY_MANAGER_BASE
#define COGS_HEADER_MEM_MEMORY_MANAGER_BASE


#include "cogs/env.hpp"
#include "cogs/math/least_multiple_of.hpp"
#include "cogs/math/const_lcm.hpp"
#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/memory_manager_base.hpp"
#include "cogs/mem/placement.hpp"


namespace cogs {

template <class memory_manager_t, bool is_static = true>
class memory_manager_base
{
};

template <class memory_manager_t>
class memory_manager_base<memory_manager_t, true>
{
public:
	static constexpr bool is_static = true;

	void shutdown() { }

	template <typename type>
	static type* allocate_type(size_t n = 1, size_t* usableCount = nullptr)
	{
		static_assert(sizeof(type) % alignof(type) == 0);
		type* result = reinterpret_cast<type*>(memory_manager_t::allocate(sizeof(type) * n, alignof(type), usableCount));
		if (usableCount)
			*usableCount /= sizeof(type);
		return result;
	}

	template <typename type>
	static void destruct_deallocate_type(type* p, size_t n = 1)
	{
		if (!!p)
		{
			placement_destruct_multiple(p, n);
			memory_manager_t::deallocate((void*)p);
		}
	}

	template <typename type>
	static bool try_reallocate_type(type* p, size_t n, size_t* usableCount = nullptr)
	{
		bool b = memory_manager_t::try_reallocate((void*)p, n * sizeof(type));
		if (usableCount && b)
			*usableCount -= *usableCount % sizeof(type);
		return b;
	}
	
	template <typename header_t, size_t align>
	static header_t* allocate_with_header(size_t n, size_t* usableSize = nullptr)
	{
		static constexpr size_t commonAlignment = const_lcm_v<alignof(header_t), align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>; // header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		header_t* result = (header_t*)memory_manager_t::allocate(n + headerSize, commonAlignment, usableSize);
		if (usableSize)
			*usableSize -= headerSize;
		return result;
	}

	template <typename header_t, typename type>
	static header_t* allocate_type_with_header(size_t n = 1, size_t* usableCount = nullptr)
	{
		header_t* result = allocate_with_header<header_t, alignof(type)>(n * sizeof(type), usableCount);
		if (usableCount)
			*usableCount /= sizeof(type);
		return result;
	}
	
	template <typename header_t, size_t align>
	static bool try_reallocate_with_header(header_t* p, size_t n, size_t* usableSize = nullptr)
	{
		static constexpr size_t commonAlignment = const_lcm_v<alignof(header_t), align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>; // header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		bool b = memory_manager_t::try_reallocate((void*)p, n + headerSize, commonAlignment, usableSize);
		if (usableSize && b)
			*usableSize -= headerSize;
		return b;
	}

	template <typename header_t, typename type>
	static bool try_reallocate_type_with_header(header_t* p, size_t n, size_t* usableCount = nullptr)
	{
		bool b = try_reallocate_with_header<header_t, alignof(type)>(p, n * sizeof(type), usableCount);
		if (usableCount && b)
			*usableCount /= sizeof(type);
		return b;
	}
};


template <class memory_manager_t>
class memory_manager_base<memory_manager_t, false>
{
public:
	static constexpr bool is_static = false;

	void shutdown() const volatile { }

	template <typename type>
	type* allocate_type(size_t n = 1, size_t* usableCount = nullptr)
	{
		static_assert(sizeof(type) % alignof(type) == 0);
		type* result = reinterpret_cast<type*>(static_cast<memory_manager_t*>(this)->allocate(sizeof(type) * n, alignof(type), usableCount));
		if (usableCount)
			*usableCount /= sizeof(type);
		return result;
	}

	template <typename type>
	type* allocate_type(size_t n = 1, size_t* usableCount = nullptr) const
	{
		static_assert(sizeof(type) % alignof(type) == 0);
		type* result = reinterpret_cast<type*>(static_cast<memory_manager_t*>(this)->allocate(sizeof(type) * n, alignof(type), usableCount));
		if (usableCount)
			*usableCount /= sizeof(type);
		return result;
	}

	template <typename type>
	type* allocate_type(size_t n = 1, size_t* usableCount = nullptr) volatile
	{
		static_assert(sizeof(type) % alignof(type) == 0);
		type* result = reinterpret_cast<type*>(static_cast<memory_manager_t*>(this)->allocate(sizeof(type) * n, alignof(type), usableCount));
		if (usableCount)
			*usableCount /= sizeof(type);
		return result;
	}

	template <typename type>
	type* allocate_type(size_t n = 1, size_t* usableCount = nullptr) const volatile
	{
		static_assert(sizeof(type) % alignof(type) == 0);
		type* result = reinterpret_cast<type*>(static_cast<memory_manager_t*>(this)->allocate(sizeof(type) * n, alignof(type), usableCount));
		if (usableCount)
			*usableCount /= sizeof(type);
		return result;
	}

	template <typename type>
	void destruct_deallocate_type(type* p, size_t n = 1)
	{
		if (!!p)
		{
			placement_destruct_multiple(p, n);
			static_cast<memory_manager_t*>(this)->deallocate((void*)p);
		}
	}

	template <typename type>
	void destruct_deallocate_type(type* p, size_t n = 1) const
	{
		if (!!p)
		{
			placement_destruct_multiple(p, n);
			static_cast<memory_manager_t*>(this)->deallocate((void*)p);
		}
	}

	template <typename type>
	void destruct_deallocate_type(type* p, size_t n = 1) volatile
	{
		if (!!p)
		{
			placement_destruct_multiple(p, n);
			static_cast<memory_manager_t*>(this)->deallocate((void*)p);
		}
	}

	template <typename type>
	void destruct_deallocate_type(type* p, size_t n = 1) const volatile 
	{
		if (!!p)
		{
			placement_destruct_multiple(p, n);
			static_cast<memory_manager_t*>(this)->deallocate((void*)p);
		}
	}

	template <typename type>
	bool try_reallocate_type(type* p, size_t n, size_t* usableCount = nullptr)
	{
		bool b = static_cast<memory_manager_t*>(this)->try_reallocate((void*)p, n * sizeof(type));
		if (usableCount && b)
			*usableCount -= *usableCount % sizeof(type);
		return b;
	}


	template <typename type>
	bool try_reallocate_type(type* p, size_t n, size_t* usableCount = nullptr) const
	{
		bool b = static_cast<memory_manager_t*>(this)->try_reallocate((void*)p, n * sizeof(type));
		if (usableCount && b)
			*usableCount -= *usableCount % sizeof(type);
		return b;
	}


	template <typename type>
	bool try_reallocate_type(type* p, size_t n, size_t* usableCount = nullptr) volatile
	{
		bool b = static_cast<memory_manager_t*>(this)->try_reallocate((void*)p, n * sizeof(type));
		if (usableCount && b)
			*usableCount -= *usableCount % sizeof(type);
		return b;
	}


	template <typename type>
	bool try_reallocate_type(type* p, size_t n, size_t* usableCount = nullptr) const volatile
	{
		bool b = static_cast<memory_manager_t*>(this)->try_reallocate((void*)p, n * sizeof(type));
		if (usableCount && b)
			*usableCount -= *usableCount % sizeof(type);
		return b;
	}

	template <typename header_t, size_t align>
	header_t* allocate_with_header(size_t n, size_t* usableSize = nullptr)
	{
		static constexpr size_t commonAlignment = const_lcm_v<alignof(header_t), align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>; // header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		header_t* result = static_cast<memory_manager_t*>(this)->allocate(n + headerSize, commonAlignment, usableSize).template reinterpret_cast_to<header_t>();
		if (usableSize)
			*usableSize -= headerSize;
		return result;
	}

	template <typename header_t, size_t align>
	header_t* allocate_with_header(size_t n, size_t* usableSize = nullptr) const
	{
		static constexpr size_t commonAlignment = const_lcm_v<alignof(header_t), align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>; // header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		header_t* result = static_cast<memory_manager_t*>(this)->allocate(n + headerSize, commonAlignment, usableSize).template reinterpret_cast_to<header_t>();
		if (usableSize)
			*usableSize -= headerSize;
		return result;
	}

	template <typename header_t, size_t align>
	header_t* allocate_with_header(size_t n, size_t* usableSize = nullptr) volatile
	{
		static constexpr size_t commonAlignment = const_lcm_v<alignof(header_t), align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>; // header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		header_t* result = static_cast<memory_manager_t*>(this)->allocate(n + headerSize, commonAlignment, usableSize).template reinterpret_cast_to<header_t>();
		if (usableSize)
			*usableSize -= headerSize;
		return result;
	}

	template <typename header_t, size_t align>
	header_t* allocate_with_header(size_t n, size_t* usableSize = nullptr) const volatile
	{
		static constexpr size_t commonAlignment = const_lcm_v<alignof(header_t), align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>; // header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		header_t* result = static_cast<memory_manager_t*>(this)->allocate(n + headerSize, commonAlignment, usableSize).template reinterpret_cast_to<header_t>();
		if (usableSize)
			*usableSize -= headerSize;
		return result;
	}

	template <typename header_t, typename type>
	header_t* allocate_type_with_header(size_t n = 1, size_t* usableCount = nullptr)
	{
		header_t* result = allocate_with_header<header_t, alignof(type)>(n * sizeof(type), usableCount);
		if (usableCount)
			*usableCount /= sizeof(type);
		return result;
	}

	template <typename header_t, typename type>
	header_t* allocate_type_with_header(size_t n = 1, size_t* usableCount = nullptr) const
	{
		header_t* result = allocate_with_header<header_t, alignof(type)>(n * sizeof(type), usableCount);
		if (usableCount)
			*usableCount /= sizeof(type);
		return result;
	}

	template <typename header_t, typename type>
	header_t* allocate_type_with_header(size_t n = 1, size_t* usableCount = nullptr) volatile
	{
		header_t* result = allocate_with_header<header_t, alignof(type)>(n * sizeof(type), usableCount);
		if (usableCount)
			*usableCount /= sizeof(type);
		return result;
	}

	template <typename header_t, typename type>
	header_t* allocate_type_with_header(size_t n = 1, size_t* usableCount = nullptr) const volatile
	{
		header_t* result = allocate_with_header<header_t, alignof(type)>(n * sizeof(type), usableCount);
		if (usableCount)
			*usableCount /= sizeof(type);
		return result;
	}

	template <typename header_t, size_t align>
	bool try_reallocate_with_header(header_t* p, size_t n, size_t* usableSize = nullptr)
	{
		static constexpr size_t commonAlignment = const_lcm_v<alignof(header_t), align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>; // header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		bool b = static_cast<memory_manager_t*>(this)->try_reallocate((void*)p, n + headerSize, usableSize);
		if (usableSize && b)
			*usableSize -= headerSize;
		return b;
	}

	template <typename header_t, size_t align>
	bool try_reallocate_with_header(header_t* p, size_t n, size_t* usableSize = nullptr) const
	{
		static constexpr size_t commonAlignment = const_lcm_v<alignof(header_t), align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>; // header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		bool b = static_cast<memory_manager_t*>(this)->try_reallocate((void*)p, n + headerSize, usableSize);
		if (usableSize && b)
			*usableSize -= headerSize;
		return b;
	}

	template <typename header_t, size_t align>
	bool try_reallocate_with_header(header_t* p, size_t n, size_t* usableSize = nullptr) volatile
	{
		static constexpr size_t commonAlignment = const_lcm_v<alignof(header_t), align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>; // header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		bool b = static_cast<memory_manager_t*>(this)->try_reallocate((void*)p, n + headerSize, usableSize);
		if (usableSize && b)
			*usableSize -= headerSize;
		return b;
	}

	template <typename header_t, size_t align>
	bool try_reallocate_with_header(header_t* p, size_t n, size_t* usableSize = nullptr) const volatile
	{
		static constexpr size_t commonAlignment = const_lcm_v<alignof(header_t), align>;
		static constexpr size_t headerSize = least_multiple_of_v<sizeof(header_t), commonAlignment>; // header_t must be padded out to next multiple of commonAlignment that is greater than or equal to sizeof(header_t)
		bool b = static_cast<memory_manager_t*>(this)->try_reallocate((void*)p, n + headerSize, usableSize);
		if (usableSize && b)
			*usableSize -= headerSize;
		return b;
	}

	template <typename header_t, typename type>
	bool try_reallocate_type_with_header(header_t* p, size_t n, size_t* usableCount = nullptr)
	{
		bool b = try_reallocate_with_header<header_t, alignof(type) >(p, n * sizeof(type), usableCount);
		if (usableCount && b)
			*usableCount /= sizeof(type);
		return b;
	}

	template <typename header_t, typename type>
	bool try_reallocate_type_with_header(header_t* p, size_t n, size_t* usableCount = nullptr) const
	{
		bool b = try_reallocate_with_header<header_t, alignof(type) >(p, n * sizeof(type), usableCount);
		if (usableCount && b)
			*usableCount /= sizeof(type);
		return b;
	}

	template <typename header_t, typename type>
	bool try_reallocate_type_with_header(header_t* p, size_t n, size_t* usableCount = nullptr) volatile
	{
		bool b = try_reallocate_with_header<header_t, alignof(type) >(p, n * sizeof(type), usableCount);
		if (usableCount && b)
			*usableCount /= sizeof(type);
		return b;
	}

	template <typename header_t, typename type>
	bool try_reallocate_type_with_header(header_t* p, size_t n, size_t* usableCount = nullptr) const volatile
	{
		bool b = try_reallocate_with_header<header_t, alignof(type) >(p, n * sizeof(type), usableCount);
		if (usableCount && b)
			*usableCount /= sizeof(type);
		return b;
	}
};


}


#endif
