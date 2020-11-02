//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good

#ifndef COGS_HEADER_MEM_PLACEMENT
#define COGS_HEADER_MEM_PLACEMENT

#include <type_traits>
#include <cstring>

#include "cogs/env/mem/alignment.hpp"
#include "cogs/mem/ptr.hpp"


namespace cogs {


// placement is a non-instantiable template class provinging placement construction, destruction, and
// moving of objects or data in memory.
//
// If is_pod, objects will not be constructed or destruct.  memcpy() is be used,
// as the compiler may directly optimize these functions.
//
// If is_not_pod, objects will be constructed, destructed, copy-constructed, etc.
//
// placement<> may be used as a type for a block of memory of the proper size and alignment to
// construct the specified type.


template <typename T, typename... args_t>
inline std::enable_if_t<
	std::is_constructible_v<T, args_t...>,
	void
>
placement_construct(T* t, args_t&&... args)
{
	new (t) T(std::forward<args_t>(args)...);
}

template <typename T>
inline std::enable_if_t<
	std::is_constructible_v<T>,
	void
>
placement_construct(T* t)
{
	new (t) T;
}

template <typename T>
inline std::enable_if_t<
	std::is_constructible_v<T>,
	void
>
placement_construct_init(T* t)
{
	new (t) T();
}

// Work around the fact that placement operator new for size_t is disallowed:
//		The placement form void* operator new(std::size_t, std::size_t) is not allowed because
//		the matching signature of the deallocation function, void operator delete(void*, std::size_t),
//		is a usual(not placement) deallocation function.
inline void placement_construct(size_t* t, size_t n)
{
	*t = n;
}

inline void placement_construct(size_t*) { }
inline void placement_construct_init(size_t*) { }


/// @ingroup Mem
/// @brief A helper class that facilitates placement storage
/// @tparam n Size of type
/// @tparam alignment Alignment of type
template <size_t n, size_t alignment>
class alignas(alignment) placement_storage
{
public:
	unsigned char m_bytes[n];

	template <typename T> T& get() { return *reinterpret_cast<T*>(this); }
	template <typename T> const T& get() const { return *reinterpret_cast<const T*>(this); }
	template <typename T> volatile T& get() volatile { return *reinterpret_cast<volatile T*>(this); }
	template <typename T> const volatile T& get() const volatile { return *reinterpret_cast<const volatile T*>(this); }
};


template <typename T, size_t alignment = alignof(T)>
struct alignas(alignment) placement : public placement_storage<sizeof(T), alignment>
{
public:
	T& get() { return *reinterpret_cast<T*>(this); }
	const T& get() const { return *reinterpret_cast<const T*>(this); }
	volatile T& get() volatile { return *reinterpret_cast<volatile T*>(this); }
	const volatile T& get() const volatile { return *reinterpret_cast<const volatile T*>(this); }

	T& operator*() { return get(); }
	const T& operator*() const { return get(); }
	volatile T& operator*() volatile { return get(); }
	const volatile T& operator*() const volatile { return get(); }

	T* operator->() { return &get(); }
	const T* operator->() const { return &get(); }
	volatile T* operator->() volatile { return &get(); }
	const volatile T* operator->() const volatile { return &get(); }

	template <typename... args_t>
	std::enable_if_t<
		std::is_constructible_v<T, args_t...>,
		void
	>
	construct(args_t&&... args)
	{
		placement_construct(&get(), std::forward<args_t>(args)...); // placement new
	}

	template <typename... args_t>
	std::enable_if_t<
		std::is_constructible_v<T, args_t...>,
		void
	>
	construct_init(args_t&&... args)
	{
		placement_construct_init(get(), std::forward<args_t>(args)...); // placement new
	}

