//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

// Status: Good

#ifndef COGS_HEADER_FUNCTION
#define COGS_HEADER_FUNCTION

#include "cogs/env.hpp"
#include "cogs/debug.hpp"
#include "cogs/mem/default_memory_manager.hpp"
#include "cogs/mem/ptr.hpp"
#include "cogs/sync/hazard.hpp"

namespace cogs {

// Only really need my own function class because std::function doesn't allow use of an allocator,
// and I want to use my lock-free allocator.  Also, being able to encapsulate a callable that accepts fewer
// args than specified, is useful.

//template <typename signature, size_t n = sizeof(void*)*5> // Enough space for 2x rcptr's, and a vtable ptr
//class function;

template <typename T>
struct is_function_class : public std::false_type { };

template <typename T>
struct is_function_class<const T> : public is_function_class<T> { };

template <typename T>
struct is_function_class<volatile T> : public is_function_class<T> { };

template <typename T>
struct is_function_class<const volatile T> : public is_function_class<T> { };

template <typename T>
constexpr bool is_function_class_v = is_function_class<T>::value;

template <typename signature, size_t n>
struct is_function_class<function<signature, n> > : public std::true_type { };

template <typename T, typename signature>
struct is_same_function_class : public std::false_type { };

template <typename T, typename signature>
struct is_same_function_class<const T, signature> : public is_same_function_class<T, signature> { };

template <typename T, typename signature>
struct is_same_function_class<volatile T, signature> : public is_same_function_class<T, signature> { };

template <typename T, typename signature>
struct is_same_function_class<const volatile T, signature> : public is_same_function_class<T, signature> { };

template <typename T, typename signature>
constexpr bool is_same_function_class_v = is_same_function_class<T, signature>::value;

template <typename signature, size_t n>
struct is_same_function_class<function<signature, n>, signature> : public std::true_type { };


template <typename return_t, typename... args_t, size_t n>
class function<return_t(args_t...), n>
{
private:
	typedef function<return_t(args_t...), n> this_t;

	class block_base
	{
	public:
		block_base()
		{ }

		virtual ~block_base() { }
		virtual block_base* copy() const = 0;
		virtual void copy(void* p) const = 0;
		virtual block_base* move() = 0;
		virtual void move(void* p) = 0;

		virtual return_t invoke(args_t...) const = 0;
	};

	// reverse1 starts reversal by accumulating the first element
	// reverse1 also supports 0 arg invoke if only 1 arg is present, ensuring at least 2 args are passed to reverse2/accumulate.
	// reverse2/accumulate completes the reverse the of remaining args
	// reverse3 removes the first (originally the last) arg
	// reverse4/accumulate reverses the remaining args again, provides invoke function, then passes args back around to reverse1

	template <typename... A>
	class reverse3;

	template <typename... A>
	class reverse1 // 0 arg version
	{
	public:
		static void invoke(); // Here to satisfy using.  Failure to match args would manifest as an error here.
	};

	template <typename... A>
	class reverse2 // 0 args left.
	{
	public:
		// Done reversing the args.
		// We know there are at least 2.
		// Now, let's remove the first one...
		// Then reverse them again to call the function!
		template <typename... A2>
		class accumulate : public reverse3<A2...>
		{
		public:
			using reverse3<A2...>::invoke;
		};
	};

	// THIRD
	template <typename T, typename... A>
	class reverse2<T, A...>
	{
	public:
		template <typename... A2>
		class accumulate : public reverse2<A...>::template accumulate<T, A2...>
		{
		public:
			using reverse2<A...>::template accumulate<T, A2...>::invoke;
		};
	};

	// SECOND
	template <typename T, typename... A>
	class reverse1<T, A...> : public reverse2<A...>::template accumulate<T>
	{
	public:
		using reverse2<A...>::template accumulate<T>::invoke;
	};

	// FIRST
	template <typename... A>
	class reverse : public reverse1<A...>
	{
	public:
		using reverse1<A...>::invoke;

		template <typename F, typename enable = std::enable_if_t<std::is_invocable_r_v<return_t, F, A...> > >
		static return_t invoke(F&& f, A... a)
		{
			return f(std::forward<A>(a)...);
		}
	};

	template <typename... A>
	class reverse4 // 0 args left.
	{
	public:
		// Done reversing the args.  Provide the new version of invoke
		template <typename... A2>
		class accumulate : public reverse1<A2...>
		{
		public:
			using reverse1<A2...>::invoke;

