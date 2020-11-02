//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//

#ifndef COGS_HEADER_IO_COMPOSITE_BUFFER
#define COGS_HEADER_IO_COMPOSITE_BUFFER


// Status: Good

#include "cogs/collections/composite_vector.hpp"
#include "cogs/collections/composite_string.hpp"
#include "cogs/io/buffer.hpp"
#include "cogs/math/const_max_int.hpp"


namespace cogs {
namespace io {


class composite_buffer;


class composite_buffer_content
{
private:
	friend class composite_buffer;

	vector<vector<char> > m_vectorVector;
	size_t m_length;

public:
	class position_t
	{
	private:
		friend class composite_buffer;
		friend class composite_buffer_content;

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

	composite_buffer_content()
		: m_length(0)
	{ }

	composite_buffer_content(const composite_buffer_content& src)
		: m_vectorVector(src.m_vectorVector),
		m_length(src.m_length)
	{ }

	composite_buffer_content(const buffer& src)
		: m_length(src.get_length())
	{
		if (!!m_length)
			m_vectorVector.assign(1, src.get_vector());
	}

	composite_buffer_content(const volatile buffer& src)
	{
		buffer tmp(src);
		m_length = tmp.get_length();
		if (!!m_length)
			m_vectorVector.assign(1, tmp.get_vector());
	}

	composite_buffer_content& operator=(const composite_buffer_content& src)
	{
		m_vectorVector = src.m_vectorVector;
		m_length = src.m_length;
		return *this;
	}

	composite_buffer_content& operator=(const buffer& src)
	{
		m_vectorVector.assign(1, src.get_vector());
		m_length = src.get_length();
		return *this;
	}

	composite_buffer_content& operator=(const volatile buffer& src)
	{
		buffer tmp(src);
		m_vectorVector.assign(1, tmp.get_vector());
		m_length = tmp.get_length();
		return *this;
	}

	void set(const buffer& src, size_t length)
	{
		m_vectorVector.assign(1, src.get_vector());
		m_length = length;
	}

	void append(const buffer& src)
	{
		size_t n = src.get_length();
		if (!!n)
		{
			if (!m_length)
				set(src, n);
			else
			{
				m_length += n;
				vector<char>& lastSubVector = m_vectorVector.get_ptr()[m_vectorVector.get_length() - 1];
				const char* lastSubVectorBasePtr = lastSubVector.get_ptr();
				size_t lastSubVectorLength = lastSubVector.get_length();
				if (src.get_const_ptr() == lastSubVectorBasePtr + lastSubVectorLength)
					lastSubVector.m_contents->m_length += lastSubVectorLength;
				else
					m_vectorVector.append(1, src.get_vector());
			}
		}
	}

	void append(const composite_buffer_content& src)
	{
		size_t srcSubVectorCount = src.m_vectorVector.get_length();
		if (srcSubVectorCount == 1)
			append(buffer::from_vector(src.m_vectorVector.get_const_ptr()[0]));
		else if (!!srcSubVectorCount)
		{
			m_vectorVector.append(src.m_vectorVector);
			m_length += src.m_length;
		}
	}

	void append(const volatile buffer& src)
	{
		buffer tmp(src);
		append(tmp);
	}

	void prepend(const buffer& src)
	{
		size_t n = src.get_length();
		if (!!n)
		{
			if (!m_length)
				set(src, n);
			else
			{
				m_length += n;
				vector<char>& firstSubVector = m_vectorVector.get_first();
				const char* firstSubVectorBasePtr = firstSubVector.get_ptr();
				if (src.get_const_ptr() == firstSubVectorBasePtr - n)
				{
					firstSubVector.m_contents->m_ptr -= n;
					firstSubVector.m_contents->m_length += n;
				}
				else
					m_vectorVector.prepend(1, src.get_vector());
			}
		}
	}

	void prepend(const composite_buffer_content& src)
	{
		size_t srcSubVectorCount = src.m_vectorVector.get_length();
		if (srcSubVectorCount == 1)
			prepend(buffer::from_vector(src.m_vectorVector.get_const_ptr()[0]));
		else if (!!srcSubVectorCount)
		{
			m_vectorVector.prepend(src.m_vectorVector);
			m_length += src.m_length;
		}
	}

