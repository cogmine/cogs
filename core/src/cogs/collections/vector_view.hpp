////
////  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
////
//
//
//// Status: WorkInProgress
//
//#ifndef COGS_HEADER_COLLECTION_VECTOR_VIEW
//#define COGS_HEADER_COLLECTION_VECTOR_VIEW
//
//
//#include "cogs/collections/array_view.hpp"
//
//namespace cogs {
//
//
//template <typename T>
//class vector_view;
//
//template <typename T> struct is_reference_container<vector_view<T> > : std::true_type {};
//template <typename T> struct is_reference_container<const vector_view<T> > : std::true_type {};
//template <typename T> struct is_reference_container<volatile vector_view<T> > : std::true_type {};
//template <typename T> struct is_reference_container<const volatile vector_view<T> > : std::true_type {};
//
//template <typename T> struct remove_extent<vector_view<T> > { public: typedef remove_extent_t<T> type; };
//template <typename T> struct remove_extent<const vector_view<T> > { public: typedef remove_extent_t<T> type; };
//template <typename T> struct remove_extent<volatile vector_view<T> > { public: typedef remove_extent_t<T> type; };
//template <typename T> struct remove_extent<const volatile vector_view<T> > { public: typedef remove_extent_t<T> type; };
//
//template <typename T> struct remove_all_extents<vector_view<T> > { public: typedef remove_all_extents_t<T> type; };
//template <typename T> struct remove_all_extents<const vector_view<T> > { public: typedef remove_all_extents_t<T> type; };
//template <typename T> struct remove_all_extents<volatile vector_view<T> > { public: typedef remove_all_extents_t<T> type; };
//template <typename T> struct remove_all_extents<const volatile vector_view<T> > { public: typedef remove_all_extents_t<T> type; };
//
//
//struct vector_view_content_t
//{
//	unsigned char* m_base;
//	size_t m_stride;
//	size_t m_length;
//
//	vector_view_content_t()
//	{
//	}
//};
//
//
//template <typename T>
//class vector_view
//{
//public:
//	typedef T type;
//	typedef  vector_view<T> this_t;
//
//private:
//	typedef vector_view_content_t content_t;
//	typedef transactable<vector_view_content_t> transactable_t;
//	transactable_t m_contents;
//
//	template <typename>
//	friend class vector_view;
//
//	typedef typename transactable_t::read_token		read_token;
//	typedef typename transactable_t::write_token	write_token;
//
//	read_token begin_read() const volatile { return m_contents.begin_read(); }
//	void begin_read(read_token& rt) const volatile { m_contents.begin_read(rt); }
//	bool end_read(read_token& t) const volatile { return m_contents.end_read(t); }
//	void begin_write(write_token& wt) volatile { m_contents.begin_write(wt); }
//	template <typename type2>
//	void begin_write(write_token& wt, type2& src) volatile { m_contents.begin_write(wt, src); }
//	bool promote_read_token(read_token& rt, write_token& wt) volatile { return m_contents.promote_read_token(rt, wt); }
//	template <typename type2>
//	bool promote_read_token(read_token& rt, write_token& wt, type2& src) volatile { return m_contents.promote_read_token(rt, wt, src); }
//	bool end_write(write_token& t) volatile { return m_contents.end_write(t); }
//	template <typename type2>
//	bool end_write(read_token& t, type2& src) volatile { return m_contents.end_write(t, src); }
//	template <class functor_t>
//	void write_retry_loop(functor_t&& fctr) volatile { m_contents.write_retry_loop(std::forward<functor_t>(fctr)); }
//	template <class functor_t>
//	auto write_retry_loop_pre(functor_t&& fctr) volatile { return m_contents.write_retry_loop_pre(std::forward<functor_t>(fctr)); }
//	template <class functor_t>
//	auto write_retry_loop_post(functor_t&& fctr) volatile { return m_contents.write_retry_loop_post(std::forward<functor_t>(fctr)); }
//	template <class functor_t, class on_fail_t>
//	void write_retry_loop(functor_t&& fctr, on_fail_t&& onFail) volatile { m_contents.write_retry_loop(std::forward<functor_t>(fctr), std::forward<on_fail_t>(onFail)); }
//	template <class functor_t, class on_fail_t>
//	auto write_retry_loop_pre(functor_t&& fctr, on_fail_t&& onFail) volatile { return m_contents.write_retry_loop_pre(std::forward<functor_t>(fctr), std::forward<on_fail_t>(onFail)); }
//	template <class functor_t, class on_fail_t>
//	auto write_retry_loop_post(functor_t&& fctr, on_fail_t&& onFail) volatile { return m_contents.write_retry_loop_post(std::forward<functor_t>(fctr), std::forward<on_fail_t>(onFail)); }
//	template <class functor_t>
//	void try_write_retry_loop(functor_t&& fctr) volatile { m_contents.try_write_retry_loop(std::forward<functor_t>(fctr)); }
//	template <class functor_t>
//	auto try_write_retry_loop_pre(functor_t&& fctr) volatile { return m_contents.try_write_retry_loop_pre(std::forward<functor_t>(fctr)); }
//	template <class functor_t>
//	auto try_write_retry_loop_post(functor_t&& fctr) volatile { return m_contents.try_write_retry_loop_post(std::forward<functor_t>(fctr)); }
//	template <class functor_t, class on_fail_t>
//	void try_write_retry_loop(functor_t&& fctr, on_fail_t&& onFail) volatile { m_contents.try_write_retry_loop(std::forward<functor_t>(fctr), std::forward<on_fail_t>(onFail)); }
//	template <class functor_t, class on_fail_t>
//	auto try_write_retry_loop_pre(functor_t&& fctr, on_fail_t&& onFail) volatile { return m_contents.try_write_retry_loop_pre(std::forward<functor_t>(fctr), std::forward<on_fail_t>(onFail)); }
//	template <class functor_t, class on_fail_t>
//	auto try_write_retry_loop_post(functor_t&& fctr, on_fail_t&& onFail) volatile { return m_contents.try_write_retry_loop_post(std::forward<functor_t>(fctr), std::forward<on_fail_t>(onFail)); }
//
//	vector_view(const content_t& c) : m_contents(typename transactable_t::construct_embedded_t(), c) { }
//
//	this_t& operator=(const content_t& c) { m_contents.set(c); return *this; }
//
//public:
//	vector_view() { m_contents->m_length = 0; }
//
//	template <typename T2, std::enable_if_t<is_array_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	vector_view(T2& t)
//	{
//		m_contents->m_base = (unsigned char*)static_cast<T*>(&(t[0]));
//		m_contents->m_stride = sizeof(remove_extent_t<T2>);
//		m_contents->m_length = extent_v<T2>;
//	}
//
//	template <size_t n, typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	vector_view(const array_view<n, T2>& t)
//	{
//		m_contents->m_base = (unsigned char*)static_cast<T*>((T2*)t.m_contents.m_base);
//		m_contents->m_stride = t.m_contents.m_stride;
//		m_contents->m_length = n;
//	}
//
//	template <size_t n, typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	vector_view(const volatile array_view<n, T2>& t)
//	{
//		decltype(auto) t2(load(t));
//		m_contents->m_base = (unsigned char*)static_cast<T*>((T2*)t2.m_contents.m_base);
//		m_contents->m_stride = t2.m_contents.m_stride;
//		m_contents->m_length = n;
//	}
//
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	vector_view(const vector_view<T2>& t)
//	{
//		m_contents->m_base = (unsigned char*)static_cast<T*>((T2*)t.m_contents->m_base);
//		m_contents->m_stride = t.m_contents->m_stride;
//		m_contents->m_length = t.m_contents->m_length;
//	}
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	vector_view(const volatile vector_view<T2>& t)
//	{
//		vector_view<T2>::read_token rt = t.begin_read();
//		m_contents->m_base = (unsigned char*)static_cast<T*>((T2*)rt->m_base);
//		m_contents->m_stride = rt->m_stride;
//		m_contents->m_length = rt->m_length;
//	}
//
//	template <typename T2, typename = std::enable_if_t<std::is_pointer_v<T2> && std::is_convertible_v<T2, T*> > >
//	vector_view(T2 t, size_t n, size_t stride = sizeof(std::remove_pointer_t<T2>))
//	{
//		m_contents->m_base = (unsigned char*)static_cast<T*>(t);
//		m_contents->m_stride = stride;
//		m_contents->m_length = n;
//	}
//
//	vector_view(this_t& src) : m_contents(typename transactable_t::construct_embedded_t(), src.m_contents) { }
//	vector_view(const this_t& src) : m_contents(typename transactable_t::construct_embedded_t(), src.m_contents) { }
//
//	vector_view(volatile this_t& src) : m_contents(typename transactable_t::construct_embedded_t(), *src.begin_read()) { }
//	vector_view(const volatile this_t& src) : m_contents(typename transactable_t::construct_embedded_t(), *src.begin_read()) { }
//
//	// operator=
//	template <typename T2, std::enable_if_t<is_array_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	this_t& operator=(T2& t)
//	{
//		m_contents.m_base = (unsigned char*)static_cast<T*>(&(t[0]));
//		m_contents.m_stride = sizeof(remove_extent_t<T2>);
//		m_contents.length = extent_v<T2>;
//		return *this;
//	}
//
//	template <typename T2, std::enable_if_t<is_array_view_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	this_t& operator=(T2& t)
//	{
//		decltype(auto) t2(load(t));
//		tmp.m_contents.m_base = (unsigned char*)static_cast<T*>((T2*)t2.m_contents.m_base);
//		m_contents.m_stride = t2.m_contents.m_stride;
//		m_contents.length = extent_v<T2>;
//		return *this;
//	}
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	this_t& operator=(const vector_view<T2>& t)
//	{
//		m_contents->m_base = (unsigned char*)static_cast<T*>((T2*)t.m_contents->m_base);
//		m_contents->m_stride = t.m_contents->m_stride;
//		m_contents->m_length = t.m_contents->m_length;
//		return *this;
//	}
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	this_t& operator=(const volatile vector_view<T2>& t)
//	{
//		vector_view<T2>::read_token rt = t.begin_read();
//		m_contents->m_base = (unsigned char*)static_cast<T*>((T2*)rt->m_base);
//		m_contents->m_stride = rt->m_stride;
//		m_contents->m_length = rt->m_length;
//		return *this;
//	}
//
//	this_t& operator=(this_t& src) { *m_contents = *src.m_contents; return *this; }
//	this_t& operator=(const this_t& src) { *m_contents = *src.m_contents; return *this; }
//
//	this_t& operator=(volatile this_t& src)
//	{
//		read_token rt = src.begin_read();
//		m_contents->m_base = rt->m_base;
//		m_contents->m_stride = rt->m_stride;
//		m_contents->m_length = rt->m_length;
//		return *this;
//	}
//
//	this_t& operator=(const volatile this_t& src)
//	{
//		read_token rt = src.begin_read();
//		m_contents->m_base = rt->m_base;
//		m_contents->m_stride = rt->m_stride;
//		m_contents->m_length = rt->m_length;
//		return *this;
//	}
//
//	template <typename T2, std::enable_if_t<is_array_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	volatile this_t& operator=(T2& t) volatile
//	{
//		m_contents.set();
//		write_token wt = src.begin_read();
//
//#error write loop
//		content_t tmp;
//		tmp.m_contents.m_base = (unsigned char*)static_cast<T*>(&(t[0]));
//		tmp.m_contents.m_stride = sizeof(remove_extent_t<T2>);
//		atomic::store(m_contents, tmp);
//		return *this;
//	}
//
//	template <typename T2, std::enable_if_t<is_array_view_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	volatile this_t& operator=(T2& t) volatile
//	{
//		decltype(auto) t2(load(t));
//		content_t tmp;
//		tmp.m_contents.m_base = (unsigned char*)static_cast<T*>((T2*)t2.m_contents.m_base);
//		tmp.m_contents.m_stride = t2.m_contents.m_stride;
//		atomic::store(m_contents, tmp);
//		return *this;
//	}
//
//	volatile this_t& operator=(this_t& src) volatile { atomic::store(m_contents, src.m_contents); return *this; }
//	volatile this_t& operator=(const this_t& src) volatile { atomic::store(m_contents, src.m_contents); return *this; }
//	volatile this_t& operator=(volatile this_t& src) volatile { atomic::store(m_contents, atomic::load(src.m_contents)); return *this; }
//	volatile this_t& operator=(const volatile this_t& src) volatile { atomic::store(m_contents, atomic::load(src.m_contents)); return *this; }
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	volatile this_t& operator=(const vector_view<T2>& t) volatile
//	{
//		m_contents->m_base = (unsigned char*)static_cast<T*>((T2*)t.m_contents->m_base);
//		m_contents->m_stride = t.m_contents->m_stride;
//		m_contents->m_length = t.m_contents->m_length;
//		return *this;
//	}
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	volatile this_t& operator=(const volatile vector_view<T2>& t) volatile
//	{
//		vector_view<T2>::read_token rt = t.begin_read();
//		m_contents->m_base = (unsigned char*)static_cast<T*>((T2*)rt->m_base);
//		m_contents->m_stride = rt->m_stride;
//		m_contents->m_length = rt->m_length;
//		return *this;
//	}
//
//	static constexpr size_t get_length() { return n; }
//
//	size_t get_stride() const { return m_contents.m_stride; }
//	size_t get_stride() const volatile { return atomic::load(m_contents.m_stride); }
//
//	type& get(size_t i) const { COGS_ASSERT(i < n); return *(type*)(m_contents.m_base + (i * m_contents.m_stride)); }
//	type& get(size_t i) const volatile
//	{
//		COGS_ASSERT(i < n);
//		content_t tmp;
//		atomic::load(m_contents, tmp);
//		return *(type*)(tmp.m_base + (i * tmp.m_stride));
//	}
//
//	type& operator[](size_t i) const { return get(i); }
//	type& operator[](size_t i) const volatile { return get(i); }
//
//	type& get_first() const { return *(type*)m_contents.m_base; }
//	type& get_first() const volatile { return *(type*)atomic::load(m_contents.m_base); }
//
//	vector_view<T> subrange(size_t i, size_t n) const
//	{
//		vector_view<n2, T> result;
//		if (!!m_contents->m_base && i < m_contents->m_length)
//		{
//			size_t newLength = m_contents->m_length - i;
//			if (newLength > n)
//				newLength = n;
//			result.m_contents->m_length = newLength;
//			result.m_base = m_contents->m_base + i;
//		}
//		return result;
//	}
//
//	vector_view<T> subrange(size_t i, size_t n) const volatile
//	{
//		vector_view<n2, T> result;
//		read_token rt = t.begin_read();
//		if (!!rt->m_base&& i < rt->m_length)
//		{
//			size_t newLength = rt->m_length - i;
//			if (newLength > n)
//				newLength = n;
//			result.m_contents->m_length = newLength;
//			result.m_base = rt->m_base + i;
//		}
//		return result;
//	}
//
//	void clear() { m_contents.m_base = 0; }
//	void clear() volatile { atomic::store(m_contents.m_base, 0); }
//	
//	// not
//	bool operator!() const { return m_contents.m_base == 0; }
//	bool operator!() const volatile { return atomic::load(m_contents.m_base) == 0; }
//
//	// equals
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	bool operator==(const vector_view<T2>& t) const
//	{
//		return (m_contents->m_base == (unsigned char*)static_cast<T*>((T2*)t.m_contents->m_base))
//			&& (m_contents->m_length == t.m_contents->m_length);
//	}
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	bool operator==(const volatile vector_view<T2>& t) const
//	{
//		read_token rt = t.begin_read();
//		return (m_contents->m_base == (unsigned char*)static_cast<T*>((T2*)rt->m_base))
//			&& (m_contents->m_length == rt->m_length);
//	}
//
//	template <typename T2, std::enable_if_t<!std::is_convertible_v<T2*, T*> && std::is_convertible_v<T*, T2*> >...>
//	bool operator==(const vector_view<T2>& t) const
//	{
//		return (t.m_contents->m_base == (unsigned char*)static_cast<T2*>((T*)m_contents->m_base))
//			&& (m_contents->m_length == t.m_contents->m_length);
//	}
//
//	template <typename T2, std::enable_if_t<!std::is_convertible_v<T2*, T*>&& std::is_convertible_v<T*, T2*> >...>
//	bool operator==(const volatile vector_view<T2>& t) const
//	{
//		read_token rt = t.begin_read();
//		return (rt->m_base == (unsigned char*)static_cast<T2*>((T*)m_contents->m_base))
//			&& (m_contents->m_length == rt->m_length);
//	}
//
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	bool operator==(const vector_view<T2>& t) const volatile
//	{
//		read_token rt = begin_read();
//		return (rt->m_base == (unsigned char*)static_cast<T*>((T2*)t.m_contents->m_base))
//			&& (rt->m_length == t.m_contents->m_length);
//	}
//
//	template <typename T2, std::enable_if_t<!std::is_convertible_v<T2*, T*> && std::is_convertible_v<T*, T2*> >...>
//	bool operator==(const vector_view<T2>& t) const volatile
//	{
//		read_token rt = begin_read();
//		return (t.m_contents->m_base == (unsigned char*)static_cast<T2*>((T*)rt->m_base))
//			&& (rt->m_length == t.m_contents->m_length);
//	}
//
//
//	template <typename T2, std::enable_if_t<is_array_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	bool operator==(T2& t2) const
//	{
//		return (m_contents.m_base == (unsigned char*)static_cast<T*>(&(t[0])))
//			&& (extent_v<T2> == m_contents.m_length);
//	}
//	
//	template <typename T2, std::enable_if_t<is_array_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	bool operator==(T2& t2) const volatile
//	{
//		read_token rt = begin_read();
//		return (rt->m_base == (unsigned char*)static_cast<T*>(&(t[0])))
//			&& (extent_v<T2> == rt->m_length);
//	}
//
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	bool operator!=(const vector_view<T2>& t) const { return !operator==(t); }
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	bool operator!=(const volatile vector_view<T2>& t) const { return !operator==(t); }
//
//	template <typename T2, std::enable_if_t<!std::is_convertible_v<T2*, T*> && std::is_convertible_v<T*, T2*> >...>
//	bool operator!=(const vector_view<T2>& t) const { return !operator==(t); }
//
//	template <typename T2, std::enable_if_t<!std::is_convertible_v<T2*, T*> && std::is_convertible_v<T*, T2*> >...>
//	bool operator!=(const volatile vector_view<T2>& t) const { return !operator==(t); }
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	bool operator!=(const vector_view<T2>& t) const volatile { return !operator==(t); }
//
//	template <typename T2, std::enable_if_t<!std::is_convertible_v<T2*, T*> && std::is_convertible_v<T*, T2*> >...>
//	bool operator!=(const vector_view<T2>& t) const volatile { return !operator==(t); }
//
//	template <typename T2, std::enable_if_t<is_array_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	bool operator!=(T2& t2) const { return !operator==(t); }
//
//	template <typename T2, std::enable_if_t<is_array_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	bool operator!=(T2& t2) const volatile { return !operator==(t); }
//
//	// swap
//	template <typename T2, std::enable_if_t<!std::is_volatile_v<T2> && is_array_view_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> && std::is_convertible_v<T*, remove_extent_t<T2>*> >...>
//	void swap(T2& t2) { t2 = exchange(t2); }
//
//	template <typename T2, std::enable_if_t<std::is_volatile_v<T2> && is_array_view_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> && std::is_convertible_v<T*, remove_extent_t<T2>*> >...>
//	void swap(T2& t2) { t2.swap(*this); }
//
//	template <typename T2, std::enable_if_t<!std::is_volatile_v<T2> && is_array_view_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> && std::is_convertible_v<T*, remove_extent_t<T2>*> >...>
//	void swap(T2& t2) volatile { t2 = exchange(t2); }
//
//	// exchange
//	template <typename T2, std::enable_if_t<is_array_view_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> && std::is_convertible_v<T*, remove_extent_t<T2>*> >...>
//	this_t exchange(T2& t2) { this_t tmp(*this); *this = t2; return tmp; }
//
//	template <typename T2, std::enable_if_t<is_array_view_v<T2> && std::is_convertible_v<remove_extent_t<T2>*, T*> && std::is_convertible_v<T*, remove_extent_t<T2>*> >...>
//	this_t exchange(T2& t2) volatile { this_t tmp(t2); return atomic::exchange(m_contents, tmp.m_contents); }
//
//	template <typename T2, typename T3, std::enable_if_t<is_array_view_v<T2> && is_array_view_v<T3> && (extent_v<T3> == n) && std::is_convertible_v<remove_extent_t<T2>*, T*> && std::is_convertible_v<T*, remove_extent_t<T3>*> >...>
//	void exchange(T2& t2, T3& t3) { this_t tmp(*this); *this = t2; t3 = tmp; }
//
//	template <typename T2, typename T3, std::enable_if_t<is_array_view_v<T2> && is_array_view_v<T3> && (extent_v<T3> == n) && std::is_convertible_v<remove_extent_t<T2>*, T*> && std::is_convertible_v<T*, remove_extent_t<T3>*> >...>
//	void exchange(T2& t2, T3& t3) volatile { this_t tmp(t2); atomic::exchange(m_contents, tmp.m_contents, t3); }
//};
//
//
//}
//
//#endif
