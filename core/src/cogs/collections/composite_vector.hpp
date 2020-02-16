//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Good, WorkInProgress

#ifndef COGS_HEADER_COLLECTION_COMPOSITE_ARRAY
#define COGS_HEADER_COLLECTION_COMPOSITE_ARRAY

#include <type_traits>

#include "cogs/collections/vector.hpp"
#include "cogs/math/const_max_int.hpp"
#include "cogs/sync/transactable.hpp"


namespace cogs {


template <typename T>
class composite_vector;


// forward declare
namespace io
{
	class composite_buffer;
}

template <typename T>
class composite_string_t;


template <typename T>
class composite_vector_content_t
{
private:
	typedef T type;
	typedef composite_vector_content_t<type> this_t;

	template <typename>
	friend class composite_vector;

	friend class io::composite_buffer;

	template <typename>
	friend class composite_string_t;

	vector<vector<type> > m_vectorVector;
	size_t m_length;

	composite_vector_content_t(const vector<vector<type> >& src, size_t n)
		: m_vectorVector(src),
		m_length(n)
	{ }

public:
	class position_t
	{
	private:
		template <typename>
		friend class composite_vector_content_t;

		template <typename>
		friend class composite_vector;

		size_t m_outerIndex;
		size_t m_innerIndex;

		size_t& get_outer_index() { return m_outerIndex; }
		size_t& get_inner_index() { return m_innerIndex; }

	public:
		position_t()
		{ }

		position_t(const position_t& pos)
			: m_outerIndex(pos.m_outerIndex),
			m_innerIndex(pos.m_innerIndex)
		{ }

		position_t(size_t outerIndex, size_t i)
			: m_outerIndex(outerIndex),
			m_innerIndex(i)
		{ }

		position_t& operator=(const position_t& pos)
		{
			m_outerIndex = pos.m_outerIndex;
			m_innerIndex = pos.m_innerIndex;
			return *this;
		}

		void set(size_t outerIndex, size_t i)
		{
			m_outerIndex = outerIndex;
			m_innerIndex = i;
		}

		bool operator==(const position_t& cmp) const
		{
			return (m_innerIndex == cmp.m_innerIndex) && (m_outerIndex == cmp.m_outerIndex);
		}

		bool operator!=(const position_t& cmp) const { return !operator==(cmp); }

		bool operator<(const position_t& cmp) const
		{
			return ((m_outerIndex < cmp.m_outerIndex) || ((m_outerIndex == cmp.m_outerIndex) && (m_innerIndex < cmp.m_innerIndex)));
		}

		bool operator>(const position_t& cmp) const
		{
			return ((m_outerIndex > cmp.m_outerIndex) || ((m_outerIndex == cmp.m_outerIndex) && (m_innerIndex > cmp.m_innerIndex)));
		}

		bool operator<=(const position_t& cmp) const { return !operator>(cmp); }
		bool operator>=(const position_t& cmp) const { return !operator<(cmp); }

		size_t get_outer_index() const { return m_outerIndex; }
		size_t get_inner_index() const { return m_innerIndex; }
	};

	size_t get_length() const { return m_length; }

	composite_vector_content_t()
		: m_length(0)
	{ }

	composite_vector_content_t(const this_t& src)
		: m_vectorVector(src.m_vectorVector),
		m_length(src.m_length)
	{ }

	composite_vector_content_t(this_t&& src)
		: m_vectorVector(std::move(src.m_vectorVector)),
		m_length(src.m_length)
	{ }

	template <typename type2>
	composite_vector_content_t(const vector<type2>& src)
		: m_length(src.get_length())
	{
		if (!!m_length)
			m_vectorVector.assign(1, src);
	}

	template <typename type2>
	composite_vector_content_t(const volatile vector<type2>& src)
	{
		vector<type> tmp(src);
		m_length = tmp.get_length();
		if (!!m_length)
			m_vectorVector.assign(1, tmp);
	}

	template <typename type2>
	composite_vector_content_t(const composite_vector_content_t<type2>& src, size_t srcIndex, size_t srcLength)
	{
		vector<type> tmp = src.template composite_as<type>(srcIndex, srcLength);
		m_length = tmp.get_length();
		if (!!m_length)
			m_vectorVector.assign(1, tmp);
	}

	template <typename type2>
	composite_vector_content_t(const composite_vector_content_t<type2>& src, const position_t& srcPos, size_t srcLength)
	{
		vector<type> tmp = src.template composite_as<type>(srcPos, srcLength);
		m_length = tmp.get_length();
		if (!!m_length)
			m_vectorVector.assign(1, tmp);
	}


	this_t& operator=(const this_t& src)
	{
		m_vectorVector = src.m_vectorVector;
		m_length = src.m_length;
		return *this;
	}

	this_t& operator=(this_t&& src)
	{
		m_vectorVector = std::move(src.m_vectorVector);
		m_length = src.m_length;
		return *this;
	}

	template <typename type2>
	this_t& operator=(const vector<type2>& src)
	{
		m_vectorVector.assign(1, src);
		m_length = src.get_length();
		return *this;
	}

	template <typename type2>
	this_t& operator=(const volatile vector<type2>& src)
	{
		vector<type> tmp(src);
		m_vectorVector.assign(1, tmp);
		m_length = tmp.get_length();
		return *this;
	}

	template <typename type2>
	void set(const vector<type2>& src, size_t length)
	{
		m_vectorVector.assign(1, src);
		m_length = length;
	}

	template <typename type2>
	void append(size_t n, const type2& src)
	{
		if (!!n)
		{
			if (!m_length)
				set(vector<type2>(n, src), n);
			else
			{
				m_length += n;
				vector<type>& innerVector = m_vectorVector.get_last();
				size_t n2 = innerVector.get_capacity() - innerVector.get_length();
				if (n2 > 0)
				{
					if (n2 > n)
						n2 = n;
					innerVector.append(n2, src);
					n -= n2;
				}
				if (n > 0)
					m_vectorVector.append(1, vector<type2>(n, src));
			}
		}
	}

	template <typename type2>
	void append(const type2* src, size_t n)
	{
		if (!!n)
		{
			if (!m_length)
				set(vector<type2>(src, n), n);
			else
			{
				m_length += n;
				vector<type>& innerVector = m_vectorVector.get_last();
				size_t n2 = innerVector.get_capacity() - innerVector.get_length();
				type2* pos = src;
				if (n2 > 0)
				{
					if (n2 > n)
						n2 = n;
					innerVector.append(n2, src);
					n -= n2;
				}
				if (n > 0)
					m_vectorVector.append(1, vector<type2>(src + n2, n));
			}
		}
	}

	void append(const type* src, size_t n)
	{
		if (!!n)
		{
			if (!m_length)
				set(vector<type>(src, n), n);
			else
			{
				m_length += n;
				vector<type> lastSubVector = m_vectorVector.get_const_ptr()[m_vectorVector.get_length() - 1];
				const type* basePtr = lastSubVector.get_ptr();
				if (src == basePtr + lastSubVector.get_length())
				{
					// Same buffer, let vector<>::prepend handle it accordingly
					lastSubVector.append(src, n);
				}
				else
				{
					vector<type>& innerVector = m_vectorVector.get_last();
					size_t n2 = innerVector.get_capacity() - innerVector.get_length();
					type* pos = src;
					if (n2 > 0)
					{
						if (n2 > n)
							n2 = n;
						innerVector.append(n2, src);
						n -= n2;
					}
					if (n > 0)
						m_vectorVector.append(1, vector<type>(src + n2, n));
				}
			}
		}
	}

	template <typename type2>
	void append(const vector<type2>& src)
	{
		size_t n = src.get_length();
		if (!!n)
		{
			m_length += n;
			m_vectorVector.append(1, src);
		}
	}

	void append(const vector<type>& src)
	{
		size_t n = src.get_length();
		if (!!n)
		{
			if (!m_length)
				set(src, n);
			else
			{
				m_length += n;
				vector<type>& lastSubVector = m_vectorVector.get_ptr()[m_vectorVector.get_length() - 1];
				const type* basePtr = lastSubVector.get_ptr();
				if (src.get_const_ptr() == basePtr + lastSubVector.get_length())
					lastSubVector.append(src);
				else
					m_vectorVector.append(1, src);
			}
		}
	}

	void append(const this_t& src)
	{
		size_t srcSubVectorCount = src.m_vectorVector.get_length();
		if (srcSubVectorCount == 1)
			append(src.m_vectorVector.get_const_ptr()[0]);
		else if (!!srcSubVectorCount)
		{
			m_vectorVector.append(src.m_vectorVector);
			m_length += src.m_length;
		}
	}

	template <typename type2>
	void append(const volatile vector<type2>& src)
	{
		vector<type2> tmp(src);
		append(tmp);
	}

	template <typename type2>
	void prepend(size_t n, const type2& src)
	{
		if (!!n)
		{
			if (!m_length)
				set(vector<type2>(n, src), n);
			else
			{
				m_length += n;
				vector<type>& innerVector = m_vectorVector.get_first();
				size_t n2 = innerVector.get_capacity() - innerVector.get_length();
				if (n2 > 0)
				{
					if (n2 > n)
						n2 = n;
					innerVector.prepend(n2, src);
					n -= n2;
				}
				if (n > 0)
					m_vectorVector.prepend(1, vector<type2>(n, src));
			}
		}
	}

	template <typename type2>
	void prepend(const type2* src, size_t n)
	{
		if (!!n)
		{
			if (!m_length)
				set(vector<type2>(n, src), n);
			else
			{
				m_length += n;
				vector<type>& innerVector = m_vectorVector.get_first();
				size_t remainingCapacity = innerVector.get_capacity() - innerVector.get_length();
				if (remainingCapacity > 0)
				{
					size_t n2 = remainingCapacity;
					if (n2 > n)
						n2 = n;
					innerVector.prepend(src + (n - n2), n2);
					n -= n2;
				}
				if (n > 0)
					m_vectorVector.prepend(1, vector<type2>(src, n));
			}
		}
	}

	void prepend(const type* src, size_t n)
	{
		if (!!n)
		{
			if (!m_length)
				set(vector<type>(n, src), n);
			else
			{
				m_length += n;
				vector<type>& firstSubVector = m_vectorVector.get_first();
				const type* basePtr = firstSubVector.get_const_ptr();
				if (src == basePtr - n)
				{
					// Same buffer, let vector<>::prepend handle it accordingly
					firstSubVector.prepend(src, n);
				}
				else
				{
					vector<type>& innerVector = m_vectorVector.get_first();
					size_t remainingCapacity = innerVector.get_capacity() - innerVector.get_length();
					if (remainingCapacity > 0)
					{
						size_t n2 = remainingCapacity;
						if (n2 > n)
							n2 = n;
						innerVector.prepend(src + (n - n2), n2);
						n -= n2;
					}
					if (n > 0)
						m_vectorVector.prepend(1, vector<type>(src, n));
				}
			}
		}
	}
	template <typename type2>
	void prepend(const vector<type2>& src)
	{
		size_t n = src.get_length();
		if (!!n)
		{
			m_length += n;
			m_vectorVector.prepend(1, src);
		}
	}

	void prepend(const vector<type>& src)
	{
		size_t n = src.get_length();
		if (!!n)
		{
			if (!m_length)
				set(src, n);
			else
			{
				m_length += n;
				vector<type>& firstSubVector = m_vectorVector.get_first();
				const type* basePtr = firstSubVector.get_ptr();
				if (src.get_const_ptr() == basePtr - n)
					firstSubVector.prepend(src);
				else
					m_vectorVector.prepend(1, src);
			}
		}
	}

	void prepend(const this_t& src)
	{
		size_t srcSubVectorCount = src.m_vectorVector.get_length();
		if (srcSubVectorCount == 1)
			prepend(src.m_vectorVector.get_const_ptr()[0]);
		else if (!!srcSubVectorCount)
		{
			m_vectorVector.prepend(src.m_vectorVector);
			m_length += src.m_length;
		}
	}

	template <typename type2>
	void prepend(const volatile vector<type2>& src)
	{
		vector<type2> tmp(src);
		prepend(tmp);
	}

	void erase(size_t i, size_t n)
	{
		size_t eraseLength = validate_length_from(i, n);
		if (!!eraseLength)
			erase(calculate_position(i), eraseLength);
	}

	void erase(const position_t& pos, size_t n)
	{
		if (!!n)
		{
			size_t outerIndex = pos.m_outerIndex;
			size_t subVectorCount = m_vectorVector.get_length();
			if (outerIndex >= subVectorCount)
				return;
			vector<type>* arrayArray = m_vectorVector.get_non_const_ptr(); // copy on write
			if (pos.m_innerIndex > 0)
			{
				vector<type>& subArray = arrayArray[outerIndex];
				size_t innerIndex = pos.m_innerIndex;
				size_t subVectorLength = subArray.get_length();
				if (innerIndex < subVectorLength)
				{
					size_t remainingLength = subVectorLength - innerIndex;
					size_t curEraseLength = remainingLength;
					if (curEraseLength > n)
						curEraseLength = n;
					subArray.erase(innerIndex, curEraseLength);
					m_length -= curEraseLength;
					n -= curEraseLength;
					if (!n)
						return;
				}
				++outerIndex;
			}

			size_t dstSubVectorIndex = outerIndex;
			for (;;)
			{
				vector<type>& subArray = arrayArray[outerIndex];
				size_t subVectorLength = subArray.get_length();
				if (subVectorLength > n)
				{
					subArray.erase(0, n);
					m_length -= n;
					break;
				}

				m_length -= subVectorLength;
				n -= subVectorLength;
				if (!n)
					break;
				if (++outerIndex == subVectorCount)
					return;
			}

			size_t removedVectorCount = outerIndex - dstSubVectorIndex;
			arrayArray->erase(dstSubVectorIndex, removedVectorCount);
		}
	}

	void clear()
	{
		m_vectorVector.clear();
		m_length = 0;
	}

	void reset()
	{
		m_vectorVector.reset();
		m_length = 0;
	}

	void swap(this_t& wth)
	{
		m_vectorVector.swap(wth.m_vectorVector);
		cogs::swap(m_length, wth.m_length);
	}

	void exchange(const this_t& src, this_t& rtn)
	{
		m_vectorVector.exchange(src.m_vectorVector, rtn.m_vectorVector);
		cogs::exchange(m_length, src.m_length, rtn.m_length);
	}


	void advance_arrays(size_t n)
	{
		if (!!n)
		{
			if (n >= m_vectorVector.get_length())
				clear();
			else
			{
				for (size_t i = 0; i < n; i++)
					m_length -= m_vectorVector.get_const_ptr()[i].get_length();
				m_vectorVector.set_to_subrange(n);
			}
		}
	}

	void truncate_arrays(size_t n)
	{
		if (!!n)
		{
			if (n >= m_vectorVector.get_length())
				clear();
			else
			{
				for (size_t i = n; !!i;)
					m_length -= m_vectorVector.get_const_ptr()[--i].get_length();
				m_vectorVector.truncate(n);
			}
		}
	}

	void truncate_arrays_to(size_t n)
	{
		if (!n)
			clear();
		else
		{
			if (n < m_vectorVector.get_length())
			{
				for (size_t i = n; i < m_vectorVector.get_length(); i++)
					m_length -= m_vectorVector.get_const_ptr()[i].get_length();
				m_vectorVector.truncate_to(n);
			}
		}
	}

	vector<type> pop_first_array()
	{
		vector<type> result;
		if (!!m_length)
		{
			result = m_vectorVector.get_const_ptr()[0];
			m_length -= result.get_length();
			m_vectorVector.advance(1);
		}
		return result;
	}

	vector<type> pop_last_array()
	{
		vector<type> result;
		size_t arrayArrayLength = m_vectorVector.get_length();
		if (!!arrayArrayLength)
		{
			result = m_vectorVector.get_const_ptr()[arrayArrayLength - 1];
			m_length -= result.get_length();
			m_vectorVector.truncate(1);
		}
		return result;
	}

	void split_arrays_fix(this_t& dst)
	{
		size_t newVectorLength = m_vectorVector.get_length();
		size_t resultVectorLength = dst.m_vectorVector.get_length();
		if (resultVectorLength <= newVectorLength) // slight optimization to count the shorter side
		{
			size_t resultLength = 0;
			for (size_t i = 0; i < resultVectorLength; i++)
				resultLength += dst.m_vectorVector.get_const_ptr()[i].get_length();
			dst.m_length = resultLength;
			m_length -= resultLength;
		}
		else
		{
			size_t newLength = 0;
			for (size_t i = 0; i < newVectorLength; i++)
				newLength += m_vectorVector.get_const_ptr()[i].get_length();
			dst.m_length = m_length - newLength;
			m_length = newLength;
		}
	}

	void split_off_arrays_before(size_t i, this_t& dst)
	{
		dst.m_vectorVector = m_vectorVector.split_off_before(i);
		split_arrays_fix(dst);
	}

	void split_off_arrays_after(size_t i, this_t& dst)
	{
		dst.m_vectorVector = m_vectorVector.split_off_after(i);
		split_arrays_fix(dst);
	}

	// caller error to call with i > length
	position_t calculate_position_from_front(size_t i) const
	{
		position_t result(0, 0);
		if (!!i)
		{
			if (i == m_length)
				result = get_end_position();
			else
			{
				result.m_innerIndex = 0;
				result.m_outerIndex = 0;
				size_t cumulativeLength = 0;
				size_t subVectorCount = m_vectorVector.get_length();
				for (size_t outerIndex = 0; outerIndex < subVectorCount; outerIndex++)
				{
					size_t curLength = m_vectorVector.get_const_ptr()[outerIndex].get_length();
					size_t prevCumulativeLength = cumulativeLength;
					cumulativeLength += curLength;
					if (i > cumulativeLength)
						continue;
					if (i == cumulativeLength)
					{
						result.m_outerIndex = outerIndex + 1;
						break;
					}
					result.m_outerIndex = outerIndex;
					result.m_innerIndex = i - prevCumulativeLength;
					break;
				}
			}
		}

		return result;
	}