			template <typename F, typename enable = std::enable_if_t<std::is_invocable_r_v<return_t, F, A2...> >, typename... A3>
			static return_t invoke(F&& f, A2... a2, A3&&...)
			{
				return f(std::forward<A2>(a2)...);
			}
		};
	};

	template <typename T, typename... A>
	class reverse4<T, A...>
	{
	public:
		template <typename... A2>
		class accumulate : public reverse4<A...>::template accumulate<T, A2...>
		{
		public:
			using reverse4<A...>::template accumulate<T, A2...>::invoke;
		};
	};

	template <typename... A>
	class reverse3 // not used
	{
	public:
		static void invoke(); // not called, just here to satisfy using
	};

	template <typename T, typename... A>
	class reverse3<T, A...> : public reverse4<A...>::template accumulate<>
	{
	public:
		using reverse4<A...>::template accumulate<>::invoke;
	};

	template <typename F>
	class block : public block_base
	{
	public:
		F m_func;

		template <typename F2>
		block(F2&& f)
			: m_func(std::forward<F2>(f))
		{ }

		template <typename F2>
		block(block<F2>&& f)
			: m_func(std::move(f.m_func))
		{ }

		template <typename F2>
		block(const block<F2>& f)
			: m_func(f.m_func)
		{ }

		block(F&& f)
			: m_func(std::move(f))
		{ }

		block(const F& f)
			: m_func(f)
		{ }

		template <size_t n2> block(block<function<return_t(args_t...), n2> >&& f) = delete;
		template <size_t n2> block(const block<function<return_t(args_t...), n2> >& f) = delete;

		block(block<function<return_t(args_t...), n> >&& f) = delete; //?
		block(const block<function<return_t(args_t...), n> >& f) = delete;

		virtual ~block() { }

		virtual block_base* copy() const
		{
			block<F>* blk = default_memory_manager::allocate_type<block<F> >();
			return new (blk) block<F>(m_func);
		}

		virtual void copy(void* p) const
		{
			new ((block<F>*)p) block<F>(m_func);
		}

		virtual block_base* move()
		{
			block<F>* blk = default_memory_manager::allocate_type<block<F> >();
			return new (blk) block<F>(std::move(m_func));
		}

		virtual void move(void* p)
		{
			new ((block<F>*)p) block<F>(std::move(m_func));
		}

		virtual return_t invoke(args_t... a) const
		{
			return reverse<args_t...>::invoke(m_func, std::forward<args_t>(a)...);
		}
	};

	unsigned char m_buffer[n];
	size_t m_size; // 0 = empty, <=n embedded, >n allocated

	void release_inner()
	{
		if (m_size > 0)
		{
			if (m_size <= n)
				((block_base*)&m_buffer)->~block_base();
			else
				default_memory_manager::destruct_deallocate_type(*(block_base**)&m_buffer);
		}
	}

	template <typename F>
	void set_func(F&& f)
	{
		typedef std::remove_reference_t<F> F_t;
		const size_t blockSize = sizeof(block<F_t>);
		m_size = blockSize;
		if constexpr (blockSize <= n)
			new ((block<F_t>*)&m_buffer) block<F_t>(std::forward<F>(f));
		else
		{
			block<F_t>* blk = default_memory_manager::allocate_type<block<F_t> >();
			*(block<F_t>**)& m_buffer = new (blk) block<F_t>(std::forward<F>(f));
		}
	}

	void move(this_t&& src)
	{
		m_size = src.m_size;
		if (src.m_size > 0)
		{
			if (src.m_size <= n)
				((block_base*)&src.m_buffer)->move(&m_buffer);
			else
			{
				*(block_base**)&m_buffer = *(block_base**)&src.m_buffer;
				src.m_size = 0;
			}
		}
	}

	void set(const this_t& src)
	{
		m_size = src.m_size;
		if (src.m_size > 0)
		{
			if (src.m_size <= n)
				((block_base*)&src.m_buffer)->copy(&m_buffer);
			else
				*(block_base**)&m_buffer = (*(block_base**)&src.m_buffer)->copy();
		}
	}