	void destruct() { get().~T(); }
	void destruct() const { get().~T(); }
	void destruct() volatile { get().~T(); }
	void destruct() const volatile { get().~T(); }
};


template <typename T, typename... args_t>
inline std::enable_if_t<
	std::is_constructible_v<T, args_t&...>,
	void
>
placement_construct_multiple(T* t, size_t n, args_t&... args)
{
	for (size_t i = 0; i < n; i++)
		new (&t[i]) T(args...);
}

template <typename T>
inline std::enable_if_t<
	std::is_constructible_v<T>,
	void
>
placement_construct_multiple(T* t, size_t n)
{
	for (size_t i = 0; i < n; i++)
		new (&t[i]) T;
}

template <typename T>
inline std::enable_if_t<
	std::is_constructible_v<T>,
	void
>
placement_construct_init_multiple(T* t, size_t n)
{
	for (size_t i = 0; i < n; i++)
		new (&t[i]) T();
}


// buffers must not overlap.  If overlaping objects, use placement_move()

template <typename T, typename T2>
inline std::enable_if_t<
	std::is_constructible_v<T, T2&>,
	void
>
placement_copy_construct_array(T* t, T2* src, size_t n)
{
	for (size_t i = 0; i < n; i++)
		new (t + i) T(src[i]);
}

template <typename T, typename T2>
inline std::enable_if_t<
	std::is_constructible_v<T, T2&>,
	void
>
placement_move_construct_array(T* t, T2* src, size_t n)
{
	for (size_t i = 0; i < n; i++)
		new (t + i) T(std::move(src[i]));
}


// buffers must not overlap.  If overlaping objects, use placement_move()
template <typename T>
inline std::enable_if_t<
	std::is_trivially_copy_constructible_v<T>,
	void
>
placement_copy_construct_array(T* t, const T* src, size_t n)
{
	memcpy(t, src, n * sizeof(T));
}

template <typename T>
inline std::enable_if_t<
	std::is_trivially_copy_constructible_v<T>,
	void
>
placement_move_construct_array(T* t, const T* src, size_t n)
{
	memcpy(t, src, n * sizeof(T));
}


template <typename T>
inline std::enable_if_t<
	std::is_destructible_v<T>,
	void
>
placement_destruct(T* t)
{
	t->T::~T();
}


template <typename T>
inline std::enable_if_t<
	std::is_destructible_v<T>
	&& !std::is_trivially_destructible_v<T>,
	void
>
placement_destruct_multiple(T* t, size_t n)
{
	for (size_t i = 0; i < n; i++)
		placement_destruct(t + i);
}

template <typename T>
inline std::enable_if_t<
	std::is_destructible_v<T>
	&& std::is_trivially_destructible_v<T>,
	void
>
placement_destruct_multiple(T*, size_t)
{
}


template <typename T, typename... args_t>
inline std::enable_if_t<
	std::is_destructible_v<T>
	&& std::is_constructible_v<T, args_t...>,
	void
>
placement_reconstruct(T* t, args_t&&... args)
{
	placement_destruct(t);
	placement_construct(t, std::forward<args_t>(args)...);
}

template <typename T, typename... args_t>
inline std::enable_if_t<
	std::is_destructible_v<T>
	&& std::is_constructible_v<T, args_t...>,
	void
>
placement_reconstruct_init(T* t, args_t&&... args)
{
	placement_destruct(t);
	placement_construct_init(t, std::forward<args_t>(args)...);
}


template <typename T, typename... args_t>
inline std::enable_if_t<
	std::is_destructible_v<T>
	&& std::is_constructible_v<T, args_t...>,
	void
>
placement_reconstruct_multiple(T* t, size_t n, args_t&&... args)
{
	for (size_t i = 0; i < n; i++)
	{
		T* t2 = t + i;
		placement_destruct(t2);
		placement_construct(t2, std::forward<args_t>(args)...);
	}
}

template <typename T, typename... args_t>
inline std::enable_if_t<
	std::is_destructible_v<T>
	&& std::is_constructible_v<T, args_t...>,
	void
>
placement_reconstruct_init_multiple(T* t, size_t n, args_t&&... args)
{
	for (size_t i = 0; i < n; i++)
	{
		T* t2 = t + i;
		placement_destruct(t2);
		placement_construct_init(t2, std::forward<args_t>(args)...);
	}
}


// buffers must not overlap.  If overlaping objects, use placement_move()
template <typename T, typename T2>
inline std::enable_if_t<
	std::is_destructible_v<T>
	&& std::is_constructible_v<T, T2&>,
	void
>
placement_copy_reconstruct_array(T* t, T2* src, size_t n)
{
	// 2 pass, but may be preferable due to resources freed first
	placement_destruct_multiple(t, n);
	placement_copy_construct_array(t, src, n);
}


// placement_move() will move n elements from src to dst, destruct vacated elements and construct new elements.
// Intended to be used at the start or end of an array, if elements need to be pushed backwards or forwards due to an insert.
// When moving forward, constructed elements are assumed to end at src+n.
// When moving backwards, elements before src are considered non-constructed.
template <typename T>
inline std::enable_if_t<
	!std::is_trivially_destructible_v<T>
	|| !std::is_trivially_copy_constructible_v<T>,
	void
>
placement_move(T* dst, T* src, size_t n)
{
	if (!!n)
	{
		if (dst < src) // move back
		{
			size_t gap = src - dst; // unconstructed portion
			if (gap >= n) // Scenario 1: dst < dst+n <= src < src+n
			{
				placement_copy_construct_array(dst, src, n);
				placement_destruct_multiple(src, n);
			}
			else // (gap < n) // Scenario 2: dst < src < dst+n < src+n
			{
				placement_copy_construct_array(dst, src, gap);
				size_t i = gap;
				T* t2 = 0; // gcc things this needs initializing.  It doesn't because i == gap, and gap is < n, so the loop will run.
				for (; i < n; i++)
				{
					t2 = dst + i;
					placement_destruct(t2);
					placement_construct(t2, src[i]);
				}
				placement_destruct_multiple(t2, gap); // t2 == dst + n
			}
		}
		else if (dst > src) // move forward
		{
			T* pastSrc = src + n;
			if (dst >= pastSrc) // Scenario 1: src < src+n <= dst < dst+n
			{
				placement_copy_construct_array(dst, src, n);
				placement_destruct_multiple(src, n);
			}
			else //  (dst < pastSrc) // Scenario 2: src < dst < src+n < dst+n
			{
				size_t outer = dst - src;
				placement_copy_construct_array(pastSrc, pastSrc - outer, outer);
				for (size_t i = n - outer; i > 0;)
				{
					--i;
					T* t2 = dst + i;
					placement_destruct(t2);
					placement_construct(t2, src[i]);
				}
				placement_destruct_multiple(src, outer);
			}
		}
	}
}


template <typename T>
inline std::enable_if_t<
	std::is_trivially_destructible_v<T>
	&& std::is_trivially_copy_constructible_v<T>,
	void
>
placement_move(T* dst, T* src, size_t n)
{
	std::memmove(dst, src, n * sizeof(T));
}


}


#endif