	// caller error to call with i > length
	position_t calculate_position_from_back(size_t i) const
	{
		position_t result(0, 0);
		size_t remaining = m_length;
		if (!!i)
		{
			size_t subVectorCount = m_vectorVector.get_length();
			for (size_t outerIndex = subVectorCount; outerIndex > 0;)
			{
				--outerIndex;
				size_t curLength = m_vectorVector.get_const_ptr()[outerIndex].get_length();
				remaining -= curLength;
				if (i >= remaining)
				{
					result.m_outerIndex = outerIndex;
					result.m_innerIndex = i - remaining;
				}
			}
		}

		return result;
	}

	// caller error to call with i >= length
	position_t calculate_position(size_t i, bool countFromFront = true) const
	{
		if (countFromFront)
			return calculate_position_from_front(i);
		return calculate_position_from_back(i);
	}

	void split_off_back(size_t n, this_t& dst)
	{
		if (n < m_length)
			split_off_after(m_length - n, dst);
		else
		{
			dst = *this;
			clear();
		}
	}

	void split_off_before(size_t i, this_t& dst)
	{
		if (!!i)
		{
			if (i >= m_length)
			{
				dst = *this;
				clear();
			}
			else
			{
				position_t pos = calculate_position(i);
				split_off_before_inner(pos, dst);
				dst.m_length = i;
				m_length -= i;
			}
		}
	}

	void split_off_after(size_t i, this_t& dst)
	{
		if (i < m_length)
		{
			if (!i)
			{
				dst = *this;
				clear();
			}
			else
			{
				position_t pos = calculate_position(i);
				split_off_after_inner(pos, dst);
				dst.m_length = m_length - i;
				m_length = i;
			}
		}
	}

	void split_off_before_inner(const position_t& pos, this_t& dst)
	{
		if (!pos.m_innerIndex)
			dst.m_vectorVector = m_vectorVector.split_off_before(pos.m_outerIndex);
		else
		{
			dst.m_vectorVector = m_vectorVector; // Copy whole vector.  A 1-level deep copy will need to take place when modified.
			dst.m_vectorVector.truncate_to(pos.m_outerIndex + 1);
			dst.m_vectorVector.get_ptr()[pos.m_outerIndex].truncate_to(pos.m_innerIndex);
			m_vectorVector.advance(pos.m_outerIndex);
			m_vectorVector.get_first().advance(pos.m_innerIndex);
		}
	}

	void split_off_before(const position_t& pos, this_t& dst)
	{
		if (pos > position_t(0, 0))
		{
			if (pos >= get_end_position())
			{
				dst = *this;
				clear();
			}
			else
			{
				split_off_before_inner(pos, dst);
				split_arrays_fix(dst);
			}
		}
	}

	void split_off_after_inner(const position_t& pos, this_t& dst)
	{
		if (!pos.m_innerIndex)
			dst.m_vectorVector = m_vectorVector.split_off_after(pos.m_outerIndex);
		else
		{
			dst.m_vectorVector = m_vectorVector; // Copy whole vector.  A 1-level deep copy will need to take place when modified.
			m_vectorVector.truncate_to(pos.m_outerIndex + 1);
			m_vectorVector.get_ptr()[pos.m_outerIndex].truncate_to(pos.m_innerIndex);
			dst.m_vectorVector.advance(pos.m_outerIndex);
			dst.m_vectorVector.get_first().advance(pos.m_innerIndex);
		}
	}

	void split_off_after(const position_t& pos, this_t& dst)
	{
		if (pos >= get_end_position())
		{
			if (pos == position_t(0, 0))
			{
				dst = *this;
				clear();
			}
			else
			{
				split_off_after_inner(pos, dst);
				split_arrays_fix(dst);
			}
		}
	}

	void set_to_subrange(size_t i)
	{
		if (!!i)
		{
			if (i >= m_length)
				clear();
			else
			{
				position_t pos = calculate_position(i);
				m_vectorVector.advance(pos.m_outerIndex);
				if (!!pos.m_innerIndex)
					m_vectorVector.get_first().advance(pos.m_innerIndex);
				m_length -= i;
			}
		}
	}

	void set_to_subrange(size_t i, size_t n)
	{
		if (!n || (i >= m_length))
			clear();
		else
		{
			set_to_subrange(i);
			truncate_to(n);
		}
	}

	void set_to_subrange(const position_t& start)
	{
		if (start > position_t(0, 0))
		{
			if (start >= get_end_position())
				clear();
			else
			{
				size_t vacatedLength = 0;
				if (start.m_outerIndex > 0)
				{
					for (size_t i = 0; i < start.m_outerIndex; i++)
						vacatedLength += m_vectorVector.get_const_ptr()[0].get_length();
					m_vectorVector.advance(start.m_outerIndex);
				}
				if (start.m_innerIndex > 0)
				{
					m_vectorVector.get_first().advance(start.m_innerIndex);
					vacatedLength += start.m_innerIndex;
				}
				m_length -= vacatedLength;
			}
		}
	}

	void set_to_subrange(const position_t& start, size_t n)
	{
		if (!n)
			clear();
		else
		{
			set_to_subrange(start);
			truncate_to(n);
		}
	}

	void set_to_subrange(const position_t& start, const position_t& end)
	{
		truncate_to(end);
		set_to_subrange(start);
	}

	position_t get_end_position() const
	{
		size_t length = m_vectorVector.get_length();
		if (!length)
			return position_t(0, 0);

		size_t lastSubVectorIndex = length - 1;
		return position_t(lastSubVectorIndex, m_vectorVector.get_const_ptr()[lastSubVectorIndex].get_length());
	}

	position_t get_last_position() const
	{
		size_t length = m_vectorVector.get_length();
		if (!length)
			return position_t(0, 0);

		size_t lastSubVectorIndex = length - 1;
		return position_t(lastSubVectorIndex, m_vectorVector.get_const_ptr()[lastSubVectorIndex].get_length() - 1);
	}

	position_t get_next_position(const position_t& pos) const
	{
		size_t subVectorCount = m_vectorVector.get_length();
		if (subVectorCount == 0)
			return position_t(0, 0);

		if (pos.m_outerIndex >= subVectorCount)
		{
			size_t lastSubVectorIndex = subVectorCount - 1;
			return position_t(lastSubVectorIndex, m_vectorVector.get_const_ptr()[lastSubVectorIndex].get_length());
		}

		const vector<type>& subArray = m_vectorVector.get_const_ptr()[pos.m_outerIndex];
		size_t subVectorLength = subArray.get_length();
		position_t result = pos;
		if (++result.m_innerIndex >= subVectorLength)
		{
			if (result.m_outerIndex == subVectorCount - 1)
				result.m_innerIndex = subVectorLength;
			else
			{
				++result.m_outerIndex;
				result.m_innerIndex = 0;
			}
		}

		return result;
	}

	// Caller error to call with (pos == 0, 0)
	position_t get_prev_position(const position_t& pos) const
	{
		size_t subVectorCount = m_vectorVector.get_length();
		if (pos.m_outerIndex >= subVectorCount)
			return position_t(0, 0);

		if (pos.m_innerIndex > 0)
			return position_t(pos.m_outerIndex, pos.m_innerIndex - 1);

		COGS_ASSERT(pos.m_outerIndex > 0);
		size_t prevSubVectorIndex = pos.m_outerIndex - 1;
		return position_t(prevSubVectorIndex, m_vectorVector.get_const_ptr()[prevSubVectorIndex].get_length() - 1);
	}

	void advance(size_t n)
	{
		if (!!n)
		{
			if (n >= m_length)
				clear();
			else
			{
				position_t pos = calculate_position(n);
				m_vectorVector.advance(pos.m_outerIndex);
				if (!!pos.m_innerIndex)
					m_vectorVector.get_first().advance(pos.m_innerIndex);
				m_length -= n;
			}
		}
	}

	void advance_to(const position_t& pos) { set_to_subrange(pos); }

	void truncate(size_t n)
	{
		if (n >= m_length)
			clear();
		else
			truncate_to(m_length - n);
	}

	void truncate_to(size_t n)
	{
		if (!n)
			clear();
		else if (n < m_length)
		{
			position_t pos = calculate_position(n);
			size_t truncateTo = pos.m_outerIndex;
			if (!!pos.m_innerIndex)
				++truncateTo;
			m_vectorVector.truncate_to(truncateTo);
			if (!!pos.m_innerIndex)
				m_vectorVector.get_ptr()[pos.m_outerIndex].truncate_to(pos.m_innerIndex);
			m_length = n;
		}
	}

	void truncate_to(const position_t& pos)
	{
		if (pos == position_t(0, 0))
			clear();
		else if (pos < get_end_position())
		{
			size_t truncateToSubVectorIndex = pos.m_outerIndex;
			if (pos.m_innerIndex > 0)
				truncateToSubVectorIndex++;
			m_vectorVector.truncate_to(truncateToSubVectorIndex);
			if (pos.m_innerIndex > 0)
				m_vectorVector.get_ptr()[pos.m_outerIndex].truncate_to(pos.m_innerIndex);
			recount();
		}
	}

	void truncate_to_right(size_t n)
	{
		if (!n)
			clear();
		else if (n < m_length)
			advance(m_length - n);
	}

	void truncate_to_right(const position_t& pos)
	{
		set_to_subrange(pos);
	}

	void recount()
	{
		m_length = 0;
		for (size_t outerIndex = 0; outerIndex < m_vectorVector.get_length(); outerIndex++)
			m_length += m_vectorVector.get_const_ptr()[outerIndex].get_length();
	}

	vector<type> composite() const
	{
		size_t arrayArrayLength = m_vectorVector.get_length();
		if (arrayArrayLength == 1)
			return m_vectorVector.get_const_ptr()[0];
		return composite_as<type>();
	}

	vector<type> composite(size_t i, size_t n) const
	{
		size_t arrayArrayLength = m_vectorVector.get_length();
		if (arrayArrayLength == 1)
			return m_vectorVector.get_const_ptr()->subrange(i, n);
		return composite_as<type>(i, n);
	}

	vector<type> composite(const position_t& pos, size_t n) const
	{
		size_t arrayArrayLength = m_vectorVector.get_length();
		if (arrayArrayLength == 1)
		{
			if (pos.m_outerIndex != 0)
			{
				vector<type> result;
				return result;
			}
			return m_vectorVector.get_const_ptr()->subrange(pos.m_innerIndex, n);
		}
		return composite_as<type>(pos, n);
	}

	template <typename type2>
	vector<type2> composite_as() const
	{
		vector<type2> result;
		result.reserve(m_length);
		size_t arrayArrayLength = m_vectorVector.get_length();
		for (size_t arrayArrayIndex = 0; arrayArrayIndex < arrayArrayLength; arrayArrayIndex++)
			result.append(m_vectorVector.get_const_ptr()[arrayArrayIndex]);
		return result;
	}

	template <typename type2>
	vector<type2> composite_as(size_t i, size_t n) const
	{
		vector<type2> result;
		size_t lengthAdjusted = validate_length_from(i, n);
		if (!!lengthAdjusted)
		{
			result.reserve(lengthAdjusted);
			position_t pos = calculate_position(i);
			for (;;)
			{
				const vector<type>& subArray = m_vectorVector.get_const_ptr()[pos.m_outerIndex];
				vector<type> tmp = subArray.subrange(pos.m_innerIndex, lengthAdjusted);
				result.append(tmp);
				lengthAdjusted -= tmp.get_length();
				if (!lengthAdjusted)
					break;
				pos.m_outerIndex++;
				pos.m_innerIndex = 0;
			}
		}
		return result;
	}

	template <typename type2>
	vector<type2> composite_as(const position_t& pos, size_t n) const
	{
		vector<type2> result;
		size_t lengthAdjusted = validate_length_from(pos, n);
		if (!!lengthAdjusted)
		{
			result.reserve(lengthAdjusted);
			position_t pos2 = pos;
			for (;;)
			{
				const vector<type>& subArray = m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
				vector<type> tmp = subArray.subrange(pos2.m_innerIndex, lengthAdjusted);
				result.append(tmp);
				lengthAdjusted -= tmp.get_length();
				if (!lengthAdjusted)
					break;
				pos2.m_outerIndex++;
				pos2.m_innerIndex = 0;
			}
		}
		return result;
	}

	size_t validate_length_from(size_t index, size_t proposedLength) const
	{
		size_t lengthAdjusted = 0;
		if (m_length > index)
		{
			lengthAdjusted = m_length - index;
			if (lengthAdjusted > proposedLength)
				lengthAdjusted = proposedLength;
		}

		return lengthAdjusted;
	}

	// fromPos must be valid
	size_t validate_length_from(const position_t& fromPos, size_t proposedLength) const
	{
		size_t remaining = m_length - calculate_index(fromPos);
		return (remaining < proposedLength) ? remaining : proposedLength;
	}

	size_t calculate_index(const position_t& pos) const
	{
		size_t outerIndex = pos.m_outerIndex;
		size_t subVectorCount = m_vectorVector.get_length();
		size_t n = 0;
		if (outerIndex <= subVectorCount / 2)
		{
			// Count sub arrays before
			size_t curSubVectorInfo = 0;
			while (curSubVectorInfo < outerIndex)
			{
				n += m_vectorVector.get_const_ptr()[outerIndex].get_length();
				++curSubVectorInfo;
			}
			n += pos.m_innerIndex;
		}
		else
		{
			// Count sub arrays after
			while (outerIndex < subVectorCount)
			{
				n += m_vectorVector.get_const_ptr()[outerIndex].get_length();
				++outerIndex;
			}
			n -= pos.m_innerIndex;
		}

		return n;
	}

	template <class comparator_t>
	class compare_helper
	{
	public:
		template <typename type2>
		static bool equals(const this_t& v, size_t i, size_t n, const vector<type2>& cmp)
		{
			size_t lengthAdjusted = v.validate_length_from(i, n);
			size_t cmpLength = cmp.get_length();
			if (lengthAdjusted != cmpLength)
				return false;
			if (!lengthAdjusted)
				return true;
			return equals_inner(v, v.calculate_position(i), cmp);
		}

		template <typename type2>
		static bool equals(const this_t& v, size_t i, size_t n, const composite_vector_content_t<type2>& cmp, size_t cmpIndex, size_t cmpLength)
		{
			size_t lengthAdjusted = v.validate_length_from(i, n);
			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
			if (lengthAdjusted != cmpLengthAdjusted)
				return false;
			if (!lengthAdjusted)
				return true;
			position_t endPos;
			return equals_inner(v, v.calculate_position(i), cmp, cmp.calculate_position(cmpIndex), cmpLengthAdjusted, endPos);
		}

		// pos must be valid.  Therefor, actual length is assumed to be >0
		template <typename type2>
		static bool equals(const this_t& v, const position_t& pos, size_t n, const vector<type2>& cmp)
		{
			size_t cmpLength = cmp.get_length();
			if (!cmpLength)
				return !n;
			size_t lengthAdjusted = v.validate_length_from(pos, n);
			if (lengthAdjusted != cmpLength)
				return false;
			return equals_inner(v, pos, cmp);
		}

		// pos must be valid.  Therefor, actual length is assumed to be >0
		template <typename type2>
		static bool equals(const this_t& v, const position_t& pos, size_t n, const composite_vector_content_t<type2>& cmp, size_t cmpIndex, size_t cmpLength)
		{
			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
			if (!cmpLengthAdjusted)
				return !n;
			size_t lengthAdjusted = v.validate_length_from(pos, n);
			if (lengthAdjusted != cmpLengthAdjusted)
				return false;
			position_t endPos;
			return equals_inner(v, pos, cmp, cmp.calculate_position(cmpIndex), cmpLengthAdjusted, endPos);
		}

		// pos and cmpPos must be valid.  Therefor, actual lengths are assumed to be >0
		template <typename type2>
		static bool equals(const this_t& v, const position_t& pos, size_t n, const composite_vector_content_t<type2>& cmp, const position_t& cmpPos, size_t cmpLength)
		{
			size_t lengthAdjusted = v.validate_length_from(pos, n);
			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
			if (lengthAdjusted != cmpLengthAdjusted)
				return false;
			if (!lengthAdjusted)
				return true;
			position_t endPos;
			return equals_inner(v, pos, cmp, cmpPos, cmpLengthAdjusted, endPos);
		}

		// cmpPos must be valid.  Therefor, actual length is assumed to be >0
		template <typename type2>
		static bool equals(const this_t& v, size_t i, size_t n, const composite_vector_content_t<type2>& cmp, const position_t& cmpPos, size_t cmpLength)
		{
			size_t lengthAdjusted = v.validate_length_from(i, n);
			if (!lengthAdjusted)
				return !cmpLength;
			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
			if (lengthAdjusted != cmpLengthAdjusted)
				return false;
			position_t endPos;
			return equals_inner(v, v.calculate_position(i), cmp, cmpPos, cmpLengthAdjusted, endPos);
		}

		// pos, cmpIndex, lengthAdjust and cmpLengthAdjusted must all be valid.
		template <typename type2>
		static bool equals_inner(const this_t& v, const position_t& pos, const vector<type2>& cmp)
		{
			size_t cmpIndex = 0;
			position_t pos2 = pos;
			size_t cmpLength = cmp.get_length();
			for (;;)
			{
				const vector<type>& currentSubVector = v.m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
				size_t subVectorLength = currentSubVector.get_length();
				size_t compareLengthThisSegment = subVectorLength - pos2.m_innerIndex;
				if (compareLengthThisSegment > cmpLength)
					compareLengthThisSegment = cmpLength;
				if (!currentSubVector.subrange(pos2.m_innerIndex).template equals<type2, comparator_t>(cmp.subrange(cmpIndex, compareLengthThisSegment)))
					return false;
				if (++pos2.m_outerIndex == v.m_vectorVector.get_length())
					return true;
				cmpIndex += compareLengthThisSegment;
				cmpLength -= compareLengthThisSegment;
				pos2.m_innerIndex = 0;
			}
		}