	template <size_t n2>
	void move(function<return_t(args_t...), n2>&& src)
	{
		m_size = src.m_size;
		if (src.m_size > 0)
		{
			if (src.m_size <= n)
			{
				if (src.m_size <= n2)
					((block_base*)&src.m_buffer)->move(&m_buffer); // embedded to embedded
				else
					(*(block_base**)&src.m_buffer)->move(&m_buffer); // allocated to embedded
			}
			else
			{
				if (src.m_size <= n2)
					*(block_base**)&m_buffer = ((block_base*)&src.m_buffer)->move(); // embedded to allocated
				else
				{
					*(block_base**)&m_buffer = *(block_base**)&src.m_buffer; // allocated to allocated
					src.m_size = 0;
				}
			}
		}
	}

	template <size_t n2>
	void set(function<return_t(args_t...), n2>& src)
	{
		m_size = src.m_size;
		if (src.m_size > 0)
		{
			if (src.m_size <= n)
			{
				if (src.m_size <= n2)
					((block_base*)&src.m_buffer)->copy(&m_buffer); // embedded to embedded
				else
					(*(block_base**)&src.m_buffer)->copy(&m_buffer); // allocated to embedded
			}
			else
			{
				if (src.m_size <= n2)
					*(block_base**)&m_buffer = ((block_base*)&src.m_buffer)->copy(); // embedded to allocated
				else
					*(block_base**)&m_buffer = (*(block_base**)&src.m_buffer)->copy(); // allocated to allocated
			}
		}
	}

public:
	function() : m_size(0) { }

	bool operator!() const { return m_size == 0; }


	template <typename F, std::enable_if_t<!is_same_function_class_v<std::remove_reference_t<F>, return_t(args_t...)> >...>
	function(F&& f)
	{
		set_func(std::forward<F>(f));
	}

	function(this_t&& src) { move(std::move(src)); }
	function(this_t& src) { set(src); }
	function(const this_t& src) { set(src); }

	template <size_t n2> function(function<return_t(args_t...), n2>&& src) { move(std::move(src)); }
	template <size_t n2> function(function<return_t(args_t...), n2>& src) { set(src); }
	template <size_t n2> function(const function<return_t(args_t...), n2>& src) { set(src); }

	~function() { release_inner(); }

	void release() { release_inner(); m_size = 0; }

	template <typename F, std::enable_if_t<!is_same_function_class_v<std::remove_reference_t<F>, return_t(args_t...)> >...>
	this_t& operator=(F&& f) { release_inner(); set_func(std::forward<F>(f)); return *this; }

	this_t& operator=(this_t&& src) { release_inner(); move(std::move(src)); return *this; }
	this_t& operator=(this_t& src) { release_inner(); set(src); return *this; }
	this_t& operator=(const this_t& src) { release_inner(); set(src); return *this; }

	template <size_t n2> this_t& operator=(function<return_t(args_t...), n2>&& src) { release_inner(); move(std::move(src)); return *this; }
	template <size_t n2> this_t& operator=(function<return_t(args_t...), n2>& src) { release_inner(); set(src); return *this; }
	template <size_t n2> this_t& operator=(const function<return_t(args_t...), n2>& src) { release_inner(); set(src); return *this; }

	return_t operator()(args_t... a) const
	{
		COGS_ASSERT(m_size > 0);
		if (m_size <= n)
			return ((block_base*)&m_buffer)->invoke(std::forward<args_t>(a)...);
		return (*(block_base**)&m_buffer)->invoke(std::forward<args_t>(a)...);
	}

};


template <typename... args_t, size_t n>
class function<void(args_t...), n>
{
private:
	typedef function<void(args_t...), n> this_t;

	class block_base
	{
	public:
		block_base()
		{ }

		virtual ~block_base() { }
		virtual block_base* copy() const = 0;
		virtual void copy(void* p) const = 0;
		virtual block_base* move() = 0;
		virtual void move(void* p) = 0;

		virtual void invoke(args_t...) const = 0;
	};

	// reverse1 starts reversal by accumulating the first element
	// reverse1 also supports 0 arg invoke if only 1 arg is present, ensuring at least 2 args are passed to reverse2/accumulate.
	// reverse2/accumulate completes the reverse the of remaining args
	// reverse3 removes the first (originally the last) arg
	// reverse4/accumulate reverses the remaining args again, provides invoke function, then passes args back around to reverse1

	template <typename... A>
	class reverse3;

	template <typename... A>
	class reverse1 // 0 arg version
	{
	public:
		static void invoke(); // Here to satisfy using.  Failure to match args would manifest as an error here.
	};

