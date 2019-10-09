//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, MayNeedCleanup

#ifndef COGS_HEADER_SYNC_VERSIONED
#define COGS_HEADER_SYNC_VERSIONED


#include "cogs/env/mem/alignment.hpp"
#include "cogs/math/bytes_to_int.hpp"
#include "cogs/operators.hpp"


namespace cogs {


/// @ingroup Synchronization
/// @brief Provides an atomic ABA solution for sufficiently small types.
/// @tparam T Type to contain.  Must be sufficiently small to support an atomic operation.  (1/2 the largest atomic supported).
template <typename T>
class versioned_t
{
public:
	typedef T type;
	typedef bytes_to_int_t< sizeof(type) > version_t;

protected:
	struct content_t
	{
		alignas (atomic::get_alignment_v<type>) type m_data;
		alignas (atomic::get_alignment_v<version_t>) version_t m_version;
	};

	alignas (atomic::get_alignment_v<content_t>) content_t m_contents;

public:
	versioned_t()
	{
	}
	
	versioned_t(const type& t)
	{
		m_contents.m_data = t;
		//m_contents.m_version = 0;	// Doesn't matter what # the version counter actually starts on
	}

	void get(type& t) const { t = m_contents.m_data; }
	void get(type& t) const volatile { t = atomic::load(m_contents.m_data); }

	void get(type& t, version_t& v) const { get(t); get_version(v); }
	void get(type& t, version_t& v) const volatile
	{
		content_t tmp;
		atomic::load(m_contents, tmp);
		t = tmp.m_data;
		v = tmp.m_version;
	}

	void get_version(version_t& v) const { v = m_contents.m_version; }
	void get_version(version_t& v) const volatile { v = atomic::load(m_contents.m_version); }
	version_t get_version() const { return m_contents.m_version; }
	version_t get_version() const volatile { version_t tmp; get_version(tmp); return tmp; }

	void touch() { m_contents.m_version++; }
	void touch() volatile
	{
		version_t oldVersion;
		get_version(oldVersion);
		do { } while (!atomic::compare_exchange(m_contents.m_version, oldVersion + 1, oldVersion, oldVersion));
	}
	
	version_t set(const type& src)
	{
		m_contents.m_data = src;
		return ++(m_contents.m_version);
	}
	
	void unversioned_set(const type& src)
	{
		m_contents.m_data = src;
	}

	version_t set(const type& src) volatile
	{
		content_t oldContents;
		atomic::load(m_contents, oldContents);
		content_t newContents;
		newContents.m_data = src;
		do {
			newContents.m_version = oldContents.m_version + 1;
		} while (!atomic::compare_exchange(m_contents, newContents, oldContents, oldContents));
		return newContents.m_version;
	}

	version_t exchange(const type& src, type& rtn)
	{
		type tmp = m_contents.m_data;
		m_contents.m_data = src;
		rtn = tmp;
		return ++(m_contents.m_version);
	}

	void unversioned_exchange(const type& src, type& rtn)
	{
		type tmp = m_contents.m_data;
		m_contents.m_data = src;
		rtn = tmp;
	}

	version_t exchange(const type& src, type& rtn) volatile
	{
		content_t newContents;
		newContents.m_data = src;
		content_t oldContents;
		atomic::load(m_contents, oldContents);
		do {
			newContents.m_version = oldContents.m_version + 1;
		} while (!atomic::compare_exchange(m_contents, newContents, oldContents, oldContents));
		rtn = oldContents.m_data;
		return newContents.m_version;
	}

	bool versioned_exchange(const type& src, version_t& version)
	{
		bool b = (m_contents.m_version == version);
		if (b)
		{
			m_contents.m_data = src;
			m_contents.m_version++;
		}
		version = m_contents.m_version;
		return b;
	}

	bool versioned_exchange(const type& src, version_t& version) volatile	// always returns latest version #
	{
		content_t oldContents;
		atomic::load(m_contents, oldContents);
		bool b = (oldContents.m_version == version);
		if (!b)
			version = oldContents.m_version;
		else
		{
			content_t newContents;
			newContents.m_data = src;
			newContents.m_version = oldContents.m_version + 1;
			b = atomic::compare_exchange(m_contents, newContents, oldContents, oldContents);
			version = b ? newContents.m_version : oldContents.m_version;
		}
		return b;
	}

	
	bool versioned_exchange(const type& src, version_t& version, type& rtn)	// always returns latest version #
	{
		bool b = (m_contents.m_version == version);
		if (!b)
			rtn = m_contents.m_data;
		else
		{
			type tmp = m_contents.m_data;
			m_contents.m_data = src;
			rtn = tmp;
			m_contents.m_version++;
		}
		version = m_contents.m_version;
		return b;
	}

	bool versioned_exchange(const type& src, version_t& version, type& rtn) volatile	// always returns latest version #
	{
		content_t oldContents;
		atomic::load(m_contents, oldContents);
		bool b = (oldContents.m_version == version);
		if (!b)
			version = oldContents.m_version;
		else
		{
			content_t newContents;
			newContents.m_version = version + 1;
			newContents.m_data = src;
			b = atomic::compare_exchange(m_contents, newContents, oldContents, oldContents);
			version = b ? newContents.m_version : oldContents.m_version;
		}
		rtn = oldContents.m_data;
		return b;
	}


	bool compare_exchange(const type& src, const type& cmp)
	{
		bool b = m_contents.m_data == cmp;
		if (b)
		{
			m_contents.m_data = src;
			m_contents.m_version++;
		}
		return  b;
	}

	bool unversioned_compare_exchange(const type& src, const type& cmp)
	{
		bool b = m_contents.m_data == cmp;
		if (b)
			m_contents.m_data = src;
		return  b;
	}

	bool compare_exchange(const type& src, const type& cmp) volatile
	{
		content_t oldContents;
		content_t newContents;
		newContents.m_data = src;
		atomic::load(m_contents, oldContents);
		bool b;
		do {
			b = oldContents.m_data == cmp;
			if (!b)
				break;
			newContents.m_version = oldContents.m_version + 1;
			b = atomic::compare_exchange(m_contents, newContents, oldContents, oldContents);
		} while (!b);
		return b;
	}

	bool compare_exchange(const type& src, const type& cmp, type& rtn)
	{
		type tmp = m_contents.m_data;
		bool b = tmp == cmp;
		if (b)
		{
			m_contents.m_data = src;
			m_contents.m_version++;
		}
		rtn = tmp;
		return  b;
	}

	bool unversioned_compare_exchange(const type& src, const type& cmp, type& rtn)
	{
		type tmp = m_contents.m_data;
		bool b = tmp == cmp;
		if (b)
			m_contents.m_data = src;
		rtn = tmp;
		return  b;
	}

	bool compare_exchange(const type& src, const type& cmp, type& rtn) volatile
	{
		content_t oldContents;
		content_t newContents;
		newContents.m_data = src;
		atomic::load(m_contents, oldContents);
		bool b;
		do {
			b = oldContents.m_data == cmp;
			if (!b)
				break;
			newContents.m_version = oldContents.m_version + 1;
			b = atomic::compare_exchange(m_contents, newContents, oldContents, oldContents);
		} while (!b);
		rtn = oldContents.m_data;
		return b;
	}
};


}


#endif