	void prepend(const volatile buffer& src)
	{
		buffer tmp(src);
		prepend(tmp);
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

	void swap(composite_buffer_content& wth)
	{
		m_vectorVector.swap(wth.m_vectorVector);
		cogs::swap(m_length, wth.m_length);
	}

	void exchange(const composite_buffer_content& src, composite_buffer_content& rtn)
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

	vector<char> pop_first_array()
	{
		vector<char> result;
		if (!!m_length)
		{
			result = m_vectorVector.get_const_ptr()[0];
			m_length -= result.get_length();
			m_vectorVector.advance(1);
		}
		return result;
	}

	vector<char> pop_last_array()
	{
		vector<char> result;
		size_t arrayArrayLength = m_vectorVector.get_length();
		if (!!arrayArrayLength)
		{
			result = m_vectorVector.get_const_ptr()[arrayArrayLength - 1];
			m_length -= result.get_length();
			m_vectorVector.truncate(1);
		}
		return result;
	}

	void split_arrays_fix(composite_buffer_content& dst)
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

	void split_off_arrays_before(size_t i, composite_buffer_content& dst)
	{
		dst.m_vectorVector = m_vectorVector.split_off_before(i);
		split_arrays_fix(dst);
	}

	void split_off_arrays_after(size_t i, composite_buffer_content& dst)
	{
		dst.m_vectorVector = m_vectorVector.split_off_after(i);
		split_arrays_fix(dst);
	}

	// caller error to call with i > length
	position_t calculate_position_from_front(size_t i) const
	{
		position_t result;
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
						//result.m_innerIndex = 0;
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

	void split_off_back(size_t n, composite_buffer_content& dst)
	{
		if (n < m_length)
			split_off_after(m_length - n, dst);
		else
		{
			dst = *this;
			clear();
		}
	}

	void split_off_before(size_t i, composite_buffer_content& dst)
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

	void split_off_after(size_t i, composite_buffer_content& dst)
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

	void split_off_before_inner(const position_t& pos, composite_buffer_content& dst)
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

	void split_off_before(const position_t& pos, composite_buffer_content& dst)
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

	void split_off_after_inner(const position_t& pos, composite_buffer_content& dst)
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

	void split_off_after(const position_t& pos, composite_buffer_content& dst)
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

		const vector<char>& subArray = m_vectorVector.get_const_ptr()[pos.m_outerIndex];
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
		else if (pos < get_last_position())
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
		size_t n = pos.m_innerIndex;
		if (outerIndex <= subVectorCount / 2)
		{
			// Count sub arrays before
			size_t curSubVectorIndex = 0;
			while (curSubVectorIndex < outerIndex)
			{
				n += m_vectorVector.get_const_ptr()[curSubVectorIndex].get_length();
				++curSubVectorIndex;
			}
		}
		else
		{
			// Count sub arrays after
			n += m_length;
			while (outerIndex < subVectorCount)
			{
				n -= m_vectorVector.get_const_ptr()[outerIndex].get_length();
				++outerIndex;
			}
		}

		return n;
	}

	bool equals(size_t i, size_t n, const buffer& cmp) const
	{
		size_t lengthAdjusted = validate_length_from(i, n);
		size_t cmpLength = cmp.get_length();
		if (lengthAdjusted != cmpLength)
			return false;
		if (!lengthAdjusted)
			return true;
		return equals_inner(calculate_position(i), cmp);
	}


	bool equals(size_t i, size_t n, const composite_buffer_content& cmp, size_t cmpIndex, size_t cmpLength) const
	{
		size_t lengthAdjusted = validate_length_from(i, n);
		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
		if (lengthAdjusted != cmpLengthAdjusted)
			return false;
		if (!lengthAdjusted)
			return true;
		position_t endPos;
		return equals_inner(calculate_position(i), cmp, cmp.calculate_position(cmpIndex), cmpLengthAdjusted, endPos);
	}

	// pos must be valid.  Therefor, actual length is assumed to be >0
	bool equals(const position_t& pos, size_t n, const buffer& cmp) const
	{
		size_t cmpLength = cmp.get_length();
		if (!cmpLength)
			return !n;
		size_t lengthAdjusted = validate_length_from(pos, n);
		if (lengthAdjusted != cmpLength)
			return false;
		return equals_inner(pos, cmp);
	}

	// pos must be valid.  Therefor, actual length is assumed to be >0
	bool equals(const position_t& pos, size_t n, const composite_buffer_content& cmp, size_t cmpIndex, size_t cmpLength) const
	{
		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
		if (!cmpLengthAdjusted)
			return !n;
		size_t lengthAdjusted = validate_length_from(pos, n);
		if (lengthAdjusted != cmpLengthAdjusted)
			return false;
		position_t endPos;
		return equals_inner(pos, cmp, cmp.calculate_position(cmpIndex), cmpLengthAdjusted, endPos);
	}

	// pos and cmpPos must be valid.  Therefor, actual lengths are assumed to be >0
	bool equals(const position_t& pos, size_t n, const composite_buffer_content& cmp, const position_t& cmpPos, size_t cmpLength) const
	{
		size_t lengthAdjusted = validate_length_from(pos, n);
		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
		if (lengthAdjusted != cmpLengthAdjusted)
			return false;
		if (!lengthAdjusted)
			return true;
		position_t endPos;
		return equals_inner(pos, cmp, cmpPos, cmpLengthAdjusted, endPos);
	}

	// cmpPos must be valid.  Therefor, actual length is assumed to be >0
	bool equals(size_t i, size_t n, const composite_buffer_content& cmp, const position_t& cmpPos, size_t cmpLength) const
	{
		size_t lengthAdjusted = validate_length_from(i, n);
		if (!lengthAdjusted)
			return !cmpLength;
		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
		if (lengthAdjusted != cmpLengthAdjusted)
			return false;
		position_t endPos;
		return equals_inner(calculate_position(i), cmp, cmpPos, cmpLengthAdjusted, endPos);
	}

	// pos, cmpIndex, lengthAdjust and cmpLengthAdjusted must all be valid.
	bool equals_inner(const position_t& pos, const buffer& cmp) const
	{
		size_t cmpIndex = 0;
		position_t pos2 = pos;
		size_t cmpLength = cmp.get_length();
		for (;;)
		{
			const vector<char>& currentSubVector = m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
			size_t subVectorLength = currentSubVector.get_length();
			size_t compareLengthThisSegment = subVectorLength - pos2.m_innerIndex;
			if (compareLengthThisSegment > cmpLength)
				compareLengthThisSegment = cmpLength;
			if (!currentSubVector.subrange(pos2.m_innerIndex).equals(cmp.get_vector().subrange(cmpIndex, compareLengthThisSegment)))
				return false;
			if (++pos2.m_outerIndex == m_vectorVector.get_length())
				return true;
			cmpIndex += compareLengthThisSegment;
			cmpLength -= compareLengthThisSegment;
			pos2.m_innerIndex = 0;
		}
	}

	// pos and cmpIndex must be valid.  lengthAdjust and cmpLengthAdjusted may extend beyond range, but must be > 0
	// endPos will be set only on success, and may require advancing to the next sub array.
	bool equals_inner(const position_t& pos, const composite_buffer_content& cmp, const position_t& cmpPos, size_t cmpLengthAdjusted, position_t& endPos) const
	{
		position_t pos2 = pos;
		position_t cmpPos2 = cmpPos;
		const vector<char>* currentSubVector = &m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
		const vector<char>* currentCmpSubVector = &cmp.m_vectorVector.get_const_ptr()[cmpPos2.m_outerIndex];
		size_t subVectorLength = currentSubVector->get_length() - pos2.m_innerIndex;
		size_t cmpSubVectorLength = currentCmpSubVector->get_length() - cmpPos2.m_innerIndex;
		for (;;)
		{
			bool isShort = (subVectorLength < cmpSubVectorLength);
			bool isCmpShort = (subVectorLength > cmpSubVectorLength);
			size_t compareLengthThisSegment = isShort ? subVectorLength : cmpSubVectorLength;
			if (compareLengthThisSegment > cmpLengthAdjusted)
				compareLengthThisSegment = cmpLengthAdjusted;
			if (!currentSubVector->subrange(pos2.m_innerIndex).equals(currentCmpSubVector->subrange(cmpPos2.m_innerIndex, compareLengthThisSegment)))
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
				currentSubVector = &m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
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


	bool ends_with(const buffer& cmp) const
	{
		size_t cmpLength = cmp.get_length();
		if (!cmpLength)
			return true;
		if (m_length < cmpLength)
			return false;
		return equals_inner(calculate_position(m_length - cmpLength), cmp);
	}

	bool ends_with(const composite_buffer_content& cmp, size_t cmpIndex, size_t cmpLength) const
	{
		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
		if (!cmpLengthAdjusted)
			return true;
		if (m_length < cmpLengthAdjusted)
			return false;
		position_t endPos;
		return equals_inner(calculate_position(m_length - cmpLengthAdjusted), cmp, cmp.calculate_position(cmpIndex), cmpLengthAdjusted, endPos);
	}

	// cmpPos must be valid.  Therefor, actual length is assumed to be >0
	bool ends_with(const composite_buffer_content& cmp, const position_t& cmpPos, size_t cmpLength) const
	{
		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
		if (!cmpLengthAdjusted)
			return true;
		if (m_length < cmpLengthAdjusted)
			return false;
		position_t endPos;
		return equals_inner(calculate_position(m_length - cmpLengthAdjusted), cmp, cmpPos, cmpLengthAdjusted, endPos);
	}


	int compare(size_t i, size_t n, const buffer& cmp) const
	{
		size_t lengthAdjusted = validate_length_from(i, n);
		if (!lengthAdjusted)
			return !cmp.get_length() ? 0 : -1;
		return compare_inner(calculate_position(i), lengthAdjusted, cmp);
	}

	int compare(size_t i, size_t n, const composite_buffer_content& cmp, size_t cmpIndex, size_t cmpLength) const
	{
		size_t lengthAdjusted = validate_length_from(i, n);
		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
		if (!lengthAdjusted)
			return !cmpLengthAdjusted ? 0 : -1;
		if (!cmpLengthAdjusted)
			return 1;
		return compare_inner(calculate_position(i), lengthAdjusted, cmp, cmp.calculate_position(cmpIndex), cmpLengthAdjusted);
	}

	int compare(const position_t& pos, size_t n, const buffer& cmp) const
	{
		size_t lengthAdjusted = validate_length_from(pos, n);
		size_t cmpLength = cmp.get_length();
		if (!lengthAdjusted)
			return !cmpLength ? 0 : -1;
		if (!cmpLength)
			return 1;
		return compare_inner(pos, lengthAdjusted, cmp);
	}

	int compare(const position_t& pos, size_t n, const composite_buffer_content& cmp, size_t cmpIndex, size_t cmpLength) const
	{
		size_t lengthAdjusted = validate_length_from(pos, n);
		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
		if (!lengthAdjusted)
			return !cmpLength ? 0 : -1;
		if (!cmpLengthAdjusted)
			return 1;
		return compare_inner(pos, n, cmp, cmp.calculate_position(cmpIndex), cmpLengthAdjusted);
	}

	int compare(const position_t& pos, size_t n, const composite_buffer_content& cmp, const position_t& cmpPos, size_t cmpLength) const
	{
		size_t lengthAdjusted = validate_length_from(pos, n);
		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
		if (!lengthAdjusted)
			return !cmpLength ? 0 : -1;
		if (!cmpLengthAdjusted)
			return 1;
		return compare_inner(pos, lengthAdjusted, cmp, cmpPos, cmpLengthAdjusted);
	}

	int compare(size_t i, size_t n, const composite_buffer_content& cmp, const position_t& cmpPos, size_t cmpLength) const
	{
		size_t lengthAdjusted = validate_length_from(i, n);
		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
		if (!lengthAdjusted)
			return !cmpLength ? 0 : -1;
		if (!cmpLengthAdjusted)
			return 1;
		return compare_inner(calculate_position(i), lengthAdjusted, cmp, cmpPos, cmpLengthAdjusted);
	}

	// pos and cmpIndex must be valid.  lengthAdjust and cmpLengthAdjusted may extend beyond range
	int compare_inner(const position_t& pos, size_t lengthAdjusted, const buffer& cmp) const
	{
		position_t pos2 = pos;
		size_t cmpIndex = 0;
		size_t cmpLength = cmp.get_length();
		for (;;)
		{
			const vector<char>& currentSubVector = m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
			size_t subVectorLength = currentSubVector.get_length();
			size_t compareLengthThisSegment = subVectorLength - pos2.m_innerIndex;
			if (compareLengthThisSegment > cmpLength)
				compareLengthThisSegment = cmpLength;
			if (compareLengthThisSegment > lengthAdjusted)
				compareLengthThisSegment = lengthAdjusted;
			int i = currentSubVector.subrange(pos2.m_innerIndex).compare(cmp.get_vector().subrange(cmpIndex, compareLengthThisSegment));
			if (i != 0)
				return i;
			if (++pos2.m_outerIndex == m_vectorVector.get_length())
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
	int compare_inner(const position_t& pos, size_t lengthAdjusted, const composite_buffer_content& cmp, const position_t& cmpPos, size_t cmpLengthAdjusted) const
	{
		position_t pos2 = pos;
		position_t cmpPos2 = cmpPos;
		const vector<char>* currentSubVector = &m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
		const vector<char>* currentCmpSubVector = &cmp.m_vectorVector.get_const_ptr()[cmpPos2.m_outerIndex];
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
			int i = currentSubVector->subrange(pos2.m_innerIndex).compare(currentCmpSubVector->subrange(cmpPos2.m_innerIndex, compareLengthThisSegment));
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
				if (++pos2.m_outerIndex == m_vectorVector.get_length())
					return !isCmpShort ? -1 : ((++cmpPos2.m_outerIndex == cmp.m_vectorVector.get_length()) ? 0 : 1);
				currentSubVector = &m_vectorVector.get_const_ptr()[pos2.m_outerIndex];
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


	size_t index_of(size_t i, size_t n, const char& cmp) const
	{
		size_t lengthAdjusted = validate_length_from(i, n);
		if (!lengthAdjusted || (i >= lengthAdjusted))
			return (size_t)-1;

		position_t pos = calculate_position(i);
		return find_inner(i, pos, lengthAdjusted, cmp);
	}


	position_t position_of(const position_t& pos, size_t n, const char& cmp) const
	{
		size_t i = calculate_index(pos);
		size_t lengthAdjusted = validate_length_from(i, n);
		if (!lengthAdjusted || (i >= lengthAdjusted))
			return get_end_position();

		position_t pos2 = pos;
		find_inner(i, pos2, lengthAdjusted, cmp);
		return pos2;
	}


	size_t find_inner(size_t i, position_t& pos, size_t n, const char& cmp) const
	{
		size_t subVectorCount = m_vectorVector.get_length();
		size_t curSize = i - pos.m_innerIndex;
		for (;;)
		{
			const vector<char>& curSubVector = m_vectorVector.get_const_ptr()[pos.m_outerIndex];
			pos.m_innerIndex = curSubVector.index_of(pos.m_innerIndex, cmp);
			if (pos.m_innerIndex != (size_t)-1)
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

	size_t index_of_any(size_t i, size_t n, const char* cmp, size_t cmpCount) const
	{
		size_t lengthAdjusted = validate_length_from(i, n);
		if (!lengthAdjusted || (i >= lengthAdjusted))
			return const_max_int_v<size_t>;

		position_t pos = calculate_position(i);
		return find_any_inner(i, pos, lengthAdjusted, cmp, cmpCount);
	}

	position_t position_of_any(const position_t& pos, size_t n, const char* cmp, size_t cmpCount) const
	{
		size_t i = calculate_index(pos);
		size_t lengthAdjusted = validate_length_from(i, n);
		if (!lengthAdjusted || (i >= lengthAdjusted))
			return get_end_position();

		position_t pos2 = pos;
		find_any_inner(i, pos2, lengthAdjusted, cmp, cmpCount);
		return pos2;
	}

	size_t find_any_inner(size_t i, position_t& pos, size_t n, const char* cmp, size_t cmpCount) const
	{
		size_t subVectorCount = m_vectorVector.get_length();
		size_t curSize = i - pos.m_innerIndex;
		for (;;)
		{
			const vector<char>& curSubVector = m_vectorVector.get_const_ptr()[pos.m_outerIndex];
			pos.m_innerIndex = curSubVector.index_of_any(pos.m_innerIndex, cmp, cmpCount);
			if (pos.m_innerIndex != (size_t)-1)
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


	size_t index_of_segment(size_t i, size_t n, const buffer& cmp) const
	{
		if (i >= m_length)
			return const_max_int_v<size_t>;

		size_t cmpLength = cmp.get_length();
		if (!cmpLength)
			return const_max_int_v<size_t>; // validate cmpLength and cmdLengthAdjusted

		size_t lengthAdjusted = m_length - i;
		if (lengthAdjusted > n)
			lengthAdjusted = n;
		if (cmpLength > lengthAdjusted) // Ensure m_length is larger enough for cmp, and >0
			return const_max_int_v<size_t>;

		position_t pos = calculate_position(i);
		position_t endPos;
		return find_segment_inner(i, pos, lengthAdjusted, cmp, endPos);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const buffer& cmp) const
	{
		size_t i = calculate_index(pos);
		size_t cmpLength = cmp.get_length();
		if (!cmpLength)
			return get_end_position(); // validate cmpLength and cmdLengthAdjusted

		size_t lengthAdjusted = m_length - i;
		if (lengthAdjusted > n)
			lengthAdjusted = n;
		if (cmpLength > lengthAdjusted) // Ensure m_length is larger enough for cmp, and >0
			return get_end_position();

		position_t newPos = pos;
		position_t endPos;
		find_segment_inner(i, newPos, lengthAdjusted, cmp, endPos);
		return newPos;
	}

	size_t index_of_segment(size_t i, size_t n, const composite_buffer_content& cmp, size_t cmpIndex, size_t cmpLength) const
	{
		if (i >= m_length)
			return const_max_int_v<size_t>;

		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
		if (!cmpLengthAdjusted)
			return const_max_int_v<size_t>; // validate cmpLength and cmdLengthAdjusted

		size_t lengthAdjusted = m_length - i;
		if (lengthAdjusted > n)
			lengthAdjusted = n;
		if (cmpLengthAdjusted > lengthAdjusted) // Ensure m_length is larger enough for cmp, and >0
			return const_max_int_v<size_t>;

		position_t pos = calculate_position(i);
		position_t cmpPos = cmp.calculate_position(cmpIndex);
		return find_segment_inner(i, pos, lengthAdjusted, cmp, cmpPos, cmpLengthAdjusted);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const composite_buffer_content& cmp, size_t cmpIndex, size_t cmpLength) const
	{
		size_t i = calculate_index(pos);
		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpIndex, cmpLength);
		if (!cmpLengthAdjusted)
			return get_end_position(); // validate cmpLength and cmdLengthAdjusted

		size_t lengthAdjusted = m_length - i;
		if (lengthAdjusted > n)
			lengthAdjusted = n;
		if (cmpLengthAdjusted > lengthAdjusted) // Ensure m_length is larger enough for cmp, and >0
			return get_end_position();

		position_t newPos = pos;
		position_t cmpPos = cmp.calculate_position(cmpIndex);
		find_segment_inner(i, newPos, lengthAdjusted, cmp, cmpPos, cmpLengthAdjusted);
		return newPos;
	}

	size_t index_of_segment(size_t i, size_t n, const composite_buffer_content& cmp, const position_t& cmpPos, size_t cmpLength) const
	{
		if (i >= m_length)
			return const_max_int_v<size_t>;

		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
		if (!cmpLengthAdjusted)
			return const_max_int_v<size_t>; // validate cmpLength and cmdLengthAdjusted

		size_t lengthAdjusted = m_length - i;
		if (lengthAdjusted > n)
			lengthAdjusted = n;
		if (cmpLengthAdjusted > lengthAdjusted) // Ensure m_length is larger enough for cmp, and >0
			return const_max_int_v<size_t>;

		position_t pos = calculate_position(i);
		return find_segment_inner(i, pos, lengthAdjusted, cmp, cmpPos, cmpLengthAdjusted);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const composite_buffer_content& cmp, const position_t& cmpPos, size_t cmpLength) const
	{
		size_t i = calculate_index(pos);
		size_t cmpLengthAdjusted = cmp.validate_length_from(cmpPos, cmpLength);
		if (!cmpLengthAdjusted)
			return get_end_position(); // validate cmpLength and cmdLengthAdjusted

		size_t lengthAdjusted = m_length - i;
		if (lengthAdjusted > n)
			lengthAdjusted = n;
		if (cmpLengthAdjusted > lengthAdjusted) // Ensure m_length is larger enough for cmp, and >0
			return get_end_position();

		position_t newPos = pos;
		find_segment_inner(i, newPos, lengthAdjusted, cmp, cmpPos, cmpLengthAdjusted);
		return newPos;
	}


	size_t find_segment_inner(size_t i, position_t& pos, size_t lengthAdjusted, const buffer& cmp, position_t& endPos) const
	{
		size_t cmpLength = cmp.get_length();
		size_t endIndex = lengthAdjusted - cmpLength;
		for (;;)
		{
			const vector<char>* subArray = &m_vectorVector.get_const_ptr()[pos.m_outerIndex];
			size_t subVectorLength = subArray->get_length() - pos.m_innerIndex;
			while (cmpLength <= subVectorLength)
			{
				// Small enough to fit in this sub vector
				if (subArray->subrange(pos.m_innerIndex).equals(cmp.get_vector()))
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
				pos = get_end_position();
				return const_max_int_v<size_t>;
			}

			while (subVectorLength > 0)
			{
				if (subArray->subrange(pos.m_innerIndex).equals(cmp.get_vector().subrange(0, subVectorLength)))
				{
					size_t curCmpIndex = subVectorLength;
					size_t subVectorIndex2 = pos.m_outerIndex;
					size_t remaining = cmpLength - subVectorLength;
					for (;;)
					{
						++subVectorIndex2;
						const vector<char>* subVector2 = &m_vectorVector.get_const_ptr()[subVectorIndex2];
						size_t subVectorLength2 = subVector2->get_length();
						size_t compareLengthThisSegment = subVectorLength2;
						if (compareLengthThisSegment > remaining)
							compareLengthThisSegment = remaining;
						if (subArray->equals(cmp.get_vector().subrange(curCmpIndex, compareLengthThisSegment)))
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
					pos = get_end_position();
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

	size_t find_segment_inner(size_t i, position_t& pos, size_t lengthAdjusted, const composite_buffer_content& cmp, const position_t& cmpPos, size_t cmpLengthAdjusted) const
	{
		const vector<char>& firstCmpVector = cmp.m_vectorVector.get_const_ptr()[cmpPos.m_outerIndex];
		size_t remainingCmpVectorLength = firstCmpVector.get_length() - cmpPos.m_innerIndex;
		size_t remainingCmpLength = cmpLengthAdjusted - remainingCmpVectorLength;
		size_t remainingLength = lengthAdjusted - remainingCmpLength;
		position_t newCmpPos;
		newCmpPos.m_outerIndex = cmpPos.m_outerIndex + 1;
		newCmpPos.m_innerIndex = 0;
		bool isLastVector = (newCmpPos.m_outerIndex == cmp.m_vectorVector.get_length());
		position_t endPos;
		for (;;)
		{
			i = find_segment_inner(i, pos, remainingLength, buffer::from_vector(firstCmpVector.subrange(cmpPos.m_innerIndex, remainingCmpVectorLength)), endPos);
			if ((i == const_max_int_v<size_t>) || isLastVector)
				return i;

			const vector<char>& endVector = m_vectorVector.get_const_ptr()[endPos.m_outerIndex];
			if (endVector.get_length() == endPos.m_innerIndex)
			{
				endPos.m_innerIndex = 0;
				endPos.m_outerIndex++;
			}

			if (equals_inner(endPos, cmp, newCmpPos, remainingCmpLength, endPos))
				return i;
			if (++i == remainingLength)
			{
				pos = get_end_position();
				return const_max_int_v<size_t>;
			}

			if (++pos.m_innerIndex == m_vectorVector.get_const_ptr()[pos.m_outerIndex].get_length())
			{
				pos.m_innerIndex = 0;
				pos.m_outerIndex++;
			}
		}
	}

};


/// @ingroup IO
/// @brief A buffer compromised of one or more sub-buffers
class composite_buffer
{
public:
	typedef composite_buffer this_t;

	typedef buffer inner_t;

	/// @brief An index position within a composite_buffer
	class position_t
	{
	private:
		friend class composite_buffer;

		composite_buffer_content::position_t m_pos;

		position_t(const composite_buffer_content::position_t& pos) : m_pos(pos) { }

		size_t& get_outer_index() { return m_pos.get_outer_index(); }
		size_t& get_inner_index() { return m_pos.get_inner_index(); }

		bool operator==(const composite_buffer_content::position_t& cmp) const { return (m_pos == cmp); }
		bool operator!=(const composite_buffer_content::position_t& cmp) const { return !operator==(cmp); }
		bool operator<(const composite_buffer_content::position_t& cmp) const { return (m_pos < cmp); }
		bool operator>(const composite_buffer_content::position_t& cmp) const { return (m_pos > cmp); }
		bool operator<=(const composite_buffer_content::position_t& cmp) const { return !operator>(cmp); }
		bool operator>=(const composite_buffer_content::position_t& cmp) const { return !operator<(cmp); }

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
	typedef transactable<composite_buffer_content> transactable_t;
	transactable_t m_contents;

	typedef typename transactable_t::read_token read_token;
	typedef typename transactable_t::write_token write_token;

	composite_buffer(const vector<vector<char> >& src, size_t n)
	{
		m_contents->m_vectorVector = src;
		m_contents->m_length = n;
	}

public:
	composite_vector<char> get_composite_vector() const
	{
		composite_vector<char> result(m_contents->m_vectorVector, m_contents->m_length);
		return result;
	}

	static composite_buffer from_composite_vector(const composite_vector<char>& v)
	{
		composite_buffer result(v.m_contents->m_vectorVector, v.m_contents->m_length);
		return result;
	}

	const composite_cstring get_composite_cstring() const
	{
		composite_cstring result(m_contents->m_vectorVector, m_contents->m_length);
		return result;
	}

	static composite_buffer from_composite_cstring(const composite_cstring& c)
	{
		composite_buffer result(c.m_contents.m_contents->m_vectorVector, c.m_contents.m_contents->m_length);
		return result;
	}

	/// @brief A composite_buffer const data iterator
	class const_iterator
	{
	protected:
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

		friend class composite_buffer;

	public:
		const_iterator()
		{ }

		const_iterator(const const_iterator& i)
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
					const vector<char>& buf = m_array->get_inner_array(m_position.get_outer_index());
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
					m_array = 0;
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
							m_position.get_inner_index() = m_array->get_inner_array(m_position.get_outer_index()).get_length() - 1;
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
		bool operator!=(const const_iterator& i) const { return !operator==(i); }
		const_iterator& operator=(const const_iterator& i) { m_array = i.m_array; m_position = i.m_position; return *this; }
		
		const char* get() const { return m_array->get_inner_array(m_position.get_outer_index()).get_const_ptr() + m_position.get_inner_index(); }
		const char& operator*() const { return *get(); }
		const char* operator->() const { return get(); }

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
		return const_iterator(this, lastSubVectorIndex, get_inner_array(lastSubVectorIndex).get_length() - 1);
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

	composite_buffer()
	{ }

	composite_buffer(const composite_buffer& src)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents))
	{ }

	composite_buffer(const volatile composite_buffer& src)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents.begin_read()))
	{ }

	composite_buffer(const composite_buffer& src, size_t srcIndex, size_t srcLength = const_max_int_v<size_t>)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents))
	{
		set_to_subrange(srcIndex, srcLength);
	}

	composite_buffer(const volatile composite_buffer& src, size_t srcIndex, size_t srcLength = const_max_int_v<size_t>)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents.begin_read()))
	{
		set_to_subrange(srcIndex, srcLength);
	}

	composite_buffer(const composite_buffer& src, const position_t& pos, size_t srcLength = const_max_int_v<size_t>)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents))
	{
		set_to_subrange(pos, srcLength);
	}

	composite_buffer(const volatile composite_buffer& src, const position_t& pos, size_t srcLength = const_max_int_v<size_t>)
		: m_contents(typename transactable_t::construct_embedded_t(), *(src.m_contents.begin_read()))
	{
		set_to_subrange(pos, srcLength);
	}

	composite_buffer(const buffer& src)
		: m_contents(typename transactable_t::construct_embedded_t(), src)
	{ }

	composite_buffer(const volatile buffer& src)
		: m_contents(typename transactable_t::construct_embedded_t(), src)
	{ }


	size_t get_length() const { return m_contents->m_length; }
	size_t get_length() const volatile { return m_contents.begin_read()->m_length; }

	bool is_empty() const { return m_contents->m_length == 0; }
	bool is_empty() const volatile { return m_contents.begin_read()->m_length == 0; }

	bool operator!() const { return m_contents->m_length == 0; }
	bool operator!() const volatile { return m_contents.begin_read()->m_length == 0; }


	bool equals(const char* cmp, size_t cmpLength) const { return m_contents->equals(0, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool equals(size_t i, const char* cmp, size_t cmpLength) const { return m_contents->equals(i, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool equals(size_t i, size_t n, const char* cmp, size_t cmpLength) const { return m_contents->equals(i, n, buffer::contain(cmp, cmpLength)); }

	bool equals(const position_t& pos, const char* cmp, size_t cmpLength) const { return m_contents->equals(pos.m_pos, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool equals(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const { return m_contents->equals(pos.m_pos, n, buffer::contain(cmp, cmpLength)); }

	bool equals(const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->equals(0, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool equals(size_t i, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->equals(i, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool equals(size_t i, size_t n, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->equals(i, n, buffer::contain(cmp, cmpLength)); }

	bool equals(const position_t& pos, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->equals(pos.m_pos, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool equals(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->equals(pos.m_pos, n, buffer::contain(cmp, cmpLength)); }

	bool equals(const buffer& cmp) const
	{
		return m_contents->equals(0, const_max_int_v<size_t>, cmp);
	}

	bool equals(const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->equals(0, const_max_int_v<size_t>, cmp);
	}

	bool equals(const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return equals(tmp);
	}

	bool equals(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool equals(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->equals(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool equals(const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(0, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool equals(size_t i, const buffer& cmp) const
	{
		return m_contents->equals(i, const_max_int_v<size_t>, cmp);
	}

	bool equals(size_t i, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->equals(i, const_max_int_v<size_t>, cmp);
	}

	bool equals(size_t i, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return equals(i, tmp);
	}


	bool equals(size_t i, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool equals(size_t i, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->equals(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool equals(size_t i, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(i, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool equals(size_t i, size_t n, const buffer& cmp) const
	{
		return m_contents->equals(i, n, cmp);
	}

	bool equals(size_t i, size_t n, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->equals(i, n, cmp);
	}

	bool equals(size_t i, size_t n, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return equals(i, n, tmp);
	}


	bool equals(size_t i, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool equals(size_t i, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->equals(i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool equals(size_t i, size_t n, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(i, n, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool equals(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool equals(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->equals(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool equals(const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(0, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool equals(size_t i, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool equals(size_t i, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->equals(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool equals(size_t i, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(i, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool equals(size_t i, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool equals(size_t i, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->equals(i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool equals(size_t i, size_t n, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(i, n, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool equals(const position_t& pos, const buffer& cmp) const
	{
		return m_contents->equals(pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	bool equals(const position_t& pos, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->equals(pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	bool equals(const position_t& pos, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return equals(pos, tmp);
	}


	bool equals(const position_t& pos, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool equals(const position_t& pos, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->equals(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool equals(const position_t& pos, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}

	bool equals(const position_t& pos, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool equals(const position_t& pos, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->equals(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool equals(const position_t& pos, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}

	bool equals(const position_t& pos, size_t n, const buffer& cmp) const
	{
		return m_contents->equals(pos.m_pos, n, cmp);
	}

	bool equals(const position_t& pos, size_t n, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->equals(pos.m_pos, n, cmp);
	}

	bool equals(const position_t& pos, size_t n, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return equals(pos, n, tmp);
	}


	bool equals(const position_t& pos, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool equals(const position_t& pos, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->equals(pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool equals(const position_t& pos, size_t n, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(pos.m_pos, n, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool equals(const position_t& pos, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool equals(const position_t& pos, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->equals(pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool equals(const position_t& pos, size_t n, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->equals(pos.m_pos, n, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool operator==(const buffer& cmp) const { return equals(cmp); }

	bool operator==(const composite_buffer& cmp) const { return equals(cmp); }

	bool operator==(const buffer& cmp) const volatile { return equals(cmp); }

	bool operator==(const composite_buffer& cmp) const volatile { return equals(cmp); }

	bool operator==(const volatile buffer& cmp) const { return equals(cmp); }

	bool operator==(const volatile composite_buffer& cmp) const { return equals(cmp); }


	bool operator!=(const buffer& cmp) const { return !operator==(cmp); }

	bool operator!=(const composite_buffer& cmp) const { return !operator==(cmp); }

	bool operator!=(const buffer& cmp) const volatile { return !operator==(cmp); }

	bool operator!=(const composite_buffer& cmp) const volatile { return !operator==(cmp); }

	bool operator!=(const volatile buffer& cmp) const { return !operator==(cmp); }

	bool operator!=(const volatile composite_buffer& cmp) const { return !operator==(cmp); }


	int compare(const char* cmp, size_t cmpLength) const { return m_contents->compare(0, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	int compare(size_t i, const char* cmp, size_t cmpLength) const { return m_contents->compare(i, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	int compare(size_t i, size_t n, const char* cmp, size_t cmpLength) const { return m_contents->compare(i, n, buffer::contain(cmp, cmpLength)); }

	int compare(const position_t& pos, const char* cmp, size_t cmpLength) const { return m_contents->compare(pos.m_pos, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	int compare(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const { return m_contents->compare(pos.m_pos, n, buffer::contain(cmp, cmpLength)); }

	int compare(const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->compare(0, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	int compare(size_t i, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->compare(i, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	int compare(size_t i, size_t n, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->compare(i, n, buffer::contain(cmp, cmpLength)); }

	int compare(const position_t& pos, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->compare(pos.m_pos, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	int compare(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->compare(pos.m_pos, n, buffer::contain(cmp, cmpLength)); }

	int compare(const buffer& cmp) const
	{
		return m_contents->compare(0, const_max_int_v<size_t>, cmp);
	}

	int compare(const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->compare(0, const_max_int_v<size_t>, cmp);
	}

	int compare(const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return compare(tmp);
	}

	int compare(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	int compare(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->compare(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	int compare(const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(0, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	int compare(size_t i, const buffer& cmp) const
	{
		return m_contents->compare(i, const_max_int_v<size_t>, cmp);
	}

	int compare(size_t i, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->compare(i, const_max_int_v<size_t>, cmp);
	}

	int compare(size_t i, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return compare(i, tmp);
	}


	int compare(size_t i, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	int compare(size_t i, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->compare(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	int compare(size_t i, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(i, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	int compare(size_t i, size_t n, const buffer& cmp) const
	{
		return m_contents->compare(i, n, cmp);
	}

	int compare(size_t i, size_t n, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->compare(i, n, cmp);
	}

	int compare(size_t i, size_t n, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return compare(i, n, tmp);
	}


	int compare(size_t i, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	int compare(size_t i, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->compare(i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	int compare(size_t i, size_t n, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(i, n, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	int compare(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	int compare(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->compare(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	int compare(const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(0, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	int compare(size_t i, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	int compare(size_t i, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->compare(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	int compare(size_t i, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(i, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	int compare(size_t i, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	int compare(size_t i, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->compare(i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	int compare(size_t i, size_t n, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(i, n, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	int compare(const position_t& pos, const buffer& cmp) const
	{
		return m_contents->compare(pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	int compare(const position_t& pos, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->compare(pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	int compare(const position_t& pos, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return compare(pos, tmp);
	}


	int compare(const position_t& pos, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	int compare(const position_t& pos, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	int compare(const position_t& pos, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	int compare(const position_t& pos, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	int compare(const position_t& pos, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	int compare(const position_t& pos, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	int compare(const position_t& pos, size_t n, const buffer& cmp) const
	{
		return m_contents->compare(pos.m_pos, n, cmp);
	}

	int compare(const position_t& pos, size_t n, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->compare(pos.m_pos, n, cmp);
	}

	int compare(const position_t& pos, size_t n, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return compare(pos, n, tmp);
	}


	int compare(const position_t& pos, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	int compare(const position_t& pos, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->compare(pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	int compare(const position_t& pos, size_t n, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(pos.m_pos, n, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	int compare(const position_t& pos, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	int compare(const position_t& pos, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->compare(pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	int compare(const position_t& pos, size_t n, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->compare(pos.m_pos, n, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool is_less_than(const char* cmp, size_t cmpLength) const { return 0 > m_contents->compare(0, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool is_less_than(size_t i, const char* cmp, size_t cmpLength) const { return 0 > m_contents->compare(i, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool is_less_than(size_t i, size_t n, const char* cmp, size_t cmpLength) const { return 0 > m_contents->compare(i, n, buffer::contain(cmp, cmpLength)); }

	bool is_less_than(const position_t& pos, const char* cmp, size_t cmpLength) const { return 0 > m_contents->compare(pos.m_pos, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool is_less_than(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const { return 0 > m_contents->compare(pos.m_pos, n, buffer::contain(cmp, cmpLength)); }

	bool is_less_than(const char* cmp, size_t cmpLength) const volatile { return 0 > m_contents.begin_read()->compare(0, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool is_less_than(size_t i, const char* cmp, size_t cmpLength) const volatile { return 0 > m_contents.begin_read()->compare(i, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool is_less_than(size_t i, size_t n, const char* cmp, size_t cmpLength) const volatile { return 0 > m_contents.begin_read()->compare(i, n, buffer::contain(cmp, cmpLength)); }

	bool is_less_than(const position_t& pos, const char* cmp, size_t cmpLength) const volatile { return 0 > m_contents.begin_read()->compare(pos.m_pos, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool is_less_than(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const volatile { return 0 > m_contents.begin_read()->compare(pos.m_pos, n, buffer::contain(cmp, cmpLength)); }

	bool is_less_than(const buffer& cmp) const
	{
		return 0 > m_contents->compare(0, const_max_int_v<size_t>, cmp);
	}

	bool is_less_than(const buffer& cmp) const volatile
	{
		return 0 > m_contents.begin_read()->compare(0, const_max_int_v<size_t>, cmp);
	}

	bool is_less_than(const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return 0 > compare(tmp);
	}

	bool is_less_than(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_less_than(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > m_contents.begin_read()->compare(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_less_than(const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(0, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool is_less_than(size_t i, const buffer& cmp) const
	{
		return 0 > m_contents->compare(i, const_max_int_v<size_t>, cmp);
	}

	bool is_less_than(size_t i, const buffer& cmp) const volatile
	{
		return 0 > m_contents.begin_read()->compare(i, const_max_int_v<size_t>, cmp);
	}

	bool is_less_than(size_t i, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return 0 > compare(i, tmp);
	}


	bool is_less_than(size_t i, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_less_than(size_t i, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > m_contents.begin_read()->compare(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_less_than(size_t i, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(i, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool is_less_than(size_t i, size_t n, const buffer& cmp) const
	{
		return 0 > m_contents->compare(i, n, cmp);
	}

	bool is_less_than(size_t i, size_t n, const buffer& cmp) const volatile
	{
		return 0 > m_contents.begin_read()->compare(i, n, cmp);
	}

	bool is_less_than(size_t i, size_t n, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return 0 > compare(i, n, tmp);
	}


	bool is_less_than(size_t i, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_less_than(size_t i, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > m_contents.begin_read()->compare(i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_less_than(size_t i, size_t n, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(i, n, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool is_less_than(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_less_than(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > m_contents.begin_read()->compare(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_less_than(const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(0, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool is_less_than(size_t i, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_less_than(size_t i, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > m_contents.begin_read()->compare(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_less_than(size_t i, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(i, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool is_less_than(size_t i, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_less_than(size_t i, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > m_contents.begin_read()->compare(i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_less_than(size_t i, size_t n, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(i, n, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool is_less_than(const position_t& pos, const buffer& cmp) const
	{
		return 0 > m_contents->compare(pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	bool is_less_than(const position_t& pos, const buffer& cmp) const volatile
	{
		return 0 > m_contents.begin_read()->compare(pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	bool is_less_than(const position_t& pos, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return 0 > compare(pos, tmp);
	}


	bool is_less_than(const position_t& pos, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_less_than(const position_t& pos, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > m_contents.begin_read()->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_less_than(const position_t& pos, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool is_less_than(const position_t& pos, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_less_than(const position_t& pos, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > m_contents.begin_read()->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_less_than(const position_t& pos, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool is_less_than(const position_t& pos, size_t n, const buffer& cmp) const
	{
		return 0 > m_contents->compare(pos.m_pos, n, cmp);
	}

	bool is_less_than(const position_t& pos, size_t n, const buffer& cmp) const volatile
	{
		return 0 > m_contents.begin_read()->compare(pos.m_pos, n, cmp);
	}

	bool is_less_than(const position_t& pos, size_t n, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return 0 > compare(pos, n, tmp);
	}


	bool is_less_than(const position_t& pos, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_less_than(const position_t& pos, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > m_contents.begin_read()->compare(pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_less_than(const position_t& pos, size_t n, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(pos.m_pos, n, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool is_less_than(const position_t& pos, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_less_than(const position_t& pos, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 > m_contents.begin_read()->compare(pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_less_than(const position_t& pos, size_t n, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 > m_contents->compare(pos.m_pos, n, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool is_greater_than(const char* cmp, size_t cmpLength) const { return 0 < m_contents->compare(0, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool is_greater_than(size_t i, const char* cmp, size_t cmpLength) const { return 0 < m_contents->compare(i, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool is_greater_than(size_t i, size_t n, const char* cmp, size_t cmpLength) const { return 0 < m_contents->compare(i, n, buffer::contain(cmp, cmpLength)); }

	bool is_greater_than(const position_t& pos, const char* cmp, size_t cmpLength) const { return 0 < m_contents->compare(pos.m_pos, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool is_greater_than(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const { return 0 < m_contents->compare(pos.m_pos, n, buffer::contain(cmp, cmpLength)); }

	bool is_greater_than(const char* cmp, size_t cmpLength) const volatile { return 0 < m_contents.begin_read()->compare(0, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool is_greater_than(size_t i, const char* cmp, size_t cmpLength) const volatile { return 0 < m_contents.begin_read()->compare(i, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool is_greater_than(size_t i, size_t n, const char* cmp, size_t cmpLength) const volatile { return 0 < m_contents.begin_read()->compare(i, n, buffer::contain(cmp, cmpLength)); }

	bool is_greater_than(const position_t& pos, const char* cmp, size_t cmpLength) const volatile { return 0 < m_contents.begin_read()->compare(pos.m_pos, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool is_greater_than(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const volatile { return 0 < m_contents.begin_read()->compare(pos.m_pos, n, buffer::contain(cmp, cmpLength)); }

	bool is_greater_than(const buffer& cmp) const
	{
		return 0 < m_contents->compare(0, const_max_int_v<size_t>, cmp);
	}

	bool is_greater_than(const buffer& cmp) const volatile
	{
		return 0 < m_contents.begin_read()->compare(0, const_max_int_v<size_t>, cmp);
	}

	bool is_greater_than(const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return 0 < compare(tmp);
	}

	bool is_greater_than(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_greater_than(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < m_contents.begin_read()->compare(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_greater_than(const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(0, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool is_greater_than(size_t i, const buffer& cmp) const
	{
		return 0 < m_contents->compare(i, const_max_int_v<size_t>, cmp);
	}

	bool is_greater_than(size_t i, const buffer& cmp) const volatile
	{
		return 0 < m_contents.begin_read()->compare(i, const_max_int_v<size_t>, cmp);
	}

	bool is_greater_than(size_t i, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return 0 < compare(i, tmp);
	}


	bool is_greater_than(size_t i, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_greater_than(size_t i, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < m_contents.begin_read()->compare(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_greater_than(size_t i, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(i, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool is_greater_than(size_t i, size_t n, const buffer& cmp) const
	{
		return 0 < m_contents->compare(i, n, cmp);
	}

	bool is_greater_than(size_t i, size_t n, const buffer& cmp) const volatile
	{
		return 0 < m_contents.begin_read()->compare(i, n, cmp);
	}

	bool is_greater_than(size_t i, size_t n, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return 0 < compare(i, n, tmp);
	}


	bool is_greater_than(size_t i, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_greater_than(size_t i, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < m_contents.begin_read()->compare(i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_greater_than(size_t i, size_t n, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(i, n, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool is_greater_than(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_greater_than(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < m_contents.begin_read()->compare(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_greater_than(const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(0, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool is_greater_than(size_t i, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_greater_than(size_t i, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < m_contents.begin_read()->compare(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_greater_than(size_t i, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(i, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool is_greater_than(size_t i, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_greater_than(size_t i, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < m_contents.begin_read()->compare(i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_greater_than(size_t i, size_t n, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(i, n, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool is_greater_than(const position_t& pos, const buffer& cmp) const
	{
		return 0 < m_contents->compare(pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	bool is_greater_than(const position_t& pos, const buffer& cmp) const volatile
	{
		return 0 < m_contents.begin_read()->compare(pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	bool is_greater_than(const position_t& pos, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return 0 < compare(pos, tmp);
	}


	bool is_greater_than(const position_t& pos, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_greater_than(const position_t& pos, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < m_contents.begin_read()->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_greater_than(const position_t& pos, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool is_greater_than(const position_t& pos, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_greater_than(const position_t& pos, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < m_contents.begin_read()->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_greater_than(const position_t& pos, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool is_greater_than(const position_t& pos, size_t n, const buffer& cmp) const
	{
		return 0 < m_contents->compare(pos.m_pos, n, cmp);
	}

	bool is_greater_than(const position_t& pos, size_t n, const buffer& cmp) const volatile
	{
		return 0 < m_contents.begin_read()->compare(pos.m_pos, n, cmp);
	}

	bool is_greater_than(const position_t& pos, size_t n, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return 0 < compare(pos, n, tmp);
	}


	bool is_greater_than(const position_t& pos, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_greater_than(const position_t& pos, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < m_contents.begin_read()->compare(pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool is_greater_than(const position_t& pos, size_t n, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(pos.m_pos, n, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool is_greater_than(const position_t& pos, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_greater_than(const position_t& pos, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return 0 < m_contents.begin_read()->compare(pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool is_greater_than(const position_t& pos, size_t n, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return 0 < m_contents->compare(pos.m_pos, n, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool operator<(const buffer& cmp) const { return is_less_than(cmp); }

	bool operator<(const buffer& cmp) const volatile { return is_less_than(cmp); }

	bool operator<(const volatile buffer& cmp) const { return is_less_than(cmp); }

	bool operator<(const composite_buffer& cmp) const { return is_less_than(cmp); }

	bool operator<(const composite_buffer& cmp) const volatile { return is_less_than(cmp); }

	bool operator<(const volatile composite_buffer& cmp) const { return is_less_than(cmp); }


	bool operator>(const buffer& cmp) const { return is_greater_than(cmp); }

	bool operator>(const buffer& cmp) const volatile { return is_greater_than(cmp); }

	bool operator>(const volatile buffer& cmp) const { return is_greater_than(cmp); }

	bool operator>(const composite_buffer& cmp) const { return is_greater_than(cmp); }

	bool operator>(const composite_buffer& cmp) const volatile { return is_greater_than(cmp); }

	bool operator>(const volatile composite_buffer& cmp) const { return is_greater_than(cmp); }


	bool operator<=(const buffer& cmp) const { return !operator>(cmp); }

	bool operator<=(const buffer& cmp) const volatile { return !operator>(cmp); }

	bool operator<=(const volatile buffer& cmp) const { return !operator>(cmp); }

	bool operator<=(const composite_buffer& cmp) const { return !operator>(cmp); }

	bool operator<=(const composite_buffer& cmp) const volatile { return !operator>(cmp); }

	bool operator<=(const volatile composite_buffer& cmp) const { return !operator>(cmp); }


	bool operator>=(const buffer& cmp) const { return !operator<(cmp); }

	bool operator>=(const buffer& cmp) const volatile { return !operator<(cmp); }

	bool operator>=(const volatile buffer& cmp) const { return !operator<(cmp); }

	bool operator>=(const composite_buffer& cmp) const { return !operator<(cmp); }

	bool operator>=(const composite_buffer& cmp) const volatile { return !operator<(cmp); }

	bool operator>=(const volatile composite_buffer& cmp) const { return !operator<(cmp); }


	bool starts_with(const char* cmp, size_t cmpLength) const { return equals(0, cmpLength, cmp, cmpLength); }

	bool starts_with(const char* cmp, size_t cmpLength) const volatile { return equals(0, cmpLength, cmp, cmpLength); }

	bool starts_with(const buffer& cmp) const { return equals(0, cmp.get_length(), cmp); }
	bool starts_with(const buffer& cmp) const volatile { return equals(0, cmp.get_length(), cmp); }
	bool starts_with(const volatile buffer& cmp) const { return equals(0, cmp.get_length(), cmp); }


	bool starts_with(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return equals(0, cmpLength, cmp, cmpIndex, cmpLength);
	}

	bool starts_with(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return equals(0, cmpLength, cmp, cmpIndex, cmpLength);
	}

	bool starts_with(const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return equals(0, cmpLength, cmp, cmpIndex, cmpLength);
	}


	bool starts_with(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return equals(0, cmpLength, cmp, cmpPos, cmpLength);
	}

	bool starts_with(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return equals(0, cmpLength, cmp, cmpPos, cmpLength);
	}

	bool starts_with(const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return equals(0, cmpLength, cmp, cmpPos, cmpLength);
	}


	bool ends_with(const char* cmp, size_t cmpLength) const { return m_contents->ends_with(buffer::contain(cmp, cmpLength)); }
	bool ends_with(const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->ends_with(buffer::contain(cmp, cmpLength)); }

	bool ends_with(const buffer& cmp) const { return m_contents->ends_with(cmp); }
	bool ends_with(const buffer& cmp) const volatile { return m_contents.begin_read()->ends_with(cmp); }
	bool ends_with(const volatile buffer& cmp) const { buffer tmp = cmp; return ends_with(tmp); }


	bool ends_with(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->ends_with(*(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool ends_with(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->ends_with(*(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool ends_with(const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->ends_with(*(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool ends_with(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->ends_with(*(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool ends_with(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->ends_with(*(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool ends_with(const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->ends_with(*(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	size_t index_of(const char& cmp) const { return m_contents->index_of(0, const_max_int_v<size_t>, cmp); }

	size_t index_of(const char& cmp) const volatile { return m_contents.begin_read()->index_of(0, const_max_int_v<size_t>, cmp); }

	size_t index_of(size_t i, const char& cmp) const { return m_contents->index_of(i, const_max_int_v<size_t>, cmp); }

	size_t index_of(size_t i, const char& cmp) const volatile { return m_contents.begin_read()->index_of(i, const_max_int_v<size_t>, cmp); }

	size_t index_of(size_t i, size_t n, const char& cmp) const { return m_contents->index_of(i, n, cmp); }

	size_t index_of(size_t i, size_t n, const char& cmp) const volatile { return m_contents.begin_read()->index_of(i, n, cmp); }


	position_t position_of(const position_t& pos, const char& cmp) const { return m_contents->position_of(pos.m_pos, const_max_int_v<size_t>, cmp); }

	position_t position_of(const position_t& pos, const char& cmp) const volatile { return m_contents.begin_read()->position_of(pos.m_pos, const_max_int_v<size_t>, cmp); }

	position_t position_of(const position_t& pos, size_t n, const char& cmp) const { return m_contents->position_of(pos.m_pos, n, cmp); }

	position_t position_of(const position_t& pos, size_t n, const char& cmp) const volatile { return m_contents.begin_read()->position_of(pos.m_pos, n, cmp); }


	size_t index_of_any(const char* cmp, size_t cmpLength) const { return m_contents->index_of_any(0, const_max_int_v<size_t>, cmp, cmpLength); }

	size_t index_of_any(const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->index_of_any(0, const_max_int_v<size_t>, cmp, cmpLength); }

	size_t index_of_any(size_t i, const char* cmp, size_t cmpLength) const { return m_contents->index_of_any(i, const_max_int_v<size_t>, cmp, cmpLength); }

	size_t index_of_any(size_t i, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->index_of_any(i, const_max_int_v<size_t>, cmp, cmpLength); }


	size_t index_of_any(size_t i, size_t n, const char* cmp, size_t cmpLength) const { return m_contents->index_of_any(i, n, cmp, cmpLength); }

	size_t index_of_any(size_t i, size_t n, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->index_of_any(i, n, cmp, cmpLength); }


	position_t position_of_any(const position_t& pos, const char* cmp, size_t cmpLength) const { return m_contents->position_of_any(pos.m_pos, const_max_int_v<size_t>, cmp, cmpLength); }

	position_t position_of_any(const position_t& pos, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->position_of_any(pos.m_pos, const_max_int_v<size_t>, cmp, cmpLength); }


	position_t position_of_any(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const { return m_contents->position_of_any(pos.m_pos, n, cmp, cmpLength); }

	position_t position_of_any(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->position_of_any(pos.m_pos, n, cmp, cmpLength); }


	size_t index_of_segment(const char* cmp, size_t cmpLength) const { return m_contents->index_of_segment(0, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	size_t index_of_segment(size_t i, const char* cmp, size_t cmpLength) const { return m_contents->index_of_segment(i, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	size_t index_of_segment(size_t i, size_t n, const char* cmp, size_t cmpLength) const { return m_contents->index_of_segment(i, n, buffer::contain(cmp, cmpLength)); }

	size_t index_of_segment(const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->index_of_segment(0, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	size_t index_of_segment(size_t i, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->index_of_segment(i, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	size_t index_of_segment(size_t i, size_t n, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->index_of_segment(i, n, buffer::contain(cmp, cmpLength)); }

	size_t index_of_segment(const buffer& cmp) const
	{
		return m_contents->index_of_segment(0, const_max_int_v<size_t>, cmp);
	}

	size_t index_of_segment(const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->index_of_segment(0, const_max_int_v<size_t>, cmp);
	}

	size_t index_of_segment(const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return index_of_segment(tmp);
	}

	size_t index_of_segment(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	size_t index_of_segment(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->index_of_segment(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	size_t index_of_segment(const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(0, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	size_t index_of_segment(size_t i, const buffer& cmp) const
	{
		return m_contents->index_of_segment(i, const_max_int_v<size_t>, cmp);
	}

	size_t index_of_segment(size_t i, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->index_of_segment(i, const_max_int_v<size_t>, cmp);
	}

	size_t index_of_segment(size_t i, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return index_of_segment(i, tmp);
	}

	size_t index_of_segment(size_t i, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	size_t index_of_segment(size_t i, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->index_of_segment(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	size_t index_of_segment(size_t i, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}

	size_t index_of_segment(size_t i, size_t n, const buffer& cmp) const
	{
		return m_contents->index_of_segment(i, n, cmp);
	}

	size_t index_of_segment(size_t i, size_t n, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->index_of_segment(i, n, cmp);
	}

	size_t index_of_segment(size_t i, size_t n, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return index_of_segment(i, n, tmp);
	}

	size_t index_of_segment(size_t i, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	size_t index_of_segment(size_t i, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->index_of_segment(i, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	size_t index_of_segment(size_t i, size_t n, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, n, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	size_t index_of_segment(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	size_t index_of_segment(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->index_of_segment(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	size_t index_of_segment(const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(0, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	size_t index_of_segment(size_t i, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	size_t index_of_segment(size_t i, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->index_of_segment(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	size_t index_of_segment(size_t i, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	size_t index_of_segment(size_t i, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	size_t index_of_segment(size_t i, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->index_of_segment(i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	size_t index_of_segment(size_t i, size_t n, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, n, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	position_t position_of_segment(const position_t& pos, const char* cmp, size_t cmpLength) const { return m_contents->position_of_segment(pos.m_pos, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	position_t position_of_segment(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const { return m_contents->position_of_segment(pos.m_pos, n, buffer::contain(cmp, cmpLength)); }

	position_t position_of_segment(const position_t& pos, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->position_of_segment(pos.m_pos, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	position_t position_of_segment(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->position_of_segment(pos.m_pos, n, buffer::contain(cmp, cmpLength)); }


	position_t position_of_segment(const position_t& pos, const buffer& cmp) const
	{
		return m_contents->position_of_segment(pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	position_t position_of_segment(const position_t& pos, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->position_of_segment(pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	position_t position_of_segment(const position_t& pos, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return position_of_segment(pos, tmp);
	}


	position_t position_of_segment(const position_t& pos, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->position_of_segment(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->position_of_segment(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->position_of_segment(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	position_t position_of_segment(const position_t& pos, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->position_of_segment(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->position_of_segment(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->position_of_segment(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	position_t position_of_segment(const position_t& pos, size_t n, const buffer& cmp) const
	{
		return m_contents->position_of_segment(pos.m_pos, n, cmp);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->position_of_segment(pos.m_pos, n, cmp);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return position_of_segment(pos, n, tmp);
	}


	position_t position_of_segment(const position_t& pos, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->position_of_segment(pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->position_of_segment(pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->position_of_segment(pos.m_pos, n, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	position_t position_of_segment(const position_t& pos, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->position_of_segment(pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->position_of_segment(pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	position_t position_of_segment(const position_t& pos, size_t n, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->position_of_segment(pos.m_pos, n, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool contains(const char& cmp) const { return m_contents->index_of(0, const_max_int_v<size_t>, cmp) != (size_t)-1; }

	bool contains(const char& cmp) const volatile { return m_contents.begin_read()->index_of(0, const_max_int_v<size_t>, cmp) != (size_t)-1; }

	bool contains(size_t i, const char& cmp) const { return m_contents->index_of(i, const_max_int_v<size_t>, cmp) != (size_t)-1; }

	bool contains(size_t i, const char& cmp) const volatile { return m_contents.begin_read()->index_of(i, const_max_int_v<size_t>, cmp) != (size_t)-1; }

	bool contains(const position_t& pos, const char& cmp) const { return get_end_position() != m_contents->position_of(pos.m_pos, const_max_int_v<size_t>, cmp); }

	bool contains(const position_t& pos, const char& cmp) const volatile { return get_end_position() != m_contents.begin_read()->position_of(pos.m_pos, const_max_int_v<size_t>, cmp); }


	bool contains(size_t i, size_t n, const char& cmp) const { return m_contents->index_of(i, n, cmp) != (size_t)-1; }

	bool contains(size_t i, size_t n, const char& cmp) const volatile { return m_contents.begin_read()->index_of(i, n, cmp) != (size_t)-1; }

	bool contains(const position_t& pos, size_t n, const char& cmp) const { return get_end_position() != m_contents->position_of(pos.m_pos, n, cmp); }

	bool contains(const position_t& pos, size_t n, const char& cmp) const volatile { return get_end_position() != m_contents.begin_read()->position_of(pos.m_pos, n, cmp); }


	bool contains_segment(const char* cmp, size_t cmpLength) const { return m_contents->index_of_segment(0, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)) != (size_t)-1; }

	bool contains_segment(size_t i, const char* cmp, size_t cmpLength) const { return m_contents->index_of_segment(i, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)) != (size_t)-1; }

	bool contains_segment(size_t i, size_t n, const char* cmp, size_t cmpLength) const { return m_contents->index_of_segment(i, n, buffer::contain(cmp, cmpLength)) != (size_t)-1; }

	bool contains_segment(const position_t& pos, const char* cmp, size_t cmpLength) const { return get_end_position() != m_contents->position_of_segment(pos.m_pos, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool contains_segment(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const { return get_end_position() != m_contents->position_of_segment(pos.m_pos, n, buffer::contain(cmp, cmpLength)); }

	bool contains_segment(const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->index_of_segment(0, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)) != (size_t)-1; }

	bool contains_segment(size_t i, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->index_of_segment(i, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)) != (size_t)-1; }

	bool contains_segment(size_t i, size_t n, const char* cmp, size_t cmpLength) const volatile { return m_contents.begin_read()->index_of_segment(i, n, buffer::contain(cmp, cmpLength)) != (size_t)-1; }

	bool contains_segment(const position_t& pos, const char* cmp, size_t cmpLength) const volatile { return get_end_position() != m_contents.begin_read()->position_of_segment(pos.m_pos, const_max_int_v<size_t>, buffer::contain(cmp, cmpLength)); }

	bool contains_segment(const position_t& pos, size_t n, const char* cmp, size_t cmpLength) const volatile { return get_end_position() != m_contents.begin_read()->position_of_segment(pos.m_pos, n, buffer::contain(cmp, cmpLength)); }


	bool contains_segment(const buffer& cmp) const
	{
		return m_contents->index_of_segment(0, const_max_int_v<size_t>, cmp) != (size_t)-1;
	}

	bool contains_segment(const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->index_of_segment(0, const_max_int_v<size_t>, cmp) != (size_t)-1;
	}

	bool contains_segment(const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return index_of_segment(tmp) != (size_t)-1;
	}

	bool contains_segment(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength) != (size_t)-1;
	}

	bool contains_segment(const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->index_of_segment(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength) != (size_t)-1;
	}

	bool contains_segment(const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(0, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength) != (size_t)-1;
	}


	bool contains_segment(size_t i, const buffer& cmp) const
	{
		return m_contents->index_of_segment(i, const_max_int_v<size_t>, cmp) != (size_t)-1;
	}

	bool contains_segment(size_t i, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->index_of_segment(i, const_max_int_v<size_t>, cmp) != (size_t)-1;
	}

	bool contains_segment(size_t i, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return index_of_segment(i, tmp) != (size_t)-1;
	}

	bool contains_segment(size_t i, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength) != (size_t)-1;
	}

	bool contains_segment(size_t i, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->index_of_segment(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength) != (size_t)-1;
	}

	bool contains_segment(size_t i, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength) != (size_t)-1;
	}


	bool contains_segment(size_t i, size_t n, const buffer& cmp) const
	{
		return m_contents->index_of_segment(i, n, cmp) != (size_t)-1;
	}

	bool contains_segment(size_t i, size_t n, const buffer& cmp) const volatile
	{
		return m_contents.begin_read()->index_of_segment(i, n, cmp) != (size_t)-1;
	}

	bool contains_segment(size_t i, size_t n, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return index_of_segment(i, n, tmp) != (size_t)-1;
	}

	bool contains_segment(size_t i, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, n, *(cmp.m_contents), cmpIndex, cmpLength) != (size_t)-1;
	}

	bool contains_segment(size_t i, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->index_of_segment(i, n, *(cmp.m_contents), cmpIndex, cmpLength) != (size_t)-1;
	}

	bool contains_segment(size_t i, size_t n, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, n, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength) != (size_t)-1;
	}


	bool contains_segment(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength) != (size_t)-1;
	}

	bool contains_segment(const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->index_of_segment(0, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength) != (size_t)-1;
	}

	bool contains_segment(const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(0, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength) != (size_t)-1;
	}


	bool contains_segment(size_t i, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength) != (size_t)-1;
	}

	bool contains_segment(size_t i, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->index_of_segment(i, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength) != (size_t)-1;
	}

	bool contains_segment(size_t i, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength) != (size_t)-1;
	}


	bool contains_segment(size_t i, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength) != (size_t)-1;
	}

	bool contains_segment(size_t i, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return m_contents.begin_read()->index_of_segment(i, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength) != (size_t)-1;
	}

	bool contains_segment(size_t i, size_t n, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return m_contents->index_of_segment(i, n, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength) != (size_t)-1;
	}


	bool contains_segment(const position_t& pos, const buffer& cmp) const
	{
		return get_end_position() != m_contents->position_of_segment(pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	bool contains_segment(const position_t& pos, const buffer& cmp) const volatile
	{
		return get_end_position() != m_contents.begin_read()->position_of_segment(pos.m_pos, const_max_int_v<size_t>, cmp);
	}

	bool contains_segment(const position_t& pos, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return get_end_position() != position_of_segment(pos, tmp);
	}


	bool contains_segment(const position_t& pos, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != m_contents->position_of_segment(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool contains_segment(const position_t& pos, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return get_end_position() != m_contents.begin_read()->position_of_segment(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool contains_segment(const position_t& pos, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != m_contents->position_of_segment(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool contains_segment(const position_t& pos, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != m_contents->position_of_segment(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool contains_segment(const position_t& pos, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return get_end_position() != m_contents.begin_read()->position_of_segment(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool contains_segment(const position_t& pos, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != m_contents->position_of_segment(pos.m_pos, const_max_int_v<size_t>, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}


	bool contains_segment(const position_t& pos, size_t n, const buffer& cmp) const
	{
		return get_end_position() != m_contents->position_of_segment(pos.m_pos, n, cmp);
	}

	bool contains_segment(const position_t& pos, size_t n, const buffer& cmp) const volatile
	{
		return get_end_position() != m_contents.begin_read()->position_of_segment(pos.m_pos, n, cmp);
	}

	bool contains_segment(const position_t& pos, size_t n, const volatile buffer& cmp) const
	{
		buffer tmp = cmp;
		return get_end_position() != position_of_segment(pos, n, tmp);
	}


	bool contains_segment(const position_t& pos, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != m_contents->position_of_segment(pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool contains_segment(const position_t& pos, size_t n, const composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return get_end_position() != m_contents.begin_read()->position_of_segment(pos.m_pos, n, *(cmp.m_contents), cmpIndex, cmpLength);
	}

	bool contains_segment(const position_t& pos, size_t n, const volatile composite_buffer& cmp, size_t cmpIndex = 0, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != m_contents->position_of_segment(pos.m_pos, n, *(cmp.m_contents.begin_read()), cmpIndex, cmpLength);
	}


	bool contains_segment(const position_t& pos, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != m_contents->position_of_segment(pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool contains_segment(const position_t& pos, size_t n, const composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const volatile
	{
		return get_end_position() != m_contents.begin_read()->position_of_segment(pos.m_pos, n, *(cmp.m_contents), cmpPos.m_pos, cmpLength);
	}

	bool contains_segment(const position_t& pos, size_t n, const volatile composite_buffer& cmp, const position_t& cmpPos, size_t cmpLength = const_max_int_v<size_t>) const
	{
		return get_end_position() != m_contents->position_of_segment(pos.m_pos, n, *(cmp.m_contents.begin_read()), cmpPos.m_pos, cmpLength);
	}

	size_t get_inner_count() const { return m_contents->m_vectorVector.get_length(); }
	size_t get_inner_count() const volatile { return m_contents.begin_read()->m_vectorVector.get_length(); }

	const vector<char>& get_inner_array(size_t i) const { return m_contents->m_vectorVector.get_const_ptr()[i]; }
	const vector<char> get_inner_array(size_t i) const volatile { return m_contents.begin_read()->m_vectorVector.get_const_ptr()[i]; }

	const buffer& get_inner_buffer(size_t i, unowned_t<buffer>& storage = unowned_t<buffer>().get_unowned()) const
	{
		return buffer::from_vector(get_inner_array(i), storage);
	}

	const buffer& get_inner_buffer(size_t i, unowned_t<buffer>& storage = unowned_t<buffer>().get_unowned()) const volatile
	{
		return buffer::from_vector(get_inner_array(i), storage);
	}

	const buffer& get_inner(size_t i, unowned_t<buffer>& storage = unowned_t<buffer>().get_unowned()) const
	{
		return buffer::from_vector(get_inner_array(i), storage);
	}

	const buffer& get_inner(size_t i, unowned_t<buffer>& storage = unowned_t<buffer>().get_unowned()) const volatile
	{
		return buffer::from_vector(get_inner_array(i), storage);
	}

	void reserve_sub_buffers(size_t n) { return m_contents->m_vectorVector.reserve(n); }


	void reset() { m_contents->reset(); }
	void reset() volatile { this_t tmp; swap(tmp); }

	void clear() { m_contents->clear(); }
	void clear() volatile
	{
		this_t tmp;
		swap(tmp);
	}

	const char& operator[](const position_t& pos) const { return get_inner_array(pos.get_outer_index())[pos.get_inner_index()]; }


	composite_buffer subrange(size_t i, size_t n = const_max_int_v<size_t>) const
	{
		composite_buffer result(*this);
		result.set_to_subrange(i, n);
		return result;
	}

	composite_buffer subrange(size_t i, size_t n = const_max_int_v<size_t>) const volatile
	{
		composite_buffer result(*this);
		result.set_to_subrange(i, n);
		return result;
	}

	composite_buffer subrange(const position_t& start, size_t n) const
	{
		composite_buffer result(*this);
		result.set_to_subrange(start, n);
		return result;
	}

	composite_buffer subrange(const position_t& start, size_t n) const volatile
	{
		composite_buffer result(*this);
		result.set_to_subrange(start, n);
		return result;
	}

	composite_buffer subrange(const position_t& start, const position_t& end = position_t(const_max_int_v<size_t>, const_max_int_v<size_t>)) const
	{
		composite_buffer result(*this);
		result.set_to_subrange(start, end);
		return result;
	}

	composite_buffer subrange(const position_t& start, const position_t& end = position_t(const_max_int_v<size_t>, const_max_int_v<size_t>)) const volatile
	{
		composite_buffer result(*this);
		result.set_to_subrange(start, end);
		return result;
	}


	void assign(size_t n, const char& src) { assign(buffer(n, src)); }

	void assign(size_t n, const char& src) volatile { assign(buffer(n, src)); }

	void assign(const char* src, size_t n) { assign(buffer(src, n)); }

	void assign(const char* src, size_t n) volatile { assign(buffer(src, n)); }

	void assign(const composite_buffer& src) { *m_contents = *(src.m_contents); }

	void assign(const volatile composite_buffer& src) { *m_contents = *(src.m_contents.begin_read()); }

	void assign(const composite_buffer& src) volatile { m_contents.set(*(src.m_contents)); }

	void assign(const buffer& src) { *m_contents = src; }

	void assign(const volatile buffer& src) { *m_contents = src; }

	void assign(const buffer& src) volatile { m_contents.set(src); }

	this_t& operator=(const composite_buffer& src) { assign(src); return *this; }
	this_t& operator=(const volatile composite_buffer& src) { assign(src); return *this; }
	volatile this_t& operator=(const composite_buffer& src) volatile { assign(src); return *this; }

	this_t& operator=(const buffer& src) { assign(src); return *this; }

	this_t& operator=(const volatile buffer& src) { assign(src); return *this; }

	volatile this_t& operator=(const buffer& src) volatile { assign(src); return *this; }

	void append(size_t n, const char& src) { m_contents->append(buffer(n, src)); }

	void append(const char* src, size_t n) { m_contents->append(buffer(src, n)); }


	void append(const volatile composite_buffer& src) { m_contents->append(*(src.m_contents.begin_read())); }

	void append(const composite_buffer& src) { m_contents->append(*(src.m_contents)); }


	void append(const buffer& src) { m_contents->append(src); }

	void append(const volatile buffer& src) { m_contents->append(src); }


	this_t& operator+=(const composite_buffer& src)
	{
		m_contents->append(*(src.m_contents));
		return *this;
	}

	this_t& operator+=(const volatile composite_buffer& src)
	{
		composite_buffer tmp(src);
		return operator+=(tmp);
	}

	this_t& operator+=(const buffer& src)
	{
		m_contents->append(src);
		return *this;
	}

	this_t& operator+=(const volatile buffer& src)
	{
		buffer tmp(src);
		return operator+=(tmp);
	}


	this_t operator+(const composite_buffer& src)
	{
		this_t result(*this);
		result.m_contents->append(*(src.m_contents));
		return result;
	}

	this_t operator+(const volatile composite_buffer& src)
	{
		this_t result(*this);
		composite_buffer tmp(src);
		result.m_contents->append(*(tmp.m_contents));
		return result;
	}

	this_t operator+(const buffer& src)
	{
		this_t result(*this);
		result.m_contents->append(src);
		return result;
	}

	this_t operator+(const volatile buffer& src)
	{
		this_t result(*this);
		buffer tmp(src);
		result.m_contents->append(tmp);
		return result;
	}


	void prepend(const composite_buffer& src) { m_contents->prepend(*(src.m_contents)); }

	void prepend(const volatile composite_buffer& src) { m_contents->prepend(*(src.m_contents.begin_read())); }


	void prepend(const buffer& src) { m_contents->prepend(src); }

	void prepend(const volatile buffer& src) { m_contents->prepend(src); }


	void swap(composite_buffer& wth) { m_contents.swap(wth.m_contents); }

	void swap(composite_buffer& wth) volatile { m_contents.swap(wth.m_contents); }
	void swap(volatile composite_buffer& wth) { wth.m_contents.swap(m_contents); }


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

	buffer pop_first_array() { return buffer::from_vector(m_contents->pop_first_array()); }

	buffer pop_first_array() volatile
	{
		vector<char> result;
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
		return buffer::from_vector(result);
	}

	buffer pop_last_array() { return buffer::from_vector(m_contents->pop_last_array()); }

	buffer pop_last_array() volatile
	{
		vector<char> result;
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
		return buffer::from_vector(result);
	}

	composite_buffer split_off_arrays_before(size_t i)
	{
		composite_buffer result;
		m_contents->split_off_arrays_before(i, *(result.m_contents));
		return result;
	}

	composite_buffer split_off_arrays_before(size_t i) volatile
	{
		composite_buffer result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			wt->split_off_arrays_before(i, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}

	composite_buffer split_off_arrays_after(size_t i)
	{
		composite_buffer result;
		m_contents->split_off_arrays_after(i, *(result.m_contents));
		return result;
	}

	composite_buffer split_off_arrays_after(size_t i) volatile
	{
		composite_buffer result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			wt->split_off_arrays_after(i, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}


	composite_buffer split_off_before(size_t i)
	{
		composite_buffer result;
		m_contents->split_off_before(i, *(result.m_contents));
		return result;
	}

	composite_buffer split_off_before(size_t i) volatile
	{
		composite_buffer result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			wt->split_off_before(i, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}

	composite_buffer split_off_before(const position_t& pos)
	{
		composite_buffer result;
		m_contents->split_off_before(pos.m_pos, *(result.m_contents));
		return result;
	}

	composite_buffer split_off_before(const position_t& pos) volatile
	{
		composite_buffer result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			wt->split_off_before(pos.m_pos, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}


	composite_buffer split_off_after(size_t i)
	{
		composite_buffer result;
		m_contents->split_off_after(i, *(result.m_contents));
		return result;
	}

	composite_buffer split_off_after(size_t i) volatile
	{
		composite_buffer result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			wt->split_off_after(i, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}


	composite_buffer split_off_after(const position_t& pos)
	{
		composite_buffer result;
		m_contents->split_off_after(pos.m_pos, *(result.m_contents));
		return result;
	}

	composite_buffer split_off_after(const position_t& pos) volatile
	{
		composite_buffer result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			wt->split_off_after(pos.m_pos, *(result.m_contents));
			if (!result) // if nothing split off, no change to write
				break;
			if (m_contents.end_write(wt))
				break;
			result.clear();
		}
		return result;
	}


	composite_buffer split_off_front(size_t n)
	{
		return split_off_before(n);
	}

	composite_buffer split_off_front(size_t n) volatile
	{
		return split_off_before(n);
	}

	composite_buffer split_off_back(size_t n)
	{
		composite_buffer result;
		m_contents->split_off_back(n, *(result.m_contents));
		return result;
	}

	composite_buffer split_off_back(size_t n) volatile
	{
		composite_buffer result;
		write_token wt;
		for (;;)
		{
			m_contents.begin_write(wt);
			wt->split_off_back(n, *(result.m_contents));
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

	template <typename char_t>
	composite_string_t<char_t> to_string_t() const
	{
		composite_string_t<char_t> result;
		size_t n = get_inner_count();
		result.reserve_sub_strings(n);
		for (size_t i = 0; i < n; i++)
			result += get_inner(i).template to_string_t<char_t>();
		return result;
	}

	composite_string to_string() const { return to_string_t<wchar_t>(); }
	composite_cstring to_cstring() const { return to_string_t<char>(); }

	const_iterator begin() const { return get_first_const_iterator(); }
	const_iterator rbegin() const { return get_last_const_iterator(); }
	const_iterator end() const { const_iterator i; return i; }
	const_iterator rend() const { const_iterator i; return i; }
};


}
}


#endif