		// pos and cmpIndex must be valid.  lengthAdjust and cmpLengthAdjusted may extend beyond range, but must be > 0
		// endPos will be set only on success, and may require advancing to the next sub array.
		template <typename type2>
		static bool equals_inner(const this_t& v, const position_t& pos, const composite_vector_content_t<type2>& cmp, const position_t& cmpPos, size_t cmpLengthAdjusted, position_t& endPos)
		{
			position_t pos2 = pos;
			position_t cmpPos2 = cmpPos;
			const vector<type>* currentSubVector = &v.m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
			const vector<type2>* currentCmpSubVector = &cmp.m_vectorVector.get_const_ptr()[cmpPos2.m_outerIndex];
			size_t subVectorLength = currentSubVector->get_length() - pos2.m_innerIndex;
			size_t cmpSubVectorLength = currentCmpSubVector->get_length() - cmpPos2.m_innerIndex;
			for (;;)
			{
				bool isShort = (subVectorLength < cmpSubVectorLength);
				bool isCmpShort = (subVectorLength > cmpSubVectorLength);
				size_t compareLengthThisSegment = isShort ? subVectorLength : cmpSubVectorLength;
				if (compareLengthThisSegment > cmpLengthAdjusted)
					compareLengthThisSegment = cmpLengthAdjusted;
				if (!currentSubVector->subrange(pos2.m_innerIndex).template equals<type2, comparator_t>(currentCmpSubVector->subrange(cmpPos2.m_innerIndex, compareLengthThisSegment)))
					return false;
				cmpLengthAdjusted -= compareLengthThisSegment;
				if (!cmpLengthAdjusted)
				{
					endPos = pos2;
					endPos.m_innerIndex += compareLengthThisSegment;
					return true;
				}
				if (isShort)
				{
					if (!isCmpShort)
					{
						cmpPos2.m_innerIndex += compareLengthThisSegment;
						cmpSubVectorLength -= compareLengthThisSegment;
					}
					pos2.m_innerIndex = 0;
					++pos2.m_outerIndex;
					currentSubVector = &v.m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
					subVectorLength = currentSubVector->get_length() - pos2.m_innerIndex;
				}
				if (isCmpShort)
				{
					if (!isShort)
					{
						pos2.m_innerIndex += compareLengthThisSegment;
						subVectorLength -= compareLengthThisSegment;
					}
					cmpPos2.m_innerIndex = 0;
					++cmpPos2.m_outerIndex;
					currentCmpSubVector = &cmp.m_vectorVector.get_const_ptr()[cmpPos2.m_outerIndex];
					cmpSubVectorLength = currentCmpSubVector->get_length();
				}
			}
		}


		template <typename type2>
		static bool ends_with(const this_t& v, const vector<type2>& cmp)
		{
			size_t cmpLength = cmp.get_length();
			if (!cmpLength)
				return true;
			size_t length = v.get_length();
			if (length < cmpLength)
				return false;
			return equals_inner(v, v.calculate_position(length - cmpLength), cmp);
		}

		template <typename type2>
		static bool ends_with(const this_t& v, const composite_vector_content_t<type2>& cmp, size_t cmpIndex, size_t cmpLength)
		{
			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
			if (!cmpLengthAdjusted)
				return true;
			size_t length = v.get_length();
			if (length < cmpLengthAdjusted)
				return false;
			position_t endPos;
			return equals_inner(v, v.calculate_position(length - cmpLengthAdjusted), cmp, cmp.calculate_position(cmpIndex), cmpLengthAdjusted, endPos);
		}

		// cmpPos must be valid.  Therefor, actual length is assumed to be >0
		template <typename type2>
		static bool ends_with(const this_t& v, const composite_vector_content_t<type2>& cmp, const position_t& cmpPos, size_t cmpLength)
		{
			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
			if (!cmpLengthAdjusted)
				return true;
			size_t length = v.get_length();
			if (length < cmpLengthAdjusted)
				return false;
			position_t endPos;
			return equals_inner(v, v.calculate_position(length - cmpLengthAdjusted), cmp, cmpPos, cmpLengthAdjusted, endPos);
		}


		template <typename type2>
		static int compare(const this_t& v, size_t i, size_t n, const vector<type2>& cmp)
		{
			size_t lengthAdjusted = v.validate_length_from(i, n);
			if (!lengthAdjusted)
				return !cmp.get_length() ? 0 : -1;
			return compare_inner(v, v.calculate_position(i), lengthAdjusted, cmp);
		}

		template <typename type2>
		static int compare(const this_t& v, size_t i, size_t n, const composite_vector_content_t<type2>& cmp, size_t cmpIndex, size_t cmpLength)
		{
			size_t lengthAdjusted = v.validate_length_from(i, n);
			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
			if (!lengthAdjusted)
				return !cmpLengthAdjusted ? 0 : -1;
			if (!cmpLengthAdjusted)
				return 1;
			return compare_inner(v, v.calculate_position(i), lengthAdjusted, cmp, cmp.calculate_position(cmpIndex), cmpLengthAdjusted);
		}

		template <typename type2>
		static int compare(const this_t& v, const position_t& pos, size_t n, const vector<type2>& cmp)
		{
			size_t lengthAdjusted = v.validate_length_from(pos, n);
			size_t cmpLength = cmp.get_length();
			if (!lengthAdjusted)
				return !cmpLength ? 0 : -1;
			if (!cmpLength)
				return 1;
			return compare_inner(v, pos, lengthAdjusted, cmp);
		}

		template <typename type2>
		static int compare(const this_t& v, const position_t& pos, size_t n, const composite_vector_content_t<type2>& cmp, size_t cmpIndex, size_t cmpLength)
		{
			size_t lengthAdjusted = v.validate_length_from(pos, n);
			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
			if (!lengthAdjusted)
				return !cmpLength ? 0 : -1;
			if (!cmpLengthAdjusted)
				return 1;
			return compare_inner(v, pos, n, cmp, cmp.calculate_position(cmpIndex), cmpLengthAdjusted);
		}

		template <typename type2>
		static int compare(const this_t& v, const position_t& pos, size_t n, const composite_vector_content_t<type2>& cmp, const position_t& cmpPos, size_t cmpLength)
		{
			size_t lengthAdjusted = v.validate_length_from(pos, n);
			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
			if (!lengthAdjusted)
				return !cmpLength ? 0 : -1;
			if (!cmpLengthAdjusted)
				return 1;
			return compare_inner(v, pos, lengthAdjusted, cmp, cmpPos, cmpLengthAdjusted);
		}

		template <typename type2>
		static int compare(const this_t& v, size_t i, size_t n, const composite_vector_content_t<type2>& cmp, const position_t& cmpPos, size_t cmpLength)
		{
			size_t lengthAdjusted = v.validate_length_from(i, n);
			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
			if (!lengthAdjusted)
				return !cmpLength ? 0 : -1;
			if (!cmpLengthAdjusted)
				return 1;
			return compare_inner(v, v.calculate_position(i), lengthAdjusted, cmp, cmpPos, cmpLengthAdjusted);
		}

		// pos and cmpIndex must be valid.  lengthAdjust and cmpLengthAdjusted may extend beyond range
		template <typename type2>
		static int compare_inner(const this_t& v, const position_t& pos, size_t lengthAdjusted, const vector<type2>& cmp)
		{
			position_t pos2 = pos;
			size_t cmpIndex = 0;
			size_t cmpLength = cmp.get_length();
			for (;;)
			{
				const vector<type>& currentSubVector = v.m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
				size_t subVectorLength = currentSubVector.get_length();
				size_t compareLengthThisSegment = subVectorLength - pos2.m_innerIndex;
				if (compareLengthThisSegment > cmpLength)
					compareLengthThisSegment = cmpLength;
				if (compareLengthThisSegment > lengthAdjusted)
					compareLengthThisSegment = lengthAdjusted;
				int i = currentSubVector.subrange(pos2.m_innerIndex).template compare<type2, comparator_t>(cmp.subrange(cmpIndex, compareLengthThisSegment));
				if (i != 0)
					return i;
				if (++pos2.m_outerIndex == v.m_vectorVector.get_length())
					lengthAdjusted = 0;
				else
					lengthAdjusted -= compareLengthThisSegment;
				cmpLength -= compareLengthThisSegment;
				if (cmpLength == 0)
					return !lengthAdjusted ? 0 : 1;
				if (!lengthAdjusted)
					return -1;
				cmpIndex += compareLengthThisSegment;
				pos2.m_innerIndex = 0;
			}
		}

		// pos and cmpPos must be valid.  lengthAdjust and cmpLengthAdjusted may extend beyond range
		template <typename type2>
		static int compare_inner(const this_t& v, const position_t& pos, size_t lengthAdjusted, const composite_vector_content_t<type2>& cmp, const position_t& cmpPos, size_t cmpLengthAdjusted)
		{
			position_t pos2 = pos;
			position_t cmpPos2 = cmpPos;
			const vector<type>* currentSubVector = &v.m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
			const vector<type2>* currentCmpSubVector = &cmp.m_vectorVector.get_const_ptr()[cmpPos2.m_outerIndex];
			size_t subVectorLength = currentSubVector->get_length() - pos2.m_innerIndex;
			size_t cmpSubVectorLength = currentCmpSubVector->get_length() - cmpPos2.m_innerIndex;
			for (;;)
			{
				bool isShort = (subVectorLength < cmpSubVectorLength);
				bool isCmpShort = (subVectorLength > cmpSubVectorLength);
				size_t compareLengthThisSegment = isShort ? subVectorLength : cmpSubVectorLength;
				if (compareLengthThisSegment > cmpLengthAdjusted)
					compareLengthThisSegment = cmpLengthAdjusted;
				if (compareLengthThisSegment > lengthAdjusted)
					compareLengthThisSegment = lengthAdjusted;
				int i = currentSubVector->subrange(pos2.m_innerIndex).template compare<type2, comparator_t>(currentCmpSubVector->subrange(cmpPos2.m_innerIndex, compareLengthThisSegment));
				if (i != 0)
					return i;

				cmpLengthAdjusted -= compareLengthThisSegment;
				lengthAdjusted -= compareLengthThisSegment;
				if (!cmpLengthAdjusted)
					return (!lengthAdjusted) ? 0 : 1;
				if (!lengthAdjusted)
					return -1;

				if (isShort)
				{
					if (!isCmpShort)
					{
						cmpPos2.m_innerIndex += compareLengthThisSegment;
						cmpSubVectorLength -= compareLengthThisSegment;
					}
					pos2.m_innerIndex = 0;
					if (++pos2.m_outerIndex == v.m_vectorVector.get_length())
						return !isCmpShort ? -1 : ((++cmpPos2.m_outerIndex == cmp.m_vectorVector.get_length()) ? 0 : 1);
					currentSubVector = &v.m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
					subVectorLength = currentSubVector->get_length() - pos2.m_innerIndex;
				}
				if (isCmpShort)
				{
					if (!isShort)
					{
						pos2.m_innerIndex += compareLengthThisSegment;
						subVectorLength -= compareLengthThisSegment;
					}
					cmpPos2.m_innerIndex = 0;
					if (++cmpPos2.m_outerIndex == cmp.m_vectorVector.get_length())
						return 1;
					currentCmpSubVector = &cmp.m_vectorVector.get_const_ptr()[cmpPos2.m_outerIndex];
					cmpSubVectorLength = currentCmpSubVector->get_length();
				}
			}
		}


		template <typename type2>
		static size_t index_of(const this_t& v, size_t i, size_t n, const type2& cmp)
		{
			size_t lengthAdjusted = v.validate_length_from(i, n);
			if (!lengthAdjusted || (i >= lengthAdjusted))
				return (size_t)-1;

			position_t pos = v.calculate_position(i);
			return find_inner(v, i, pos, lengthAdjusted, cmp);
		}


		template <typename type2>
		static position_t position_of(const this_t& v, const position_t& pos, size_t n, const type2& cmp)
		{
			size_t i = v.calculate_index(pos);
			size_t lengthAdjusted = v.validate_length_from(i, n);
			if (!lengthAdjusted || (i >= lengthAdjusted))
				return v.get_end_position();

			position_t pos2 = pos;
			find_inner(v, i, pos2, lengthAdjusted, cmp);
			return pos2;
		}


		template <typename type2>
		static size_t find_inner(const this_t& v, size_t i, position_t& pos, size_t n, const type2& cmp)
		{
			size_t subVectorCount = v.m_vectorVector.get_length();
			size_t curSize = i - pos.m_innerIndex;
			for (;;)
			{
				const vector<type>& curSubVector = v.m_vectorVector.get_const_ptr()[pos.m_outerIndex];
				pos.m_innerIndex = curSubVector.index_of(pos.m_innerIndex, cmp);
				if (pos.m_innerIndex != -1)
				{
					size_t foundIndex = curSize + pos.m_innerIndex;
					if (foundIndex >= n)
						return (size_t)-1;
					return foundIndex;
				}
				curSize += curSubVector.get_length();
				pos.m_innerIndex = 0;
				if (++pos.m_outerIndex == subVectorCount)
					return (size_t)-1;
			}
		}

		template <typename type2>
		static size_t index_of_any(const this_t& v, size_t i, size_t n, const type2* cmp, size_t cmpCount)
		{
			size_t lengthAdjusted = v.validate_length_from(i, n);
			if (!lengthAdjusted || (i >= lengthAdjusted))
				return (size_t)-1;

			position_t pos = v.calculate_position(i);
			return find_any_inner(v, i, pos, lengthAdjusted, cmp, cmpCount);
		}

		template <typename type2>
		static position_t position_of_any(const this_t& v, const position_t& pos, size_t n, const type2* cmp, size_t cmpCount)
		{
			size_t i = v.calculate_index(pos);
			size_t lengthAdjusted = v.validate_length_from(i, n);
			if (!lengthAdjusted || (i >= lengthAdjusted))
				return v.get_end_position();

			position_t pos2 = pos;
			find_any_inner(v, i, pos2, lengthAdjusted, cmp, cmpCount);
			return pos2;
		}

		template <typename type2>
		static size_t find_any_inner(const this_t& v, size_t i, position_t& pos, size_t n, const type2* cmp, size_t cmpCount)
		{
			size_t subVectorCount = v.m_vectorVector.get_length();
			size_t curSize = i - pos.m_innerIndex;
			for (;;)
			{
				const vector<type>& curSubVector = v.m_vectorVector.get_const_ptr()[pos.m_outerIndex];
				pos.m_innerIndex = curSubVector.index_of_any(pos.m_innerIndex, cmp, cmpCount);
				if (pos.m_innerIndex != -1)
				{
					size_t foundIndex = curSize + pos.m_innerIndex;
					if (foundIndex >= n)
						return (size_t)-1;
					return foundIndex;
				}
				curSize += curSubVector.get_length();
				pos.m_innerIndex = 0;
				if (++pos.m_outerIndex == subVectorCount)
					return (size_t)-1;
			}
		}


		template <typename type2>
		static size_t index_of_segment(const this_t& v, size_t i, size_t n, const vector<type2>& cmp)
		{
			if (i >= v.m_length)
				return const_max_int_v<size_t>;

			size_t cmpLength = cmp.get_length();
			if (!cmpLength)
				return const_max_int_v<size_t>; // validate cmpLength and cmdLengthAdjusted

			size_t lengthAdjusted = v.m_length - i;
			if (lengthAdjusted > n)
				lengthAdjusted = n;
			if (cmpLength > lengthAdjusted) // Ensure m_length is larger enough for cmp, and >0
				return const_max_int_v<size_t>;

			position_t pos = v.calculate_position(i);
			position_t endPos;
			return find_segment_inner(v, i, pos, lengthAdjusted, cmp, endPos);
		}

		template <typename type2>
		static position_t position_of_segment(const this_t& v, const position_t& pos, size_t n, const vector<type2>& cmp)
		{
			size_t i = v.calculate_index(pos);
			size_t cmpLength = cmp.get_length();
			if (!cmpLength)
				return v.get_end_position(); // validate cmpLength and cmdLengthAdjusted

			size_t lengthAdjusted = v.m_length - i;
			if (cmpLength > lengthAdjusted) // Ensure m_length is larger enough for cmp, and >0
				return v.get_end_position();

			position_t newPos = pos;
			position_t endPos;
			find_segment_inner(v, i, newPos, lengthAdjusted, cmp, endPos);
			return newPos;
		}

		template <typename type2>
		static size_t index_of_segment(const this_t& v, size_t i, size_t n, const composite_vector_content_t<type2>& cmp, size_t cmpIndex, size_t cmpLength)
		{
			if (i >= v.m_length)
				return const_max_int_v<size_t>;

			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
			if (!cmpLengthAdjusted)
				return const_max_int_v<size_t>; // validate cmpLength and cmdLengthAdjusted

			size_t lengthAdjusted = v.m_length - i;
			if (lengthAdjusted > n)
				lengthAdjusted = n;
			if (cmpLengthAdjusted > lengthAdjusted) // Ensure m_length is larger enough for cmp, and >0
				return const_max_int_v<size_t>;

			position_t pos = v.calculate_position(i);
			position_t cmpPos = cmp.calculate_position(cmpIndex);
			return find_segment_inner(v, i, pos, lengthAdjusted, cmp, cmpPos, cmpLengthAdjusted);
		}