	template <typename... A>
	class reverse2 // 0 args left.
	{
	public:
		// Done reversing the args.
		// We know there are at least 2.
		// Now, let's remove the first one...
		// Then reverse them again to call the function!
		template <typename... A2>
		class accumulate : public reverse3<A2...>
		{
		public:
			using reverse3<A2...>::invoke;
		};
	};

	// THIRD
	template <typename T, typename... A>
	class reverse2<T, A...>
	{
	public:
		template <typename... A2>
		class accumulate : public reverse2<A...>::template accumulate<T, A2...>
		{
		public:
			using reverse2<A...>::template accumulate<T, A2...>::invoke;
		};
	};

	// SECOND
	template <typename T, typename... A>
	class reverse1<T, A...> : public reverse2<A...>::template accumulate<T>
	{
	public:
		using reverse2<A...>::template accumulate<T>::invoke;
	};

	// FIRST
	template <typename... A>
	class reverse : public reverse1<A...>
	{
	public:
		using reverse1<A...>::invoke;

		template <typename F, typename enable = std::enable_if_t<std::is_invocable_v<F, A...> > >
		static void invoke(F&& f, A... a)
		{
			f(std::forward<args_t>(a)...);
		}
	};

	template <typename... A>
	class reverse4 // 0 args left.
	{
	public:
		// Done reversing the args.  Provide the new version of invoke
		template <typename... A2>
		class accumulate : public reverse1<A2...>
		{
		public:
			using reverse1<A2...>::invoke;

			template <typename F, typename enable = std::enable_if_t<std::is_invocable_v<F, A2...> >, typename... A3>
			static void invoke(F&& f, A2... a2, A3&&...)
			{
				f(std::forward<A2>(a2)...);
			}
		};
	};

	template <typename T, typename... A>
	class reverse4<T, A...>
	{
	public:
		template <typename... A2>
		class accumulate : public reverse4<A...>::template accumulate<T, A2...>
		{
		public:
			using reverse4<A...>::template accumulate<T, A2...>::invoke;
		};
	};

	template <typename... A>
	class reverse3 // not used
	{
	public:
		static void invoke(); // not called, just here to satisfy using
	};

	template <typename T, typename... A>
	class reverse3<T, A...> : public reverse4<A...>::template accumulate<>
	{
	public:
		using reverse4<A...>::template accumulate<>::invoke;
	};

	template <typename F>
	class block : public block_base
	{
	public:
		F m_func;

		template <typename F2>
		block(F2&& f)
			: m_func(std::forward<F2>(f))
		{ }

		template <typename F2>
		block(block<F2>&& f)
			: m_func(std::move(f.m_func))
		{ }

		template <typename F2>
		block(const block<F2>& f)
			: m_func(f.m_func)
		{ }

		block(F&& f)
			: m_func(std::move(f))
		{ }

		block(const F& f)
			: m_func(f)
		{ }

		template <size_t n2> block(function<void(args_t...), n2>&& f) = delete; // Should have been copied
		template <size_t n2> block(const function<void(args_t...), n2>& f) = delete;

		block(function<void(args_t...), n>&& f) = delete; //?
		block(const function<void(args_t...), n>& f) = delete;

		virtual ~block() { }

		virtual block_base* copy() const
		{
			block<F>* blk = default_memory_manager::allocate_type<block<F> >();
			return new (blk) block<F>(m_func);
		}

		virtual void copy(void* p) const
		{
			new ((block<F>*)p) block<F>(m_func);
		}

		virtual block_base* move()
		{
			block<F>* blk = default_memory_manager::allocate_type<block<F> >();
			return new (blk) block<F>(std::move(m_func));
		}

		virtual void move(void* p)
		{
			new ((block<F>*)p) block<F>(std::move(m_func));
		}

		virtual void invoke(args_t... a) const { reverse<args_t...>::invoke(m_func, std::forward<args_t>(a)...); }
	};

	unsigned char m_buffer[n];
	size_t m_size; // 0 = empty, <=n embedded, >n allocated

	void release_inner()
	{
		if (m_size > 0)
		{
			if (m_size <= n)
				((block_base*)&m_buffer)->~block_base();
			else
				default_memory_manager::destruct_deallocate_type(*(block_base**)&m_buffer);
		}
	}

	template <typename F>
	void set_func(F&& f)
	{
		typedef std::remove_reference_t<F> F_t;
		const size_t blockSize = sizeof(block<F_t>);
		m_size = blockSize;
		if constexpr (blockSize <= n)
			new ((block<F_t>*)&m_buffer) block<F_t>(std::forward<F>(f));
		else
		{
			block<F>* blk = default_memory_manager::allocate_type<block<F_t> >();
			*(block<F_t>**)&m_buffer = new (blk) block<F_t>(std::forward<F>(f));
		}
	}

