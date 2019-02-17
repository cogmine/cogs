////
////  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
////
//
//
//// Status: WorkInProgress
//
//#ifndef COGS_HEADER_COLLECTION_ARRAY_VIEW
//#define COGS_HEADER_COLLECTION_ARRAY_VIEW
//
//
//#pragma warning(push)
//#pragma warning (disable: 4521)	// multiple copy constructors specified
//#pragma warning (disable: 4522)	// multiple assignment operators specified
//
//
//namespace cogs {
//
//
//template <size_t n, class T>
//class array_view;
//
//
//template <size_t n, typename T> struct is_array_type<array_view<n, T> > : std::true_type {};
//template <size_t n, typename T> struct is_array_type<const array_view<n, T> > : std::true_type {};
//template <size_t n, typename T> struct is_array_type<volatile array_view<n, T> > : std::true_type {};
//template <size_t n, typename T> struct is_array_type<const volatile array_view<n, T> > : std::true_type {};
//
//template <size_t n, typename T> struct is_reference_container<array_view<n, T> > : std::true_type {};
//template <size_t n, typename T> struct is_reference_container<const array_view<n, T> > : std::true_type {};
//template <size_t n, typename T> struct is_reference_container<volatile array_view<n, T> > : std::true_type {};
//template <size_t n, typename T> struct is_reference_container<const volatile array_view<n, T> > : std::true_type {};
//
//template <size_t n, typename T> struct extent<array_view<n, T>, 0> : std::integral_constant<size_t, n> {};
//template <size_t n, typename T> struct extent<const array_view<n, T>, 0> : std::integral_constant<size_t, n> {};
//template <size_t n, typename T> struct extent<volatile array_view<n, T>, 0> : std::integral_constant<size_t, n> {};
//template <size_t n, typename T> struct extent<const volatile array_view<n, T>, 0> : std::integral_constant<size_t, n> {};
//
//template <size_t n, typename T, unsigned N> struct extent<array_view<n, T>, N, std::enable_if_t<(N > 0)> > : extent<T, N - 1> {};
//template <size_t n, typename T, unsigned N> struct extent<const array_view<n, T>, N, std::enable_if_t<(N > 0)> > : extent<T, N - 1> {};
//template <size_t n, typename T, unsigned N> struct extent<volatile array_view<n, T>, N, std::enable_if_t<(N > 0)> > : extent<T, N - 1> {};
//template <size_t n, typename T, unsigned N> struct extent<const volatile array_view<n, T>, N, std::enable_if_t<(N > 0)> > : extent<T, N - 1> {};
//
//template <size_t n, typename T> struct remove_extent<array_view<n, T> > { public: typedef remove_extent_t<T> type; };
//template <size_t n, typename T> struct remove_extent<const array_view<n, T> > { public: typedef remove_extent_t<T> type; };
//template <size_t n, typename T> struct remove_extent<volatile array_view<n, T> > { public: typedef remove_extent_t<T> type; };
//template <size_t n, typename T> struct remove_extent<const volatile array_view<n, T> > { public: typedef remove_extent_t<T> type; };
//
//template <size_t n, typename T> struct remove_all_extents<array_view<n, T> > { public: typedef remove_all_extents_t<T> type; };
//template <size_t n, typename T> struct remove_all_extents<const array_view<n, T> > { public: typedef remove_all_extents_t<T> type; };
//template <size_t n, typename T> struct remove_all_extents<volatile array_view<n, T> > { public: typedef remove_all_extents_t<T> type; };
//template <size_t n, typename T> struct remove_all_extents<const volatile array_view<n, T> > { public: typedef remove_all_extents_t<T> type; };
//
//
//// i.e. make_array_view<10>(tPtr);
//template <size_t n, typename T>
//array_view<n, T> make_array_view(T* t, size_t stride = sizeof(T))
//{
//	return array_view<n, T>(t, stride);
//}
//
//
//// i.e. make_array_view<array_type>(a.member);
//template <typename A, typename T>
//array_view<extent_v<A>, T> make_array_view(T& t)
//{
//	return array_view<extent_v<A>, T>(&t, sizeof(remove_extent_t<A>));
//}
//
//
//struct array_view_content_t
//{
//	alignas (atomic::get_alignment_v<unsigned char*>) unsigned char* m_base;
//	alignas (atomic::get_alignment_v<size_t>) size_t m_stride;
//};
//
//template <size_t n, typename T>
//class array_view
//{
//public:
//	typedef T type;
//	typedef array_view<n, T> this_t;
//
//private:
//	static_assert(n > 0);
//
//	typedef array_view_content_t content_t;
//	alignas (atomic::get_alignment_v<content_t>) content_t m_contents;
//
//	template <size_t, typename>
//	friend class array_view;
//
//	template <size_t, typename>
//	friend array_view<n, T> make_array_view(T* t, size_t stride);
//
//	template <size_t i, size_t n3, size_t n2 = (n3 - i), std::enable_if_t<(i < n3) && (n2 + i <= n3)>...>
//	static array_view<n2, T> sub_array(const array_view<n3, T>& src)
//	{
//		array_view<n2, T> tmp(src.m_contents.m_base + (i * src.m_contents.m_stride), src.m_contents.m_stride);
//		return tmp;
//	}
//
//	array_view(const content_t& c) : m_contents(c) { }
//
//	this_t& operator=(const content_t& c) { m_contents = c; return *this; }
//
//public:
//	array_view() { m_contents.m_base = 0; }
//
//	template <typename T2, std::enable_if_t<is_array_v<T2> && (extent_v<T2> == n) && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	array_view(T2& t) : m_contents{ (unsigned char*)static_cast<T*>(&(t[0])), sizeof(remove_extent_t<T2>) } { }
//	
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	array_view(const array_view<n, T2>& t)
//	{
//		m_contents.m_base = (unsigned char*)static_cast<T*>((T2*)t.m_contents.m_base);
//		m_contents.m_stride = t.m_contents.m_stride;
//	}
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	array_view(const volatile array_view<n, T2>& t)
//	{
//		decltype(auto) t2(load(t));
//		m_contents.m_base = (unsigned char*)static_cast<T*>((T2*)t2.m_contents.m_base);
//		m_contents.m_stride = t2.m_contents.m_stride;
//	}
//
//	template <typename T2, typename = std::enable_if_t<std::is_pointer_v<T2> && std::is_convertible_v<T2, T*> > >
//	array_view(T2 t, size_t stride = sizeof(std::remove_pointer_t<T2>)) : m_contents{ (unsigned char*)static_cast<T*>(t), stride } { }
//
//	array_view(this_t& t) : m_contents(t.m_contents) { }
//	array_view(const this_t& t) : m_contents(t.m_contents) { }
//	array_view(volatile this_t& t) : m_contents(atomic::load(t.m_contents)) { }
//	array_view(const volatile this_t& t) : m_contents(atomic::load(t.m_contents)) { }
//
//
//	// operator=
//	template <typename T2, std::enable_if_t<is_array_v<T2> && (extent_v<T2> == n) && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	this_t& operator=(T2& t)
//	{
//		m_contents.m_base = (unsigned char*)static_cast<T*>(&(t[0]));
//		m_contents.m_stride = sizeof(remove_extent_t<T2>);
//		return *this;
//	}
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	this_t& operator=(const array_view<n, T2>& t)
//	{
//		m_contents.m_base = (unsigned char*)static_cast<T*>((T2*)t.m_contents.m_base);
//		m_contents.m_stride = t.m_contents.m_stride;
//		return *this;
//	}
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	this_t& operator=(const volatile array_view<n, T2>& t)
//	{
//		decltype(auto) t2(load(t));
//		m_contents.m_base = (unsigned char*)static_cast<T*>((T2*)t2.m_contents.m_base);
//		m_contents.m_stride = t2.m_contents.m_stride;
//		return *this;
//	}
//
//	this_t& operator=(this_t& src) { m_contents = src.m_contents; return *this; }
//	this_t& operator=(const this_t& src) { m_contents = src.m_contents; return *this; }
//	this_t& operator=(volatile this_t& src) { m_contents = atomic::load(src.m_contents); return *this; }
//	this_t& operator=(const volatile this_t& src) { m_contents = atomic::load(src.m_contents); return *this; }
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	volatile this_t& operator=(const array_view<n, T2>& t) volatile
//	{
//		content_t tmp;
//		tmp.m_base = (unsigned char*)static_cast<T*>((T2*)t.m_contents.m_base);
//		tmp.m_stride = t.m_contents.m_stride;
//		atomic::store(m_contents, tmp);
//		return *this;
//	}
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	volatile this_t& operator=(const volatile array_view<n, T2>& t) volatile
//	{
//		decltype(auto) t2(load(t));
//		content_t tmp;
//		tmp.m_base = (unsigned char*)static_cast<T*>((T2*)t2.m_contents.m_base);
//		tmp.m_stride = t2.m_contents.m_stride;
//		atomic::store(m_contents, tmp);
//		return *this;
//	}
//
//	volatile this_t& operator=(this_t& src) volatile { atomic::store(m_contents, src.m_contents); return *this; }
//	volatile this_t& operator=(const this_t& src) volatile { atomic::store(m_contents, src.m_contents); return *this; }
//	volatile this_t& operator=(volatile this_t& src) volatile { atomic::store(m_contents, atomic::load(src.m_contents)); return *this; }
//	volatile this_t& operator=(const volatile this_t& src) volatile { atomic::store(m_contents, atomic::load(src.m_contents)); return *this; }
//
//
//
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
//	template <size_t i, size_t n2 = (n - i), std::enable_if_t<(i < n) && (n2 + i <= n)>...>
//	array_view<n2, T> subrange() const
//	{
//		array_view<n2, T> result((T*)(m_contents.m_base + (i + m_contents.m_stride)), m_contents.m_stride);
//		return result;
//	}
//
//	template <size_t i, size_t n2 = (n - i), std::enable_if_t<(i < n) && (n2 + i <= n)>...>
//	array_view<n2, T> subrange() const volatile
//	{
//		content_t tmp;
//		atomic::load(m_contents, tmp);
//		array_view<n2, T> result((T*)(tmp.m_base + (i + tmp.m_stride)), tmp.m_stride);
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
//	bool operator==(const array_view<n, T2>& t) const { return m_contents.m_base == (unsigned char*)static_cast<T*>((T2*)t.m_contents.m_base); }
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	bool operator==(const volatile array_view<n, T2>& t) const { return m_contents.m_base == (unsigned char*)static_cast<T*>((T2*)atomic::load(t.m_contents.m_base)); }
//
//	template <typename T2, std::enable_if_t<!std::is_convertible_v<T2*, T*> && std::is_convertible_v<T*, T2*> >...>
//	bool operator==(const array_view<n, T2>& t) const { return t.m_contents.m_base == (unsigned char*)static_cast<T2*>((T*)m_contents.m_base); }
//
//	template <typename T2, std::enable_if_t<!std::is_convertible_v<T2*, T*> && std::is_convertible_v<T*, T2*> >...>
//	bool operator==(const volatile array_view<n, T2>& t) const { return atomic::load(t.m_contents.m_base) == (unsigned char*)static_cast<T2*>((T*)m_contents.m_base); }
//	
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	bool operator==(const array_view<n, T2>& t) const volatile { return atomic::load(m_contents.m_base) == (unsigned char*)static_cast<T*>((T2*)t.m_contents.m_base); }
//
//	template <typename T2, std::enable_if_t<!std::is_convertible_v<T2*, T*>&& std::is_convertible_v<T*, T2*> >...>
//	bool operator==(const array_view<n, T2>& t) const volatile { return t.m_contents.m_base == (unsigned char*)static_cast<T2*>((T*)atomic::load(m_contents.m_base)); }
//
//
//	template <typename T2, std::enable_if_t<is_array_v<T2> && (extent_v<T2> == n) && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	bool operator==(T2& t2) const { return m_contents.m_base == (unsigned char*)static_cast<T*>(&(t[0])); }
//
//	template <typename T2, std::enable_if_t<is_array_v<T2> && (extent_v<T2> == n) && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	bool operator==(T2& t2) const volatile { return atomic::load(m_contents.m_base) == (unsigned char*)static_cast<T*>(&(t[0])); }
//
//	// not_equals
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	bool operator!=(const array_view<n, T2>& t) const { return m_contents.m_base != (unsigned char*)static_cast<T*>((T2*)t.m_contents.m_base); }
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	bool operator!=(const volatile array_view<n, T2>& t) const { return m_contents.m_base != (unsigned char*)static_cast<T*>((T2*)atomic::load(t.m_contents.m_base)); }
//
//	template <typename T2, std::enable_if_t<!std::is_convertible_v<T2*, T*>&& std::is_convertible_v<T*, T2*> >...>
//	bool operator!=(const array_view<n, T2>& t) const { return t.m_contents.m_base != (unsigned char*)static_cast<T2*>((T*)m_contents.m_base); }
//
//	template <typename T2, std::enable_if_t<!std::is_convertible_v<T2*, T*>&& std::is_convertible_v<T*, T2*> >...>
//	bool operator!=(const volatile array_view<n, T2>& t) const { return atomic::load(t.m_contents.m_base) != (unsigned char*)static_cast<T2*>((T*)m_contents.m_base); }
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<T2*, T*> >...>
//	bool operator!=(const array_view<n, T2>& t) const volatile { return atomic::load(m_contents.m_base) != (unsigned char*)static_cast<T*>((T2*)t.m_contents.m_base); }
//
//	template <typename T2, std::enable_if_t<!std::is_convertible_v<T2*, T*>&& std::is_convertible_v<T*, T2*> >...>
//	bool operator!=(const array_view<n, T2>& t) const volatile { return t.m_contents.m_base != (unsigned char*)static_cast<T2*>((T*)atomic::load(m_contents.m_base)); }
//
//	template <typename T2, std::enable_if_t<is_array_v<T2> && (extent_v<T2> == n) && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	bool operator!=(T2& t2) const { return m_contents.m_base != (unsigned char*)static_cast<T*>(&(t[0])); }
//
//	template <typename T2, std::enable_if_t<is_array_v<T2> && (extent_v<T2> == n) && std::is_convertible_v<remove_extent_t<T2>*, T*> >...>
//	bool operator!=(T2& t2) const volatile { return atomic::load(m_contents.m_base) != (unsigned char*)static_cast<T*>(&(t[0])); }
//
//	// swap
//	template <typename T2, std::enable_if_t<std::is_convertible_v<remove_extent_t<T2>*, T*> && std::is_convertible_v<remove_extent_t<T>*, T2*> >...>
//	void swap(array_view<n, T2>& t2)
//	{
//		this_t tmp(t2);
//		t2 = *this;
//		*this = tmp;
//	}
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<remove_extent_t<T2>*, T*> && std::is_convertible_v<remove_extent_t<T>*, T2*> >...>
//	void swap(array_view<n, T2>& t2) volatile
//	{
//		this_t tmp(t2);
//		atomic::exchange(m_contents, tmp.m_contents, tmp.m_contents);
//		t2 = tmp;
//	}
//
//	template <typename T2, std::enable_if_t<std::is_convertible_v<remove_extent_t<T2>*, T*> && std::is_convertible_v<remove_extent_t<T>*, T2*> >...>
//	void swap(volatile array_view<n, T2>& t2)
//	{ t2.swap(*this); }
//
//
//	// exchange
//	template <typename S, std::enable_if_t<std::is_constructible_v<this_t, S> >...>
//	this_t exchange(S&& src)
//	{
//		this_t result(*this);
//		this_t tmp(std::forward<S>(src));
//		*this = tmp;
//		return result;
//	}
//
//	template <typename S, std::enable_if_t<std::is_constructible_v<this_t, S> >...>
//	this_t exchange(S&& src) volatile
//	{
//		this_t tmp(std::forward<S>(src));
//		atomic::exchange(m_contents, tmp.m_contents, tmp.m_contents);
//		return tmp;
//	}
//
//	template <typename S, typename R, std::enable_if_t<std::is_constructible_v<this_t, S> && std::is_assignable_v<R&, this_t> >...>
//	void exchange(S&& src, R& rtn)
//	{
//		this_t result(*this);
//		this_t tmp(std::forward<S>(src));
//		*this = tmp;
//		rtn = result;
//	}
//
//	template <typename S, typename R, std::enable_if_t<std::is_constructible_v<this_t, S> && std::is_assignable_v<R&, this_t> >...>
//	void exchange(S&& src, R& rtn) volatile
//	{
//		this_t tmp(std::forward<S>(src));
//		atomic::exchange(m_contents, tmp.m_contents, tmp.m_contents);
//		rtn = tmp;
//	}
//
//
//	// compare_exchange
//	template <typename S, typename C, std::enable_if_t<std::is_constructible_v<this_t, S> && std::is_constructible_v<this_t, C> >...>
//	bool compare_exchange(S&& src, C&& cmp)
//	{
//		this_t cmp2(std::forward<C>(cmp));
//		if (m_contents != cmp2.m_contents)
//			return false;
//		this_t src2(std::forward<S>(src));
//		*this = src2;
//		return true;
//	}
//
//	template <typename S, typename C, std::enable_if_t<std::is_constructible_v<this_t, S> && std::is_constructible_v<this_t, C> >...>
//	bool compare_exchange(S&& src, C&& cmp) volatile
//	{
//		this_t src2(std::forward<S>(src));
//		this_t cmp2(std::forward<C>(cmp));
//		return atomic::compare_exchange(m_contents, src2.m_contents, cmp2.m_contents);
//	}
//
//
//	template <typename S, typename C, typename R, std::enable_if_t<std::is_constructible_v<this_t, S> && std::is_constructible_v<this_t, C> && std::is_assignable_v<R&, this_t> >...>
//	bool compare_exchange(S&& src, C&& cmp, R& rtn)
//	{
//		this_t cmp2(std::forward<C>(cmp));
//		if (m_contents != cmp2.m_contents)
//			return false;
//		rtn = exchange(src);
//		return true;
//	}
//
//	template <typename S, typename C, typename R, std::enable_if_t<std::is_constructible_v<this_t, S> && std::is_constructible_v<this_t, C> && std::is_assignable_v<R&, this_t> >...>
//	bool compare_exchange(S&& src, C&& cmp, R& rtn) volatile
//	{
//		this_t src2(std::forward<S>(src));
//		this_t cmp2(std::forward<C>(cmp));
//		this_t rtn2;
//		bool b = atomic::compare_exchange(m_contents, src2.m_contents, cmp2.m_contents, rtn2.m_contents);
//		rtn = rtn2;
//		return b;
//	}
//};
//
//
//template <typename T, typename enable = void> struct is_array_view : std::false_type {};
//template <size_t n, typename T> struct is_array_view<array_view<n, T> > : std::true_type {};
//template <size_t n, typename T> struct is_array_view<const array_view<n, T> > : std::true_type {};
//template <size_t n, typename T> struct is_array_view<volatile array_view<n, T> > : std::true_type {};
//template <size_t n, typename T> struct is_array_view<const volatile array_view<n, T> > : std::true_type {};
//template <typename T> constexpr bool is_array_view_v = is_array_view<T>::value;
//
//
//}
//
//#pragma warning(pop)
//
//
//#endif