		template <typename type2>
		static position_t position_of_segment(const this_t& v, position_t& pos, size_t n, const composite_vector_content_t<type2>& cmp, size_t cmpIndex, size_t cmpLength)
		{
			size_t i = v.calculate_index(pos);
			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
			if (!cmpLengthAdjusted)
				return v.get_end_position(); // validate cmpLength and cmdLengthAdjusted

			size_t lengthAdjusted = v.m_length - i;
			if (lengthAdjusted > n)
				lengthAdjusted = n;
			if (cmpLengthAdjusted > lengthAdjusted) // Ensure m_length is larger enough for cmp, and >0
				return v.get_end_position();

			position_t newPos = pos;
			position_t cmpPos = cmp.calculate_position(cmpIndex);
			find_segment_inner(v, i, newPos, lengthAdjusted, cmp, cmpPos, cmpLengthAdjusted);
			return newPos;
		}

		template <typename type2>
		static size_t index_of_segment(const this_t& v, size_t i, size_t n, const composite_vector_content_t<type2>& cmp, const position_t& cmpPos, size_t cmpLength)
		{
			if (i >= v.m_length)
				return const_max_int_v<size_t>;

			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
			if (!cmpLengthAdjusted)
				return const_max_int_v<size_t>; // validate cmpLength and cmdLengthAdjusted

			size_t lengthAdjusted = v.m_length - i;
			if (lengthAdjusted > n)
				lengthAdjusted = n;
			if (cmpLengthAdjusted > lengthAdjusted) // Ensure m_length is larger enough for cmp, and >0
				return const_max_int_v<size_t>;

			position_t pos = v.calculate_position(i);
			return find_segment_inner(v, i, pos, lengthAdjusted, cmp, cmpPos, cmpLengthAdjusted);
		}

		template <typename type2>
		static position_t position_of_segment(const this_t& v, position_t& pos, size_t n, const composite_vector_content_t<type2>& cmp, const position_t& cmpPos, size_t cmpLength)
		{
			size_t i = v.calculate_index(pos);
			size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
			if (!cmpLengthAdjusted)
				return v.get_end_position(); // validate cmpLength and cmdLengthAdjusted

			size_t lengthAdjusted = v.m_length - i;
			if (lengthAdjusted > n)
				lengthAdjusted = n;
			if (cmpLengthAdjusted > lengthAdjusted) // Ensure m_length is larger enough for cmp, and >0
				return v.get_end_position();

			position_t newPos = pos;
			find_segment_inner(v, i, newPos, lengthAdjusted, cmp, cmpPos, cmpLengthAdjusted);
			return newPos;
		}


		template <typename type2>
		static size_t find_segment_inner(const this_t& v, size_t i, position_t& pos, size_t lengthAdjusted, const vector<type2>& cmp, position_t& endPos)
		{
			size_t cmpLength = cmp.get_length();
			size_t endIndex = lengthAdjusted - cmpLength;
			for (;;)
			{
				const vector<type>* subArray = &v.m_vectorVector.get_const_ptr()[pos.m_outerIndex];
				size_t subVectorLength = subArray->get_length() - pos.m_innerIndex;
				while (cmpLength <= subVectorLength)
				{
					// Small enough to fit in this sub vector
					if (subArray->subrange(pos.m_innerIndex).template equals<type2, comparator_t>(cmp))
					{
						endPos = pos;
						endPos.m_innerIndex += cmpLength;
						return i;
					}

					++i;
					++pos.m_innerIndex;
					--subVectorLength;
				}
				if (i == endIndex)
				{
					pos = v.get_end_position();
					return const_max_int_v<size_t>;
				}

				while (subVectorLength > 0)
				{
					if (subArray->subrange(pos.m_innerIndex).template equals<type2, comparator_t>(cmp.subrange(0, subVectorLength)))
					{
						size_t curCmpIndex = subVectorLength;
						size_t subVectorIndex2 = pos.m_outerIndex;
						size_t remaining = cmpLength - subVectorLength;
						for (;;)
						{
							++subVectorIndex2;
							const vector<type>* subVector2 = &v.m_vectorVector.get_const_ptr()[subVectorIndex2];
							size_t subVectorLength2 = subVector2->get_length();
							size_t compareLengthThisSegment = subVectorLength2;
							if (compareLengthThisSegment > remaining)
								compareLengthThisSegment = remaining;
							if (subArray->template equals<type2, comparator_t>(cmp.subrange(curCmpIndex, compareLengthThisSegment)))
							{
								remaining -= compareLengthThisSegment;
								if (!remaining)
								{
									endPos.m_outerIndex = subVectorIndex2;
									endPos.m_innerIndex = compareLengthThisSegment;
									return i;
								}

								curCmpIndex += compareLengthThisSegment;
								continue;
							}
							break;
						}
					}
					if (++i >= endIndex)
					{
						pos = v.get_end_position();
						return const_max_int_v<size_t>;
					}

					++pos.m_innerIndex;
					--subVectorLength;
				}

				// No match
				++pos.m_outerIndex;
				pos.m_innerIndex = 0;
				continue;
			}
		}

		template <typename type2>
		static size_t find_segment_inner(const this_t& v, size_t i, position_t& pos, size_t lengthAdjusted, const composite_vector_content_t<type2>& cmp, const position_t& cmpPos, size_t cmpLengthAdjusted)
		{
			const vector<type2>& firstCmpVector = cmp.get_inner(cmpPos.m_outerIndex);
			size_t remaingCmpVectorLength = firstCmpVector.get_length() - cmpPos.m_innerIndex;
			size_t remainingCmpLength = cmpLengthAdjusted - remaingCmpVectorLength;
			size_t remainingLength = lengthAdjusted - remainingCmpLength;
			position_t newCmpPos;
			newCmpPos.m_outerIndex = cmpPos.m_outerIndex + 1;
			newCmpPos.m_innerIndex = 0;
			bool isLastVector = (newCmpPos.m_outerIndex == cmp.get_inner_count());
			position_t endPos;
			for (;;)
			{
				i = find_segment_inner(i, pos, remainingLength, firstCmpVector.subrange(cmpPos.m_innerIndex, remaingCmpVectorLength), endPos);
				if ((i == const_max_int_v<size_t>) || isLastVector)
					return i;

				const vector<type>& endVector = v.m_vectorVector.get_const_ptr()[endPos.m_outerIndex];
				if (endVector.get_length() == endPos.m_innerIndex)
				{
					endPos.m_innerIndex = 0;
					endPos.m_outerIndex++;
				}

				if (equals_inner(endPos, cmp, newCmpPos, remainingCmpLength, endPos))
					return i;
				if (++i == remainingLength)
				{
					pos = v.get_end_position();
					return const_max_int_v<size_t>;
				}

				if (++pos.m_innerIndex == v.m_vectorVector.get_const_ptr()[pos.m_outerIndex].get_length())
				{
					pos.m_innerIndex = 0;
					pos.m_outerIndex++;
				}
			}
		}
	};

	void insert(size_t i, const vector<type>& src)
	{
		if (!m_length)
			assign(src);
		else if (src.get_length() > 0)
		{
			if (!i)
				prepend(src);
			else if (i >= m_length)
				append(src);
			else
				insert_inner(calculate_position(i), src);
		}
	}

	void insert(const position_t& pos, const vector<type>& src)
	{
		if (!m_length)
			assign(src);
		else if (src.get_length() > 0)
		{
			if (!pos)
				prepend(src);
			else if (pos == get_end_position())
				append(src);
			else
				insert_inner(pos, src);
		}
	}

	void insert(size_t i, const this_t& src)
	{
		if (!m_length)
			assign(src);
		else if (src.get_length() > 0)
		{
			if (!i)
				prepend(src);
			else if (i >= m_length)
				append(src);
			else
				insert_inner(calculate_position(i), src);
		}
	}

	void insert(const position_t& pos, const this_t& src)
	{
		if (!m_length)
			assign(src);
		else if (src.get_length() > 0)
		{
			if (!pos)
				prepend(src);
			else if (pos == get_end_position())
				append(src);
			else
				insert_inner(pos, src);
		}
	}

	void insert_inner(const position_t& pos, const vector<type>& src)
	{
		if (pos.m_innerIndex == 0)
			m_vectorVector.insert(pos.m_outerIndex, src);
		else
		{
			size_t subVectorCount = m_vectorVector.get_length();
			size_t newSubVectorCount = subVectorCount + 2;
			if (m_vectorVector.get_capacity() >= newSubVectorCount) // It will fit in the existing buffer
			{
				m_vectorVector.resize(newSubVectorCount);
				vector<type>* arrayArrayPtr = m_vectorVector.get_ptr();
				size_t curSubVector = subVectorCount;
				while (--curSubVector > pos.m_outerIndex)
					arrayArrayPtr[curSubVector + 2] = arrayArrayPtr[curSubVector];
				vector<type>& arrayToSplit = arrayArrayPtr[pos.m_outerIndex];
				arrayArrayPtr[curSubVector + 2] = arrayToSplit.subrange(pos.m_innerIndex);
				arrayArrayPtr[curSubVector + 1] = src;
				arrayToSplit.subrange(0, pos.m_innerIndex);
			}
			else // Need a new buffer
			{
				const vector<type>* oldVectorVectorPtr = m_vectorVector.get_const_ptr();
				vector<vector<type> > newOuterVector(newSubVectorCount);
				vector<type>* newOuterVectorPtr = newOuterVector.get_ptr();
				size_t outerIndex = 0;
				while (outerIndex < pos.m_outerIndex)
				{
					newOuterVectorPtr[outerIndex] = oldVectorVectorPtr[outerIndex];
					++outerIndex;
				}
				const vector<type>& arrayToSplit = oldVectorVectorPtr[outerIndex];
				newOuterVectorPtr[outerIndex] = arrayToSplit.subrange(0, pos.m_innerIndex);
				newOuterVectorPtr[++outerIndex] = src;
				newOuterVectorPtr[++outerIndex] = arrayToSplit.subrange(pos.m_innerIndex);
				while (++outerIndex < newSubVectorCount)
					newOuterVectorPtr[outerIndex] = oldVectorVectorPtr[outerIndex - 2];
			}
		}
		m_length += src.get_length();
	}

	void insert_inner(const position_t& pos, const this_t& src)
	{
		if (pos.m_innerIndex == 0)
			m_vectorVector.insert(pos.m_outerIndex, src);
		else
		{
			size_t srcSubVectorCount = src.get_inner_count();
			size_t subVectorCount = m_vectorVector.get_length();
			size_t gap = srcSubVectorCount + 1;
			size_t newSubVectorCount = subVectorCount + gap;
			if (m_vectorVector.get_capacity() >= newSubVectorCount) // It will fit in the existing buffer
			{
				m_vectorVector.resize(newSubVectorCount);
				vector<type>* arrayArrayPtr = m_vectorVector.get_ptr();
				size_t curDstSubVector = newSubVectorCount;
				size_t gap = srcSubVectorCount + 1;
				while (--curDstSubVector > pos.m_outerIndex)
					arrayArrayPtr[curDstSubVector] = arrayArrayPtr[curDstSubVector - gap];
				vector<type>& arrayToSplit = arrayArrayPtr[pos.m_outerIndex];
				arrayArrayPtr[curDstSubVector] = arrayToSplit.subrange(pos.m_innerIndex);
				arrayArrayPtr[--curDstSubVector] = src;
				COGS_ASSERT(--curDstSubVector == pos.m_outerIndex);
				arrayToSplit.subrange(0, pos.m_innerIndex);
			}
			else // Need a new buffer
			{
				const vector<type>* oldVectorVectorPtr = m_vectorVector.get_const_ptr();
				vector<vector<type> > newOuterVector(newSubVectorCount);
				vector<type>* newOuterVectorPtr = newOuterVector.get_ptr();
				size_t outerIndex = 0;
				while (outerIndex < pos.m_outerIndex)
				{
					newOuterVectorPtr[outerIndex] = oldVectorVectorPtr[outerIndex];
					++outerIndex;
				}
				const vector<type>& arrayToSplit = oldVectorVectorPtr[outerIndex];
				newOuterVectorPtr[outerIndex] = arrayToSplit.subrange(0, pos.m_innerIndex);
				size_t oldSubVectorIndex = ++outerIndex;
				size_t srcSubVectorIndex = 0;
				for (;;)
				{
					newOuterVectorPtr[outerIndex] = src.get_inner(srcSubVectorIndex);
					++outerIndex;
					if (++srcSubVectorIndex == srcSubVectorCount)
						break;
				}
				newOuterVectorPtr[outerIndex] = arrayToSplit.subrange(pos.m_innerIndex);
				while (++outerIndex < newSubVectorCount)
				{
					newOuterVectorPtr[outerIndex] = oldVectorVectorPtr[oldSubVectorIndex];
					++oldSubVectorIndex;
				}
			}
		}
		m_length += src.get_length();
	}

	void replace(size_t i, const vector<type>& src)
	{
		size_t replaceLength = validate_length_from(i, src.get_length());
		if (!replaceLength)
			return;

		insert_replace_inner(i, calculate_position(i), replaceLength, src.subrange(0, replaceLength));
	}

	void replace(const position_t& pos, const vector<type>& src)
	{
		size_t i = calculate_index(pos);
		size_t replaceLength = validate_length_from(i, src.get_length());
		if (!replaceLength)
			return;
		insert_replace_inner(i, pos, replaceLength, src.get_subrange(0, replaceLength));
	}


	void replace(size_t i, const this_t& src, size_t srcIndex, size_t srcLength)
	{
		size_t srcLengthAdjusted = validate_length_from(src, srcIndex, srcLength);
		if (!srcLengthAdjusted)
			return;

		size_t replaceLength = validate_length_from(i, srcLengthAdjusted);
		if (!replaceLength)
			return;

		insert_replace_inner(i, calculate_position(i), replaceLength, src, srcIndex, replaceLength);
	}

	void replace(size_t i, const this_t& src, const position_t& srcPos, size_t srcLength)
	{
		size_t srcLengthAdjusted = validate_length_from(src, srcPos, srcLength);
		if (!srcLengthAdjusted)
			return;

		size_t replaceLength = validate_length_from(i, srcLengthAdjusted);
		if (!replaceLength)
			return;

		insert_replace_inner(i, calculate_position(i), replaceLength, src, srcPos, replaceLength);
	}

	void replace(const position_t& pos, const this_t& src, size_t srcIndex, size_t srcLength)
	{
		size_t i = calculate_index(pos);
		size_t srcLengthAdjusted = validate_length_from(src, srcIndex, srcLength);
		if (!srcLengthAdjusted)
			return;

		size_t replaceLength = validate_length_from(i, srcLengthAdjusted);
		if (!replaceLength)
			return;

		insert_replace_inner(i, pos, replaceLength, src, srcIndex, replaceLength);
	}

	void replace(const position_t& pos, const this_t& src, const position_t& srcPos, size_t srcLength)
	{
		size_t i = calculate_index(pos);
		size_t srcLengthAdjusted = validate_length_from(src, srcPos, srcLength);
		if (!srcLengthAdjusted)
			return;

		size_t replaceLength = validate_length_from(i, srcLengthAdjusted);
		if (!replaceLength)
			return;

		insert_replace_inner(i, pos, replaceLength, src, srcPos, replaceLength);
	}


	void insert_replace(size_t i, size_t replaceLength, const vector<type>& src)
	{
		size_t replaceLengthAdjusted = validate_length_from(i, replaceLength);
		position_t pos = calculate_position(i);
		insert_replace_inner(i, pos, replaceLengthAdjusted, src);
	}

	void insert_replace(const position_t& pos, size_t replaceLength, const vector<type>& src)
	{
		size_t i = calculate_index(pos);
		size_t replaceLengthAdjusted = validate_length_from(i, replaceLength);
		insert_replace_inner(i, pos, replaceLengthAdjusted, src);
	}

	void insert_replace_inner(size_t i, const position_t& pos, size_t replaceLengthAdjusted, const vector<type>& src)
	{
		size_t srcLength = src.get_length();
		if (!srcLength)
			erase(i, replaceLengthAdjusted);
		else if (!replaceLengthAdjusted)
			insert_inner(pos, src);
		else if (!i && (replaceLengthAdjusted == m_length))
			assign(src);
		else
		{
			size_t replaceEndIndex = i + replaceLengthAdjusted;
			if (!pos.m_innerIndex)
			{
				if (replaceEndIndex == m_length)
				{
					// Starts at a boundary, ends at the very end.  Efficient to truncate first, then insert.
					m_vectorVector.truncate_to(pos.m_outerIndex);
					m_length -= replaceLengthAdjusted;
					insert_inner(pos, src);
					return;
				}
			}

			m_length += src.get_length();
			m_length -= replaceLengthAdjusted;
			position_t replaceEndPos = calculate_position(replaceEndIndex);
			size_t subVectorRemoveCount = replaceEndPos.m_outerIndex - pos.m_outerIndex;
			if (!pos.m_innerIndex && !replaceEndPos.m_innerIndex)
			{
				if (subVectorRemoveCount == 1)
				{
					m_vectorVector.get_ptr()[pos.m_outerIndex] = src;
					return;
				}
			}

			// Need to calculate a new total size
			ptrdiff_t delta = 1 - subVectorRemoveCount;
			if (!subVectorRemoveCount != !pos.m_innerIndex)
				++delta;

			size_t newSubVectorCount = m_vectorVector.get_length() + delta;
			vector<vector<type> > origVectorVector;
			const vector<type>* srcVector = m_vectorVector.get_const_ptr();
			if (m_vectorVector.get_capacity() < newSubVectorCount) // Need a new buffer (growing, or was shared)
			{
				origVectorVector = m_vectorVector; // Keep it in scope so srcVector remains valid
				size_t tmpSubVectorCount = pos.outerIndex;
				if (!!pos.m_innerIndex)
					++tmpSubVectorCount;
				m_vectorVector.truncate_to(tmpSubVectorCount); // Copies first portion for us when we resize
				m_vectorVector.resize(newSubVectorCount);
			}
			vector<type>* dstVector = m_vectorVector.get_ptr();
			size_t curSubVectorIndex;
			if (delta == 0)
			{
				curSubVectorIndex = pos.m_outerIndex;
				if (!pos.m_innerIndex)
					dstVector[curSubVectorIndex++] = src;
				else
				{
					dstVector[curSubVectorIndex++].truncate_to(pos.m_innerIndex);
					dstVector[curSubVectorIndex++] = src;
				}

				if (!!replaceEndPos.m_innerIndex)
					dstVector[curSubVectorIndex].advance(replaceEndPos.m_innerIndex);
			}
			else
			{
				size_t subVectorCount = m_vectorVector.get_length();
				if (delta < 0)
				{
					curSubVectorIndex = replaceEndPos.m_outerIndex;
					while (curSubVectorIndex < subVectorCount) // If shrinking, we need to copy left to right
					{
						dstVector[curSubVectorIndex + delta] = srcVector[curSubVectorIndex];
						++curSubVectorIndex;
					}
					m_vectorVector.resize(newSubVectorCount);
				}
				else // if (delta > 0)
				{
					m_vectorVector.resize(newSubVectorCount);
					curSubVectorIndex = subVectorCount;
					while (--curSubVectorIndex >= replaceEndPos.m_outerIndex) // If growing, we need to copy right to left
						dstVector[curSubVectorIndex + delta] = srcVector[curSubVectorIndex];
				}

				curSubVectorIndex = replaceEndPos.m_outerIndex + delta;
				dstVector[curSubVectorIndex].advance(replaceEndPos.m_innerIndex);
				dstVector[--curSubVectorIndex] = src;
				if (!!pos.m_innerIndex)
					dstVector[pos.m_outerIndex].truncate_to(pos.m_innerIndex);
			}
		}
	}