	void move(this_t&& src)
	{
		m_size = src.m_size;
		if (src.m_size > 0)
		{
			if (src.m_size <= n)
				((block_base*)&src.m_buffer)->move(&m_buffer);
			else
			{
				*(block_base**)&m_buffer = *(block_base**)&src.m_buffer;
				src.m_size = 0;
			}
		}
	}

	void set(const this_t& src)
	{
		m_size = src.m_size;
		if (src.m_size > 0)
		{
			if (src.m_size <= n)
				((block_base*)&src.m_buffer)->copy(&m_buffer);
			else
				*(block_base**)&m_buffer = (*(block_base**)&src.m_buffer)->copy();
		}
	}

	template <size_t n2>
	void move(function<void(args_t...), n2>&& src)
	{
		m_size = src.m_size;
		if (src.m_size > 0)
		{
			if (src.m_size <= n)
			{
				if (src.m_size <= n2)
					((block_base*)&src.m_buffer)->move(&m_buffer); // embedded to embedded
				else
					(*(block_base**)&src.m_buffer)->move(&m_buffer); // allocated to embedded
			}
			else
			{
				if (src.m_size <= n2)
					*(block_base**)&m_buffer = ((block_base*)&src.m_buffer)->move(); // embedded to allocated
				else
				{
					*(block_base**)&m_buffer = *(block_base**)&src.m_buffer; // allocated to allocated
					src.m_size = 0;
				}
			}
		}
	}

	template <size_t n2>
	void set(const function<void(args_t...), n2>& src)
	{
		m_size = src.m_size;
		if (src.m_size > 0)
		{
			if (src.m_size <= n)
			{
				if (src.m_size <= n2)
					((block_base*)&src.m_buffer)->copy(&m_buffer); // embedded to embedded
				else
					(*(block_base**)&src.m_buffer)->copy(&m_buffer); // allocated to embedded
			}
			else
			{
				if (src.m_size <= n2)
					*(block_base**)&m_buffer = ((block_base*)&src.m_buffer)->copy(); // embedded to allocated
				else
					*(block_base**)&m_buffer = (*(block_base**)&src.m_buffer)->copy(); // allocated to allocated
			}
		}
	}

public:
	function() : m_size(0) { }

	bool operator!() const { return m_size == 0; }

	template <typename F, std::enable_if_t<!is_same_function_class_v<std::remove_reference_t<F>, void(args_t...)> >...>
	function(F&& f) { set_func(std::forward<F>(f)); }

	function(this_t&& src) { move(std::move(src)); }
	function(this_t& src) { set(src); }
	function(const this_t& src) { set(src); }

	template <size_t n2> function(function<void(args_t...), n2>&& src) { move(std::move(src)); }
	template <size_t n2> function(function<void(args_t...), n2>& src) { set(src); }
	template <size_t n2> function(const function<void(args_t...), n2>& src) { set(src); }

	~function() { release_inner(); }

	void release() { release_inner(); m_size = 0; }

	template <typename F, std::enable_if_t<!is_same_function_class_v<std::remove_reference_t<F>, void(args_t...)> >...>
	this_t& operator=(F&& f) { release_inner(); set_func(std::forward<F>(f)); return *this; }

	this_t& operator=(this_t&& src) { release_inner(); move(std::move(src)); return *this; }
	this_t& operator=(this_t& src) { release_inner(); set(src); return *this; }
	this_t& operator=(const this_t& src) { release_inner(); set(src); return *this; }

	template <size_t n2> this_t& operator=(function<void(args_t...), n2>&& src) { release_inner(); move(std::move(src)); return *this; }
	template <size_t n2> this_t& operator=(function<void(args_t...), n2>& src) { release_inner(); set(src); return *this; }
	template <size_t n2> this_t& operator=(const function<void(args_t...), n2>& src) { release_inner(); set(src); return *this; }

	void operator()(args_t... a) const
	{
		if (m_size > 0)
		{
			if (m_size <= n)
				((block_base*)&m_buffer)->invoke(std::forward<args_t>(a)...);
			else
				(*(block_base**)&m_buffer)->invoke(std::forward<args_t>(a)...);
		}
	}
};


typedef function<void()> void_function;


}


#endif