	void insert_replace(size_t i, size_t replaceLength, const this_t& src)
	{
		size_t replaceLengthAdjusted = validate_length_from(i, replaceLength);
		position_t pos = calculate_position(i);
		insert_replace_inner(i, pos, replaceLengthAdjusted, src);
	}

	void insert_replace(const position_t& pos, size_t replaceLength, const this_t& src)
	{
		size_t i = calculate_index(pos);
		size_t replaceLengthAdjusted = validate_length_from(i, replaceLength);
		insert_replace_inner(i, pos, replaceLengthAdjusted, src);
	}

private:
	void insert_replace_inner(size_t i, const position_t& pos, size_t replaceLengthAdjusted, const this_t& src)
	{
		size_t srcLength = src.get_length();
		if (!srcLength)
			erase(i, replaceLengthAdjusted);
		else if (!replaceLengthAdjusted)
			insert_inner(pos, src);
		else if (!i && (replaceLengthAdjusted == m_length))
			assign(src);
		else
		{
			size_t replaceEndIndex = i + replaceLengthAdjusted;
			if (!pos.m_innerIndex)
			{
				if (replaceEndIndex == m_length)
				{
					// Starts at a boundary, ends at the very end.  Efficient to truncate first, then insert.
					m_vectorVector.truncate_to(pos.m_outerIndex);
					m_length -= replaceLengthAdjusted;
					insert_inner(pos, src);
					return;
				}
			}

			m_length += src.get_length();
			m_length -= replaceLengthAdjusted;

			position_t replaceEndPos = calculate_position(replaceEndIndex);
			size_t subVectorRemoveCount = replaceEndPos.m_outerIndex - pos.m_outerIndex;
			size_t srcSubVectorCount = src.get_inner_count();
			const vector<type>* srcVector = src.m_vectorVector.get_const_ptr();
			if (!pos.m_innerIndex && !replaceEndPos.m_innerIndex)
			{
				if (subVectorRemoveCount == srcSubVectorCount)
				{
					vector<type>* dstVector = m_vectorVector.get_ptr();
					for (size_t i = 0; i < srcSubVectorCount; i++)
						dstVector[i + pos.m_outerIndex] = srcVector[i];
					return;
				}
			}

			// Need to calculate a new total size
			ptrdiff_t delta = src.get_inner_count() - subVectorRemoveCount;
			if (!subVectorRemoveCount != !pos.m_innerIndex)
				++delta;

			size_t newSubVectorCount = m_vectorVector.get_length() + delta;
			vector<vector<type> > origVectorVector;
			const vector<type>* oldVector = m_vectorVector.get_const_ptr();
			if (m_vectorVector.get_capacity() < newSubVectorCount) // Need a new buffer (growing, or was shared)
			{
				origVectorVector = m_vectorVector; // Keep it in scope so oldVector remains valid
				size_t tmpSubVectorCount = pos.outerIndex;
				if (!!pos.m_innerIndex)
					++tmpSubVectorCount;
				m_vectorVector.truncate_to(tmpSubVectorCount); // Copies first portion for us when we resize
				m_vectorVector.resize(newSubVectorCount);
			}
			vector<type>* dstVector = m_vectorVector.get_ptr();
			size_t curSubVectorIndex;
			if (delta == 0)
			{
				curSubVectorIndex = pos.m_outerIndex;
				if (!pos.m_innerIndex)
				{
					for (size_t i = 0; i < srcSubVectorCount; i++)
						dstVector[curSubVectorIndex++] = srcVector[i];
				}
				else
				{
					dstVector[curSubVectorIndex++].truncate_to(pos.m_innerIndex);
					for (size_t i = 0; i < srcSubVectorCount; i++)
						dstVector[curSubVectorIndex++] = srcVector[i];
				}

				if (!!replaceEndPos.m_innerIndex)
					dstVector[curSubVectorIndex].advance(replaceEndPos.m_innerIndex);
			}
			else
			{
				size_t subVectorCount = m_vectorVector.get_length();
				if (delta < 0)
				{
					curSubVectorIndex = replaceEndPos.m_outerIndex;
					while (curSubVectorIndex < subVectorCount) // If shrinking, we need to copy left to right
					{
						dstVector[curSubVectorIndex + delta] = oldVector[curSubVectorIndex];
						++curSubVectorIndex;
					}
					m_vectorVector.resize(newSubVectorCount);
				}
				else // if (delta > 0)
				{
					m_vectorVector.resize(newSubVectorCount);
					curSubVectorIndex = subVectorCount;
					while (--curSubVectorIndex >= replaceEndPos.m_outerIndex) // If growing, we need to copy right to left
						dstVector[curSubVectorIndex + delta] = oldVector[curSubVectorIndex];
				}

				curSubVectorIndex = replaceEndPos.m_outerIndex + delta;
				dstVector[curSubVectorIndex].advance(replaceEndPos.m_innerIndex);
				curSubVectorIndex -= srcSubVectorCount;
				for (size_t i = 0; i < srcSubVectorCount; i++)
				{
					dstVector[curSubVectorIndex + i] = srcVector[i];
				}
				if (!!pos.m_innerIndex)
					dstVector[pos.m_outerIndex].truncate_to(pos.m_innerIndex);
			}
		}
	}
};

/// @ingroup LockFreeCollections
/// @brief A vector comprised of one or more sub-arrays.
/// @tparam T Type to contain.
template <typename T>
class composite_vector
{
private:
	static_assert(!std::is_const_v<T>);
	static_assert(!std::is_volatile_v<T>);
	static_assert(!std::is_void_v<T>);

public:
	typedef T type;
	typedef composite_vector<type> this_t;

	typedef vector<type> inner_t;

	/// @brief An index position within a composite_string
	class position_t
	{
	private:
		template <typename>
		friend class composite_vector;

		typename composite_vector_content_t<type>::position_t m_pos;

		position_t(const typename composite_vector_content_t<type>::position_t& pos) : m_pos(pos) { }

		size_t& get_outer_index() { return m_pos.get_outer_index(); }
		size_t& get_inner_index() { return m_pos.get_inner_index(); }

	public:
		position_t() { }
		position_t(const position_t& pos) : m_pos(pos.m_pos) { }
		position_t(size_t subStringIndex, size_t i) : m_pos(subStringIndex, i) { }

		position_t& operator=(const position_t& pos) { m_pos = pos.m_pos; return *this; }

		void set(size_t subStringIndex, size_t i) { m_pos.set(subStringIndex, i); }
		bool operator==(const position_t& cmp) const { return (m_pos == cmp.m_pos); }
		bool operator!=(const position_t& cmp) const { return !operator==(cmp); }
		bool operator<(const position_t& cmp) const { return (m_pos < cmp.m_pos); }
		bool operator>(const position_t& cmp) const { return (m_pos > cmp.m_pos); }
		bool operator<=(const position_t& cmp) const { return !operator>(cmp); }
		bool operator>=(const position_t& cmp) const { return !operator<(cmp); }

		size_t get_outer_index() const { return m_pos.get_outer_index(); }
		size_t get_inner_index() const { return m_pos.get_inner_index(); }
	};

protected:
	typedef composite_vector_content_t<type> content_t;

	typedef transactable<content_t> transactable_t;
	transactable_t m_contents;

	typedef typename transactable_t::read_token read_token;
	typedef typename transactable_t::write_token write_token;

	friend class io::composite_buffer;

	template <typename>
	friend class composite_string_t;

	composite_vector(const vector<vector<type> >& src, size_t n)
	{
		m_contents->m_vectorVector = src;
		m_contents->m_length = n;
	}

public:
	/// @brief A composite_vector element iterator
	class iterator
	{
	protected:
		this_t* m_array;
		position_t m_position;

		iterator(this_t* v, const position_t& pos)
			: m_array(v),
			m_position(pos)
		{ }

		iterator(this_t* v, size_t outerIndex, size_t i)
			: m_array(v),
			m_position(outerIndex, i)
		{ }

		template <typename>
		friend class composite_vector;

	public:
		iterator()
		{ }

		iterator(const iterator& i)
			: m_array(i.m_array),
			m_position(i.m_position)
		{ }

		void release() { m_array = 0; }

		iterator& operator++()
		{
			if (m_array)
			{
				size_t subVectorCount = m_array->get_inner_count();
				if (m_position.get_outer_index() >= subVectorCount)
					m_array = 0;
				else
				{
					vector<type> buf = m_array->get_inner(m_position.get_outer_index());
					if (++m_position.get_inner_index() >= buf.get_length())
					{
						if (m_position.get_outer_index() >= subVectorCount - 1)
							m_array = 0;
						else
						{
							m_position.get_outer_index()++;
							m_position.get_inner_index() = 0;
						}
					}
				}
			}
			return *this;
		}

		iterator& operator--()
		{
			if (m_array)
			{
				size_t subVectorCount = m_array->get_inner_count();
				if (m_position.get_outer_index() >= subVectorCount)
				{
					if ((m_position.get_outer_index() == subVectorCount)
						&& (m_position.get_inner_index() == 0))
					{
						--m_position.get_outer_index();
						m_position.get_inner_index() = m_array->get_inner(m_position.get_outer_index()).get_length() - 1;
					}
					else
						m_array = 0;
				}
				else
				{
					if (!!m_position.get_inner_index())
						--m_position.get_inner_index();
					else
					{
						if (!m_position.get_outer_index())
							m_array = 0;
						else
						{
							--m_position.get_outer_index();
							m_position.get_inner_index() = m_array->get_inner(m_position.get_outer_index()).get_length() - 1;
						}
					}
				}
			}
			return *this;
		}

		iterator operator++(int) { iterator i(*this); ++*this; return i; }
		iterator operator--(int) { iterator i(*this); --*this; return i; }

		bool operator!() const { return !m_array || (m_position >= m_array->get_end_position()); }

		bool operator==(const iterator& i) const { return (m_array == i.m_array) && (!m_array || (m_position == i.m_position)); }
		bool operator!=(const iterator& i) const { return !operator==(i); }

		iterator& operator=(const iterator& i) { m_array = i.m_array; m_position = i.m_position; return *this; }

		type* get() const { return m_array->get_inner(m_position.get_outer_index()).get_ptr() + m_position.get_inner_index(); }
		type& operator*() const { return *get(); }
		type* operator->() const { return get(); }

		position_t get_position() const
		{
			if (!m_array)
				return position_t(const_max_int_v<size_t>, const_max_int_v<size_t>);
			return m_position;
		}

		iterator next() const { iterator result(*this); ++result; return result; }

		iterator prev() const { iterator result(*this); --result; return result; }
	};

	iterator get_first_iterator() { size_t sz = get_length(); return iterator(!!sz ? this : 0, 0, 0); }
	iterator get_last_iterator()
	{
		size_t subVectorCount = get_inner_count();
		if (!subVectorCount)
			return iterator(0, 0, 0);
		size_t lastSubVectorIndex = subVectorCount - 1;
		return iterator(this, lastSubVectorIndex, get_inner(lastSubVectorIndex).get_length() - 1);
	}

	/// @brief A composite_vector constant element iterator
	class const_iterator
	{
	protected:
		template <typename>
		friend class composite_vector;

		const this_t* m_array;
		position_t m_position;

		const_iterator(const this_t* v, const position_t& pos)
			: m_array(v),
			m_position(pos)
		{ }

		const_iterator(const this_t* v, size_t outerIndex, size_t i)
			: m_array(v),
			m_position(outerIndex, i)
		{ }

	public:
		const_iterator()
		{ }

		const_iterator(const const_iterator& i)
			: m_array(i.m_array),
			m_position(i.m_position)
		{ }

		const_iterator(const iterator& i)
			: m_array(i.m_array),
			m_position(i.m_position)
		{ }

		void release() { m_array = 0; }

		const_iterator& operator++()
		{
			if (m_array)
			{
				size_t subVectorCount = m_array->get_inner_count();
				if (m_position.get_outer_index() >= subVectorCount)
					m_array = 0;
				else
				{
					const vector<type>& buf = m_array->get_inner(m_position.get_outer_index());
					if (++m_position.get_inner_index() >= buf.get_length())
					{
						if (m_position.get_outer_index() >= subVectorCount - 1)
							m_array = 0;
						else
						{
							m_position.get_outer_index()++;
							m_position.get_inner_index() = 0;
						}
					}
				}
			}

			return *this;
		}

		const_iterator& operator--()
		{
			if (m_array)
			{
				size_t subVectorCount = m_array->get_inner_count();
				if (m_position.get_outer_index() >= subVectorCount)
				{
					if ((m_position.get_outer_index() == subVectorCount)
						&& (m_position.get_inner_index() == 0))
					{
						--m_position.get_outer_index();
						m_position.get_inner_index() = m_array->get_inner(m_position.get_outer_index()).get_length() - 1;
					}
					else
						m_array = 0;
				}
				else
				{
					if (!!m_position.get_inner_index())
						--m_position.get_inner_index();
					else
					{
						if (!m_position.get_outer_index())
							m_array = 0;
						else
						{
							--m_position.get_outer_index();
							m_position.get_inner_index() = m_array->get_inner(m_position.get_outer_index()).get_length() - 1;
						}
					}
				}
			}

			return *this;
		}

		const_iterator operator++(int) { const_iterator i(*this); ++*this; return i; }
		const_iterator operator--(int) { const_iterator i(*this); --*this; return i; }

		bool operator!() const { return !m_array || (m_position >= m_array->get_end_position()); }

		bool operator==(const const_iterator& i) const { return (m_array == i.m_array) && (!m_array || (m_position == i.m_position)); }
		bool operator==(const iterator& i) const { return (m_array == i.m_array) && (!m_array || (m_position == i.m_position)); }
		bool operator!=(const const_iterator& i) const { return !operator==(i); }
		bool operator!=(const iterator& i) const { return !operator==(i); }
		const_iterator& operator=(const const_iterator& i) { m_array = i.m_array; m_position = i.m_position; return *this; }
		const_iterator& operator=(const iterator& i) { m_array = i.m_array; m_position = i.m_position; return *this; }

		const type* get() const { return m_array->get_inner(m_position.get_outer_index()).get_const_ptr() + m_position.get_inner_index(); }
		const type& operator*() const { return *get(); }
		const type* operator->() const { return get(); }

		position_t get_position() const
		{
			if (!m_array)
				return position_t(const_max_int_v<size_t>, const_max_int_v<size_t>);
			return m_position;
		}

		const_iterator next() const { const_iterator result(*this); ++result; return result; }
		const_iterator prev() const { const_iterator result(*this); --result; return result; }

	};

	const_iterator get_first_const_iterator() const { size_t sz = get_length(); return const_iterator((!!sz ? this : 0), 0, 0); }

	const_iterator get_last_const_iterator() const
	{
		size_t subVectorCount = get_inner_count();
		if (!subVectorCount)
			return const_iterator(0, 0, 0);
		size_t lastSubVectorIndex = subVectorCount - 1;
		return const_iterator(this, lastSubVectorIndex, get_inner(lastSubVectorIndex).get_length() - 1);
	}

	position_t get_first_position() const { return position_t(0, 0); }
	position_t get_first_position() const volatile { return position_t(0, 0); }
	position_t get_last_position() const { return m_contents->get_last_position(); }
	position_t get_last_position() const volatile { return m_contents.begin_read()->get_last_position(); }
	position_t get_end_position() const { return m_contents->get_end_position(); }
	position_t get_end_position() const volatile { return m_contents.begin_read()->get_end_position(); }

	position_t get_next_position(const position_t& pos) const { return m_contents->get_next_position(pos.m_pos); }
	position_t get_next_position(const position_t& pos) const volatile { return m_contents.begin_read()->get_next_position(pos.m_pos); }

	// Caller error to call with (pos == 0, 0)
	position_t get_prev_position(const position_t& pos) const { return m_contents->get_prev_position(pos.m_pos); }
	position_t get_prev_position(const position_t& pos) const volatile { return m_contents.begin_read()->get_prev_position(pos.m_pos); }

	// Caller error to call with i >= length
	position_t calculate_position(size_t i, bool countFromFront = true) const
	{
		return m_contents->calculate_position(i, countFromFront);
	}

	size_t calculate_index(const position_t& pos) const
	{
		return m_contents->calculate_index(pos.m_pos);
	}

	composite_vector()
	{ }

	composite_vector(const this_t& src)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents))
	{ }

	composite_vector(const volatile this_t& src)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents.begin_read()))
	{ }

	composite_vector(this_t&& src)
		: m_contents(std::move(src.m_contents))
	{ }

	composite_vector(const this_t& src, size_t srcIndex, size_t srcLength = const_max_int_v<size_t>)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents))
	{
		set_to_subrange(srcIndex, srcLength);
	}

	composite_vector(const volatile this_t& src, size_t srcIndex, size_t srcLength = const_max_int_v<size_t>)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents.begin_read()))
	{
		set_to_subrange(srcIndex, srcLength);
	}

	composite_vector(const this_t& src, const position_t& pos, size_t srcLength = const_max_int_v<size_t>)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents))
	{
		set_to_subrange(pos, srcLength);
	}

	composite_vector(const volatile this_t& src, const position_t& pos, size_t srcLength = const_max_int_v<size_t>)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents.begin_read()))
	{
		set_to_subrange(pos, srcLength);
	}


	template <typename type2>
	composite_vector(const vector<type2>& src)
		: m_contents(typename transactable_t::construct_embedded_t(), src)
	{ }

	template <typename type2>
	composite_vector(const volatile vector<type2>& src)
		: m_contents(typename transactable_t::construct_embedded_t(), src)
	{ }

	template <typename type2>
	composite_vector(const composite_vector<type2>& src)
		: m_contents(typename transactable_t::construct_embedded_t(), src)
	{ }

	template <typename type2>
	composite_vector(const volatile composite_vector<type2>& src)
		: m_contents(typename transactable_t::construct_embedded_t(), src)
	{ }


	template <typename type2>
	composite_vector(const composite_vector<type2>& src, size_t srcIndex, size_t srcLength = const_max_int_v<size_t>)
		: m_contents(typename transactable_t::construct_embedded_t(), src, srcIndex, srcLength)
	{ }

	template <typename type2>
	composite_vector(const volatile composite_vector<type2>& src, size_t srcIndex, size_t srcLength = const_max_int_v<size_t>)
		: m_contents(typename transactable_t::construct_embedded_t(), src, srcIndex, srcLength)
	{ }

	template <typename type2>
	composite_vector(const composite_vector<type2>& src, const position_t& srcPos, size_t srcLength = const_max_int_v<size_t>)
		: m_contents(typename transactable_t::construct_embedded_t(), src, srcPos, srcLength)
	{ }

	template <typename type2>
	composite_vector(const volatile composite_vector<type2>& src, const position_t& srcPos, size_t srcLength = const_max_int_v<size_t>)
		: m_contents(typename transactable_t::construct_embedded_t(), src, srcPos, srcLength)
	{ }

	vector<type> composite() const { return m_contents->composite(); }
	vector<type> composite() const volatile { return m_contents.begin_read()->composite(); }

	vector<type> composite(size_t i, size_t n = const_max_int_v<size_t>) const { return m_contents->composite(i, n); }
	vector<type> composite(size_t i, size_t n = const_max_int_v<size_t>) const volatile { return m_contents.begin_read()->composite(i, n); }

	vector<type> composite(const position_t& pos, size_t n = const_max_int_v<size_t>) const { return m_contents->composite(pos.m_pos, n); }
	vector<type> composite(const position_t& pos, size_t n = const_max_int_v<size_t>) const volatile { return m_contents.begin_read()->composite(pos.m_pos, n); }

	template <typename type2>
	vector<type2> composite_as() const { return m_contents->template composite_as<type2>(); }

	template <typename type2>
	vector<type2> composite_as() const volatile { return m_contents.begin_read()->template composite_as<type2>(); }

	template <typename type2>
	vector<type2> composite_as(size_t i, size_t n = const_max_int_v<size_t>) const { return m_contents->template composite_as<type2>(i, n); }

	template <typename type2>
	vector<type2> composite_as(size_t i, size_t n = const_max_int_v<size_t>) const volatile { return m_contents.begin_read()->template composite_as<type2>(i, n); }

	template <typename type2>
	vector<type2> composite_as(const position_t& pos, size_t n = const_max_int_v<size_t>) const { return m_contents->template composite_as<type2>(pos.m_pos, n); }

	template <typename type2>
	vector<type2> composite_as(const position_t& pos, size_t n = const_max_int_v<size_t>) const volatile { return m_contents.begin_read()->template composite_as<type2>(pos.m_pos, n); }

	size_t get_length() const { return m_contents->m_length; }
	size_t get_length() const volatile { return m_contents.begin_read()->m_length; }

	bool is_empty() const { return get_length() == 0; }
	bool is_empty() const volatile { return get_length() == 0; }

	bool operator!() const { return is_empty(); }
	bool operator!() const volatile { return is_empty(); }


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::equals(*m_contents, 0, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, size_t n, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(position_t& pos, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), i, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, size_t n, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), i, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(position_t& pos, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), pos.m_pos, n, vector<type2>::contain(cmp, cmpLength)); }


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, 0, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, cmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, 0, const_max_int_v<size_t>, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, 0, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), i, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, const_max_int_v<size_t>, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, size_t n, const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, n, cmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, size_t n, const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), i, n, cmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, size_t n, const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, n, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, size_t n, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, n, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, 0, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(size_t i, size_t n, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, i, n, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, const_max_int_v<size_t>, cmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, cmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, const volatile vector<type2>& cmp) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, const_max_int_v<size_t>, tmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, size_t n, const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, size_t n, const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), pos.m_pos, n, cmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, size_t n, const volatile vector<type2>& cmp) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, n, tmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, size_t n, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, n, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::equals(*(m_contents.begin_read()), pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool equals(const position_t& pos, size_t n, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::equals(*m_contents, pos.m_pos, n, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2>
	bool operator==(const vector<type2>& cmp) const { return m_contents->template equals<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator==(const composite_vector<type2>& cmp) const { return m_contents->template equals<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator==(const vector<type2>& cmp) const volatile { return m_contents.begin_read()->template equals<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator==(const composite_vector<type2>& cmp) const volatile { return m_contents.begin_read()->template equals<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator==(const volatile vector<type2>& cmp) const { vector<type2> tmp = cmp; return m_contents->template equals<type2, default_comparator >(tmp); }

	template <typename type2>
	bool operator==(const volatile composite_vector<type2>& cmp) const { vector<type2> tmp = cmp; return m_contents->template equals<type2, default_comparator >(tmp); }


	template <typename type2>
	bool operator!=(const vector<type2>& cmp) const { return !operator==(cmp); }

	template <typename type2>
	bool operator!=(const composite_vector<type2>& cmp) const { return !operator==(cmp); }

	template <typename type2>
	bool operator!=(const vector<type2>& cmp) const volatile { return !operator==(cmp); }

	template <typename type2>
	bool operator!=(const composite_vector<type2>& cmp) const volatile { return !operator==(cmp); }

	template <typename type2>
	bool operator!=(const volatile vector<type2>& cmp) const { return !operator==(cmp); }

	template <typename type2>
	bool operator!=(const volatile composite_vector<type2>& cmp) const { return !operator==(cmp); }


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, size_t n, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(position_t& pos, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, size_t n, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(position_t& pos, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, n, vector<type2>::contain(cmp, cmpLength)); }


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, size_t n, const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, size_t n, const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, size_t n, const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, size_t n, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(size_t i, size_t n, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, const volatile vector<type2>& cmp) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, tmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, size_t n, const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, size_t n, const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, size_t n, const volatile vector<type2>& cmp) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, tmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, size_t n, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	int compare(const position_t& pos, size_t n, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const type2* cmp, size_t cmpLength) const { return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, const type2* cmp, size_t cmpLength) const { return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, size_t n, const type2* cmp, size_t cmpLength) const { return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, const type2* cmp, size_t cmpLength) const { return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const { return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const type2* cmp, size_t cmpLength) const volatile { return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, const type2* cmp, size_t cmpLength) const volatile { return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, size_t n, const type2* cmp, size_t cmpLength) const volatile { return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, const type2* cmp, size_t cmpLength) const volatile { return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const volatile { return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, n, vector<type2>::contain(cmp, cmpLength)); }


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const vector<type2>& cmp) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const vector<type2>& cmp) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, const vector<type2>& cmp) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, const vector<type2>& cmp) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, size_t n, const vector<type2>& cmp) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, size_t n, const vector<type2>& cmp) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, size_t n, const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, size_t n, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(size_t i, size_t n, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, tmp, cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, const vector<type2>& cmp) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, const vector<type2>& cmp) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, const volatile vector<type2>& cmp) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, tmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, size_t n, const vector<type2>& cmp) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, size_t n, const vector<type2>& cmp) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, size_t n, const volatile vector<type2>& cmp) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, tmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, size_t n, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_less_than(const position_t& pos, size_t n, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 > content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const type2* cmp, size_t cmpLength) const { return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, const type2* cmp, size_t cmpLength) const { return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, size_t n, const type2* cmp, size_t cmpLength) const { return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, const type2* cmp, size_t cmpLength) const { return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const { return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const type2* cmp, size_t cmpLength) const volatile { return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, const type2* cmp, size_t cmpLength) const volatile { return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, size_t n, const type2* cmp, size_t cmpLength) const volatile { return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, const type2* cmp, size_t cmpLength) const volatile { return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const volatile { return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, n, vector<type2>::contain(cmp, cmpLength)); }


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const vector<type2>& cmp) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const vector<type2>& cmp) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, const vector<type2>& cmp) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, cmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, const vector<type2>& cmp) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, size_t n, const vector<type2>& cmp) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, size_t n, const vector<type2>& cmp) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, size_t n, const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, size_t n, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, 0, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(size_t i, size_t n, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, i, n, tmp, cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, const vector<type2>& cmp) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, const vector<type2>& cmp) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, const volatile vector<type2>& cmp) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, tmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, size_t n, const vector<type2>& cmp) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, size_t n, const vector<type2>& cmp) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, size_t n, const volatile vector<type2>& cmp) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, tmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, size_t n, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < content_t::template compare_helper<comparator_t>::compare(*(m_contents.begin_read()), pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool is_greater_than(const position_t& pos, size_t n, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return 0 < content_t::template compare_helper<comparator_t>::compare(*m_contents, pos.m_pos, n, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2>
	bool operator<(const vector<type2>& cmp) const { return is_less_than<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator<(const vector<type2>& cmp) const volatile { return is_less_than<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator<(const volatile vector<type2>& cmp) const { return is_less_than<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator<(const composite_vector<type2>& cmp) const { return is_less_than<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator<(const composite_vector<type2>& cmp) const volatile { return is_less_than<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator<(const volatile composite_vector<type2>& cmp) const { return is_less_than<type2, default_comparator >(cmp); }


	template <typename type2>
	bool operator>(const vector<type2>& cmp) const { return is_greater_than<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator>(const vector<type2>& cmp) const volatile { return is_greater_than<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator>(const volatile vector<type2>& cmp) const { return is_greater_than<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator>(const composite_vector<type2>& cmp) const { return is_greater_than<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator>(const composite_vector<type2>& cmp) const volatile { return is_greater_than<type2, default_comparator >(cmp); }

	template <typename type2>
	bool operator>(const volatile composite_vector<type2>& cmp) const { return is_greater_than<type2, default_comparator >(cmp); }


	template <typename type2>
	bool operator<=(const vector<type2>& cmp) const { return !operator>(cmp); }

	template <typename type2>
	bool operator<=(const vector<type2>& cmp) const volatile { return !operator>(cmp); }

	template <typename type2>
	bool operator<=(const volatile vector<type2>& cmp) const { return !operator>(cmp); }

	template <typename type2>
	bool operator<=(const composite_vector<type2>& cmp) const { return !operator>(cmp); }

	template <typename type2>
	bool operator<=(const composite_vector<type2>& cmp) const volatile { return !operator>(cmp); }

	template <typename type2>
	bool operator<=(const volatile composite_vector<type2>& cmp) const { return !operator>(cmp); }


	template <typename type2>
	bool operator>=(const vector<type2>& cmp) const { return !operator<(cmp); }

	template <typename type2>
	bool operator>=(const vector<type2>& cmp) const volatile { return !operator<(cmp); }

	template <typename type2>
	bool operator>=(const volatile vector<type2>& cmp) const { return !operator<(cmp); }

	template <typename type2>
	bool operator>=(const composite_vector<type2>& cmp) const { return !operator<(cmp); }

	template <typename type2>
	bool operator>=(const composite_vector<type2>& cmp) const volatile { return !operator<(cmp); }

	template <typename type2>
	bool operator>=(const volatile composite_vector<type2>& cmp) const { return !operator<(cmp); }


	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const type2* cmp, size_t cmpLength) const { return equals<type2, comparator_t>(0, cmpLength, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const type2* cmp, size_t cmpLength) const volatile { return equals<type2, comparator_t>(0, cmpLength, cmp, cmpLength); }


	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const vector<type2>& cmp) const
	{
		return equals<type2, comparator_t>(0, cmp.get_length(), cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const vector<type2>& cmp) const volatile
	{
		return equals<type2, comparator_t>(0, cmp.get_length(), cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const volatile vector<type2>& cmp) const
	{
		return equals<type2, comparator_t>(0, cmp.get_length(), cmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return equals<type2, comparator_t>(0, cmpLength, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return equals<type2, comparator_t>(0, cmpLength, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return equals<type2, comparator_t>(0, cmpLength, cmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return equals<type2, comparator_t>(0, cmpLength, *(cmp.m_contents), cmpPos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return equals<type2, comparator_t>(0, cmpLength, *(cmp.m_contents), cmpPos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool starts_with(const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return equals<type2, comparator_t>(0, cmpLength, cmp, cmpPos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const type2* cmp, size_t cmpLength) const
	{
		return content_t::template compare_helper<comparator_t>::ends_with(*m_contents, vector<type2>::contain(cmp, cmpLength));
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const type2* cmp, size_t cmpLength) const volatile
	{
		return content_t::template compare_helper<comparator_t>::ends_with(*(m_contents.begin_read()), vector<type2>::contain(cmp, cmpLength));
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::ends_with(*m_contents, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::ends_with(*(m_contents.begin_read()), cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return ends_with<type2, comparator_t>(tmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::ends_with(*m_contents, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::ends_with(*(m_contents.begin_read()), *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return ends_with<type2, comparator_t>(tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::ends_with(*m_contents, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::ends_with(*(m_contents.begin_read()), *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool ends_with(const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::ends_with(*(m_contents.begin_read()), tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of(const type2& cmp) const { return content_t::template compare_helper<comparator_t>::index_of(*m_contents, 0, const_max_int_v<size_t>, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of(const type2& cmp) const volatile { return content_t::template compare_helper<comparator_t>::index_of(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of(size_t i, const type2& cmp) const { return content_t::template compare_helper<comparator_t>::index_of(*m_contents, i, const_max_int_v<size_t>, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of(size_t i, const type2& cmp) const volatile { return content_t::template compare_helper<comparator_t>::index_of(*(m_contents.begin_read()), i, const_max_int_v<size_t>, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of(size_t i, size_t n, const type2& cmp) const { return content_t::template compare_helper<comparator_t>::index_of(*m_contents, i, n, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of(size_t i, size_t n, const type2& cmp) const volatile { return content_t::template compare_helper<comparator_t>::index_of(*(m_contents.begin_read()), i, n, cmp); }


	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of(const position_t& pos, const type2& cmp) const { return content_t::template compare_helper<comparator_t>::position_of(*m_contents, pos.m_pos, const_max_int_v<size_t>, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of(const position_t& pos, const type2& cmp) const volatile { return content_t::template compare_helper<comparator_t>::position_of(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of(const position_t& pos, size_t n, const type2& cmp) const { return content_t::template compare_helper<comparator_t>::position_of(*m_contents, pos.m_pos, n, cmp); }

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of(const position_t& pos, size_t n, const type2& cmp) const volatile { return content_t::template compare_helper<comparator_t>::position_of(*(m_contents.begin_read()), pos.m_pos, n, cmp); }


	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_any(const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::index_of_any(*m_contents, 0, const_max_int_v<size_t>, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_any(const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::index_of_any(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_any(size_t i, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::index_of_any(*m_contents, i, const_max_int_v<size_t>, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_any(size_t i, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::index_of_any(*(m_contents.begin_read()), i, const_max_int_v<size_t>, cmp, cmpLength); }


	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_any(size_t i, size_t n, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::index_of_any(*m_contents, i, n, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_any(size_t i, size_t n, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::index_of_any(*(m_contents.begin_read()), i, n, cmp, cmpLength); }


	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_any(const position_t& pos, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::position_of_any(*m_contents, pos.m_pos, const_max_int_v<size_t>, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_any(const position_t& pos, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::position_of_any(*(m_contents.begin_read()), pos.m_pos, const_max_int_v<size_t>, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_any(const position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::position_of_any(*m_contents, pos.m_pos, n, cmp, cmpLength); }

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_any(const position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::position_of_any(*(m_contents.begin_read()), pos.m_pos, n, cmp, cmpLength); }


	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, 0, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, i, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, size_t n, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, i, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::index_of_segment(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::index_of_segment(*(m_contents.begin_read()), i, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, size_t n, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::index_of_segment(*(m_contents.begin_read()), i, n, vector<type2>::contain(cmp, cmpLength)); }


	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, 0, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, 0, const_max_int_v<size_t>, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, 0, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, i, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*(m_contents.begin_read()), i, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, i, const_max_int_v<size_t>, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*(m_contents.begin_read()), i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, i, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, size_t n, const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, i, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, size_t n, const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*(m_contents.begin_read()), i, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, size_t n, const volatile vector<type2>& cmp) const
	{
		vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, i, n, tmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, i, n, cmp, cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*(m_contents.begin_read()), i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(size_t i, size_t n, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, i, n, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::index_of_segment(*(m_contents.begin_read()), 0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	size_t index_of_segment(const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::index_of_segment(*m_contents, 0, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const { return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::position_of_segment(*(m_contents.begin_read()), pos, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const volatile { return content_t::template compare_helper<comparator_t>::position_of_segment(*(m_contents.begin_read()), pos, n, vector<type2>::contain(cmp, cmpLength)); }


	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::position_of_segment(*(m_contents.begin_read()), pos, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, const volatile vector<type2>& cmp) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, const_max_int_v<size_t>, tmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::position_of_segment(*(m_contents.begin_read()), pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, const_max_int_v<size_t>, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::position_of_segment(*(m_contents.begin_read()), pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, const_max_int_v<size_t>, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, size_t n, const vector<type2>& cmp) const
	{
		return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, size_t n, const vector<type2>& cmp) const volatile
	{
		return content_t::template compare_helper<comparator_t>::position_of_segment(*(m_contents.begin_read()), pos, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, size_t n, const volatile vector<type2>& cmp) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, n, tmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::position_of_segment(*(m_contents.begin_read()), pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, size_t n, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, n, tmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return content_t::template compare_helper<comparator_t>::position_of_segment(*(m_contents.begin_read()), pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	position_t position_of_segment(const position_t& pos, size_t n, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		composite_vector<type2> tmp = cmp;
		return content_t::template compare_helper<comparator_t>::position_of_segment(*m_contents, pos, n, tmp, cmpPos.m_pos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains(const type2& cmp) const { return index_of<type2, comparator_t>(0, const_max_int_v<size_t>, cmp) != -1; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains(const type2& cmp) const volatile { return index_of<type2, comparator_t>(0, const_max_int_v<size_t>, cmp) != -1; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains(size_t i, const type2& cmp) const { return index_of<type2, comparator_t>(i, const_max_int_v<size_t>, cmp) != -1; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains(size_t i, const type2& cmp) const volatile { return index_of<type2, comparator_t>(i, const_max_int_v<size_t>, cmp) != -1; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains(const position_t& pos, const type2& cmp) const { return position_of<type2, comparator_t>(pos, const_max_int_v<size_t>, cmp) != get_end_position(); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains(const position_t& pos, const type2& cmp) const volatile { return position_of<type2, comparator_t>(pos, const_max_int_v<size_t>, cmp) != get_end_position(); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains(size_t i, size_t n, const type2& cmp) const { return index_of<type2, comparator_t>(i, n, cmp) != -1; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains(size_t i, size_t n, const type2& cmp) const volatile { return index_of<type2, comparator_t>(i, n, cmp) != -1; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains(const position_t& pos, size_t n, const type2& cmp) const { return position_of<type2, comparator_t>(pos, n, cmp) != get_end_position(); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains(const position_t& pos, size_t n, const type2& cmp) const volatile { return position_of<type2, comparator_t>(pos, n, cmp) != get_end_position(); }


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_any(const type2* cmp, size_t n) const { return index_of_any<type2, comparator_t>(cmp, n) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_any(const type2* cmp, size_t n) const volatile { return index_of_any<type2, comparator_t>(cmp, n) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_any(size_t i, const type2* cmp, size_t n) const { return index_of_any<type2, comparator_t>(i, cmp, n) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_any(size_t i, const type2* cmp, size_t n) const volatile { return index_of_any<type2, comparator_t>(i, cmp, n) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_any(const position_t& pos, const type2* cmp, size_t n) const { return index_of_any<type2, comparator_t>(pos, cmp, n) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_any(const position_t& pos, const type2* cmp, size_t n) const volatile { return index_of_any<type2, comparator_t>(pos, cmp, n) != const_max_int_v<size_t>; }


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_any(size_t i, size_t n, const type2* cmp, size_t cmpLength) const { return index_of_any<type2, comparator_t>(i, n, cmp, cmpLength) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_any(size_t i, size_t n, const type2* cmp, size_t cmpLength) const volatile { return index_of_any<type2, comparator_t>(i, n, cmp, cmpLength) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_any(const position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const { return index_of_any<type2, comparator_t>(pos, n, cmp, cmpLength) != const_max_int_v<size_t>; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_any(const position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const volatile { return index_of_any<type2, comparator_t>(pos, n, cmp, cmpLength) != const_max_int_v<size_t>; }


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const type2* cmp, size_t cmpLength) const { return index_of_segment<type2, comparator_t>(0, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)) != -1; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, const type2* cmp, size_t cmpLength) const { return index_of_segment<type2, comparator_t>(i, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)) != -1; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, size_t n, const type2* cmp, size_t cmpLength) const { return index_of_segment<type2, comparator_t>(i, n, vector<type2>::contain(cmp, cmpLength)) != -1; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, const type2* cmp, size_t cmpLength) const { return get_end_position() != position_of_segment<type2, comparator_t>(pos, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const { return get_end_position() != position_of_segment<type2, comparator_t>(pos, n, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const type2* cmp, size_t cmpLength) const volatile { return index_of_segment<type2, comparator_t>(0, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)) != -1; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, const type2* cmp, size_t cmpLength) const volatile { return index_of_segment<type2, comparator_t>(i, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)) != -1; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, size_t n, const type2* cmp, size_t cmpLength) const volatile { return index_of_segment<type2, comparator_t>(i, n, vector<type2>::contain(cmp, cmpLength)) != -1; }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, const type2* cmp, size_t cmpLength) const volatile { return get_end_position() != position_of_segment<type2, comparator_t>(pos, const_max_int_v<size_t>, vector<type2>::contain(cmp, cmpLength)); }

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, size_t n, const type2* cmp, size_t cmpLength) const volatile { return get_end_position() != position_of_segment<type2, comparator_t>(pos, n, vector<type2>::contain(cmp, cmpLength)); }


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const vector<type2>& cmp) const
	{
		return index_of_segment<type2, comparator_t>(0, const_max_int_v<size_t>, cmp) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const vector<type2>& cmp) const volatile
	{
		return index_of_segment<type2, comparator_t>(0, const_max_int_v<size_t>, cmp) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const volatile vector<type2>& cmp) const
	{
		return index_of_segment<type2, comparator_t>(0, const_max_int_v<size_t>, cmp) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return index_of_segment<type2, comparator_t>(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return index_of_segment<type2, comparator_t>(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return index_of_segment<type2, comparator_t>(0, const_max_int_v<size_t>, cmp, cmpIndex, cmpLength) != -1;
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, const vector<type2>& cmp) const
	{
		return index_of_segment<type2, comparator_t>(i, const_max_int_v<size_t>, cmp) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, const vector<type2>& cmp) const volatile
	{
		return index_of_segment<type2, comparator_t>(i, const_max_int_v<size_t>, cmp) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, const volatile vector<type2>& cmp) const
	{
		return index_of_segment<type2, comparator_t>(i, const_max_int_v<size_t>, cmp) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return index_of_segment<type2, comparator_t>(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return index_of_segment<type2, comparator_t>(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return index_of_segment<type2, comparator_t>(i, const_max_int_v<size_t>, cmp, cmpIndex, cmpLength) != -1;
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, size_t n, const vector<type2>& cmp) const
	{
		return index_of_segment<type2, comparator_t>(i, n, cmp) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, size_t n, const vector<type2>& cmp) const volatile
	{
		return index_of_segment<type2, comparator_t>(i, n, cmp) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, size_t n, const volatile vector<type2>& cmp) const
	{
		return index_of_segment<type2, comparator_t>(i, n, cmp) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return index_of_segment<type2, comparator_t>(i, n, *(cmp.m_contents), cmpIndex, cmpLength) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return index_of_segment<type2, comparator_t>(i, n, *(cmp.m_contents), cmpIndex, cmpLength) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, size_t n, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return index_of_segment<type2, comparator_t>(i, n, cmp, cmpIndex, cmpLength) != -1;
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return index_of_segment<type2, comparator_t>(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos, cmpLength) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return index_of_segment<type2, comparator_t>(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos, cmpLength) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return index_of_segment<type2, comparator_t>(0, const_max_int_v<size_t>, cmp, cmpPos, cmpLength) != -1;
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return index_of_segment<type2, comparator_t>(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos, cmpLength) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return index_of_segment<type2, comparator_t>(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos, cmpLength) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return index_of_segment<type2, comparator_t>(i, const_max_int_v<size_t>, cmp, cmpPos, cmpLength) != -1;
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return index_of_segment<type2, comparator_t>(i, n, *(cmp.m_contents), cmpPos, cmpLength) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return index_of_segment<type2, comparator_t>(i, n, *(cmp.m_contents), cmpPos, cmpLength) != -1;
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(size_t i, size_t n, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return index_of_segment<type2, comparator_t>(i, n, cmp, cmpPos, cmpLength) != -1;
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, const vector<type2>& cmp) const
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, const vector<type2>& cmp) const volatile
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, const_max_int_v<size_t>, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, const volatile vector<type2>& cmp) const
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, const_max_int_v<size_t>, cmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, const_max_int_v<size_t>, cmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, const_max_int_v<size_t>, cmp, cmpPos, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, size_t n, const vector<type2>& cmp) const
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, n, cmp);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, size_t n, const vector<type2>& cmp) const volatile
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, n, cmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, size_t n, const volatile vector<type2>& cmp) const
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, n, cmp);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, size_t n, const composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, size_t n, const volatile composite_vector<type2>& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, n, cmp, cmpIndex, cmpLength);
	}


	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, n, *(cmp.m_contents), cmpPos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, size_t n, const composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, n, *(cmp.m_contents), cmpPos, cmpLength);
	}

	template <typename type2 = type, class comparator_t = default_comparator >
	bool contains_segment(const position_t& pos, size_t n, const volatile composite_vector<type2>& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != position_of_segment<type2, comparator_t>(pos, n, cmp, cmpPos, cmpLength);
	}


	void reverse()
	{
		vector<type> v = composite();
		v.reverse();
		assign(v);
	}

	size_t get_inner_count() const { return m_contents->m_vectorVector.get_length(); }
	size_t get_inner_count() const volatile { return m_contents.begin_read()->m_vectorVector.get_length(); }

	const vector<type>& get_inner_array(size_t i) const { return m_contents->m_vectorVector.get_const_ptr()[i]; }
	const vector<type> get_inner_array(size_t i) const volatile { return m_contents.begin_read()->m_vectorVector.get_const_ptr()[i]; }

	const vector<type>& get_inner(size_t i) const { return m_contents->m_vectorVector.get_const_ptr()[i]; }
	const vector<type> get_inner(size_t i) const volatile { return m_contents.begin_read()->m_vectorVector.get_const_ptr()[i]; }

	void reserve_sub_arrays(size_t n) { return m_contents->m_vectorVector.reserve(n); }

	void reset() { m_contents->reset(); }
	void reset() volatile { this_t tmp; exchange(tmp); }

	const type& operator[](const position_t& pos) const { return get_inner(pos.get_outer_index())[pos.get_inner_index()]; }


	this_t subrange(size_t i) const
	{
		this_t result(*this);
		result.set_to_subrange(i);
		return result;
	}

	this_t subrange(size_t i, size_t n) const
	{
		this_t result(*this);
		result.set_to_subrange(i, n);
		return result;
	}

	this_t subrange(size_t i) const volatile
	{
		this_t result(*this);
		result.set_to_subrange(i);
		return result;
	}

	this_t subrange(size_t i, size_t n) const volatile
	{
		this_t result(*this);
		result.set_to_subrange(i, n);
		return result;
	}

	this_t subrange(const position_t& start) const
	{
		this_t result(*this);
		result.set_to_subrange(start);
		return result;
	}

	this_t subrange(const position_t& start, size_t n) const
	{
		this_t result(*this);
		result.set_to_subrange(start, n);
		return result;
	}

	this_t subrange(const position_t& start) const volatile
	{
		this_t result(*this);
		result.set_to_subrange(start);
		return result;
	}

	this_t subrange(const position_t& start, size_t n) const volatile
	{
		this_t result(*this);
		result.set_to_subrange(start, n);
		return result;
	}

	this_t subrange(const position_t& start, const position_t& end) const
	{
		this_t result(*this);
		result.set_to_subrange(start, end);
		return result;
	}

	this_t subrange(const position_t& start, const position_t& end) const volatile
	{
		this_t result(*this);
		result.set_to_subrange(start, end);
		return result;
	}

	template <typename type2>
	void assign(size_t n, const type2& src) { assign(vector<type>(n, src)); }

	template <typename type2>
	void assign(size_t n, const type2& src) volatile { assign(vector<type>(n, src)); }

	template <typename type2>
	void assign(const type2* src, size_t n) { assign(vector<type>(src, n)); }

	template <typename type2>
	void assign(const type2* src, size_t n) volatile { assign(vector<type>(src, n)); }

	void assign(const this_t& src) { m_contents = src.m_contents; }

	void assign(const volatile this_t& src) { m_contents = *(src.m_contents.begin_read()); }

	void assign(const this_t& src) volatile { m_contents = src.m_contents; }

	template <typename type2>
	void assign(const vector<type2>& src) { *m_contents = src; }

	template <typename type2>
	void assign(const volatile vector<type2>& src) { *m_contents = src; }

	template <typename type2>
	void assign(const vector<type2>& src) volatile { m_contents.set(src); }

	this_t& operator=(const this_t& src) { assign(src); return *this; }
	this_t& operator=(const volatile this_t& src) { assign(src); return *this; }
	volatile this_t& operator=(const this_t& src) volatile { assign(src); return *this; }

	this_t& operator=(this_t&& src)
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}

	volatile this_t& operator=(this_t&& src) volatile
	{
		m_contents = std::move(src.m_contents);
		return *this;
	}

	template <typename type2>
	this_t& operator=(const vector<type2>& src) { assign(src); return *this; }

	template <typename type2>
	this_t& operator=(const volatile vector<type2>& src) { assign(src); return *this; }

	template <typename type2>
	volatile this_t& operator=(const vector<type2>& src) volatile { assign(src); return *this; }

	template <typename type2>
	void append(size_t n, const type2& src) { m_contents->append(n, src); }

	template <typename type2>
	void append(const type2* src, size_t n) { m_contents->append(src, n); }


	void append(const volatile this_t& src) { m_contents->append(*(src.m_contents.begin_read())); }

	void append(const this_t& src) { m_contents->append(*(src.m_contents)); }


	template <typename type2>
	void append(const vector<type2>& src) { m_contents->append(src); }

	template <typename type2>
	void append(const volatile vector<type2>& src) { m_contents->append(src); }


	template <typename type2>
	this_t& operator+=(const composite_vector<type2>& src)
	{
		m_contents->append(src);
		return *this;
	}

	template <typename type2>
	this_t& operator+=(const volatile composite_vector<type2>& src)
	{
		m_contents->append(src);
		return *this;
	}

	template <typename type2>
	this_t& operator+=(const vector<type2>& src)
	{
		m_contents->append(src);
		return *this;
	}

	template <typename type2>
	this_t& operator+=(const volatile vector<type2>& src)
	{
		m_contents->append(src);
		return *this;
	}


	template <typename type2>
	this_t operator+(const composite_vector<type2>& src)
	{
		this_t result(*this);
		result.m_contents->append(src);
		return result;
	}

	template <typename type2>
	this_t operator+(const volatile composite_vector<type2>& src)
	{
		this_t result(*this);
		result.m_contents->append(src);
		return result;
	}

	template <typename type2>
	this_t operator+(const vector<type2>& src)
	{
		this_t result(*this);
		result.m_contents->append(src);
		return result;
	}

	template <typename type2>
	this_t operator+(const volatile vector<type2>& src)
	{
		this_t result(*this);
		result.m_contents->append(src);
		return result;
	}


	template <typename type2>
	void prepend(size_t n, const type2& src) { m_contents->prepend(n, src); }

	template <typename type2>
	void prepend(const type2* src, size_t n) { m_contents->prepend(src, n); }


	void prepend(const this_t& src) { m_contents->prepend(*(src.m_contents)); }

	void prepend(const volatile this_t& src) { m_contents->prepend(*(src.m_contents.begin_read())); }


	template <typename type2>
	void prepend(const vector<type2>& src) { m_contents->prepend(src); }

	template <typename type2>
	void prepend(const volatile vector<type2>& src) { m_contents->prepend(src); }


	void insert(size_t i, size_t n) { m_contents->insert(i, vector<type>(n)); }

	template <typename type2>
	void insert(size_t i, size_t n, type2& src) { m_contents->insert(i, vector<type>(n, src)); }

	template <typename type2>
	void insert(size_t i, type2* src, size_t n) { m_contents->insert(i, vector<type2>::contain(src, n)); }


	void insert(const position_t& pos, size_t n) { m_contents->insert(pos.m_pos, vector<type>(n)); }

	template <typename type2>
	void insert(const position_t& pos, size_t n, type2& src) { m_contents->insert(pos.m_pos, vector<type>(n, src)); }

	template <typename type2>
	void insert(const position_t& pos, type2* src, size_t n) { m_contents->insert(pos.m_pos, vector<type2>::contain(src, n)); }


	template <typename type2>
	void insert(size_t i, const vector<type2>& src)
	{
		m_contents->insert(i, src);
	}

	template <typename type2>
	void insert(size_t i, const volatile vector<type2>& src)
	{
		m_contents->insert(i, src);
	}

	template <typename type2>
	void insert(size_t i, const composite_vector<type2>& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert(i, this_t(src, srcIndex, n));
	}

	template <typename type2>
	void insert(size_t i, const volatile composite_vector<type2>& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert(i, this_t(src, srcIndex, n));
	}

	template <typename type2>
	void insert(size_t i, const composite_vector<type2>& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert(i, this_t(src, srcPos, n));
	}

	template <typename type2>
	void insert(size_t i, const volatile composite_vector<type2>& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert(i, this_t(src, srcPos, n));
	}


	template <typename type2>
	void insert(const position_t& pos, const vector<type2>& src)
	{
		m_contents->insert(pos.m_pos, src);
	}

	template <typename type2>
	void insert(const position_t& pos, const volatile vector<type2>& src)
	{
		m_contents->insert(pos.m_pos, src);
	}

	template <typename type2>
	void insert(const position_t& pos, const composite_vector<type2>& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert(pos.m_pos, this_t(src, srcIndex, n));
	}

	template <typename type2>
	void insert(const position_t& pos, const volatile composite_vector<type2>& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert(pos.m_pos, this_t(src, srcIndex, n));
	}

	template <typename type2>
	void insert(const position_t& pos, const composite_vector<type2>& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert(pos.m_pos, this_t(src, srcPos, n));
	}

	template <typename type2>
	void insert(const position_t& pos, const volatile composite_vector<type2>& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert(pos.m_pos, this_t(src, srcPos, n));
	}


	template <typename type2>
	void replace(size_t i, size_t replaceLength, type2& src)
	{
		m_contents->replace(i, vector<type>(replaceLength, src));
	}

	template <typename type2>
	void replace(size_t i, const type2* src, size_t replaceLength)
	{
		m_contents->replace(i, vector<type2>::contain(src, replaceLength));
	}

	template <typename type2>
	void replace(const position_t& pos, size_t replaceLength, type2& src)
	{
		m_contents->replace(pos.m_pos, vector<type>(replaceLength, src));
	}

	template <typename type2>
	void replace(const position_t& pos, type2* src, size_t replaceLength)
	{
		m_contents->replace(pos.m_pos, vector<type2>::contain(src, replaceLength));
	}

	template <typename type2>
	void replace(size_t i, const vector<type2>& src)
	{
		m_contents->replace(i, src);
	}

	template <typename type2>
	void replace(size_t i, const volatile vector<type2>& src)
	{
		m_contents->replace(i, vector<type>(src));
	}

	template <typename type2>
	void replace(size_t i, const composite_vector<type2>& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents->replace(i, src, srcIndex, n);
	}

	template <typename type2>
	void replace(size_t i, const volatile composite_vector<type2>& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents->replace(i, src, srcIndex, n);
	}

	template <typename type2>
	void replace(size_t i, const composite_vector<type2>& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents->replace(i, src, srcPos, n);
	}

	template <typename type2>
	void replace(size_t i, const volatile composite_vector<type2>& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents->replace(i, src, srcPos, n);
	}


	template <typename type2>
	void replace(const position_t& pos, const vector<type2>& src)
	{
		m_contents->replace(pos.m_pos, src);
	}

	template <typename type2>
	void replace(const position_t& pos, const volatile vector<type2>& src)
	{
		m_contents->replace(pos.m_pos, vector<type>(src));
	}

	template <typename type2>
	void replace(const position_t& pos, const composite_vector<type2>& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents->replace(pos.m_pos, src, srcIndex, n);
	}

	template <typename type2>
	void replace(const position_t& pos, const volatile composite_vector<type2>& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents->replace(pos.m_pos, src, srcIndex, n);
	}

	template <typename type2>
	void replace(const position_t& pos, const composite_vector<type2>& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents->replace(pos.m_pos, src, srcPos, n);
	}

	template <typename type2>
	void replace(const position_t& pos, const volatile composite_vector<type2>& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents->replace(pos.m_pos, src, srcPos, n);
	}


	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, size_t insertLength, type2& src)
	{
		m_contents->insert_replace(i, replaceLength, vector<type>(insertLength, src));
	}

	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, type2* src, size_t insertLength)
	{
		m_contents->insert_replace(i, replaceLength, vector<type>(src, insertLength));
	}

	template <typename type2>
	void insert_replace(const position_t& pos, size_t replaceLength, size_t insertLength, type2& src)
	{
		m_contents->insert_replace(pos.m_pos, replaceLength, vector<type>(insertLength, src));
	}

	template <typename type2>
	void insert_replace(const position_t& pos, size_t replaceLength, type2* src, size_t insertLength)
	{
		m_contents->insert_replace(pos.m_pos, replaceLength, vector<type>(src, insertLength));
	}


	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, const vector<type2>& src)
	{
		m_contents->insert_replace(i, replaceLength, src);
	}

	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, const volatile vector<type2>& src)
	{
		m_contents->insert_replace(i, replaceLength, vector<type>(src));
	}

	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, const composite_vector<type2>& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert_replace(i, replaceLength, this_t(src, srcIndex, n));
	}

	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, const volatile composite_vector<type2>& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert_replace(i, replaceLength, this_t(src, srcIndex, n));
	}

	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, const composite_vector<type2>& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert_replace(i, replaceLength, this_t(src, srcPos, n));
	}

	template <typename type2>
	void insert_replace(size_t i, size_t replaceLength, const volatile composite_vector<type2>& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert_replace(i, replaceLength, this_t(src, srcPos, n));
	}


	template <typename type2>
	void insert_replace(const position_t& pos, size_t replaceLength, const vector<type2>& src)
	{
		m_contents->insert_replace(pos.m_pos, replaceLength, src);
	}

	template <typename type2>
	void insert_replace(const position_t& pos, size_t replaceLength, const volatile vector<type2>& src)
	{
		m_contents->insert_replace(pos.m_pos, replaceLength, vector<type>(src));
	}

	template <typename type2>
	void insert_replace(const position_t& pos, size_t replaceLength, const composite_vector<type2>& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert_replace(pos.m_pos, replaceLength, this_t(src, srcIndex, n));
	}

	template <typename type2>
	void insert_replace(const position_t& pos, size_t replaceLength, const volatile composite_vector<type2>& src, size_t srcIndex = 0, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert_replace(pos.m_pos, replaceLength, this_t(src, srcIndex, n));
	}

	template <typename type2>
	void insert_replace(const position_t& pos, size_t replaceLength, const composite_vector<type2>& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert_replace(pos.m_pos, replaceLength, this_t(src, srcPos, n));
	}

	template <typename type2>
	void insert_replace(const position_t& pos, size_t replaceLength, const volatile composite_vector<type2>& src, const position_t& srcPos, size_t n = const_max_int_v<size_t>)
	{
		m_contents->insert_replace(pos.m_pos, replaceLength, this_t(src, srcPos, n));
	}


	void erase(size_t i, size_t n = const_max_int_v<size_t>) { m_contents->erase(i, n); }
	void erase(const position_t& startPos, size_t n = const_max_int_v<size_t>) { m_contents->erase(startPos, n); }

	void clear() { m_contents->clear(); }
	void clear() volatile
	{
		this_t tmp;
		exchange(tmp);
	}

	void swap(this_t& wth) { m_contents.swap(wth.m_contents); }

	void swap(this_t& wth) volatile { m_contents.swap(wth.m_contents); }
	void swap(volatile this_t& wth) { wth.m_contents.swap(m_contents); }

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


	void advance_arrays(size_t n) { m_contents->advance_arrays(n); }
	void advance_arrays(size_t n) volatile
	{
		write_token wt;
		do {
			m_contents.begin_write(wt);
			wt->advance_arrays(n);
		} while (!m_contents.end_write(wt));
	}

	void truncate_arrays(size_t n) { m_contents->truncate_arrays(n); }

	void truncate_arrays(size_t n) volatile
	{
		write_token wt;
		do {
			m_contents.begin_write(wt);
			wt->truncate_arrays(n);
		} while (!m_contents.end_write(wt));
	}


	void truncate_arrays_to(size_t n) { m_contents->truncate_arrays_to(n); }

	void truncate_arrays_to(size_t n) volatile
	{
		write_token wt;
		do {
			m_contents.begin_write(wt);
			wt->truncate_arrays_to(n);
		} while (!m_contents.end_write(wt));
	}

	void advance_array() { advance_arrays(1); }
	void truncate_array() { truncate_arrays(1); }

	vector<type> pop_first_array() { return m_contents->pop_first_array(); }

	vector<type> pop_first_array() volatile
	{
		vector<type> result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			result = wt->pop_first_array();
			if (!result)
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}

	vector<type> pop_last_array() { return m_contents->pop_last_array(); }

	vector<type> pop_last_array() volatile
	{
		vector<type> result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			result = wt->pop_last_array();
			if (!result)
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}

	this_t split_off_arrays_before(size_t i)
	{
		this_t result;
		m_contents->split_off_arrays_before(i, *(result.m_contents));
		return result;
	}

	this_t split_off_arrays_before(size_t i) volatile
	{
		this_t result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			result = wt->split_off_arrays_before(i, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}

	this_t split_off_arrays_after(size_t i)
	{
		this_t result;
		m_contents->split_off_arrays_after(i, *(result.m_contents));
		return result;
	}

	this_t split_off_arrays_after(size_t i) volatile
	{
		this_t result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			result = wt->split_off_arrays_after(i, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}


	this_t split_off_before(size_t i)
	{
		this_t result;
		m_contents->split_off_before(i, *(result.m_contents));
		return result;
	}

	this_t split_off_before(size_t i) volatile
	{
		this_t result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			result = wt->split_off_before(i, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}

	this_t split_off_before(const position_t& pos)
	{
		this_t result;
		m_contents->split_off_before(pos.m_pos, *(result.m_contents));
		return result;
	}

	this_t split_off_before(const position_t& pos) volatile
	{
		this_t result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			result = wt->split_off_before(pos.m_pos, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}


	this_t split_off_after(size_t i)
	{
		this_t result;
		m_contents->split_off_after(i, *(result.m_contents));
		return result;
	}

	this_t split_off_after(size_t i) volatile
	{
		this_t result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			result = wt->split_off_after(i, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}


	this_t split_off_after(const position_t& pos)
	{
		this_t result;
		m_contents->split_off_after(pos.m_pos, *(result.m_contents));
		return result;
	}

	this_t split_off_after(const position_t& pos) volatile
	{
		this_t result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			result = wt->split_off_after(pos.m_pos, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}


	this_t split_off_front(size_t n)
	{
		return split_off_before(n);
	}

	this_t split_off_front(size_t n) volatile
	{
		return split_off_before(n);
	}

	this_t split_off_back(size_t n)
	{
		this_t result;
		m_contents->split_off_back(n, *(result.m_contents));
		return result;
	}

	this_t split_off_back(size_t n) volatile
	{
		this_t result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			result = wt->split_off_back(n, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}

	void set_to_subrange(size_t i, size_t n = const_max_int_v<size_t>)
	{
		m_contents->set_to_subrange(i, n);
	}

	void set_to_subrange(size_t i, size_t n = const_max_int_v<size_t>) volatile
	{
		if (!n)
			clear();
		else
		{
			write_token wt;
			do {
				m_contents.begin_write(wt);
				wt->set_to_subrange(i, n);
			} while (!m_contents.end_write(wt));
		}
	}

	void set_to_subrange(const position_t& start, size_t n)
	{
		m_contents->set_to_subrange(start.m_pos, n);
	}

	void set_to_subrange(const position_t& start, size_t n) volatile
	{
		if (!n)
			clear();
		else
		{
			write_token wt;
			do {
				m_contents.begin_write(wt);
				wt->set_to_subrange(start.m_pos, n);
			} while (!m_contents.end_write(wt));
		}
	}

	void set_to_subrange(const position_t& start, const position_t& end = position_t(const_max_int_v<size_t>, const_max_int_v<size_t>))
	{
		m_contents->set_to_subrange(start.m_pos, end.m_pos);
	}

	void set_to_subrange(const position_t& start, const position_t& end = position_t(const_max_int_v<size_t>, const_max_int_v<size_t>)) volatile
	{
		if (start == end)
			clear();
		else
		{
			write_token wt;
			do {
				m_contents.begin_write(wt);
				wt->set_to_subrange(start.m_pos, end.m_pos);
			} while (!m_contents.end_write(wt));
		}
	}

	void advance(size_t n = 1)
	{
		m_contents->advance(n);
	}

	void advance(size_t n = 1) volatile
	{
		if (!!n)
		{
			write_token wt;
			do {
				m_contents.begin_write(wt);
				wt->advance(n);
			} while (!m_contents.end_write(wt));
		}
	}

	void advance_to(const position_t& pos)
	{
		m_contents->advance_to(pos.m_pos);
	}

	void advance_to(const position_t& pos) volatile
	{
		if (pos > position_t(0, 0))
		{
			write_token wt;
			do {
				m_contents.begin_write(wt);
				wt->advance_to(pos.m_pos);
			} while (!m_contents.end_write(wt));
		}
	}

	void truncate_to(size_t n)
	{
		m_contents->truncate_to(n);
	}

	void truncate_to(size_t n) volatile
	{
		if (!n)
			clear();
		else
		{
			write_token wt;
			do {
				m_contents.begin_write(wt);
				wt->truncate_to(n);
			} while (!m_contents.end_write(wt));
		}
	}

	void truncate_to(const position_t& pos)
	{
		m_contents->truncate_to(pos.m_pos);
	}

	void truncate_to(const position_t& pos) volatile
	{
		if (pos == position_t(0, 0))
			clear();
		else
		{
			write_token wt;
			do {
				m_contents.begin_write(wt);
				wt->truncate_to(pos.m_pos);
			} while (!m_contents.end_write(wt));
		}
	}

	void truncate(size_t n)
	{
		m_contents->truncate(n);
	}

	void truncate(size_t n) volatile
	{
		if (!!n)
		{
			write_token wt;
			do {
				m_contents.begin_write(wt);
				wt->truncate(n);
			} while (!m_contents.end_write(wt));
		}
	}

	void truncate_to_right(size_t n)
	{
		m_contents->truncate_to_right(n);
	}

	void truncate_to_right(size_t n) volatile
	{
		if (!n)
			clear();
		else
		{
			write_token wt;
			do {
				m_contents.begin_write(wt);
				wt->truncate_to_right(n);
			} while (!m_contents.end_write(wt));
		}
	}

	template <typename type2>
	vector<this_t> split_on(const type2& splitOn, include_empty_segments opt = include_empty_segments::yes) const
	{
		return split_on_any_inner<this_t, type2>(*this, &splitOn, 1, opt);
	}

	template <typename type2>
	vector<this_t> split_on(const type2& splitOn, include_empty_segments opt = include_empty_segments::yes) const volatile
	{
		this_t cpy(*this);
		return split_on_any_inner<this_t, type2>(cpy, &splitOn, 1, opt);
	}

	template <typename type2>
	vector<this_t> split_on_any(const type2* splitOn, size_t n, include_empty_segments opt = include_empty_segments::yes) const
	{
		return split_on_any_inner<this_t, type2>(*this, splitOn, n, opt);
	}

	template <typename type2>
	vector<this_t> split_on_any(const type2* splitOn, size_t n, include_empty_segments opt = include_empty_segments::yes) const volatile
	{
		this_t cpy(*this);
		return split_on_any_inner<this_t, type2>(cpy, splitOn, n, opt);
	}

	template <typename type2>
	vector<this_t> split_on_any(const vector<type2>& splitOn, include_empty_segments opt = include_empty_segments::yes) const
	{
		return split_on_any_inner<this_t, type2>(*this, splitOn.get_const_ptr(), splitOn.get_length(), opt);
	}

	template <typename type2>
	vector<this_t> split_on_any(const vector<type2>& splitOn, include_empty_segments opt = include_empty_segments::yes) const volatile
	{
		this_t cpy(*this);
		return split_on_any_inner<this_t, type2>(cpy, splitOn.get_const_ptr(), splitOn.get_length(), opt);
	}

	template <typename type2>
	vector<this_t> split_on_any(const volatile vector<type2>& splitOn, include_empty_segments opt = include_empty_segments::yes) const
	{
		vector<type2> splitOnCpy(splitOn);
		return split_on_any_inner<this_t, type2>(*this, splitOnCpy.get_const_ptr(), splitOnCpy.get_length(), opt);
	}


protected:
	template <class composite_vector_t, typename type2>
	static vector<composite_vector_t> split_on_any_inner(const composite_vector_t& src, const type2* splitOn, size_t n, include_empty_segments opt = include_empty_segments::yes)
	{
		vector<composite_vector_t> result;
		composite_vector_t currentCompositeVector;
		size_t len = src.get_inner_count();
		for (size_t i = 0; i < len; i++)
		{
			typename composite_vector_t::inner_t curSrc = src.get_inner(i);
			COGS_ASSERT(curSrc.get_length() > 0); // I don't think we ever have empty segments.

			// including empty segments gives us a clue as to where each split actually is.
			vector<typename composite_vector_t::inner_t> v = curSrc.split_on_any(splitOn, n, include_empty_segments::yes);
			size_t len2 = v.get_length();
			for (size_t j = 0; j < len2; j++)
			{
				const typename composite_vector_t::inner_t& tmp = v.get_const_ptr()[j];
				if (!!tmp)
					currentCompositeVector += tmp;
				else
				{
					if (!!currentCompositeVector)
						result += currentCompositeVector;
					if (opt == include_empty_segments::yes)
						result += composite_vector_t();
					currentCompositeVector.clear();
				}
			}
		}
		if (!!currentCompositeVector)
			result += currentCompositeVector;

		return result;
	}
};


}


#endif
