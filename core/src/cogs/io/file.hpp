////
////  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
////
//
//
//// Status: Placeholder, WorkInProgress, Ancient
//
//#ifndef COGS_HEADER_IO_FILE
//#define COGS_HEADER_IO_FILE
//
//#include <type_traits>
//
//#include "cogs/collections/dlink.hpp"
//#include "cogs/collections/map.hpp"
//#include "cogs/collections/string.hpp"
//#include "cogs/collections/vector.hpp"
//#include "cogs/function.hpp"
//#include "cogs/env.hpp"
//#include "cogs/io/datastream.hpp"
//#include "cogs/mem/object.hpp"
//#include "cogs/mem/ptr.hpp"
//#include "cogs/operators.hpp"
//#include "cogs/sync/delegate_guard.hpp"
//
//
//namespace cogs {
//namespace io {
//
//
//class bits { };
//class bytes { };
//class kilobytes { };
//class megabytes { };
//class gigabytes { };
//class terabytes { };
//class petabytes { };
//class exabytes { };
//class zettabytes { };
//class yottabytes { };
//class brontobytes { };
//class geopbytes { };
//
//
////typedef linear::segment<file_size_t, bytes> file_range_segment_t;
//
//class file_segment
//{
//private:
//	linear::segment<dynamic_integer, bytes> m_segment;
//
//public:
//	file_segment()
//	{ }
//
//	file_segment(const this_t& src)
//		: m_segment(src.m_segment)
//	{ }
//
//	
//	file_segment(const num_t& start, const num_t& length)
//		: m_start(start),
//		m_length(length)
//	{ }
//
//	const num_t& get_length() const { return m_length; }
//	const num_t& get_start() const { return m_start; }
//	const num_t get_end() const { return m_start + m_length; }
//
//	void set_start(const num_t& s) { m_start = s; }
//	void set_length(const num_t& l) { m_length = l; }
//
//	this_t split_off_after(num_t n)
//	{
//		num_t resultLength = 0;
//		if (n < m_length)
//		{
//			resultLength = m_length - n;
//			m_length = n;
//		}
//		return this_t(m_start + n, resultLength);
//	}
//
//	this_t split_off_before(num_t n)
//	{
//		num_t n2 = n;
//		if (n2 > m_length)
//			n2 = m_length;
//		this_t result(m_start, n2);
//		m_length -= n2;
//		m_start += n;
//		return result;
//	}
//
//	this_t get_trailing(num_t n) const
//	{
//		num_t resultLength = 0;
//		if (n < m_length)
//			resultLength = m_length - n;
//		return this_t(m_start + n, resultLength);
//	}
//
//	this_t get_leading(num_t n) const
//	{
//		num_t n2 = n;
//		if (n2 > m_length)
//			n2 = m_length;
//		this_t result(m_start, n2);
//		return result;
//	}
//
//	void advance(num_t n)
//	{
//		num_t resultLength = 0;
//		if (n < m_length)
//			resultLength = m_length - n;
//		m_length = resultLength;
//		m_start += n;
//	}
//
//	void truncate_to(num_t n)
//	{
//		if (n < m_length)
//			m_length = n;
//	}
//
//	void truncate(num_t n)
//	{
//		if (n <= m_length)
//			m_length -= n;
//	}
//
//	void truncate_to_right(num_t n)
//	{
//		if (n < m_length)
//		{
//			m_start += (m_length - n);
//			m_length = n;
//		}
//	}
//
//	// merge() stores the union of both segments.  It's likely an error to use merge() with
//	// segments that are not overlaping or adjacent.
//	void merge(const this_t& s)
//	{
//		if (m_start < s.m_start)
//		{
//			num_t gap = s.m_start - m_start;
//			num_t sLength = s.m_length + gap;
//			if (m_length < sLength)
//				m_length = sLength;
//		}
//		else // if (s.m_start <= m_start)
//		{
//			if (s.m_start < m_start)
//			{
//				num_t gap = m_start - s.m_start;
//				m_length += gap;
//				m_start = s.m_start;
//			}
//			if (m_length < s.m_length)
//				m_length = s.m_length;
//		}
//	}
//
//	bool does_overlap(const this_t& s) const
//	{
//		if (m_start <= s.m_start)
//			return s.m_start < get_end();
//		//else if (s.m_start < m_start)
//		return m_start < s.get_end();
//	}
//
//	bool operator<(const this_t& s2) const
//	{
//		return m_start < s2.m_start;
//	}
//};
//
//class data_segment
//{
//};
//
//
//
//#if 0
//
//// A file_size_t is a native unsigned integer type with sufficient range to store a position
//// or size of a file within a particular file system.  For example, a file stored as a
//// block of memory may use size_t (which is 32-bit on a 32-bit system) as its file_size_t.
//// If that block of memory were known at compile time to be less than 64K, a uint16_t could
//// be used.  An OS's file system likely supports 64-bit file sizes/positions.  The cogs OS
//// header "os.hpp" provides this information in os::file_system::size_bits.
//typedef bits_to_uint_t<os::file_system::size_bits> default_file_size_t;
//
//// A segment<> combines a file position and length.
//// It's used to identify a single contiguous area within a file, such as to read from or write to.
//template <typename file_size_t>
//class segment;
//
//// A segment_buffer<> combines a start position and a const_buffer (containing data of some length).
//// It's used to identify the contents (read from or to write to) a single contiguous area within a file.
//template <typename file_size_t>
//class segment_buffer;
//
//// A segment_buffer<> combines a start position and a composite_buffer.  It's similar to a segment_buffer<>,
//// but uses a composite_buffer instead of a single const_buffer, allowing multiple buffers to be used to
//// identify the contents (read from or to write to) a single contiguous area within a file.
//template <typename file_size_t>
//class segment_bufferlist;
//
//// A segment_map<> contains a balanced binary tree of segments sorted by start position. It is maintained
//// such that overlaps and adjacent segments are coalesced.  All segments are adjacent only to gaps.
//// A segment_map<> is useful to identify a non-contiguous area (multiple contiguous areas) within a file.
//template <typename file_size_t>
//class segment_map;
//
//// A segment_buffer_map<> contains a balanced binary tree of segment_buffers<> sorted by start position.
//// It is maintained such that there are no overlaps, by overwriting older buffers when new ones overlap them.
//// It's used to identify the contents (read from or to write to) a non-contiguous area (multiple contiguous
//// areas) within a file.
//template <typename file_size_t>
//class segment_buffer_map;
//
//// A file_mask<> contains a segment_map<> and a bool indicating if the EOF (or past the EOF) should also
//// be considered.  It is useful as a mask for range locking IO transactions.  Reading the current EOF
//// position is a read operation.  If the EOF is unknown and must be read before written to, a write
//// mask should include (past) the EOF. 
//// synchronized_file<read_write_access>::locking_transaction uses a file_mask<> to indication ranges to write-lock.
//template <typename file_size_t>
//class file_mask;
//
//// file<> is an interface base class for objects that represent a randomly accessible block of binary data
//// of a given size (and may potentially be resizable).  (i.e. files, or a block of memory)
//// Like stream objects (derived from datasource/datasink), all operations on a file<> are asynchronous.
//
//// Because the access_mode is built into the type as a template arg, file<> provides compile-time
//// enforcement of read/write access permission.  file<read_write_access> is redrived from file<read_access>
//// and file<write_access>, so can be cast down.
//enum class access_mode
//{
//	read_access       = 0x01, // 01
//	write_access      = 0x02, // 10
//	read_write_access = 0x03  // 11
//};
//
//template <access_mode mode, typename file_size_t = default_file_size_t>
//class file;
//
//// Unlike steam objects, a file<> does not have a 'current position'.
//// However, any number of cursor<>'s may be created for a file<>. 
//// cursor<>'s are stream objects (derived from datasource/datasink).
////
////template <access_mode mode>
////class file<>::cursor<mode>;
//
//// Vectored IO / Scatter/Gather IO
////
//// Traditional file IO APIs generally provide ways to read and write single contiguous segments per API call.
//// The terms Vectored IO or Scatter/Gather IO refer to file IO APIs that allow non-contiguous (multiple)
//// segments of a file, and/or multiple buffers, to be read or written per call.
//// 
//// file<> supports a variation of these concepts in which:
////
////		- In a single write operation, data (gathered) from multiple buffers can be written (scattered)
////			to multiple locations within a file.
////
////			Traditional scatter/gather IO APIs for writing generally only provide gathering of buffers,
////			and not scattering into the file.
////
////		- In a single read operation, data can be read (gathered) from multiple locations within a file.
////
////			Traditional scatter/gather IO APIs 'scatter' a contiguous read into multiple buffers.
////			Instead, file<> 'gathers' multiple file segments in a single read.
////
////			The IO model used by the cogs library is designed to minimize the need to copy data, particularly
////			when IO involves layers such as when both reader and writer are in the same process, or a protocol
////			parsing data out of a stream.  Rather than allocate new buffers at each layer, existing internal
////			buffers are returned from read operations when possible (perhaps the very same buffers passed to a
////			write operation elsewhere), at the expense of a single potentially redundant buffer copy at the
////			top level read.  (See: cogs IO Model in buffer.h)
//
//// synchronized_file<> is a concrete class implementing the file<> interface, which adds parallel IO management,
//// transactions, caching, etc.. 
//// synchronized_file<> wraps another file<>, so essential adapts an existing file<> to add synchronization APIs.
//template <access_mode mode, typename file_size_t = default_file_size_t>
//class synchronized_file;
//
//// synchronized_file_impl<> is internal.  It implements the core functionality of synchronized_file<>.
//// synchronized_file<read_write_access> is derived from both synchronized_file<read_access> and 
//// synchronized_file<write_access>.  To avoid multiply-deriving from a common base class, core functionality
//// resides in a protected synchronized_file_impl<> member instance.  
//template <typename file_size_t = default_file_size_t>
//class synchronized_file_impl;
//
//// Because parallel file IO is only within the context of a synchronized_file<> instance,
//// it's important to ensure that the same file is not also accessible through a (non-synchronized)
//// file<> and that there are not other instances of synchronized_file<> referring to the same
//// file.  The underlying implementation that originates a synchronized_file<> must ensure that
//// multiple attempts to open the same file result in the return of the same synchronized_file<>
//// object.  (Note: this may mean the underlying implementation must instantiate a synchronized_file<read_write_access>
//// even if only read_access is requested, in case write access to the same file is requested later.)
//// 
//// It's also not supported to concurrently write to a file also from another process, as this
//// would circumvent the parallel file IO management.
//// 
//// A write operation is immediately available to subsequent read operations that overlap it.
////
//// A synchronized_file<> may linger until all underlying IO operations are complete.
////
//// TBD: Internal buffers from completed reads and writes can be configured to remain cached in memory,
//// TBD: speeding up subsequent reads.
////
//// synchronized_file<> assumes that the storage media will not become unavailable, and
//// will not fail write operations (such as out of disk space).  This can be mitigated by
//// the lower level file IO object by prompting the user to restore the media or make
//// additional disk space available, and delaying completion of the IO request until that
//// is complete.  If failure cannot be mitigated, the file should become closed and pending
//// IO operations aborted, leaving the contents of the file in an undefined state.
////
//// synchronized_file<> supports 3 types of transactions:
//
//// composed_transaction
////
//// 		A composed_transaction accumulates requested read and write operations, and does
//// 		not commit any of them until the composed_transaction is submitted.  No further blocking
////		occurs, as each submitted composed_transaction is effectively commited atomically.
////
////		Using file<>'s read/write APIs to read individual segments at a time, results in
////		internally using simple composed_transaction's containing single read/write operations.
////
////		A single composed_transaction may result in multiple parallel calls to the underlying
////		file system.	
////
////class synchronized_file<read_access>::composed_transaction;
////class synchronized_file<write_access>::composed_transaction;
////class synchronized_file<read_write_access>::composed_transaction;
//
//// locking_transaction
////
////		A locking_transaction provides a consistent view of a file, by blocking writes by other
////		transactions that might compromise the consistency of that view.  Not all writes are
////		blocked.  If another transaction's write can be committed without overlapping any range
////		already read by a pending locking_transaction, it is allowed as it does not compromise
////		a consistent view of the file.
////
////			synchronized_file<read_access>::locking_transaction	
////
////				A read-only locking_transaction provides a consistent, read-only view of a file, by effectively
////				locking out writes.  This is useful when it's necessary to read some contents in order
////				to determine where else to read from.  
////
////			synchronized_file<read_write_access>::locking_transaction	
////
////				A read/write locking_transaction provides a consistent, read/write view of a file.
////				This is useful when contents need to be read, in order to determine what else needs
////				to be read and/or written.
////
////				A read/write locking_transaction processes reads when issued, but accumulates writes until
////				the read/write locking_transaction is submitted.  When reading a portion that had previously
////				been written within the same transaction, the (pre) written data is read.
////
////		To ensure consistency is maintained for the duration of a locking_transaction:
////	
////			On creation, a read/write locking_transaction must specify the areas it may write to.
////			Writes outside of these areas could cause inconsistencies.  Specifying the entire
////			file is valid (and the default), but will cause all reads issued by subsequent concurrent
////			transactions to be deferred.
////
////			If a new read overlaps a write of an existing (lower transaction_id)
////			read/write locking_transaction, the read is deferred (until no locker blocked by writes).
////
////			If a new read overlaps a write by a subsequently started (higher transaction_id) read/write locking_transaction,
////			that read/write locking_transaction is blocked from submitting its writes (until no longer blocked by reads).
////			
////			Conversely, if a new (potential) write overlaps reads of an existing (lower transaction_id) 
////			transaction, the read/write locking_transaction is blocked from submitting its writes
////			(until no longer blocked by reads).
////
////class synchronized_file<read_access>::locking_transaction;
////class synchronized_file<read_write_access>::locking_transaction;
//
//// failable_transaction
////
////		A failable_transaction is like a locking_transaction, but instead
////		of blocking other transactions, any operation that compromises the consistency of
////		its view of a file will cause the failable_transaction to abort.  When
////		aborted, all subsequent read operations will fail, and any writes will fail once
////		the aborted transaction is submitted.
////		
////		This is useful when a transaction become irrelevant if the data it uses is altered.
////		It could be used in a retry loop to accomplish the equivalent of a locking_transaction
////		that does not block any other transactions.  However, that approach would be vulnerable to a
////		starvation deadlock due to persistent interruption.  It should generally be avoided unless
////		this deadlock scenario can be otherwise mitigated.
////
////class synchronized_file<read_access>::failable_transaction;
////class synchronized_file<read_write_access>::failable_transaction;
//
//// File Size
////
////	Getting the size of a file is considered a read operation.  Setting the size of a
////	file is considered a write operation.
////
////	A common scenario is needing to append data to the end of a file, and store
////	the file position that was written to somewhere else.  That should be done using a
////	read/write locking_transaction to first read the file size, then write, within the
////	same transaction.
////
////old
//// NOT DONE WITH EOF!:
////	The constant atEOF can be used as a start position of a write to indicate that the
////  write occurs at the current EOF.  Writes at the EOF are serialized.  To managed
////	serializing writes at EOF, and arbitrary changes in EOF, a file<read_write_access>
////	caches its EOF value.  Also extending the file from another process, is unsupported.
////
////	When an EOF is extended by a write operation, that operation is issued normally.
////	Any write that completes successfully results in the cached EOF being updated.
////	If a read is issued that extends beyond the current caches EOF, it is clipped.
////	If the EOF is truncated such that any already issued reads extend beyond the new 
////	EOF, the EOF is not successfully truncated until those reads are complete.  If
////	there is an attempt to truncate an EOF while there are issued or pending writes
////	beyond the new EOF, the EOF-setting operation is completed when any write equal
////	or beyond it is completed, and the EOF will be set to the end of that write
////	operation instead.
//
////---------------------------------------------------------------
//
//template <typename file_size_t>
//class segment
//{
//private:
//	file_size_t	m_start;
//	file_size_t	m_length;
//
//public:
//	typedef segment<file_size_t> this_t;
//
//	segment()
//	{ }
//
//	segment(const this_t& s)
//		: m_start(s.m_start),
//		m_length(s.m_length)
//	{ }
//
//	segment(const file_size_t& start, const file_size_t& length)
//		: m_start(start),
//		m_length(length)
//	{ }
//	
//	const file_size_t& get_length() const { return m_length; }
//	const file_size_t& get_start() const { return m_start; }
//	const file_size_t get_end() const { return m_start + m_length; }
//
//	void set_start(const file_size_t& s) { m_start = s; }
//	void set_length(const file_size_t& l) { m_length = l; }
//
//	this_t split_off_after(file_size_t n)
//	{
//		file_size_t resultLength = 0;
//		if (n < m_length)
//		{
//			resultLength = m_length - n;
//			m_length = n;
//		}
//		return this_t(m_start + n, resultLength);
//	}
//
//	this_t split_off_before(file_size_t n)
//	{
//		file_size_t n2 = n;
//		if (n2 > m_length)
//			n2 = m_length;
//		this_t result(m_start, n2);
//		m_length -= n2;
//		m_start += n;
//		return result;
//	}
//	
//	this_t get_trailing(file_size_t n) const
//	{
//		file_size_t resultLength = 0;
//		if (n < m_length)
//			resultLength = m_length - n;
//		return this_t(m_start + n, resultLength);
//	}
//
//	this_t get_leading(file_size_t n) const
//	{
//		file_size_t n2 = n;
//		if (n2 > m_length)
//			n2 = m_length;
//		this_t result(m_start, n2);
//		return result;
//	}		
//
//	void advance(file_size_t n)
//	{
//		file_size_t resultLength = 0;
//		if (n < m_length)
//			resultLength = m_length - n;
//		m_length = resultLength;
//		m_start += n;
//	}
//
//	void truncate_to(file_size_t n)
//	{
//		if (n < m_length)
//			m_length = n;
//	}
//		
//	void truncate(file_size_t n)
//	{
//		if (n <= m_length)
//			m_length -= n;
//	}
//		
//	// merge() stores the union of both segments.  It's likely an error to use merge() with
//	// segments that are not overlaping or adjacent.
//	void merge(const this_t& s)
//	{
//		if (m_start < s.m_start)
//		{
//			file_size_t gap = s.m_start - m_start;
//			file_size_t sLength = s.m_length + gap;
//			if (m_length < sLength)
//				m_length = sLength;
//		}
//		else // if (s.m_start <= m_start)
//		{
//			if (s.m_start < m_start)
//			{
//				file_size_t gap = m_start - s.m_start;
//				m_length += gap;
//				m_start = s.m_start;
//			}
//			if (m_length < s.m_length)
//				m_length = s.m_length;
//		}
//	}
//
//	bool does_overlap(const this_t& s) const
//	{
//		if (m_start <= s.m_start)
//			return s.m_start < get_end();
//		//else if (s.m_start < m_start)
//		return m_start < s.get_end();
//	}
//
//	bool operator<(const this_t& s2) const
//	{
//		return m_start < s2.m_start;
//	}
//};
//
//template <typename file_size_t>
//class segment_buffer
//{
//private:
//	typedef segment_buffer<file_size_t>	this_t;
//
//	file_size_t		m_start;
//	const_buffer	m_buffer;
//
//public:
//	segment_buffer()
//	{ }
//
//	segment_buffer(const segment_buffer& s)
//		: m_start(s.m_start),
//		m_buffer(s.m_buffer)
//	{ }
//
//	segment_buffer(const file_size_t& start, const const_buffer& b)
//		: m_start(start),
//		m_buffer(b)
//	{ }
//		
//	const size_t get_length() const { return m_buffer.size(); }
//	const file_size_t& get_start() const { return m_start; }
//	const file_size_t get_end() const { return m_start + m_buffer.size(); }
//	const segment<file_size_t> get_segment() const { return segment<file_size_t>(m_start, m_buffer.size()); }
//	const const_buffer& get_buffer() const { return m_buffer; }
//
//	this_t split_off_after(size_t n)
//	{
//		return this_t(m_start + n, m_buffer.split_off_after(n));
//	}
//
//	this_t split_off_before(size_t n)
//	{
//		this_t result(m_start, m_buffer.split_off_before(n));
//		m_start += n;
//		return result;
//	}
//		
//	this_t get_trailing(size_t n)
//	{
//		return this_t(m_start + n, m_buffer.get_trailing(n));
//	}
//
//	this_t get_leading(size_t n)
//	{
//		return this_t(m_start, m_buffer.get_leading(n));
//	}		
//
//	void advance(size_t n)
//	{
//		m_buffer.advance(n);
//		m_start += n;
//	}
//
//	void truncate_to(size_t n)
//	{
//		m_buffer.truncate_to(n);
//	}
//		
//	void truncate(size_t n)
//	{
//		m_buffer.truncate(n);
//	}
//
//	bool operator<(const this_t& s2) const
//	{
//		return m_start < s2.m_start;
//	}
//};
//
//template <typename file_size_t>
//class segment_bufferlist
//{
//private:
//	typedef segment_bufferlist<file_size_t> this_t;
//	file_size_t	m_start;
//	composite_buffer	m_buffer;
//
//public:
//	segment_bufferlist()
//	{ }
//
//	segment_bufferlist(const this_t& s)
//		:m_start(s.m_start),
//		m_buffer(s.m_buffer)
//	{ }
//
//	segment_bufferlist(const segment_buffer<file_size_t>& s)
//		: m_start(s.get_start()),
//		m_buffer(s.get_buffer())
//	{ }
//
//	segment_bufferlist(const file_size_t& start, const composite_buffer& b)
//		: m_start(start),
//		m_buffer(b)
//	{ }
//		
//	const size_t get_length() const { return m_buffer.get_length(); }
//	const file_size_t& get_start() const { return m_start; }
//	const file_size_t get_end() const { return m_start + m_buffer.size(); }
//	const segment<file_size_t> get_segment() const { return segment<file_size_t>(m_start, m_buffer.size()); }
//	const composite_buffer& get_buffer() const { return m_buffer; }
//
//	this_t split_off_after(size_t n)
//	{
//		return this_t(m_start + n, m_buffer.split_off_after(n));
//	}
//
//	this_t split_off_before(size_t n)
//	{
//		this_t result(m_start, m_buffer.split_off_before(n));
//		m_start += n;
//		return result;
//	}
//		
//	this_t get_trailing(size_t n)
//	{
//		return this_t(m_start + n, m_buffer.get_trailing(n));
//	}
//
//	this_t get_leading(size_t n)
//	{
//		return this_t(m_start, m_buffer.get_leading(n));
//	}		
//
//	void advance(size_t n)
//	{
//		m_buffer.advance(n);
//		m_start += n;
//	}
//
//	void truncate_to(size_t n)
//	{
//		m_buffer.truncate_to(n);
//	}
//		
//	void truncate(size_t n)
//	{
//		m_buffer.truncate(n);
//	}
//
//	// merge() stores the union of both segment_bufferlists.  It's an error to use merge() with
//	// segment_bufferlists that are not overlaping or adjacent.
//	void merge(const this_t& s)
//	{
//		composite_buffer newBuffer;
//		if (m_start < s.m_start)
//		{
//			newBuffer = m_buffer;
//			file_size_t gap = s.m_start - m_start;
//			newBuffer.truncate_to(gap);
//			newBuffer.append(s.m_buffer);
//
//			file_size_t sLength = gap + s.m_length;
//			if (sLength < m_length)
//			{
//				m_buffer.advance(sLength);
//				newBuffer.append(m_buffer);
//			}
//		}
//		else // if (s.m_start <= m_start)
//		{
//			newBuffer = s.m_buffer;
//			file_size_t gap = m_start - s.m_start;
//			m_start = s.m_start;
//			if (s.m_length < gap + m_length)
//			{
//				m_buffer.advance(s.m_length - gap);
//				newBuffer.append(m_buffer);
//			}
//		}
//		m_buffer = newBuffer;
//	}
//
//	bool operator<(const this_t& s2) const
//	{
//		return m_start < s2.m_start;
//	}
//};
//
//// segment_map, segment_buffer, and state_map
////
//// These three types all store a balancing tree of nodes sorted by the start positions
//// of non-overlapping segments.  In addition to the obvious differences in data each
//// node associates with segments, their behavior with regards to coalescing and
//// dividing their component nodes, differs subtly.
//// 
//// A segment_map has no data associated with the segment, other than the segment itself.
//// When adjacent or overlapping segments are added to the segment_map, they are all
//// coalesced into a single segment, such that there are never any adjacent segments.
//// All segments have gaps between them.
////
//// A segment_buffer_map associates a buffer with each segment.  When a segment_buffer is
//// added to the map that overlaps any existing segment_buffers, they will be overwritten.
//// Wholy overlapping nodes are released, and partially overlapping nodes are truncated.
//// However, adjacent nodes are not coalesced.  They must remain separate as they are
//// associated with different buffers.  (const_buffer's cannot be coalesced, unlike composite_buffer's).
////
//// A state_map associates various data with a segment, such as read and write operations
//// that are currently in progress or pending.  As new IO operations are requested,
//// the target segment may undergo  a 'create/split' operation.  If there are any portions
//// of the requested segment that do not already have nodes tracking them, then those
//// additional nodes are created and the gaps filled in.  If the boundaries of the
//// requested segment partially overlap existing nodes, those nodes will be split such
//// that node bounardies exist at the boundaries of the requested segment.  No nodes
//// are overwritten.  Rather, the 'create/split' operation ensures that a set of state
//// nodes exist to fully represent only the requested segment.
//
//template <typename file_size_t>
//class segment_map
//{
//private:
//	typedef segment_map<file_size_t> this_t;
//
//	segment_map(const this_t&);
//	segment_map& operator=(const this_t&);
//		
//	class node : public sorted_list_node<true, node>
//	{
//	public:
//		segment<file_size_t> m_segment;
//
//		node(const segment<file_size_t>& s)
//			: m_segment(s)
//		{ }
//
//		const file_size_t& get_key() const { return m_segment.get_start(); }
//
//		void merge(const segment<file_size_t>& s) { m_segment.merge(s); }
//
//		const file_size_t& get_start() const { return m_segment.get_start(); }
//		const file_size_t& get_length() const { return m_segment.get_length(); }
//		const file_size_t  get_end() const { return m_segment.get_end(); }
//	};
//
//	typedef sorted_list<file_size_t, true, node> list_t;
//
//	list_t	m_list;
//	size_t	m_count;
//
//	void clear_inner()
//	{
//		ptr<node> n = m_list.get_first_postorder();
//		while (!!n)
//		{
//			ptr<node> n2 = node::get_next_postorder(n);
//			default_allocator::destruct_deallocate_type(n.get());
//			n = n2;
//		}
//	}
//
//public:
//	// An iterator will remain valid until the referenced node has been removed.  (Such as when
//	// a segment is added, and coalescing causes nodes to be removed.)
//	class iterator
//	{
//	private:
//		ptr<node> m_node;
//
//	protected:
//		iterator(const ptr<node>& n) : m_node(n) { }
//
//		friend class segment_map;
//
//	public:
//		iterator() { }
//
//		iterator(const iterator& i) : m_node(i.m_node) { }
//
//		iterator& operator++() { if (!!m_node) m_node = node::get_next(m_node); return *this; }
//		iterator& operator--() { if (!!m_node) m_node = node::get_prev(m_node); return *this; }
//
//		bool operator!() const { return !m_node; }
//
//		bool operator==(const iterator& i) const { return m_node == i.m_node; }
//		bool operator!=(const iterator& i) const { return m_node != i.m_node; }
//
//		segment<file_size_t>* get() const { return (!m_node) ? (segment_t<file_size_t>*)0 : &(m_node->m_segment); }
//		segment<file_size_t>& operator*() const { return m_node->m_segment; }
//		segment<file_size_t>* operator->() const { return &(m_node->m_segment); }
//			
//		void release() { m_node = 0; }
//
//		iterator& operator=(const iterator& i) { m_node = i.m_node; return *this; }
//
//		iterator next() const { return iterator((!m_node) ? ptr<node>(0) : node::get_next(m_node)); }
//		iterator prev() const { return iterator((!m_node) ? ptr<node>(0) : node::get_prev(m_node)); }
//	};
//
//	segment_map() : m_count(0) { }
//	segment_map(const segment<file_size_t>& s) : m_count(1) { m_list.insert(new (default_allocator::get()) node(s)); }
//
//	~segment_map() { clear_inner(); }
//
//	void clear()
//	{
//		clear_inner();
//		m_list.clear();
//		m_count = 0;
//	}
//
//	bool is_empty() const { return !m_count; }
//	bool operator!() const { return !m_count; }
//	size_t size() const { return m_count; }
//		
//	bool does_overlap(const segment<file_size_t>& s) const
//	{
//		ptr<node> n = m_list.find_nearest_less_than(s.get_end());
//		// If nothing starts before the end of this segment, then there is no overlap.
//		// Otherwise, it overlaps if it ends after we start.
//		return (!!n) && (s.m_start < n->get_end());
//	}
//
//	bool does_overlap(const this_t& sm) const
//	{
//		// The shorter list should iterate through all blocks.
//		// The longer list should be doing binary lookups, in inner loop.
//		if (m_count > sm.size())
//			return sm.does_overlap(*this);
//
//		ptr<node> n = m_list.get_first();
//		while (!!n)
//		{
//			if (sm.does_overlap(n->get_segment()))
//				return true;
//			n = node::get_next(n);
//		}
//		return false;
//	}
//
//	bool does_overlap(const segment_buffer_map<file_size_t>& sbm) const;
//	
//	iterator add(const file_size_t& s, const file_size_t& len)
//	{
//		return add(segment<file_size_t>(s, len));
//	}
//
//	iterator add(const segment<file_size_t>& s)
//	{
//		iterator itor;
//		file_size_t trailingEnd;
//		// Find first one that starts before or at the end of the new segment.
//		ptr<node> trailingNode = m_list.find_any_equal_or_nearest_less_than(s.get_end());
//		bool overlapping = false;
//		if (!!trailingNode)
//		{
//			trailingEnd = trailingNode->get_end();
//			overlapping = (trailingEnd >= s.m_start);
//		}
//		if (!overlapping)
//		{
//			ptr<node> newNode = new (default_allocator::get()) node(s);
//			itor.m_node = newNode;
//			m_list.insert(newNode);
//			++m_count;
//		}
//		else	// trailingEnd will be set
//		{
//			bool doneMerge = false;
//			if ((trailingEnd != s.m_start) && (s.m_start < trailingNode->get_start()))
//			{
//				ptr<node> leadingNode = m_list.find_any_equal_or_nearest_less_than(s.m_start); // We know leadingNode != trailingNode
//				if (!leadingNode)
//					leadingNode = m_list.get_first(); // Might be some cruft overlapping, so start at beginning cleaning stuff out.
//				else if (leadingNode->get_end() < s.m_start) // If leadingNode is well before us.
//					++leadingNode;
//				else											// If leadingNode overlaps or is adjacent
//				{
//					leadingNode->merge(s);
//					trailingNode->merge(leadingNode->m_segment);
//					doneMerge = true;
//				}
//				// Delete all the junk starting at leadingNode, up to and not including trailingNode.
//				while (leadingNode != trailingNode)
//				{
//					ptr<node> next = node::get_next(leadingNode);
//					m_list.remove(leadingNode);
//					default_allocator::destruct_deallocate_type(leadingNode);
//					--m_count;
//					leadingNode = next;
//				}
//			}
//			if (!doneMerge)
//				trailingNode->merge(s);
//			itor.m_node = trailingNode;
//		}
//		return itor;
//	}
//
//	void add(const this_t& sm)
//	{
//		iterator itor = sm.get_first();
//		while (!!itor)
//		{
//			add(*itor);
//			++itor;
//		}
//	}
//
//	void remove(iterator& itor)
//	{
//		m_list.remove(itor.m_node);
//		default_allocator::destruct_deallocate_type(itor.m_node.get());
//		itor.m_node.clear();
//		--m_count;
//	}
//
//	void remove(const segment<file_size_t>& s)
//	{
//		file_size_t segmentEnd = s.get_end();
//		ptr<node> n = m_list.find_nearest_less_than(segmentEnd); // node starts before segment ends
//		while (!!n)
//		{
//			file_size_t nodeEnd = n->get_end();
//			if (segmentEnd < nodeEnd) // if node extends beyond the segment
//			{
//				if (n->get_start() < s->get_start()) // Surrounds it.  Need to poke a hole.
//				{
//					n->m_segment.truncate_to(s->get_start() - n->get_start());
//					m_list.insert(new (default_allocator::get()) node(segment<file_size_t>(segmentEnd, nodeEnd - segmentEnd)));
//					break;
//				}
//				n->advance(segmentEnd - n->get_start()); // include only the trailing region
//				n = n->get_prev();
//				if (!n)
//					break;
//				nodeEnd = n->get_end();
//			}
//
//			for (;;)
//			{
//				if (nodeEnd <= s.get_start())
//					break;
//
//				if (n->get_start() < s->get_start()) // if node is entirely within the segment, remove it.
//				{
//					n->m_segment.truncate_to(s->get_start() - n->get_start());
//					break;
//				}
//				ptr<node> prev = n->get_prev();
//				m_list.remove(n);
//				n = prev;
//				if (!n)
//					break;
//				nodeEnd = n->get_end();
//			}
//			break;
//		}
//	}
//
//	void remove(const this_t& sm)
//	{
//		iterator itor = sm.get_first();
//		while (!!itor)
//		{
//			remove(*itor);
//			++itor;
//		}
//	}
//
//	void combine(this_t& src) // Only use on known non-overlapping.  Removes all from src.
//	{
//		for (;;)
//		{
//			ptr<node> n = src.m_list.get_first();
//			if (!n)
//				break;
//			src.m_list.remove(n);
//			m_list.insert(n);
//		}
//	}
//
//	iterator get_first() const { return iterator(m_list.get_first()); }
//	iterator get_last() const { return iterator(m_list.get_last()); }
//
//	iterator find(const file_size_t& n) const { return iterator(m_list.find_any_equal(n)); }
//	iterator find_any_before(const file_size_t& n) const { return iterator(m_list.find_any_less_than(n)); }
//	iterator find_any_after(const file_size_t& n) const { return iterator(m_list.find_any_greater_than(n)); }
//	iterator find_equal_or_any_before(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_less_than(n)); }
//	iterator find_equal_or_any_after(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_greater_than(n)); }
//	iterator find_nearest_before(const file_size_t& n) const { return iterator(m_list.find_nearest_less_than(n)); }
//	iterator find_nearest_after(const file_size_t& n) const { return iterator(m_list.find_nearest_greater_than(n)); }
//	iterator find_equal_or_nearest_before(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_nearest_less_than(n)); }
//	iterator find_equal_or_nearest_after(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_nearest_greater_than(n)); }
//};
//
//template <typename file_size_t>
//class segment_buffer_map
//{
//private:
//	typedef segment_buffer_map<file_size_t> this_t;
//
//	segment_buffer_map(const this_t&);
//	this_t& operator=(const this_t&);
//	
//	class node : public sorted_list_node<true, node>
//	{
//	public:
//		segment_buffer<file_size_t> m_segmentBuffer;
//	
//		node(const segment_buffer<file_size_t>& sb)
//			: m_segmentBuffer(sb)
//		{ }
//
//		const file_size_t& get_key() const { return m_segmentBuffer.get_start(); }
//
//		const file_size_t& get_start() const { return m_segmentBuffer.get_start(); }
//		const file_size_t& get_length() const { return m_segmentBuffer.get_length(); }
//		const file_size_t  get_end() const { return m_segmentBuffer.get_end(); }
//
//		void advance(size_t n) { m_segmentBuffer.advance(n); }
//		void truncate_to(size_t n) { m_segmentBuffer.truncate_to(n); }
//
//		segment<file_size_t> get_segment() const { return m_segmentBuffer.get_segment(); }
//	};
//
//	typedef sorted_list<file_size_t, true, node> list_t;
//
//	list_t m_list;
//	size_t m_count;
//
//	void clear_inner()
//	{
//		ptr<node> n = m_list.get_first_postorder();
//		while (!!n)
//		{
//			ptr<node> n2 = node::get_next_postorder(n);
//			default_allocator::destruct_deallocate_type(n.get());
//			n = n2;
//		}
//	}
//
//public:
//	// An iterator will remain valid until the referenced node has been removed.  (Such as when
//	// a segment_buffer is added and the referenced node is released because it was replaced
//	// entirely.)
//	class iterator
//	{
//	private:
//		ptr<node> m_node;
//
//	protected:
//		iterator(const ptr<node>& n) : m_node(n) { }
//
//		friend class segment_buffer_map<file_size_t>;
//
//	public:
//		iterator() { }
//
//		iterator(const iterator& i) : m_node(i.m_node) { }
//
//		iterator& operator++() { if (!!m_node) m_node = node::get_next(m_node); return *this; }
//		iterator& operator--() { if (!!m_node) m_node = node::get_prev(m_node); return *this; }
//
//		bool operator!() const { return !m_node; }
//
//		bool operator==(const iterator& i) const { return m_node == i.m_node; }
//		bool operator!=(const iterator& i) const { return m_node != i.m_node; }
//
//		segment_buffer<file_size_t>* get() const { return (!m_node) ? (segment_t*)0 : &(m_node->m_segmentBuffer); }
//		segment_buffer<file_size_t>& operator*() const { return m_node->m_segmentBuffer; }
//		segment_buffer<file_size_t>* operator->() const { return &(m_node->m_segmentBuffer); }
//			
//		void release() { m_node = 0; }
//
//		iterator& operator=(const iterator& i) { m_node = i.m_node; return *this; }
//
//		iterator next() const { return iterator((!m_node) ? ptr<node>(0) : node::get_next(m_node)); }
//		iterator prev() const { return iterator((!m_node) ? ptr<node>(0) : node::get_prev(m_node)); }
//	};
//		
//	segment_buffer_map()
//		: m_count(0)
//	{ }
//
//	segment_buffer_map(const segment_buffer<file_size_t>& sb)
//		: m_count(1)
//	{ m_list.insert(new (default_allocator::get()) node(sb)); }
//
//	~segment_buffer_map() { clear_inner(); }
//
//	void clear()
//	{
//		clear_inner();
//		m_list.clear();
//		m_count = 0;
//	}
//
//	bool is_empty() const { return !m_count; }
//	bool operator!() const { return !m_count; }
//	size_t size() const { return m_count; }
//
//	iterator add(const segment_buffer<file_size_t>& sb)
//	{
//		file_size_t sbEnd = sb.get_end();
//		file_size_t trailingEnd; // Find first one that starts before the end of the new segment.
//		ptr<node> trailingNode = m_list.find_nearest_less_than(sbEnd);
//		bool overlapping = false;
//		if (!!trailingNode)
//		{
//			trailingEnd = trailingNode->get_end();
//			overlapping = (trailingEnd > sb.get_start());
//		}
//		if (!overlapping)
//			trailingNode = 0;
//		else // sb.get_start() < trailingEnd
//		{
//			if (trailingNode->get_start() < sb.get_start()) // If we need to split off the front of the buffer
//			{
//				size_t startGap = (size_t)(sb.get_start() - trailingNode->get_start());
//				if (trailingEnd <= sbEnd)
//					trailingNode->truncate_to(startGap); // Just need to split off the front.
//				else // if (sbEnd < trailingEnd) // need to break it into 3 parts, to split off the end also
//				{
//					m_list.insert(new (default_allocator::get()) node(trailingNode->m_segmentBuffer.split_off_after(startGap).split_off_before(sb.get_length())));
//					++m_count;
//				}
//				trailingNode = 0;
//			}
//			else 
//			{
//				if (sb.get_start() < trailingNode->get_start())
//				{
//					ptr<node> leadingNode = m_list.find_any_equal_or_nearest_less_than(sb.get_start()); // We know leadingNode != trailingNode
//					if (!leadingNode)
//						leadingNode = m_list.get_first(); // Might be some cruft overlapping, so start at beginning cleaning stuff out.
//					else if (leadingNode->get_end() <= sb.get_start()) // If leadingNode ends before us.
//						++leadingNode;
//					else if (leadingNode->get_start() < sb.get_start()) // Only truncate leadingNode if there is something left-over at the start of it.
//					{
//						leadingNode->truncate_to((size_t)(sb.get_start() - leadingNode->get_start()));
//						++leadingNode;
//					}
//					// Delete all the junk starting at leadingNode, up to and not including trailingNode.
//					while (leadingNode != trailingNode)
//					{
//						ptr<node> next = node::get_next(leadingNode);
//						m_list.remove(leadingNode);
//						--m_count;
//						default_allocator::destruct_deallocate_type(leadingNode);
//						leadingNode = next;
//					}
//				}
//				if (trailingEnd <= sbEnd)
//					trailingNode->m_segmentBuffer = sb;
//				else // if (sbEnd < trailingEnd)
//				{
//					trailingNode->advance((size_t)(sbEnd - trailingNode->get_start()));
//					trailingNode = 0;
//				}
//			}
//		}
//		if (!trailingNode)
//		{
//			trailingNode = new (default_allocator::get()) node(sb);
//			m_list.insert(trailingNode);
//			++m_count;
//		}
//		return iterator(trailingNode);
//	}
//	
//	iterator add(const file_size_t& start, const const_buffer& b)
//	{
//		return add(segment_buffer<file_size_t>(start, b));
//	}
//
//	void add(const this_t& sbm)
//	{
//		iterator itor = sbm.get_first();
//		while (!!itor)
//		{
//			add(*itor);
//			++itor;
//		}
//	}
//
//	void remove(iterator& itor)
//	{
//		m_list.remove(itor.m_node);
//		default_allocator::destruct_deallocate_type(itor.m_node.get());
//		itor.m_node.clear();
//		--m_count;
//	}
//
//	void remove(const segment<file_size_t>& s)
//	{
//		file_size_t segmentEnd = s.get_end();
//		ptr<node> n = m_list.find_nearest_less_than(segmentEnd); // node starts before segment ends
//		while (!!n)
//		{
//			file_size_t nodeEnd = n->get_end();
//			if (segmentEnd < nodeEnd) // if node extends beyond the segment
//			{
//				if (n->get_start() < s->get_start()) // Surrounds it.  Need to poke a hole.
//				{
//					n->m_segment.truncate_to(s->get_start() - n->get_start());
//					m_list.insert(new (default_allocator::get()) node(segment<file_size_t>(segmentEnd, nodeEnd - segmentEnd)));
//					break;
//				}
//				n->advance(segmentEnd - n->get_start()); // include only the trailing region
//				n = n->get_prev();
//				if (!n)
//					break;
//				nodeEnd = n->get_end();
//			}
//
//			for (;;)
//			{
//				if (nodeEnd <= s.get_start())
//					break;
//
//				if (n->get_start() < s->get_start()) // if node is entirely within the segment, remove it.
//				{
//					n->m_segment.truncate_to(s->get_start() - n->get_start());
//					break;
//				}
//				ptr<node> prev = n->get_prev();
//				m_list.remove(n);
//				n = prev;
//				if (!n)
//					break;
//				nodeEnd = n->get_end();
//			}
//			break;
//		}
//	}
//
//	void remove(const segment_map<file_size_t>& sm)
//	{
//		iterator itor = sm.get_first();
//		while (!!itor)
//		{
//			remove(*itor);
//			++itor;
//		}
//	}
//
//	void combine(this_t& src) // Only use on known non-overlapping.  Removes all from src.
//	{
//		for (;;)
//		{
//			ptr<node> n = src.m_list.get_first();
//			if (!n)
//				break;
//			src.m_list.remove(n);
//			m_list.insert(n);
//		}
//	}
//	
//	bool does_overlap(const segment<file_size_t>& s) const
//	{
//		ptr<node> n = m_list.find_nearest_less_than(s.get_end());
//		// If nothing starts before the end of this segment, then there is no overlap.
//		// Otherwise, it overlaps if it ends after we start.
//		return (!!n) && (s.m_start < n->get_end());
//	}
//
//	bool does_overlap(const this_t& sbm) const
//	{
//		// The shorter list should iterate through all blocks.
//		// The longer list should be doing binary lookups, in inner loop.
//		if (m_count > sbm.size())
//			return sbm.does_overlap(*this);
//
//		ptr<node> n = m_list.get_first();
//		while (!!n)
//		{
//			if (sbm.does_overlap(n->get_segment()))
//				return true;
//			n = node::get_next(n);
//		}
//		return false;
//	}
//
//	bool does_overlap(const segment_map<file_size_t>& sm) const
//	{
//		// The shorter list should iterate through all blocks.
//		// The longer list should be doing binary lookups, in inner loop.
//		if (m_count > sm.size())
//			return sm.does_overlap(*this);
//
//		ptr<node> n = m_list.get_first();
//		while (!!n)
//		{
//			if (sm.does_overlap(n->get_segment()))
//				return true;
//			n = node::get_next(n);
//		}
//		return false;
//	}
//
//private:
//	// On entry, the iterators must point to overlapping segments.  Both iterators
//	// will be advanced as needed until the segments they point to no longer overlap.
//	void read_overlap_inner(segment_map<file_size_t>& segmentMap, iterator& segmentBufferMapItor, typename segment_map<file_size_t>::iterator& segmentMapItor)
//	{
//		bool isSegmentSplit = false;
//		segment_buffer<file_size_t> curSegmentBuffer = *segmentBufferMapItor; // copied, modified.
//		segment<file_size_t> curSegment = *segmentMapItor; // copied, modified.
//		
//		for (;;)
//		{
//			if (curSegmentBuffer.get_start() < curSegment.get_start()) // Ignore any segmentBuffer before segment.
//				curSegmentBuffer.advance((size_t)(curSegment.get_start() - curSegmentBuffer.get_start()));
//			else if (curSegment.get_start() < curSegmentBuffer.get_start()) // Split off unmatched segment start
//			{
//				file_size_t dif = curSegmentBuffer.get_start() - curSegment.get_start();
//				if (!isSegmentSplit) // if curSegment is in segmentMap
//				{
//					curSegment = segmentMapItor->split_off_after(dif); // if in the map, skip past unused portion, leaving it in the map.
//					isSegmentSplit = true; // indicates that curSegment is not currently reflected in segmentMap.
//				}
//				else //if (!!isSegmentSplit) // if curSegment is not in segmentMap
//				{
//					segmentMap.add(curSegment.get_start(), dif); // Add unused portion back into the map
//					curSegment.advance(dif);
//				}
//			}
//			// At this point, both start at the same position.
//
//			// Cache next segmentMapItor, so we can still advance after removing the currrent one.
//			segment_map<file_size_t>::iterator nextSegmentMapItor = segmentMapItor.next();
//
//			file_size_t curSegmentEnd = curSegment.get_end(); // cache end positions
//			file_size_t curSegmentBufferEnd = curSegmentBuffer.get_end(); // cache end positions
//			if (curSegmentEnd < curSegmentBufferEnd) // If segment is done before the end of this segmentBuffer,
//			{ // Add matched segment
//				add(curSegmentBuffer.split_off_before((size_t)(curSegmentEnd - curSegmentBuffer.get_start()))); // curSegmentBuffer retains next buffer to match.
//				if (!isSegmentSplit) // Remove segment.  if isSegmentSplit, then no need to remove it since we haven't added it.
//					segmentMap.remove(segmentMapItor);
//
//				// We have left-over contents in curSegmentBuffer.
//				segmentMapItor = nextSegmentMapItor; // Move on to next segment.
//				if (!segmentMapItor)
//					break; // No more segments, we're done.
//				
//				curSegment = *segmentMapItor;
//				if (curSegment.get_start() >= curSegmentBuffer.get_end()) // If starts past buffer, we're done with this buffer
//				{
//					curSegmentBuffer = *++segmentBufferMapItor;
//					if (!curSegmentBuffer.get_segment().does_overlap(curSegment))
//						break;
//				}
//				isSegmentSplit = false; // Otherwise, we take this new curSegment back to the start of the loop.
//				//continue;
//			}
//			else // if (curSegmentBufferEnd <= curSegmentEnd)
//			{
//				add(curSegmentBuffer);
//				if (curSegmentBufferEnd == curSegmentEnd)
//				{
//					if (!isSegmentSplit) // Entirely remove segment.  if isSegmentSplit, then no need to remove it since we haven't added it.
//						segmentMap.remove(segmentMapItor);
//					curSegment = *(segmentMapItor = nextSegmentMapItor); // Move on to next segment.
//					curSegmentBuffer = *++segmentBufferMapItor; // Move on to next segment buffer.
//					if (!curSegmentBuffer.get_segment().does_overlap(curSegment))
//						break;
//					isSegmentSplit = false;
//				}
//				else // if (curSegmentBufferEnd < curSegmentEnd) // We have left over contents in curSegment
//				{
//					if (!isSegmentSplit)
//						segmentMapItor->advance(curSegmentBufferEnd - segmentMapItor->get_start());
//					curSegment.advance(curSegmentBufferEnd - curSegment.get_start()); // curSegment retains segment to match
//					curSegmentBuffer = *++segmentBufferMapItor; // Move on to next segment buffer.
//					if (curSegment.get_end() <= curSegmentBuffer.get_start())
//					{
//						if (isSegmentSplit)
//							segmentMap.add(curSegment);
//						curSegment = *(segmentMapItor = nextSegmentMapItor); // Move on to next segment.
//						if (!curSegmentBuffer.get_segment().does_overlap(curSegment))
//							break;
//						isSegmentSplit = false;
//					}
//					// else //continue;
//				}
//				//continue; // Otherwise, we take this new curSegmentBuffer back to the start of the loop.
//			}
//		}
//	}
//	
//public:
//	// read_overlap modifies srcDst.  All segment buffers successfully read from sbm will be removed
//	// from srcDst.  Any segments that were not found, remain in srcDst.  sbm will not be modified.
//	void read_overlap(const this_t& sbm, segment_map<file_size_t>& srcDst)
//	{
//		if (srcDst.is_empty() || sbm.is_empty())
//			return;
//		
//		iterator segmentBufferMapItor;
//		typename segment_map<file_size_t>::iterator segmentMapItor;
//
//		if ((srcDst.get_last()->get_end() < sbm.get_first()->get_start())
//			|| (sbm.get_last()->get_end() < srcDst.get_first()->get_start()))
//			return; // If either is empty, or one starts after the other ends, don't bother.
//
//		if (srcDst.size() <= sbm.size())
//		{ // srcDst is smaller, iterate through it looking for overlaps in sbm.
//			segmentMapItor = srcDst.get_first();
//			do {
//				bool foundAny = false;
//				file_size_t segmentStartPos = segmentMapItor->get_start();
//				segmentBufferMapItor = sbm.find_equal_or_nearest_before(segmentStartPos);
//				if (!segmentBufferMapItor) // if none found, check the first
//				{
//					segmentBufferMapItor = sbm.get_first();
//					if (segmentBufferMapItor->get_end() <= segmentMapItor->get_end())
//						foundAny = true;
//				}
//				else // if (!!segmentBufferMapItor) // found one that starts before or at the start position
//				{
//					if ((segmentBufferMapItor->get_end() <= segmentStartPos)
//						&& (!++segmentBufferMapItor))
//						break;
//					if (segmentBufferMapItor->get_start() < segmentMapItor->get_end())
//						foundAny = true;
//				}
//				if (!foundAny)
//					++segmentMapItor;
//				else
//				{
//					read_overlap_inner(srcDst, segmentBufferMapItor, segmentMapItor);
//					if (!segmentBufferMapItor)
//						break;
//				}
//			} while (!!segmentMapItor);
//		}
//		else // sbm is smaller, iterate through it looking for overlaps in srcDst.
//		{
//			segmentBufferMapItor = sbm.get_first();
//			do {
//				bool foundAny = false;
//				file_size_t segmentBufferStartPos = segmentBufferMapItor->get_start();
//				segmentMapItor = srcDst.find_equal_or_nearest_before(segmentBufferStartPos);
//				if (!segmentMapItor) // if none found, check the first
//				{
//					segmentMapItor = srcDst.get_first();
//					if (segmentMapItor->get_end() <= segmentBufferMapItor->get_end())
//						foundAny = true;
//				}
//				else // if (!!segmentMapItor) // found one that starts before or at the start position
//				{
//					if ((segmentMapItor->get_end() <= segmentBufferStartPos)
//						&& (!++segmentMapItor))
//						break;
//					if (segmentMapItor->get_start() < segmentBufferMapItor->get_end())
//						foundAny = true;
//				}
//				if (!foundAny)
//					++segmentBufferMapItor;
//				else
//				{
//					read_overlap_inner(srcDst, segmentBufferMapItor, segmentMapItor);
//					if (!segmentMapItor)
//						break;
//				}
//			} while (!!segmentBufferMapItor);
//		}
//	}
//
//	iterator get_first() const { return iterator(m_list.get_first()); }
//	iterator get_last() const { return iterator(m_list.get_last()); }
//
//	iterator find(const file_size_t& n) const { return iterator(m_list.find_any_equal(n)); }
//	iterator find_any_before(const file_size_t& n) const { return iterator(m_list.find_any_less_than(n)); }
//	iterator find_any_after(const file_size_t& n) const { return iterator(m_list.find_any_greater_than(n)); }
//	iterator find_equal_or_any_before(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_less_than(n)); }
//	iterator find_equal_or_any_after(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_greater_than(n)); }
//	iterator find_nearest_before(const file_size_t& n) const { return iterator(m_list.find_nearest_less_than(n)); }
//	iterator find_nearest_after(const file_size_t& n) const { return iterator(m_list.find_nearest_greater_than(n)); }
//	iterator find_equal_or_nearest_before(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_nearest_less_than(n)); }
//	iterator find_equal_or_nearest_after(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_nearest_greater_than(n)); }
//};
//
//template <typename file_size_t>
//inline bool segment_map<file_size_t>::does_overlap(const segment_buffer_map<file_size_t>& sbm) const
//{
//	// The shorter list should iterate through all blocks.
//	// The longer list should be doing binary lookups, in inner loop.
//	if (m_count > sbm.size())
//		return sbm.does_overlap(*this);
//
//	ptr<node> n = m_list.get_first();
//	while (!!n)
//	{
//		if (sbm.does_overlap(n->m_segment))
//			return true;
//		n = node::get_next(n);
//	}
//	return false;
//}
//
//template <typename file_size_t>
//class file_mask
//{
//private:
//	// Note: If a locking_transaction needs to shrink a file, its mask must include
//	// the area between the new EOF, the old EOF, as well as past the EOF.
//
//	typedef file_mask<file_size_t> this_t;
//
//	file_mask(const this_t&);
//	this_t& operator=(const this_t&);
//	
//	segment_map<file_size_t> m_segments;
//	bool m_pastEof; // true if mask incldes everything past the (unknown) EOF position
//
//	explicit file_mask(bool spanPastEof)
//		: m_pastEof(spanPastEof)
//	{ }
//
//public:
//	file_mask()
//		: m_pastEof(false)
//	{ }
//
//	static this_t all(bool spanPastEof = true) { this_t result(true);  result.m_segments.add(0, -1); return result; }
//	static this_t eof() { return this_t(true); }
//	
//	void clear()
//	{
//		m_pastEof = false;
//		m_segments.clear();
//	}
//
//	void set_to_eof()
//	{
//		m_pastEof = true;
//		m_segments.clear();
//	}
//
//	void set_to_all()
//	{
//		m_pastEof = true;
//		m_segments.clear();
//		m_segments.add(0, -1);
//	}
//
//	void include(const segment<file_size_t>& s) { m_segments.add(s); }
//	void exclude(const segment<file_size_t>& s) { m_segments.add(s); }
//		
//	void include(const segment_map<file_size_t>& sm)
//	{
//		segment_map<file_size_t>::iterator itor = sm.get_first();
//		while (!!itor)
//		{
//			m_segments.add(*itor);
//			++itor;
//		}
//	}
//
//	void exclude(const segment_map<file_size_t>& sm)
//	{
//		segment_map<file_size_t>::iterator itor = sm.get_first();
//		while (!!itor)
//		{
//			m_segments.remove(*itor);
//			++itor;
//		}
//	}
//
//	void include_past_eof(bool includePastEof = true) { m_pastEof = includePastEof; }
//	void exclude_past_eof() { m_pastEof = false; }
//
//	// does_overlap() assumes both masks are of the same file.
//	bool does_overlap(const this_t& fm, const file_size_t& currentEof) const
//	{
//		if (!!m_pastEof)
//		{
//			if (!!fm.m_pastEof)
//				return true;
//			segment_map<file_size_t>::iterator lastItor = fm.m_segments.get_last();
//			if (!lastItor)
//				return false; // no overlap if nothing is there.
//			if (lastItor->get_end() > currentEof)
//				return true;
//		}
//		else if (!!fm.m_pastEof)
//		{
//			segment_map<file_size_t>::iterator lastItor = m_segments.get_last();
//			if (!lastItor)
//				return false; // no overlap if nothing is there.
//			if (lastItor->get_end() > currentEof)
//				return true;
//		}
//		return m_segments.does_overlap(fm.m_segments);
//	}
//};
//
//template <typename file_size_t>
//class segment_state_map_node : public sorted_list_node<true, segment_state_map_node<file_size_t> >
//{
//private:
//	segment<file_size_t> m_segment;
//
//public:
//	segment_state_map_node(const segment<file_size_t>& s)
//		: m_segment(s)
//	{ }
//
//	segment_state_map_node(const segment_state_map_node<file_size_t>& s, const file_size_t& i)
//		: m_segment(s.m_segment.split_off_after(i))
//	{ }
//
//	const segment<file_size_t>& get_segment() const { return m_segment; }
//
//	const file_size_t& get_key() const { return m_segment.get_start(); }
//	const file_size_t& get_start() const { return m_segment.get_start(); }
//	const file_size_t& get_length() const { return m_segment.get_length(); }
//	const file_size_t  get_end() const { return m_segment.get_end(); }
//};
//
//template <typename dereived_segment_state_map_node_t, typename file_size_t>
//class segment_state_map
//{
//public:
//	typedef dereived_segment_state_map_node_t node;
//
//	segment_state_map(const segment_state_map&);
//	segment_state_map& operator=(const segment_state_map&);
//		
//	typedef sorted_list<file_size_t, true, node> list_t;
//
//	list_t m_list;
//
//	void clear_inner()
//	{
//		ptr<node> n = m_list.get_first_postorder();
//		while (!!n)
//		{
//			ptr<node> n2 = node::get_next_postorder(n);
//			default_allocator::destruct_deallocate_type(n.get());
//			n = n2;
//		}
//	}
//
//	node* split_off_after(node& n, file_size_t i)
//	{
//		node* newNode = new (default_allocator::get()) node(n, i);
//		m_list.insert(newNode);
//		return newNode;
//	}
//
//public:
//	// An iterator will remain valid until the referenced node has been removed.
//	class iterator
//	{
//	private:
//		ptr<node> m_node;
//
//	protected:
//		iterator(const ptr<node>& n) : m_node(n) { }
//
//		friend class segment_buffer_map<file_size_t>;
//
//	public:
//		iterator() { }
//
//		iterator(const iterator& i) : m_node(i.m_node) { }
//
//		iterator& operator++() { if (!!m_node) m_node = node::get_next(m_node); return *this; }
//		iterator& operator--() { if (!!m_node) m_node = node::get_prev(m_node); return *this; }
//
//		bool operator!() const { return !m_node; }
//
//		bool operator==(const iterator& i) const { return m_node == i.m_node; }
//		bool operator!=(const iterator& i) const { return m_node != i.m_node; }
//
//		node* get() const { return m_node.get(); }
//		node& operator*() const { return *m_node; }
//		node* operator->() const { return m_node.get(); }
//			
//		void clear() { m_node = 0; }
//
//		iterator& operator=(const iterator& i) { m_node = i.m_node; return *this; }
//
//		iterator next() const { return iterator((!m_node) ? ptr<node>(0) : node::get_next(m_node)); }
//		iterator prev() const { return iterator((!m_node) ? ptr<node>(0) : node::get_prev(m_node)); }
//	};
//
//	segment_state_map()
//	{ }
//
//	~segment_state_map() { clear_inner(); }
//
//	void clear()
//	{
//		clear_inner();
//		m_list.clear();
//	}
//
//	bool is_empty() const { return !m_list; }
//	bool operator!() const { return !m_list; }
//
//	// split_create() ensures there are states encompassing the entirety of the specified segment.
//	// If there are any gaps, there are filled in with new nodes and default states.
//	// If any blocks are found that span the borders, they are split such that the start and end
//	// positions of the specified segment will corespond with the start and end of blocks.
//	iterator split_create(const segment<file_size_t>& s)
//	{
//		file_size_t segmentEnd = s.get_end();
//		node* n = m_list.find_nearest_less_than(segmentEnd);
//		file_size_t nodeEnd;
//		if (!!n)
//			nodeEnd = n->get_end();
//		if ((!n) || (nodeEnd <= s.get_start()))
//		{
//			n = new (default_allocator::get()) node(s);
//			m_list.insert(n);
//		}
//		else // Node ends after the segment starts
//		{
//			if (nodeEnd < segmentEnd) // If extending past the new range, create a new block to bridge the gap
//				m_list.insert(new (default_allocator::get()) node(segment<file_size_t>(nodeEnd, segmentEnd - nodeEnd)));
//			else if (segmentEnd < nodeEnd) // If splitting the block at the end
//				split_off_after(*n, segmentEnd - n->get_start());
//
//			if (n->get_start() < s.get_start()) // trim leading, then done.
//				n = split_off_after(*n, s.get_start() - n->get_start());
//			else if (s.get_start() < n->get_start()) // block starts before curSubrange.  We need to look for prev blocks.
//			{
//				for (;;) // just to use break/continue as goto labels
//				{
//					node* prev = node::get_prev(n);
//					if ((!prev) || (prev->get_end() <= s.get_start())) // Nothing prior within this segment, create 1 new segment
//					{
//						n = new (default_allocator::get()) node(segment<file_size_t>(s.get_start(), n->get_start()));
//						m_list.insert(n);
//						break;
//					}
//					//else // (s.get_start() < prev->m_end) // If prev ends within our block
//					if (prev->get_end() < n->get_start()) // If prev ends before subsequent block starts, fill the gap
//						m_list.insert(new (default_allocator::get()) node(segment<file_size_t>(prev->get_end(), n->get_start())));
//					n = prev;
//					//continue;
//				}
//			}
//		}
//		return iterator(n);
//	}
//
//	void remove(iterator& itor)
//	{
//		m_list.remove(itor.m_node);
//		default_allocator::destruct_deallocate_type(itor.m_node.get());
//		itor.m_node.clear();
//	}
//};
//
//template <typename file_size_t>
//class file<read_access, file_size_t>
//{
//public:
//	typedef typename file<read_access, file_size_t> this_t;
//	typedef typename smaller_type<file_size_t, size_t>::type buffer_size_t;
//
//	class reader : public waitable
//	{
//	private:
//		reader(const reader&);
//		reader& operator=(const reader&);
//
//		event m_event;
//		rcref<segment_map<file_size_t> > m_unreadSegments; // Takes ownership of map passed in.
//		rcref<segment_buffer_map<file_size_t> > m_readSegmentBuffers; // Starts out empty
//
//	protected:
//		reader(const rcref<segment_map<file_size_t> >& sm)
//			: m_unreadSegments(sm),
//			m_readSegmentBuffers(rcnew(segment_buffer_map<file_size_t>))
//		{
//			self_acquire();
//		}
//
//		void complete() { m_event.set(); self_release(); }
//
//	public:
//		const rcref<segment_map<file_size_t> >& get_unread_segments() const { return m_unreadSegments; }
//		const rcref<segment_buffer_map<file_size_t> >& get_buffers() const { return m_readSegmentBuffers; }
//
//		typedef delegate_t<void, const rcref<const reader>&> dispatch_t;
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_event.timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d, size_t n = 1) const volatile { m_event.dispatch(d, n); }
//		void dispatch(const dispatch_t& d, size_t n = 1) const { m_event.dispatch(delegate(d, this_rcref)); }
//	};
//
//	rcref<reader> read(file_size_t offset, buffer_size_t n) { return begin_read(rcnew(segment_map<file_size_t>, segment<file_size_t>(offset, n))); }
//	rcref<reader> read(const segment<file_size_t>& s) { return begin_read(rcnew(segment_map<file_size_t>, s)); }
//	rcref<reader> read(const rcref<segment_map<file_size_t> >& sm) { return begin_read(sm); }
//
//	class size_reader : public waitable
//	{
//	private:
//		size_reader(const size_reader&);
//		size_reader& operator=(const size_reader&);
//
//		event m_event;
//		file_size_t m_size;
//
//	protected:
//		explicit size_reader(file_size_t sz = 0)
//			: m_size(sz)
//		{
//			self_acquire();
//		}
//		
//		void complete() { m_event.set(); self_release(); }
//
//		void set_size(const file_size_t& sz) { m_size = sz; }
//
//	public:
//		const file_size_t& get_size() const { return m_size; }
//
//		typedef delegate_t<void, const rcref<const size_reader>&> dispatch_t;
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_event.timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d, size_t n = 1) const volatile { m_event.dispatch(d, n); }
//		void dispatch(const dispatch_t& d, size_t n = 1) const { m_event.dispatch(delegate(d, this_rcref), n); }
//	};
//
//	virtual rcref<size_reader> get_size() = 0;
//
//protected:
// 	virtual rcref<reader> begin_read(const rcref<segment_map<file_size_t> >& sm) = 0;
//};
//
//template <typename file_size_t>
//class file<write_access, file_size_t>
//{
//public:
//	class writer : public waitable
//	{
//	private:
//		writer(const writer&);
//		writer& operator=(const writer&);
//
//		event m_event;
//		rcref<segment_buffer_map<file_size_t> > m_unwrittenBuffers; // Takes ownership of map passed in.
//
//	protected:
//		writer(const rcref<segment_buffer_map<file_size_t> >& sbm)
//			: m_unwrittenBuffers(sbm)
//		{
//			self_acquire();
//		}
//
//		void complete() { m_event.set(); self_release(); }
//
//	public:
//		const rcref<segment_buffer_map<file_size_t> >& get_unwritten_buffers() const { return m_unwrittenBuffers; }
//
//		typedef delegate_t<void, const rcref<const writer>&> dispatch_t;
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_event.timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d, size_t n = 1 const volatile { m_event.dispatch(d, n); }
//		void dispatch(const dispatch_t& d, size_t n = 1) const { m_event.dispatch(delegate(d, this_rcref), n); }
//	};
//	
//	rcref<writer> write(file_size_t offset, void* b, size_t n) { return begin_write(rcnew(segment_buffer_map, offset, const_buffer::contain(b, n))); }
//	rcref<writer> write(file_size_t offset, const const_buffer& b) { return begin_write(rcnew(segment_buffer_map, offset, b)); }
//	rcref<writer> write(file_size_t offset, const composite_buffer& b) { return begin_write(rcnew(segment_buffer_map, offset, b)); }
//	rcref<writer> write(const rcref<segment_buffer_map<file_size_t> >& sbm) { return begin_write(sbm); }
//	
//	class size_writer : public waitable
//	{
//	private:
//		size_writer(const size_writer&);
//		size_writer& operator=(const size_writer&);
//
//		event m_event;
//		file_size_t m_size;
//
//	protected:
//		size_writer(file_size_t sz)
//			: m_size(sz)
//		{
//			self_acquire();
//		}
//
//		void complete() { m_event.set(); self_release(); }
//
//	public:
//		const file_size_t& get_size() const { return m_size; }
//
//		typedef delegate_t<void, const rcref<const size_writer>&> dispatch_t;
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_event.timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d, size_t n = 1) const volatile { m_event.dispatch(d, n); }
//		void dispatch(const dispatch_t& d, size_t n = 1) const { m_event.dispatch(delegate(d, this_rcref), n); }
//	};
//	
//	// May fail if the underlying file implementation does not support resizing.
//	virtual rcref<size_writer> set_size(const file_size_t& sz) = 0;
//	
//protected:
//	virtual rcref<writer> begin_write(const rcref<segment_buffer_map<file_size_t> >& sbm) = 0;
//};
//
//template <typename file_size_t>
//class file<read_write_access, file_size_t> : public file<read_access, file_size_t>, public file<write_access, file_size_t>
//{
//public:
//	virtual rcref<size_reader> get_size() = 0;
//	virtual rcref<size_writer> set_size(const file_size_t& sz) = 0;
//
//protected:
//	virtual rcref<reader> begin_read(const rcref<segment_map<file_size_t> >& sm) = 0;
//	virtual rcref<writer> begin_write(const rcref<segment_buffer_map<file_size_t> >& sbm) = 0;
//};
//
//
//// synchronized_file_impl<> is an internal class that implements most of the functionality provided by
//// synchronized_file<>.
//template <typename file_size_t>
//class synchronized_file_impl : public object
//{
//public:
//	typedef file_size_t file_size_t;
//	typedef synchronized_file_impl<file_size_t> this_t;
//
//private:
//	synchronized_file_impl(const this_t&) = delete;
//	this_t& operator=(const this_t&) = delete;
//	
//	typedef size_t transaction_id;
//
//	// Some terms for clarity:
//	//
//	//	queued		-	IO operations can be queued to a transaction.  These IO operations are not submitted,
//	//					until the transaction is submitted, and are not issued until the transaction is issued.
//	//
//	//	submitted	-	A transaction is submitted by the caller.  A submitted transaction may not yet actually,
//	//					have been issued, due to contention management.  Individual reads on locking
//	//					and failable transactions may also be considered to have submitted vs. issued state.
//	//
//	//	issued		-	A transaction is issued once it has progressed through contention management.  IO operations
//	//					associated with a transaction are issued when their transaction is issued.  Individual reads
//	//					on locking and failable transactions are also considered issued after progressing through
//	//					contention management.  Low level file IO may also be compromised of multiple sub IO operations
//	//					which are issued within the concurrency management layer.
//
//	//	transactions
//	//		composed transactions	- Queues reads and writes to be committed atomically.
//	//		locking transactions	- Submits reads but queus all writes to be committed atomically.
//	//									Effectively blocks writes outside of this transaction.
//	//		failable transactions	- Submits reads but queues all writes to be committed atomically.
//	//									Does not lock.  Aborted if read areas are changed before writes are commited.
//	//
//	//	concurrency management		- Facilitates multiple concurrent composed transactions.
//	//
//	//	contention management		- Facilitates locking transactions, potentially blocking operations from reaching concurrency management.
//	//
//	//		transaction id				- Used by contention management to resolve priority of multiple contentous operations.
//	//										Decrementally assigned int.  Higher id's have higher priority.
//	//										Proper comparison requires consideration of loop position.
//	//
//	//		may-write lock			- Range an unsubmitted locking transactions might write to, but does not yet contain data it will write.
//	//									Any read attempts made by a composed or locking transaction with a lower priority transaction_id will be blocked.
//	//
//	//		waiting-read			- Pending read from either a locking transaction or submitted composed transaction that is either blocked
//	//									on a may-write lock or is contained within a submitted composed transaction that is blocked elsewhere.
//	//									A waiting read will block writes from transactions with lower priority transaction_id's.
//	//
//	//		was-read lock			- Range read by an unsubmitted locking transaction.  Transactions with lower transaction_id's that attempt
//	//									to write to this range will be blocked.  (higher transaction_ids transaction are not possible, because
//	//									they would have indicated a may-write lock, which would result in a waiting-read instead of a was-read lock).
//	//
//	//		waiting-write			- Pending write from either a composed or locking transaction that is blocked on a waiting-read or was-read lock.
//	//									Read with higher priority transaction_ids, which are not more directly blocked on a may-write, may complete
//	//									using the contents of a waiting-write.
//	//
//	//	The progression of a composed transaction:
//	//
//	//	1. Create composed transaction
//	//	2. issue read and write IO operations using the transaction - IO operations are queued until the transaction is submitted
//	//	3. Submit the composed transaction - IO operations are submitted
//	//	4. contention management
//	//		4a. If a write overlaps a waiting-read or was-read lock OR if a read overlaps a may-write lock.
//	//			- allocate a transaction_id if none yet (gets lowest pri transaction_id)
//	//			- create waiting-write segments, for all writes made by this composed transaction.
//	//			- create waiting-read segments, for all reads made by this composed transaction.
//	//			- defer composed_transaction until all waiting-read and waiting-write segments are unblocked
//	//		4b. If a read overlaps a waiting-write, it may complete with the contents of the waiting-write.
//	//	5. Once unblocked by contention management, the transaction is issued (IO operations issued)
//	//	6. concurrency management
//	//		6a. sub IO operations issued to File IO subsystem, completed
//	//	7. IO operations completed / transaction completed
//	//
//	//
//	//	The progression of a locking transaction:
//	//
//	//	1. Create locking transaction
//	//		1a. Specify may-write lock, if any. Default is full-file may-write lock.
//	//	2. Write IO operations are queued
//	//		2a. clip to may-write lock
//	//	3. read IO operations submitted to cpontention management.
//	//	4. contention management:
//	//		4a. If first read, allocate transaction_id and apply may-write mask to contention state.
//	//		4b. Add read to contention state.
//	//		4c. If read overlaps write lock with lower transaction_id, defer read until that transaction is completed.
//	//		4d. Once unblocked, issue low level read
//	//	5. transaction submitted (IO operations submitted)
//	//		5a. Strip any may-write lock that wasn't actually written to, and release any newly unblocked transactions
//	//	6. contention management
//	//		6a. Once unblocked, remove transaction contention state and release any newly unblocked transactions.
//	//	7. transaction issued (IO operations issued)
//	//	8. concurrency management
//	//		8a. sub IO operations issued to File IO subsystem, completed
//	//	9. IO operations completed / transaction completed
//	//	
//	//	
//
//	//	IO operations queued
//	//	transaction submitted / IO operations submitted
//	//	contention management
//	//	transaction issued / IO operations issued
//	//	concurrency management
//	//	sub IO operations submitted to File IO subsystem
//	//	sub IO operations completed
//	//	IO operations completed / transaction completed
//
//	// A synchronized_file<> manages 2 distinct levels of synchronization; concurrency and contention.
//	//
//	// Concurrency	-	Concurrency refers to multiple things happening at the same time.  After being submitted,
//	//					transactions undergo concurrency management until all of their IO operations are complete.
//	//					By managing concurrent operations, a synchronized_file<> avoids redundant IO operations,
//	//					provides some basic caching, and adds parallelism.  A synchronized_file manages concurrency
//	//					by maintaining a map of all read/write operations currently in progress, and by serializing
//	//					reads and writes that overlap.  This is necessary to ensure the same data is not being
//	//					concurrently written to, or read from while written to, as both can have unpredictable
//	//					results.
//	//						
//	//						If a read operation is submitted against a range that has a (deferred or in-progress) read
//	//							operation immediately ahead of it, the read operations are coalesced such that only 1
//	//							low-level read operation is needed.
//	//
//	//						If a read operation is submitted against a range that has a (deferred or in-progress) write
//	//							operation immediately ahead of it, the buffer from that pending write operation is used
//	//							to immediately complete the read.
//	//
//	//						If a write operation is submitted against a range that has an in-progress read or write
//	//							operation immediately ahead of it (and no deferred writes), the write operation is
//	//							deferred until the in-progress operation is completed.
//	//
//	//						If a write operation is submitted against a range that already has deferred writes,
//	//							it is coalesced with the deferred writes, such that only 1 low-level write is required,
//	//							and uses the most recently provided buffer.
//	//
//	// Contention	-	Contention referred to multiple things attempting to interact with a resource that
//	//					does not support multiple concurrent interactions.  Contention is created when locking
//	//					transactions are used.  A synchronized_file manages contention by:
//	//
//	//						Assigning transaction_id's to locking transactions, which it uses to establish which
//	//							transaction has priority when there is a conflict.
//	//
//	//						Deferring reads from transactions of higher transaction_id when they overlap with
//	//							the write lock of a transaction with a lower transaction_id.
//	//
//	//						Deferring writes from transactions of higher transaction_id when they overlap with
//	//							reads from an incomplete transaction with a lower transaction_id.
//	//
//
//	class transaction;
//
//	class reader : public file<read_access, file_size_t>::reader
//	{
//	public:
//		transaction* m_transaction;
//	};
//
//	class writer : public file<write_access, file_size_t>::writer
//	{
//	public:
//		transaction* m_transaction;
//	};
//
//	class transaction
//	{
//	public:
//		segment_map m_wasReadMap;
//		segment_map m_mayWriteMap;
//		segment_buffer_map m_composedWrites;
//
//		reader* m_composedReaders;
//		writer* m_composedWriters; // all completed at once, when entire write buffer map is complete.
//	};
//
//	typedef map<transaction_id, transaction*, true, default_comparator, vector_index, ptr> transaction_map;
//	typedef map<transaction_id, reader*, true, default_comparator, vector_index, ptr> waiting_read_map;
//	typedef map<transaction_id, writer*, true, default_comparator, vector_index, ptr> waiting_write_map;
//
//	class contention_state : public segment_state_map_node<file_size_t>
//	{
//	public:
//		// Use allocator_vectors so entire maps can be duplicated in O(n) time, whenever a contention_state is split.
//		allocator_vector<sizeof(transaction_map::node)> m_wasReadLocksAllocator;
//		allocator_vector<sizeof(transaction_map::node)> m_mayWriteLocksAllocator;
//		allocator_vector<sizeof(waiting_read_map::node)> m_waitingReadsAllocator;
//		allocator_vector<sizeof(waiting_write_map::node)> m_waitingWritesAllocator;
//
//		transaction_map m_wasReadLocks;
//		transaction_map m_mayWriteLocks;
//		waiting_read_map m_waitingReads;
//		waiting_write_map m_waitingWrites;
//		
//		bool m_initialized;
//
//		contention_state()
//			: m_initialized(false)
//		{ }
//
//		contention_state(const contention_state& st, const file_size_t& i)
//			: segment_state_map_node<file_size_t>(st, i),
//			m_mayWriteLocksAllocator(st.m_mayWriteLocksAllocator),
//			m_waitingWritesAllocator(st.m_waitingWritesAllocator),
//			m_wasReadLocksAllocator(st.m_wasReadLocksAllocator),
//			m_waitingReadsAllocator(st.m_waitingReadsAllocator),
//			m_mayWriteLocks(st.m_mayWriteLocks, m_mayWriteLocksAllocator),
//			m_waitingWrites(st.m_waitingWrites, m_waitingWritesAllocator),
//			m_wasReadLocks(st.m_wasReadLocks, m_wasReadLocksAllocator),
//			m_waitingReads(st.m_waitingReads, m_waitingReadsAllocator),
//			m_initialized(true)
//		{ }
//	};
//
//	class concurrency_state : public segment_state_map_node<file_size_t>
//	{
//	public:
//		const_buffer m_buffer; // Either of write operation still in progress, or cached.
//
//		// All of the segments in single inner-read are linked together in a thread.
//		// This makes it possible for the completed read to iterate directly through
//		// all concurrency_state's that need to receive read data, without incurring any
//		// additional tree lookups.  As node is split, the link is divide, but both
//		// are kept in the list.
//		ptr<concurrency_state> m_nextInReadThread;
//		ptr<concurrency_state>& get_next_in_read_thread() { return m_nextInReadThread; }
//
//		// All of the segments in single inner-write are linked together in a thread.
//		// This makes it possible for the completed write to iterate directly through
//		// all concurrency_state's that need to receive read data, without incurring any
//		// additional tree lookups.  As node is split, the link is divide, but both
//		// are kept in the list.
//		ptr<concurrency_state> m_nextInWriteThread;
//		ptr<concurrency_state>& get_next_in_write_thread() { return m_nextInWriteThread; }
//
//		ptr<waiting_read_segment> m_firstWaitingReadSegment;
//
//		// When a write is deferred due to a read or write already in progress, a
//		// waiting_write_segment is added to the m_firstWaitingWriteSegment.  Each
//		// new write updates the buffer to be written.  When the original operation
//		// is complete, these waiting_write_segment's are moved into m_firstWaitingIssuedWriteSegment
//		// for the duration of the write operation associated with them.
//		ptr<waiting_write_segment> m_firstWaitingWriteSegment;
//		ptr<waiting_write_segment> m_firstWaitingIssuedWriteSegment;
//
//		bool m_initialized; // If true, a read or write is in progress.
//
//		concurrency_state()
//			: m_initialized(false)
//		{ }
//
//		concurrency_state(const concurrency_state& st, const file_size_t& i)
//			: segment_state_map_node<file_size_t>(st, i),
//			m_buffer(st.m_buffer.split_off_after((size_t)i)),
//			m_nextInReadThread(st.m_nextInReadThread),
//			m_nextInWriteThread(st.m_nextInWriteThread),
//			m_firstWaitingReadSegment(st.m_firstWaitingReadSegment),
//			m_firstWaitingWriteSegment(st.m_firstWaitingWriteSegment),
//			m_firstWaitingIssuedWriteSegment(st.m_firstWaitingIssuedWriteSegment),
//			m_initialized(true)
//		{
//			if (!!m_firstWaitingReadSegment)
//				++(m_firstWaitingReadSegment->m_refCount);
//			if (!!m_firstWaitingWriteSegment)
//				++(m_firstWaitingWriteSegment->m_refCount);
//			if (!!m_firstWaitingIssuedWriteSegment)
//				++(m_firstWaitingIssuedWriteSegment->m_refCount);
//			st.m_nextInReadThread = this;
//			st.m_nextInWriteThread = this;
//		}
//	};
//
//	segment_state_map< contention_state, file_size_t> m_contentionMap;
//	segment_state_map<concurrency_state, file_size_t> m_concurrencyMap;
//
//	file_size_t m_eof;
//
//	void initialize_locking_transaction(transaction& t, const file_mask& mayWriteMask)
//	{
//
//	}
//
//	void initialize_composed_transaction(transaction& t)
//	{
//	}
//
//	void commit_locking_reader(transaction& t, reader& r)
//	{
//		;
//	}
//
//	void commit_transaction(transaction& t)
//	{
//		// If this is a composed-transaction, there is nothing in the contention_state yet.
//		//		If there are no collisions with the contention-state, apply directly to concurrency-state.
//		//		If there are collisions, apply waiting-writes and waiting-reads to contention-state????
//
//		// If this is a locking-transaction, was-read segments will be present in contention_state.  waiting-reads may also be present.
//		//		If waiting-reads are present, 
//
//	}
//
//public:
//
//};
//
///*
//enum class transaction_submit_type
//{
//	auto_submit_transaction = 0,
//	manual_submit_transaction = 1
//};
//
//enum class file_transaction_type
//{
//	composed_transaction = 0,
//	locking_transaction = 1,
//	failable_transaction = 2
//};
//
//
//// synchronized_file_impl<> is an internal class that implements most of the functionality provided by
//// synchronized_file<>.
//template <typename file_size_t>
//class synchronized_file_impl : public object
//{
//protected:
//	typedef file_size_t file_size_t;
//	typedef synchronized_file_impl<file_size_t> this_t;
//
//	class transaction_internals;
//	class reader;
//	class writer;
//
//	typedef delegate_t<void, const rcref<reader>&> reader_arg_delegate_t;
//	typedef delegate_t<void, const rcref<writer>&> writer_arg_delegate_t;
//	typedef delegate_t<void, const rcref<transaction_internals>&> transaction_arg_delegate_t;
//
//	friend class synchronized_file<read_access>;
//	friend class synchronized_file<write_access>;
//	friend class synchronized_file<read_write_access>;
//
//private:
//	synchronized_file_impl(const this_t&);
//	this_t& operator=(const this_t&);
//
//	// Segment state and read operations.  (See: class synchronized_file_impl<>::reader)
//	//
//	// Some segments of a read may overlap with reads already in progress.  Those segments are
//	// removed from the 'unread segments' map and moved to a list of 'waiting segments'.
//	// As data arrives on those reads, they are added to the reader, and the waiting
//	// segment released.
//	class waiting_read_segment
//	{
//	public:
//		reader& m_reader;
//		size_t m_refCount;
//
//		ptr<waiting_read_segment> m_nextWaitingReadSegment;
//
//		waiting_read_segment(reader& r)
//			: m_reader(r),
//			m_refCount(0)
//		{ }
//	};
//
//	// When a write is commited, all 'unwritten segment buffers' are added to the state_map.
//	//
//	// For segments of the write that do not overlap existing reads and writes, their concurrency_state's
//	// are threaded together.  This is to to allow them to be removed quickly when the primary
//	// inner write operation completes.
//	//
//	// Any segments of a write that does not overlap with reads or writes that are already in progress
//	// are removed from the 'unwritten segment buffers' map.  This removes them from the primary
//	// write operation.  waiting_write_segment's are allocated and added to a list on each
//	// concurrency_state to tracking all writers that are waiting for the segment's operation to complete.
//	// The segment buffers will be written later, within the contexts of the completing reads and/or
//	// writes that had been blocking them.  If a write were to fail, the unwritten buffers are
//	// restored to the 'unwritten segment buffers' map (after the primary inner write completes).
//	class waiting_write_segment
//	{
//	public:
//		writer& m_writer;
//		size_t m_refCount;
//
//		ptr<waiting_write_segment> m_nextWaitingWriteSegment;
//
//		waiting_write_segment(writer& w)
//			: m_writer(w),
//			m_refCount(0)
//		{ }
//	};
//	
//	class concurrency_state : public segment_state_map_node<file_size_t>
//	{
//	public:
////		segment<file_size_t> m_segment;
//		const_buffer m_buffer; // Either of write operation still in progress, or cached.
//
//		ptr<waiting_read_segment> m_firstWaitingReadSegment;
//
//		// When a write is deferred due to a read or write already in progress, a
//		// waiting_write_segment is added to the m_firstWaitingWriteSegment.  Each
//		// new write updates the buffer to be written.  When the original operation
//		// is complete, these waiting_write_segment's are moved into m_firstWaitingIssuedWriteSegment
//		// for the duration of the write operation associated with them.
//		ptr<waiting_write_segment> m_firstWaitingWriteSegment;
//		ptr<waiting_write_segment> m_firstWaitingIssuedWriteSegment;
//
//		// All of the segments in single inner-read are linked together in a thread.
//		// This makes it possible for the completed read to iterate directly through
//		// all concurrency_state's that need to receive read data, without incurring any
//		// additional tree lookups.  As node is split, the link is divide, but both
//		// are kept in the list.
//		ptr<concurrency_state> m_nextInReadThread;
//		ptr<concurrency_state>& get_next_in_read_thread() { return m_nextInReadThread; }
//
//		// All of the segments in single inner-write are linked together in a thread.
//		// This makes it possible for the completed write to iterate directly through
//		// all concurrency_state's that need to receive read data, without incurring any
//		// additional tree lookups.  As node is split, the link is divide, but both
//		// are kept in the list.
//		ptr<concurrency_state> m_nextInWriteThread;
//		ptr<concurrency_state>& get_next_in_write_thread() { return m_nextInWriteThread; }
//
//		bool m_initialized; // If true, a read or write is in progress.
//		rcptr<reader> m_outerReader; // Set if this is the first node associated with an inner reader
//		rcptr<writer> m_outerWriter; // Set if this is the first node associated with an inner writer
//
//		concurrency_state()
//			: m_initialized(false)
//		{ }
//
//		concurrency_state(const concurrency_state& st, const file_size_t& i)
//			: segment_state_map_node<file_size_t>(st, i),
//			m_buffer(st.m_buffer.split_off_after((size_t)i)),
//			m_firstWaitingReadSegment(st.m_firstWaitingReadSegment),
//			m_firstWaitingWriteSegment(st.m_firstWaitingWriteSegment),
//			m_firstWaitingIssuedWriteSegment(st.m_firstWaitingIssuedWriteSegment),
//			m_nextInReadThread(st.m_nextInReadThread),
//			m_nextInWriteThread(st.m_nextInWriteThread),
//			m_initialized(true)
//		{
//			if (!!m_firstWaitingReadSegment)
//				++(m_firstWaitingReadSegment->m_refCount);
//			if (!!m_firstWaitingWriteSegment)
//				++(m_firstWaitingWriteSegment->m_refCount);
//			if (!!m_firstWaitingIssuedWriteSegment)
//				++(m_firstWaitingIssuedWriteSegment->m_refCount);
//			st.m_nextInReadThread = this;
//			st.m_nextInWriteThread = this;
//		}
//	};
//
//	typedef size_t transaction_id;
//
//	class contention_state : public segment_state_map_node<file_size_t>
//	{
//	public:
//		// A locking_transaction does not initialize until it's first read operation is attempted.
//		// (A locking_transaction that does not issue any reads is treated the same as a composed_transaction).
//		
//		// As a locking transaction is initialized, it is assigned a transaction_id.  Increasing values are
//		// assigned for transaction_ids, so can be used to determine transaction order.
//		// (transaction_id's may loop back to 0, so the next unassigned transaction_id must be considered for
//		// accurate comparisons of 2 assigned transaction_ids.  Also, maps of transaction_id should be
//		// considered circular.  When traversing beyond the end of a transaction_id map, the start of the
//		// map should also be visited.  Transactions should be short-lived.  It's not valid for a locking_transaction
//		// to remain valid so long that assigned transaction_ids loop past it.)
//
//		// On creation, a locking_transaction must specify the areas it may write to.  Writes
//		// outside of these areas could cause inconsistencies.  Specifying the entire file is valid, but will
//		// block all reads issued by subsequent locking_transactions.
//
//		typedef map<transaction_id, locking_transaction, true, default_comparator, vector_index, ptr> transaction_map;
//
//		allocator_vector<sizeof(transaction_map::node)> m_waitingWritesAllocator;
//		allocator_vector<sizeof(transaction_map::node)> m_waitingReadsAllocator;
//
//		allocator_vector<sizeof(transaction_map::node)> m_readingTransactionsAllocator;
//		allocator_vector<sizeof(transaction_map::node)> m_activeTransactionsAllocator;
//
//		transaction_map m_waitingWrites;
//		transaction_map m_waitingReads;
//
//		transaction_map m_readingTransactions;
//		transaction_map m_activeTransactions;
//
//		// 
//
//		// When a locking_transaction issues a read, it blocks subsequent locking_transaction's which might write to that
//		// area, from starting.  Note that write blocks cascade, such that locking_transactions are unblocked in the order
//		// they were queue, if overlapping.  For example:
//		//
//		//  1. Write A is requested at positions 0-49, and issued.
//		//  2. Write B is requested at positions 50-99, and issued.
//		//  3. Write C is requested at positions 0-99, which causes it to be blocked on A and B.
//		//  4. Write D is requested at positions 0-49, which cases it to be blocked on C.
//		//  5. Write A completes, releasing range 0-49 to write C, which is still blocked.
//		//  6. Write B completes, releasing range 50-99 to write C, which is then unblocked.
//		//  7. Write C completes, releasing range 0-49 to write D, which is then unblocked.
//		//
//		// Note it would have been possible to issue write D between steps 5 and 6, but
//		// it was delayed until step 7.  This is to prevent the potential starvation of write C.
//
//		// need? :
//		// a map of transaction_ids, of active locking_transactions (which have already aquired write access)
//
//		// a map of transaction_ids, of transactions waiting to write to this segment.
//		// a map of transaction_ids, of transactions currently reading from this segment.
//		// a map of transaction_ids, of transactions waiting to read from this segment.
//
//
//		ptr<contention_state> m_nextInSameTransaction;
//		ptr<contention_state> m_prevInSameTransaction;
//		bool m_initialized;
//
//		contention_state()
//			: m_initialized(false)
//		{ }
//
//		contention_state(const contention_state& st, const file_size_t& i)
//			: segment_state_map_node<file_size_t>(st, i),
//			m_prevInSameTransaction(&st)
//			m_nextInSameTransaction(st.m_nextInSameTransaction),
//			m_waitingTransactions(st.m_waitingTransactions),
//			m_initialized(true)
//		{
//			if (!!m_nextInSameTransaction)
//				m_nextInSameTransaction->m_prevInSameTransaction = this;
//			st.m_nextInSameTransaction = this;
//		}
//	};
//
//protected:
//	class writer : public file<write_access, file_size_t>::writer
//	{
//	public:
//		size_t m_numWaitingWrites;
//
//		// If 'waiting write segments' fail to be properly written before the primary write
//		// operation is completed, they are kept in m_failedWriteSegments until the primary
//		// write has completed.
//		ptr<waiting_write_segment> m_failedWriteSegments;
//
//		ptr<concurrency_state> m_firstInWriteThread; // Nodes are split from the end, so this always remains valid.
//
//		rcptr<transaction_internals> m_transaction;
//
//		rcptr<writer> m_nextWriter;
//
//		writer(const rcref<segment_buffer_map<file_size_t> >& sbm, const rcptr<transaction_internals>& t)
//			: file<write_access, file_size_t>::writer(sbm),
//			m_transaction(t),
//			m_numWaitingWrites(0)
//		{ }
//
//		void complete() { file<write_access, file_size_t>::writer::complete(); }
//
//		// completion routine for inner writer.  This node is the first node in the chain associated with this writer.
//		void done_inner_write(const rcref<const typename file<write_access, file_size_t>::writer>& w)
//		{
//			typedef delegate_t<void, const rcref<const typename file<write_access, file_size_t>::writer>&> file_writer_arg_delegate_t;
//			m_transaction->m_synchronizedFileImpl->m_serialQueue->submit(delegate(file_writer_arg_delegate_t(&writer::done_inner_write2, this_rcref), w));
//		}
//		
//		void done_inner_write2(const rcref<const typename file<write_access, file_size_t>::writer>& w) // serialized w/ synchronized_file
//		{
//			rcptr<segment_buffer_map<file_size_t> > accumulatedWriteBuffers;
//			ptr<concurrency_state> firstNodeInInnerWrite;
//			ptr<concurrency_state> lastNodeInInnerWrite;
//			rcptr<writer> anyWriter;
//
//			//  What to do if a write fails?
//			//	If it's while trying to extend the size of the file, that would mean out of disk space.
//			//		So serialize writes past the current EOF, and defer reads of that area until those writes completes.
//			//	If it's in the middle of the file, that is bad news.
//			//		Assert.  This would mean bad media, or some other unsupported circumstance.
//			//		Need to do more. Close the file?
//
//			// TBD: What to do is a write fails?
////#error
//			bool writeFailed = !!w->get_unwritten_buffers();
//
//
//			// The unwrittenBuffers represent failed writes
//			rcref<segment_buffer_map<file_size_t> > unwrittenBuffers = w->get_unwritten_buffers();
//			
//			COGS_ASSERT(!unwrittenBuffers); // TBD: What happens when file writes fail????!
//
//			segment_buffer_map<file_size_t>::iterator itor = unwrittenBuffers->get_first();
//			ptr<concurrency_state> curNode = m_firstInWriteThread;
//
//			file_size_t nodeStart;
//			if (!!curNode)
//				nodeStart = curNode->get_start();
//
//			while (!!curNode)
//			{
//			}
//			// TBD
//		}
//	};
//	
////	void done_secondary_write(const rcref<const typename file<write_access, file_size_t>::writer>& w) // serialized w/ synchronized_file
////	{
////		typedef delegate_t<void, const rcref<const typename file<read_access, file_size_t>::reader>&> file_reader_arg_delegate_t;
////		m_serialQueue->submit(delegate(file_reader_arg_delegate_t(&synchronized_file_impl<file_size_t>::done_secondary_write2, this_rcref), w));
////	}
////
////	void done_secondary_write2(const rcref<const typename file<write_access, file_size_t>::writer>& w) // serialized w/ synchronized_file
////	{
////		// TBD
////	}
//
//	class reader : public file<read_access, file_size_t>::reader
//	{
//	public:
//		// Segment state and read operations.  (See also: class segment_map::node)
//		
//		// (Phase 1) If pieces of a requested read operation are already in memory, such as if cached or
//		// if there is an overlapping write operation, those segments are removed from the
//		// 'unread segments', and content is added directly to the 'read segment buffers'.
//		// 
//		// (Phase 2) Some segments may overlap reads already in progress.  Those segments are
//		// removed from the 'unread segments' map and move to a 'waiting segments' list.
//
//		// When data arrives for a waiting segment, all waiting readers receive the contents, and
//		// write them to their 'read segment buffers', clipped to the range of the waiting-segment
//		// (in case they had been waiting only for a piece of the original read).  They also release
//		// a reference count.  When that reference count hits zero, there are no more reads expected.
//
//		// (Phase 3) At this point, the 'unread segments' map contains only segments that need to
//		// actually be read.  If not empty, the derived reader is invoked.  Upon completion, the
//		// read operation is not fully completed until all 'waiting segments' are also complete.
//
//		// (Phase 4) When the inner read is complete, all concurrency_state's associated with the 
//		// ranges requested are traversed (using a linked-list that threads them together).
//		ptr<concurrency_state> m_firstInReadThread; // Nodes are split from the end, so this always remains valid.
//		size_t m_numWaitingReads;
//
//		// These threaded segments will be a superset of the segments described in m_unreadSegments
//		// and m_readSegmentBuffers, so those can also be iterated through instead of inccuring any
//		// lookups.  Any successfully read buffers are provided to all waiting readers on that concurrency_state.
//		// In case some of the other readers we were waiting on did not complete successfully, we
//		// copy the left-over contents from the 'waiting segments' back into the 'unread segments'.
//		
//		rcptr<transaction_internals> m_transaction;
//
//		rcptr<reader> m_nextReader;
//
//		reader(const rcref<segment_map<file_size_t> >& sm, const rcptr<transaction_internals>& t)
//			: file<read_access, file_size_t>::reader(sm),
//			m_transaction(t),
//			m_numWaitingReads(0)
//		{ }
//
//		void complete() { file<read_access, file_size_t>::reader::complete(); }
//
//		// completion routine for inner reader.  This node is the first node in the chain associated with this reader.
//		void done_inner_read(const rcref<const typename file<read_access, file_size_t>::reader>& r)
//		{
//			typedef delegate_t<void, const rcref<const typename file<read_access, file_size_t>::reader>&> file_reader_arg_delegate_t;
//			m_transaction->m_synchronizedFileImpl->m_serialQueue->submit(delegate(file_reader_arg_delegate_t(&reader::done_inner_read2, this_rcref), r));
//		}
//
//		concurrency_state* done_read_node(concurrency_state& curNode, segment_buffer_map<file_size_t>& accumulatedReadSegmentBuffers, segment_map<file_size_t>& accumulatedUnreadSegments)
//		{
//			// For each waiting read segment waiting for this node's segment
//			bool doneCleaning = false;
//			ptr<waiting_read_segment> waitingReadSegment = curNode.m_firstWaitingReadSegment;
//			curNode.m_firstWaitingReadSegment.clear();
//			while (!!waitingReadSegment)
//			{
//				ptr<waiting_read_segment> nextWaitingReadSegment = waitingReadSegment->m_nextWaitingReadSegment;
//				reader& r = waitingReadSegment->m_reader;
//				r.get_unread_segments()->add(accumulatedUnreadSegments);
//				r.get_buffers()->add(accumulatedReadSegmentBuffers);
//
//				if (!doneCleaning)
//				{
//					if (!!waitingReadSegment->m_refCount--)
//						doneCleaning = true;
//					else
//					{
//						if (!--r.m_numWaitingReads)
//							r.complete(); // Might have been the last thing that reader was waiting for.
//						default_allocator::destruct_deallocate_type(waitingReadSegment);
//					}
//				}
//				waitingReadSegment = nextWaitingReadSegment;
//			}
//			accumulatedReadSegmentBuffers.clear();
//			accumulatedUnreadSegments.clear();
//
//			concurrency_state* nextNode = curNode.m_nextInReadThread;
//
//			// Figure out if node needs to be cleared out or not
//			if (!curNode.m_firstWaitingIssuedWriteSegment) // No reads, no writes, means not needed.  TBD: Linger if caching?
//				m_transaction->m_synchronizedFileImpl->m_concurrencyMap.remove(curNode);
//
//			return nextNode;
//		}
//
//		void done_inner_read2(const rcref<const typename file<read_access, file_size_t>::reader>& r) // serialized w/ synchronized_file
//		{
//			rcptr<segment_buffer_map<file_size_t> > accumulatedWriteBuffers;
//			ptr<concurrency_state> firstNodeInInnerWrite;
//			ptr<concurrency_state> lastNodeInInnerWrite;
//			rcptr<writer> anyWriter;
//
//			rcref<segment_buffer_map<file_size_t> > readBuffers = r->get_buffers();
//			segment_buffer_map<file_size_t>::iterator itor = readBuffers->get_first();
//			ptr<concurrency_state> curNode = m_firstInReadThread;
//			segment_buffer_map<file_size_t> accumulatedReadSegmentBuffers;
//			segment_map<file_size_t> accumulatedUnreadSegments;
//
//			file_size_t nodeStart;
//			if (!!curNode)
//				nodeStart = curNode->get_start();
//
//			while (!!curNode)
//			{
//				// If any write is pending, accumulate it into a write we will issue at the end of the loop.
//				if (!!curNode->m_firstWaitingWriteSegment)
//				{
//					COGS_ASSERT(!curNode->m_firstWaitingIssuedWriteSegment);
//					if (!firstNodeInInnerWrite)
//					{
//						lastNodeInInnerWrite = firstNodeInInnerWrite = curNode;
//						//curNode->m_nextInWriteThread.clear();
//					}
//					else
//					{
//						lastNodeInInnerWrite->m_nextInWriteThread = curNode;
//						lastNodeInInnerWrite = curNode;
//					}
//
//					ptr<waiting_write_segment> waitingWriteSegment = curNode->m_firstWaitingWriteSegment;
//					curNode->m_firstWaitingIssuedWriteSegment = waitingWriteSegment;
//					curNode->m_firstWaitingWriteSegment.clear();
//					anyWriter = waitingWriteSegment->m_writer;
//					if (!accumulatedWriteBuffers)
//						accumulatedWriteBuffers = rcnew(segment_buffer_map<file_size_t>);
//					accumulatedWriteBuffers->add(curNode->m_segment.get_start(), curNode->m_buffer);
//				}
//
//				if (!curNode->m_firstWaitingReadSegment) // If no waiting segments, no need to split out portions
//				{
//					curNode = done_read_node(*curNode, accumulatedReadSegmentBuffers, accumulatedUnreadSegments);
//					if (!!curNode)
//						nodeStart = curNode->get_start();
//					continue;
//				}
//				file_size_t nodeEnd = curNode->get_end();
//				if (!itor || (itor->get_start() > nodeEnd)) // whole node is missing.
//				{
//					accumulatedUnreadSegments.add(nodeStart, nodeEnd - nodeStart);
//					curNode = done_read_node(*curNode, accumulatedReadSegmentBuffers, accumulatedUnreadSegments);
//					if (!!curNode)
//						nodeStart = curNode->get_start();
//					continue;
//				}
//				// At this point the read buffer starts before this node ends.
//				// If it also ends before this node starts, then skip it.
//				file_size_t itorEnd = itor->get_end();
//				if (itorEnd <= nodeStart)
//				{
//					++itor;
//					continue;
//				}
//
//				// At this point the read buffer starts before this node ends.
//				// And the read buffer ends after the node starts.
//				// So there must be an overlap
//
//				// If the read buffer starts before the node starts, we don't care about that part
//				// (Normally, this wouldn't happen, if not for the skipping of nodes that don't have waiting read segments).
//				file_size_t itorStart = itor->get_start();
//				if (nodeStart < itorStart)
//				{
//					accumulatedUnreadSegments.add(nodeStart, itorStart - nodeStart);
//					nodeStart = itorStart;
//				}
//
//				// We know the overlap starts at the beginning of the node, ...
//				file_size_t endPos = nodeEnd;
//				bool readBlockEndsBeforeNodeEnd = (itorEnd < nodeEnd);
//				if (readBlockEndsBeforeNodeEnd)
//					endPos = itorEnd;
//				accumulatedReadSegmentBuffers.add(itor->get_trailing((size_t)(nodeStart - itorStart)).get_leading((size_t)(endPos - nodeStart)));
//				
//				if (readBlockEndsBeforeNodeEnd)
//				{
//					// ... and extends to itorEnd
//					++itor;
//					nodeStart = endPos;
//				}
//				else
//				{
//					// ... and extends to nodeEnd
//					curNode = done_read_node(*curNode, accumulatedReadSegmentBuffers, accumulatedUnreadSegments);
//					if (!!curNode)
//						nodeStart = curNode->get_start();
//					if (nodeEnd == itorEnd)
//						++itor;
//				}
//				//continue;
//			}
//			get_unread_segments()->combine(*(r->get_unread_segments()));
//			get_buffers()->combine(*(r->get_buffers()));
//			if (!--m_numWaitingReads)
//				complete();
//
//			if (!!accumulatedWriteBuffers)
//			{
//				typedef file<write_access, file_size_t>::writer::dispatch_t dispatch_t;
//	//			anyWriter->m_transaction->m_rawWritable->write(accumulatedWriteBuffers.get_ref())->dispatch(dispatch_t(&concurrency_state::done_secondary_write, firstNodeInInnerWrite.get_ref()));
//			}
//		}
//	};
//
//	class size_reader : public file<read_access, file_size_t>::size_reader
//	{
//	public:
//		rcptr<transaction_internals> m_transaction;
//
//		size_reader(const rcptr<transaction_internals>& t)
//			: m_transaction(t)
//		{ }
//
//		void set_size(const file_size_t& sz) { file<read_access, file_size_t>::size_reader::set_size(sz); }
//		void complete() { file<read_access, file_size_t>::size_reader::complete(); }
//	};
//	
//	class size_writer : public file<write_access, file_size_t>::size_writer
//	{
//	public:
//		rcptr<transaction_internals> m_transaction;
//
//		size_writer(const file_size_t& sz, const rcptr<transaction_internals>& t)
//			: file<write_access, file_size_t>::size_writer(sz),
//			m_transaction(t)
//		{ }
//
//		void complete() { file<write_access, file_size_t>::size_writer::complete(); }
//	};
//
////	virtual rcref<size_reader> get_size() // TEMP!
////	{
////		rcref<size_reader> result = rcnew(size_reader, m_eof);
////		result->complete();
////		return result;
////	}
////
////	virtual rcref<size_writer> set_size(const file_size_t& sz) // TEMP!
////	{
////		m_eof = sz;
////		rcref<size_writer> result = rcnew(size_writer, sz);
////		result->complete();
////		return result;
////	}
//
//	segment_state_map<concurrency_state, file_size_t> m_concurrencyMap;
//	segment_state_map<contention_state, file_size_t> m_contentionMap;
//
//	file_size_t m_eof;
//
//	//	If any of these segments are written to before the failable_transaction completes, it fails.
//	rcptr<transaction_internals> m_failableTransactions;
//
//	// If a queued locking transaction attempts to read, that read is deferred until that transaction
//	// becomes the current one.
//	// Only the current (first) locking transaction may commit reads.
//	rcptr<transaction_internals> m_firstDeferredLockingTransaction;
//	rcptr<transaction_internals> m_lastDeferredLockingTransaction;
//
//	// Synchronizes reads, and queueing of locking transactions.
////	rcref<delegate_chain> m_lockingReadDeferQueue;
//
//	// Any submitted composed transaction containg writes overlapping the lock-mask, is queued.
//	rcptr<transaction_internals> m_firstDeferredComposedTransaction;
//	rcptr<transaction_internals> m_lastDeferredComposedTransaction;
//
//	// Any submitted failable-transaction contain writes overlapping the lock-mask, is queued.
//	rcptr<transaction_internals> m_firstDeferredFailableTransaction;
//	rcptr<transaction_internals> m_lastDeferredFailableTransaction;
//
//	rcref<delegate_chain> m_serialQueue;
//
//	typedef delegate_t<void, const rcref<transaction_internals>&> submit_delegate_t;
//
//	class transaction_internals : public dlink_t<transaction_internals, rcptr>, public object
//	{
//	public:
//		segment_buffer_map<file_size_t> m_writeMap; // All contents writen.  Serialized in transaction_internals.
//
//		// read-map
//		//
//		//	The read-map keeps track of any segments read by a locking_transaction or failable_transaction.
//		//	Any transaction that contains writes to a segment that has been read by a locking_transaction,
//		//	will be blocked until the locking_transaction is complete.  If a transaction contains writes
//		//  to a segment read by a failable_transaction, the failable_transaction is aborted.
//		segment_map<file_size_t> m_readMap; // Issued read map.  Serialized in synchronized_file_impl.
//		bool m_wasEofRead; // Whether or not transaction has read the current EOF position.
//
//		rcptr<reader> m_firstReader; // list of deferred high level readers
//		rcptr<reader> m_lastReader;
//
//		rcptr<writer> m_firstWriter; // list of deferred high level writers
//		rcptr<writer> m_lastWriter;
//
//		rcptr<size_reader> m_firstSizeReader; // list of deferred high level size readers
//		rcptr<size_reader> m_lastSizeReader;
//
//		rcptr<size_writer> m_firstSizeWriter; // list of deferred high level size writers
//		rcptr<size_writer> m_lastSizeWriter;
//
//		submit_delegate_t m_submitDelegate;
//
//		rcref<synchronized_file_impl<file_size_t> >	m_synchronizedFileImpl;
//
//		// A note on scope and ownership:
//		//
//		// A transaction (transaction_internals) extends the scope of the high level reader and/or writer.
//		// An operation issued on a transaction will extend the scope of the transaction.
//
//		rcptr<synchronized_file<read_access> > m_syncReadable; // Just here to extend scope if readable this transaction was created for.
//		rcptr<synchronized_file<write_access> > m_syncWritable; // Just here to extend scope if writable this transaction was created for.
//
//		rcptr<file<read_access> > m_rawReadable; // Coped from the readable this transaction was created for.
//		rcptr<file<write_access> > m_rawWritable; // Coped from the writable this transaction was created for.
//
//		file_mask<file_size_t> m_writeMask; // If locking_transaction, identifies target write mask.
//
//		enum class state
//		{
//			pending              = 0x00,          // 0000
//			submit_initiated_bit = 0x01,          // 0001
//			submit_issued_bit    = 0x02,          // 0010
//
//			abort_initiated_bit  = 0x04,          // 0100
//			abort_issued_bit     = 0x08,          // 1000
//
//			abort_or_submit_initiated_mask = 0x05 // 0101
//		};
//
//		volatile number<int_type, void, thread_safe> m_state;
//		bool m_submitCompleted; // only accessed while serialized in submit delegates, so not voltaile.
//
//		// Serializes the following operations:
//		//	Aborting	- draining the reader and writer lists
//		//	Submitting	- draining the reader and writer lists
//		//	Reading		- Adding an element to the reader list, or completing immediately if aborted or already submitted
//		//	Writing		- Adding an element to the writer list, or completing immediately if aborted or already submitted
//		rcref<delegate_chain>						m_serialQueue;
//
//		transaction_internals(const submit_delegate_t& submitDelegate, const rcref<synchronized_file<read_access, file_size_t> >& syncReable, const rcref<file<read_access> >& rawReader, const rcref<synchronized_file_impl<file_size_t> >& impl)
//			: m_submitDelegate(submitDelegate),
//			m_state(0),
//			m_serialQueue(delegate_chain::create()),
//			m_synchronizedFileImpl(impl),
//			m_rawReader(rawReader),
//			m_syncReadable(syncReadable),
//			m_submitCompleted(false)
//		{ }
//
//		transaction_internals(const submit_delegate_t& submitDelegate, const rcref<synchronized_file<write_access, file_size_t> >& syncWritable, const rcref<file<write_access> >& rawWritable, const rcref<synchronized_file_impl<file_size_t> >& impl)
//			: m_submitDelegate(submitDelegate),
//			m_state(0),
//			m_serialQueue(delegate_chain::create()),
//			m_syncWritable(syncWritable),
//			m_rawWritable(rawWritable),
//			m_synchronizedFileImpl(impl),
//			m_submitCompleted(false)
//		{ }
//
//		transaction_internals(const submit_delegate_t& submitDelegate, const rcref<synchronized_file<read_write_access, file_size_t> >& syncFile, const rcref<file<read_write_access> >& rawFile, const rcref<synchronized_file_impl<file_size_t> >& impl)
//			: m_submitDelegate(submitDelegate),
//			m_state(0),
//			m_serialQueue(delegate_chain::create()),
//			m_syncReadable(syncFile),
//			m_syncWritable(syncFile),
//			m_rawReadable(rawFile),
//			m_rawWritable(rawFile),
//			m_synchronizedFileImpl(impl),
//			m_submitCompleted(false)
//		{ }
//
//		transaction_internals(const submit_delegate_t& submitDelegate, const rcref<synchronized_file<read_write_access, file_size_t> >& syncFile, const rcref<file<read_access> >& rawReadable, const rcref<file<write_access> >& rawWritable, const rcref<synchronized_file_impl<file_size_t> >& impl)
//			: m_submitDelegate(submitDelegate),
//			m_state(0),
//			m_serialQueue(delegate_chain::create()),
//			m_syncReadable(syncFile),
//			m_syncWritable(syncFile),
//			m_rawReadable(rawReadable),
//			m_rawWritable(rawWritable),
//			m_synchronizedFileImpl(impl),
//			m_submitCompleted(false)
//		{ }
//
//		void set_write_mask(const file_mask<file_size_t>& fm) { m_writeMask = fm; }
//
//		bool was_submit_initiated() const { return (m_state.get_int<int>() & submit_initiated_bit) != 0; }
//		bool was_submit_issued() const { return (m_state.get_int<int>() & submit_issued_bit) != 0; }
//		bool was_submit_completed() const { return m_submitCompleted; }
//
//		bool was_abort_initiated() const { return (m_state.get_int<int>() & abort_initiated_bit) != 0; }
//		bool was_abort_issued() const { return (m_state.get_int<int>() & abort_issued_bit) != 0; }
//
//		bool was_submit_or_abort_initiated() const { return (m_state.get_int<int>() & (submit_initiated_bit | abort_initiated_bit)) != 0; }
//		bool was_submit_or_abort_issued() const { return (m_state.get_int<int>() & (submit_issued_bit | abort_issued_bit)) != 0; }
//
//		void register_failable_transaction()
//		{ // Because this is called from a constructor, we know it's first in the queue
//			m_serialQueue->submit(COGS_CONST_DELEGATE_FROM_RC_MEMBER(&transaction_internals::register_failable_transaction2, this_rcref, reference_strength::strong));
//		}
//
//		void register_locking_transaction()
//		{ // Because this is called from a constructor, we know it's first in the queue
//			m_serialQueue->submit(COGS_CONST_DELEGATE_FROM_RC_MEMBER(&transaction_internals::register_locking_transaction2, this_rcref, reference_strength::strong));
//		}
//
//		void register_failable_transaction2()
//		{
//			m_synchronizedFileImpl->m_serialQueue->submit(delegate(transaction_arg_delegate_t(&synchronized_file_impl<file_size_t>::register_failable_transaction, m_synchronizedFileImpl), this_rcref));
//		}
//
//		void register_locking_transaction2()
//		{
//			m_synchronizedFileImpl->m_serialQueue->submit(delegate(transaction_arg_delegate_t(&synchronized_file_impl<file_size_t>::register_locking_transaction, m_synchronizedFileImpl), this_rcref));
//		}
//
//		void abort() // defers abort, if not already initiated
//		{
//			number<int_type> oldState = m_state;
//			for (;;)
//			{
//				if ((oldState.get_internal().get_int() & abort_initiated_bit) != 0)
//					break; // serializes parallel calls to abort()
//
//				number<int_type> newState = (oldState.get_internal().get_int() | abort_initiated_bit);
//				if (m_state.compare_exchange(newState, oldState, oldState))
//				{
//					m_serialQueue->submit(COGS_CONST_DELEGATE_FROM_RC_MEMBER(&transaction_internals::abort2, this_rcref, reference_strength::strong));
//					break;
//				}
//			}
//		}
//
//		void abort2()
//		{
//			number<int_type> newState;
//			number<int_type> oldState = m_state;
//			for (;;)
//			{
//				newState = oldState.get_internal().get_int() | abort_issued_bit;
//				if (m_state.compare_exchange(newState, oldState, oldState))
//				{
//					if (!m_submitCompleted)
//					{
//						abort_queued();
//						m_synchronizedFileImpl = 0;
//						m_submitDelegate.release(); // releases reference to impl
//					}
//					m_serialQueue->run_next();
//					break;
//				}
//			}
//		}
//
//		void submit() // submit() will only be called once, so no need to check if already initiated
//		{
//			number<int_type> newState;
//			number<int_type> oldState = m_state;
//			for (;;)
//			{
//				if ((oldState.get_internal().get_int() & abort_issued_bit) != 0)
//					break; // Abort was already issued, don't bother submitting.
//
//				newState = oldState.get_internal().get_int() | submit_initiated_bit; // set initiated bit
//				if (m_state.compare_exchange(newState, oldState, oldState))
//				{
//					m_serialQueue->submit(COGS_CONST_DELEGATE_FROM_RC_MEMBER(&transaction_internals::submit2, this_rcref, reference_strength::strong));
//					break;
//				}
//			}
//		}
//
//		void submit2()
//		{
//			number<int_type> newState;
//			number<int_type> oldState = m_state;
//			for (;;)
//			{
//				if ((oldState.get_internal().get_int() & abort_initiated_bit) != 0)
//					break;
//
//				newState = (oldState.get_internal().get_int() | submit_issued_bit);
//				if (m_state.compare_exchange(newState, oldState, oldState))
//				{
//					m_synchronizedFileImpl->m_serialQueue->submit(delegate(m_submitDelegate, this_rcref));
//					m_synchronizedFileImpl = 0;
//					m_submitDelegate.release(); // releases reference to impl
//					break;
//				}
//			}
//		}
//
//		void submit_queued() // Serialized by fileImpl, so should not be in aborted state
//		{
//			;
//		}
//
//		void abort_queued() // Serialized by fileImpl if submitted, so should not conflict with submit_queued().
//		{
//			rcptr<reader> r = m_firstReader;
//			m_firstReader = 0;
//			while (!!r)
//			{
//				r->complete();
//				r = r->m_nextReader.get();
//			}
//			
//			rcptr<writer> w = m_firstWriter;
//			m_firstWriter = 0;
//			while (!!w)
//			{
//				w->complete();
//				w = w->m_nextWriter.get();
//			}
//		}
//
//		void queue_read(const rcref<reader>& r)
//		{
//			m_serialQueue->submit(delegate(reader_arg_delegate_t(&transaction_internals::queue_read2, this_rcref), r));
//		}
//
//		void queue_write(const rcref<writer>& w)
//		{
//			m_serialQueue->submit(delegate(writer_arg_delegate_t(&transaction_internals::queue_write2, this_rcref), w));
//		}
//
//		void defer_issue_locking_read(const rcref<reader>& r)
//		{
//			m_serialQueue->submit(delegate(reader_arg_delegate_t(&transaction_internals::issue_locking_read2, this_rcref), r));
//		}
//
//		void defer_issue_failable_read(const rcref<reader>& r)
//		{
//			m_serialQueue->submit(delegate(reader_arg_delegate_t(&transaction_internals::issue_failable_read2, this_rcref), r));
//		}
//
//		bool prefill_read_inner(const rcref<reader>& r) // returns true if completed using only data in m_writeMap
//		{
//			r->get_buffers()->read_overlap(m_writeMap, *(r->get_unread_segments()));
//			return r->get_unread_segments().is_empty();
//		}
//
//		void issue_locking_read2(const rcref<reader>& r)
//		{
//			if (was_submit_or_abort_issued() // It's OK to abort a locking read immediately
//				|| prefill_read_inner(r))
//			{
//				r->complete();
//				m_serialQueue->run_next();
//			}
//			else
//			{
//				r->m_transaction = this_rcref;
//				m_synchronizedFileImpl->m_serialQueue->submit(delegate(reader_arg_delegate_t(&synchronized_file_impl<file_size_t>::commit_locking_read, m_synchronizedFileImpl), r));
//			}
//		}
//
//		void issue_failable_read2(const rcref<reader>& r)
//		{
//			if (was_submit_or_abort_issued() // It's OK to abort a failable read immediately
//				|| prefill_read_inner(r))
//			{
//				r->complete();
//				m_serialQueue->run_next();
//			}
//			else
//			{
//				r->m_transaction = this_rcref;
//				m_synchronizedFileImpl->m_serialQueue->submit(delegate(reader_arg_delegate_t(&synchronized_file_impl<file_size_t>::commit_failable_read, m_synchronizedFileImpl), r));
//			}
//		}
//
//		void queue_read3(const rcref<reader>& r)
//		{
//			if (!m_lastReader)
//				m_firstReader = r;
//			else
//				m_lastReader->m_nextReader = r;
//			m_lastReader = r;
//		}
//
//		void queue_read2(const rcref<reader>& r)
//		{
//			if (was_submit_or_abort_issued()
//				|| prefill_read_inner(r))
//				r->complete();
//			else
//				queue_read3(r);
//			m_serialQueue->run_next();
//		}
//
//		void queue_write2(const rcref<writer>& w)
//		{
//			m_writeMap.add(*(w->get_unwritten_buffers()));
//
//			if (!m_lastWriter)
//				m_firstWriter = w;
//			else
//				m_lastWriter->m_nextWriter = w;
//			m_lastWriter = w;
//		}
//	};
//
//	void register_failable_transaction(const rcref<transaction_internals>& t)
//	{
//		// Just adds it to a list of all failable transactions on this file.
//
//		t->set_next_link(m_failableTransactions);
//		m_failableTransactions = t;
//	}
//
//	void register_locking_transaction(const rcref<transaction_internals>& t)
//	{
//		//
//		//
////#error going to need a new object for this.  Like file_mask<>, but with data associated with each segment.  Cant use segment_map<>.
//		// contention_state
//
//		if (!m_firstDeferredLockingTransaction)
//			m_firstDeferredLockingTransaction = m_lastDeferredLockingTransaction = t;
//		else
//		{
//			m_lastDeferredLockingTransaction->set_next_link(t);
//			t->set_prev_link(m_lastDeferredLockingTransaction);
//			m_lastDeferredLockingTransaction = t;
//		}
//	}
//
//	void run_write_notifications(const rcref<transaction_internals>& t)
//	{
//		transaction_internals* prev = 0;
//		transaction_internals* cur = m_failableTransactions.get();
//		while (!!cur)
//		{
//			rcptr<transaction_internals> next = t->get_next_link();
//			if (cur != t.get()) // In case this is a failable_transaction
//			{
//				if (!t->m_writeMap.does_overlap(cur->m_readMap))
//					prev = cur;
//				else
//				{
//					cur->abort();
//					if (!prev)
//						m_failableTransactions = next;
//					else
//						prev->set_next_link(next);
//				}
//			}
//			cur = next.get();
//		}
//	}
//
//	void issue_read(const rcref<reader>& r)
//	{
//		rcref<segment_map<file_size_t> > segmentMap = r->get_unread_segments();
//		segment_map<file_size_t>::iterator itor = segmentMap->get_first();
//		ptr<concurrency_state> firstNodeInInnerRead; // Keep track of the first node that we need to issue an inner read for.
//		ptr<concurrency_state> lastNodeInInnerRead;
//		while (!!itor)
//		{
//			segment_map<file_size_t>::iterator nextItor = itor.next();
//
//			bool retainSegment = false;
//			segment<file_size_t> curSegment = *itor;
//			concurrency_state* curNode = m_concurrencyMap.split_create(curSegment);
//			for (;;)
//			{
//				if (!!curNode->m_buffer) // Write in progress, or cached. Grab buffer.
//				{
//					r->get_buffers()->add(segment_buffer<file_size_t>(curNode->get_start(), curNode->m_buffer));
//				}
//				else if (!!curNode->m_initialized) // Read in progress.  Wait for it to complete.
//				{
//					waiting_read_segment* waitingSegment = new (default_allocator::get()) waiting_read_segment(*r);
//
//					++(r->m_numWaitingReads);
//					
//					// Keep track of all of the segment waiting on this concurrency_state.
//					waitingSegment->m_nextWaitingReadSegment = curNode->m_firstWaitingReadSegment;
//					curNode->m_firstWaitingReadSegment = waitingSegment;
//				}
//				else // Brand new node, queue up for inner read
//				{
//					curNode->m_initialized = true;
//					if (!firstNodeInInnerRead)
//					{
//						firstNodeInInnerRead = lastNodeInInnerRead = curNode;
//						firstNodeInInnerRead->m_outerReader = r;
//					}
//					else
//					{
//						lastNodeInInnerRead->m_nextInReadThread = curNode;
//						lastNodeInInnerRead = curNode;
//					}
//
//					if (!!retainSegment) // If someone else claimed itor, we need to add a new one
//						itor = segmentMap->add(curNode->m_segment);
//					else // If the first to want to retain itor.
//					{
//						retainSegment = true;
//						*itor = curNode->m_segment;
//					}
//				}
//				curSegment.advance(curNode->get_length());
//				if (curSegment.get_length() == 0)
//				{
//					if (!retainSegment)
//						segmentMap->remove(itor);
//					break;
//				}
//
//				curNode = concurrency_state::get_next(curNode);
//			}
//			itor = nextItor;
//		}
//		
//		if (!!firstNodeInInnerRead) // If something is left, issue an inner read
//		{
//			++(r->m_numWaitingReads);
//			r->m_firstInReadThread = firstNodeInInnerRead;
//			rcref<segment_map<file_size_t> > segmentMapCopy = rcnew(segment_map<file_size_t>);
//			segmentMapCopy.exchange(segmentMap); // exchange contents
//			typedef file<read_access, file_size_t>::reader::dispatch_t dispatch_t;
//			r->m_transaction->m_rawReadable->read(segmentMapCopy)->dispatch(dispatch_t(&synchronized_file_impl<file_size_t>::reader::done_inner_read, r));
//		}
//		else if (!r->m_numWaitingReads)
//			r->complete(); // Must be done already!
//	}
//
//	void issue_write(const rcref<writer>& w)
//	{
//		ptr<concurrency_state> firstNodeInInnerWrite;
//		ptr<concurrency_state> lastNodeInInnerWrite;
//		rcref<segment_buffer_map<file_size_t> > segmentBufferMap = w->get_unwritten_buffers();
//		segment_buffer_map<file_size_t>::iterator itor = segmentBufferMap->get_first();
//		while (!!itor)
//		{
//			segment_buffer_map<file_size_t>::iterator nextItor = itor.next();
//
//			bool retainSegment = false;
//			segment_buffer<file_size_t>& curSegmentBuffer = *itor;
//			concurrency_state* curNode = m_concurrencyMap.split_create(curSegmentBuffer.get_segment());
//			for (;;)
//			{
//				// Set m_buffer to our buffer, regardless of what it had been before.
//				curNode->m_buffer = curSegmentBuffer.get_buffer().get_leading(curNode->get_length());
//
//				if (!!curNode->m_initialized) // if existing read or write.
//				{ // If read or write in progress.  Queue ours up for later.  (TBD: What about caching?)
//					waiting_write_segment* waitingSegment = new (default_allocator::get()) waiting_write_segment(*w);
//
//					++(w->m_numWaitingWrites);
//					
//					// Keep track of all of the waiting segment waiting on this concurrency_state.
//					waitingSegment->m_nextWaitingWriteSegment = curNode->m_firstWaitingWriteSegment;
//					curNode->m_firstWaitingWriteSegment = waitingSegment;
//				}
//				else // Brand new node, queue up for inner write.
//				{
//					curNode->m_initialized = true;
//					if (!firstNodeInInnerWrite)
//					{
//						firstNodeInInnerWrite = curNode;
//						firstNodeInInnerWrite->m_outerWriter = w;
//					}
//					else
//					{
//						lastNodeInInnerWrite->m_nextInWriteThread = curNode;
//						lastNodeInInnerWrite = curNode;
//					}
//
//					if (!!retainSegment) // If someone else claimed itor, we need to add a new one
//						itor = segmentBufferMap->add(curNode->m_segment.get_start(), curNode->m_buffer);
//					else // If the first to want to retain itor.
//					{
//						retainSegment = true;
//						*itor = segment_buffer<file_size_t>(curNode->m_segment.get_start(), curNode->m_buffer);
//					}
//				}
//				curSegmentBuffer.advance(curNode->get_length());
//				if (curSegmentBuffer.get_length() == 0)
//				{
//					if (!retainSegment)
//						segmentBufferMap->remove(itor);
//					break;
//				}
//
//				curNode = concurrency_state::get_next(curNode);
//			}
//			itor = nextItor;
//		}
//		if (!!firstNodeInInnerWrite) // If something is left, issue an inner read
//		{
//			w->m_firstInWriteThread = firstNodeInInnerWrite;
//			rcref<segment_buffer_map<file_size_t> > segmentBufferMapCopy = rcnew(segment_buffer_map<file_size_t>);
//			segmentBufferMapCopy.exchange(segmentBufferMap); // exchange contents
//			typedef file<write_access, file_size_t>::writer::dispatch_t dispatch_t;
//			w->m_transaction->m_rawWritable->write(segmentBufferMapCopy)->dispatch(dispatch_t(&synchronized_file_impl<file_size_t>::writer::done_inner_write, w));
//		}
//	}
//
//	void issue_reads(const rcref<transaction_internals>& t)
//	{
//		rcptr<reader> r = t->m_firstReader;
//		t->m_firstReader = 0;
//		while (!!r)
//		{
//			rcptr<reader> next = r->m_nextReader;
//			issue_read(r.get_ref());
//			r = next;
//		}
//	}
//
//	void issue_writes(const rcref<transaction_internals>& t)
//	{
//		rcptr<writer> w = t->m_firstWriter;
//		t->m_firstWriter = 0;
//		while (!!w)
//		{
//			rcptr<writer> next = w->m_nextWriter;
//			issue_write(w.get_ref());
//			w = next;
//		}
//	}
//
//	void issue_transaction(const rcref<transaction_internals>& t)
//	{
//		issue_reads(t);
//		issue_writes(t);
//	}
//	
//	void commit_failable_read(const rcref<reader>& r)
//	{
//		// Check for aborted or not?  We know it won't have been submitted yet.
//		// Lets go ahead and allow the read to complete, rather than risk returning
//		// a partial read (if some contents were prefilled)>
//
//		// keep track of reads, in case written to, which would cause failable transaction to abort.
//		r->m_transaction->m_readMap.add(*(r->get_unread_segments()));
//		issue_read(r);
//
//		r->m_transaction->m_serialQueue->run_next();
//		m_serialQueue->run_next();
//	}
//
//	void commit_locking_read(const rcref<reader>& r)
//	{
//		r->m_transaction->m_readMap.add(*(r->get_unread_segments())); // Lock out writes to this region.
//
//		if (m_firstDeferredLockingTransaction != r->m_transaction)
//			r->m_transaction->queue_read3(r); // If not the current locking transaction, queue the read.
//		else
//			issue_read(r);
//		r->m_transaction->m_serialQueue->run_next();
//		m_serialQueue->run_next();
//	}
//
//	void commit_transaction_inner(const rcref<transaction_internals>& t)
//	{
//		issue_transaction(t); //Issue both reads and writers directly to issued-map
//		t->m_submitCompleted = true;
//	}
//
//	bool commit_transaction(const rcref<transaction_internals>& t)
//	{
//		if (!!m_firstDeferredLockingTransaction && t->m_writeMap.does_overlap(m_firstDeferredLockingTransaction->m_readMap))
//			return false;
//		run_write_notifications(t); // Next check for overlaps with pending failable_transactions and abort if necessary.
//		commit_transaction_inner(t);
//		return true;
//	}
//
//	void commit_composed(const rcref<transaction_internals>& t)
//	{
//		if (!commit_transaction(t))
//		{
//			if (!m_firstDeferredComposedTransaction)
//				m_firstDeferredComposedTransaction = m_lastDeferredComposedTransaction = t;
//			else
//			{
//				m_lastDeferredComposedTransaction->set_next_link(t);
//				t->set_prev_link(m_lastDeferredComposedTransaction);
//				m_lastDeferredComposedTransaction = t;
//			}
//		}
//		m_serialQueue->run_next();
//	}
//
//	void commit_failable_inner(const rcref<transaction_internals>& t)
//	{
//		// Commit the writes as if they are a composed transaction containing only writes.
//		if (!commit_transaction(t))
//		{
//			if (!m_firstDeferredFailableTransaction)
//				m_firstDeferredFailableTransaction = m_lastDeferredFailableTransaction = t;
//			else
//			{
//				m_lastDeferredFailableTransaction->set_next_link(t);
//				t->set_prev_link(m_lastDeferredFailableTransaction);
//				m_lastDeferredFailableTransaction = t;
//			}
//		}
//		m_serialQueue->run_next();
//	}
//
//	void commit_failable(const rcref<transaction_internals>& t)
//	{
//		if (!t->was_abort_initiated()) // Need to check again if aborted, because we can't issue its write if
//		{                              // something has aborted it since submitted.
//			if (m_failableTransactions == t)
//			{
//				m_failableTransactions = t->get_next_link();
//				m_failableTransactions->set_prev_link(0); // needed?
//			}
//			else
//			{
//				rcptr<transaction_internals> next = t->get_next_link();
//				rcptr<transaction_internals> prev = t->get_prev_link();
//				if (!!next)
//					next->set_prev_link(prev);
//				if (!!prev)
//					prev->set_next_link(next);
//			}
//
//			commit_failable_inner(t);
//		}
//	}
//
//	void commit_locking(const rcref<transaction_internals>& tIn)
//	{
//		rcref<transaction_internals> t = tIn;
//		COGS_ASSERT(m_firstDeferredLockingTransaction != 0);
//		t->m_submitCompleted = true;
//		if (m_firstDeferredLockingTransaction == t) // If not the current transaction, queue it.
//		{
//			for (;;)
//			{
//				run_write_notifications(t); // Next check for overlaps with pending failable_transactions and abort if necessary.
//			
//				issue_writes(t);
//
//				m_firstDeferredLockingTransaction = 0; // Temporarily unset locking transactions, and let all pending failable and composed transactions through.
//
//				// If a deferred transaction had been blocked trying to write to a portion the current locking transaction had read,
//				// It will be waiting in a list.  Issue everything in that list.
//				while (!!m_firstDeferredFailableTransaction)
//				{
//					rcptr<transaction_internals> t2 = m_firstDeferredFailableTransaction;
//					m_firstDeferredFailableTransaction = t2->get_next_link();
//					if (!m_firstDeferredFailableTransaction)
//						m_lastDeferredFailableTransaction = 0;
//					run_write_notifications(t2.get_ref());
//					commit_failable(t2.get_ref());
//				}
//				m_lastDeferredFailableTransaction = 0;
//
//				while (!!m_firstDeferredComposedTransaction)
//				{
//					rcptr<transaction_internals> t2 = m_firstDeferredComposedTransaction;
//					m_firstDeferredComposedTransaction = t2->get_next_link();
//					if (!m_firstDeferredComposedTransaction)
//						m_lastDeferredComposedTransaction = 0;
//					run_write_notifications(t2.get_ref());
//					commit_composed(t2.get_ref());
//				}
//
//				m_firstDeferredLockingTransaction = t->get_next_link(); // Start the next m_waitingLockingTransactions
//				if (!m_firstDeferredLockingTransaction)
//				{
//					m_lastDeferredLockingTransaction = 0;
//					break;
//				}
//				issue_reads(m_firstDeferredLockingTransaction.get_ref());
//				if (!m_firstDeferredLockingTransaction->m_submitCompleted)
//					break;
//
//				t = m_firstDeferredLockingTransaction.get_ref(); // commit the next one.
//				//continue;
//			}
//		}
//	}
//
//	template <file_transaction_type type>
//	submit_delegate_t get_commit_delegate()
//	{
//		if (type == composed_transaction)
//			return submit_delegate_t(&synchronized_file_impl<file_size_t>::commit_composed, this_rcref);
//
//		if (type == locking_transaction)
//			return submit_delegate_t(&synchronized_file_impl<file_size_t>::commit_locking, this_rcref);
//
//		if (type == failable_transaction)
//			return submit_delegate_t(&synchronized_file_impl<file_size_t>::commit_failable, this_rcref);
//	}
//	
//	template <file_transaction_type type>
//	class transaction : public object
//	{
//	private:
//		typedef transaction<type> this_t;
//
//		transaction();
//		transaction(const this_t&);
//		this_t& operator=(const this_t&);
//
//		// When a transaction is submitted, m_transaction is cleared to NULL.
//		// However, it's still possible that read/write have a reference to it,
//		// and may yet queue additional reads/writes.  If there is an expectation
//		// that a read or write would not complete until after the transaction
//		// is submitted, an abort() will not complete() pending reads/write until
//		// after the transaction is submitted (at whch point they are aborted -
//		// completed with no buffers read/written).
//		volatile rcptr<transaction_internals> m_transaction;
//		transaction_submit_type m_submitType;
//
//		void register_transaction()
//		{
//			if (type == failable_transaction)
//				m_transaction->register_failable_transaction();
//			if (type == locking_transaction)
//				m_transaction->register_locking_transaction();
//		}
//
//	public:
//		friend class synchronized_file_impl<file_size_t>;
//
//		transaction(transaction_submit_type submitType, const rcref<synchronized_file<read_access, file_size_t> >& syncReable, const rcref<file<read_access> >& rawReadable, const rcref<synchronized_file_impl<file_size_t> >& impl)
//			: m_submitType(submitType),
//			m_transaction(rcnew(transaction_internals, impl->get_commit_delegate<type>(), syncReable, rawReadable, impl))
//		{ register_transaction(); }
//
//		transaction(transaction_submit_type submitType, const rcref<synchronized_file<write_access, file_size_t> >& syncWritable, const rcref<file<write_access> >& rawWritable, const rcref<synchronized_file_impl<file_size_t> >& impl)
//			: m_submitType(submitType),
//			m_transaction(rcnew(transaction_internals, impl->get_commit_delegate<type>(), syncWritable, rawWritable, impl))
//		{ register_transaction(); }
//
//		transaction(transaction_submit_type submitType, const rcref<synchronized_file<read_write_access, file_size_t> >& syncFile, const rcref<file<read_write_access> >& rawFile, const rcref<synchronized_file_impl<file_size_t> >& impl)
//			: m_submitType(submitType),
//			m_transaction(rcnew(transaction_internals, impl->get_commit_delegate<type>(), syncFile, rawFile, impl))
//		{ register_transaction(); }
//
//		transaction(transaction_submit_type submitType, const rcref<synchronized_file<read_write_access, file_size_t> >& syncFile, const rcref<file<read_access> >& rawReadable, const rcref<file<read_access> >& rawWritable, const rcref<synchronized_file_impl<file_size_t> >& impl)
//			: m_submitType(submitType),
//			m_transaction(rcnew(transaction_internals, impl->get_commit_delegate<type>(), syncFile, rawReadable, rawWritable, impl))
//		{ register_transaction(); }
//
//		void set_write_mask(const file_mask<file_size_t>& fm)
//		{
//			rcptr<transaction_internals> t = m_transaction;
//			if (!!t)
//				t->set_write_mask(fm);
//		}
//
//		void submit()
//		{
//			rcptr<transaction_internals> t;
//			m_transaction.exchange(t);
//			if (!!t)
//				t->submit();
//		}
//
//		void abort()
//		{
//			rcptr<transaction_internals> t = m_transaction;
//			if (!!t)
//				t->abort();
//		}
//
//		bool was_submitted() const { return !m_transaction; }
//		bool was_submitted_or_aborted() const
//		{
//			rcptr<transaction_internals> t = m_transaction;
//			return !t || t->was_abort_initiated();
//		}
//
//		rcref<typename file<read_access, file_size_t>::reader> read(const rcref<segment_map<file_size_t> >& sm)
//		{
//			rcptr<transaction_internals> t = m_transaction;
//			rcref<reader> r = rcnew(reader, sm, t);
//			if (!t)
//				r->complete();
//			else if (type == failable_transaction)
//				t->defer_issue_failable_read(r);
//			else if (type == composed_transaction)
//				t->queue_read(r);
//			else if (type == locking_transaction)
//				t->defer_issue_locking_read(r);
//			return r;
//		}
//
//		rcref<typename file<write_access, file_size_t>::writer> write(const rcref<segment_buffer_map<file_size_t> >& sbm)
//		{
//			rcptr<transaction_internals> t = m_transaction;
//			rcref<writer> w = rcnew(writer, sbm, t);
//			if (!t)
//				w->complete();
//			else
//				t->queue_write(w);
//			return w;
//		}
//
//		rcref<typename file<read_access, file_size_t>::size_reader> get_size()
//		{
//			rcptr<transaction_internals> t = m_transaction;
//			rcref<size_reader> sr = rcnew(size_reader, t);
//			if (!t)
//				sr->complete();
//			else if (type == failable_transaction)
//				t->defer_issue_failable_read_size(sr);
//			else if (type == composed_transaction)
//				t->queue_read_size(sr);
//			else if (type == locking_transaction)
//				t->defer_issue_locking_read_size(sr);
//			return sr;
//		}
//
//		rcref<typename file<write_access, file_size_t>::size_writer> set_size(const file_size_t& sz)
//		{
//			rcptr<transaction_internals> t = m_transaction;
//			rcref<size_writer> sw = rcnew(size_writer, sz, t);
//			if (!t)
//				sw->complete();
//			else
//				t->queue_write_size(sw);
//			return sw;
//		}
//
//	public:
//		~transaction()
//		{
//			if (m_submitType == auto_submit_transaction)
//				submit();
//			else if (m_submitType == manual_submit_transaction)
//				abort();
//		}
//	};
//
//	template <access_mode accessMode, file_transaction_type type>
//	rcref<transaction<type> > create_transaction(transaction_submit_type submitType, const rcref<synchronized_file<accessMode, file_size_t> >& syncFile, const rcref<file<accessMode, file_size_t> >& rawFile)
//	{
//		return rcnew(transaction<type>, submitType, syncFile, rawFile, this_rcref);
//	}
//
//	template <access_mode accessMode, file_transaction_type type>
//	rcref<transaction<type> > create_transaction(transaction_submit_type submitType, const rcref<synchronized_file<accessMode, file_size_t> >& syncFile, const rcref<file<io::read_access, file_size_t> >& rawReadable, const rcref<file<io::write_access, file_size_t> >& rawWritable)
//	{
//		return rcnew(transaction<type>, submitType, syncFile, rawReadable, rawWritable, this_rcref);
//	}
//
//	rcref<transaction<locking_transaction> > create_lock(const file_mask<file_size_t>& fileMask, transaction_submit_type submitType, const rcref<synchronized_file<read_write_access, file_size_t> >& syncFile, const rcref<file<io::read_access, file_size_t> >& rawReadable, const rcref<file<io::write_access, file_size_t> >& rawWritable)
//	{
//		typedef transaction<locking_transaction, type> transaction_t;
//		rcref<transaction<locking_transaction> > result = rcnew(transaction_t, submitType, syncFile, rawReadable, rawWritable, this_rcref);
//		results->set_write_mask(fileMask);
//		return result;
//	}
//
//public:
//	synchronized_file_impl()
//		: m_serialQueue(delegate_chain::create())
//	{ }
//};
//
//
//// A synchronized_file<> is a concrete class.  It is what the caller interfaces with, so it manages the lifetime
//// of the resources.  After a synchronized_file<> is released, the synchronized_file_impl it wrappers may be internally
//// referenced by pending IO operations.  Only when those IO operations are complete would the synchronized_file_impl be released.
//// Because it's important for the originator of a synchronized_file<> to always re-use the same synchronized_file_impl if the
//// same file is requested, it may need to associate an existing synchronized_file_impl (from a released synchronized_file<>) with
//// a newly created synchronized_file<>, if requested before a lingering synchronized_file_impl<> is fully released.
//template <typename file_size_t>
//class synchronized_file<read_access, file_size_t> : public file<read_access, file_size_t>
//{
//private:
//	typedef synchronized_file<read_access, file_size_t> this_t;
//
//	synchronized_file();
//	synchronized_file(const this_t&);
//	this_t& operator=(const this_t&);
//
//protected:
//	rcref<synchronized_file_impl<file_size_t> > m_impl;
//	rcref<file<read_access> > m_rawReadable;
//	weak_rcptr<this_t> m_selfRef;
//
//	virtual rcref<typename file<read_access, file_size_t>::reader> begin_read(const rcref<segment_map<file_size_t> >& sm)
//	{
//		rcptr<this_t> selfRef = m_selfRef;
//		return m_impl->create_transaction<read_access, composed_transaction>(auto_submit_transaction, selfRef.get_ref(), m_rawReadable)->read(sm);
//	}
//
//	explicit synchronized_file(const rcref<file<read_access, file_size_t> >& rawReadable, const rcref<synchronized_file_impl<file_size_t> >& impl = rcnew(synchronized_file_impl<file_size_t>))
//		: m_rawReadable(rawReadable),
//		m_impl(impl)
//	{ }
//
//	void set_self_ref(const rcref<synchronized_file<read_access, file_size_t> >& selfRef) { m_selfRef = selfRef; }
//
//public:
//	virtual rcref<size_reader> get_size()
//	{
//		rcptr<this_t> selfRef = m_selfRef;
//		return m_impl->create_transaction<read_access, composed_transaction>(auto_submit_transaction, selfRef.get_ref(), m_rawReadable)->get_size();
//	}
//
//	static rcref<synchronized_file<read_access, file_size_t> > create(const rcref<file<read_access, file_size_t> >& rawReadable, const rcref<synchronized_file_impl<file_size_t> >& impl = rcnew(synchronized_file_impl<file_size_t>))
//	{
//		rcref<this_t> syncFile = rcnew(this_t, rawReadable, impl);
//		syncFile->set_self_ref(syncFile);
//		return syncFile;
//	}
//
//	template <file_transaction_type type> // read-only transaction
//	class transaction : public file<read_access, file_size_t>
//	{
//	private:
//		typedef transaction<type> this_t;
//
//		transaction(const this_t&);
//		this_t& operator=(const this_t&);
//
//		rcref<typename synchronized_file_impl<file_size_t>::transaction<type> > m_transaction;
//
//	protected:
//		explicit transaction(const rcref<typename synchronized_file_impl<file_size_t>::transaction<type> >& t)
//			: m_transaction(t)
//		{ }
//
//		virtual rcref<typename file<read_access, file_size_t>::reader> begin_read(const rcref<segment_map<file_size_t> >& sm) { return m_transaction->read(sm); }
//
//	public:
//		void submit() { m_transaction->submit(); }
//		void abort() { m_transaction->abort(); }
//
//		bool was_submitted() const { return m_transaction->was_submitted(); }
//		bool was_submitted_or_aborted() const { return m_transaction->was_submitted_or_aborted(); }
//	};
//
//	template <file_transaction_type type>
//	rcref<transaction<type> > create_transaction(transaction_submit_type submitType = auto_submit_transaction)
//	{
//		return rcnew(transaction<type>, m_impl->create_transaction<read_access, type>(submitType, m_impl, m_rawReadable));
//	}
//};
//
//
//template <typename file_size_t>
//class synchronized_file<write_access, file_size_t> : public file<write_access, file_size_t>
//{
//protected:
//	typedef synchronized_file<write_access, file_size_t> this_t;
//
//	synchronized_file(const this_t&);
//	this_t& operator=(const this_t&);
//
//	rcref<synchronized_file_impl<file_size_t> > m_impl;
//	rcref<file<write_access, file_size_t> > m_rawWritable;
//	weak_rcptr<this_t> m_selfRef;
//
//	virtual rcref<typename file<write_access, file_size_t>::writer> begin_write(const rcref<segment_buffer_map<file_size_t> >& sbm)
//	{
//		rcptr<this_t> selfRef = m_selfRef;
//		return m_impl->create_transaction<write_access, composed_transaction>(auto_submit_transaction, selfRef.get_ref(), m_rawWritable)->write(sbm);
//	}
//
//	explicit synchronized_file(const rcref<file<write_access, file_size_t> >& rawWritable, const rcref<synchronized_file_impl<file_size_t> >& impl = rcnew(synchronized_file_impl<file_size_t>))
//		: m_rawWritable(rawWritable),
//		m_impl(impl)
//	{ }
//
//	void set_self_ref(const rcref<synchronized_file<write_access, file_size_t> >& selfRef) { m_selfRef = selfRef; }
//
//public:
//	virtual rcref<size_writer> set_size(const file_size_t& sz)
//	{
//		rcptr<this_t> selfRef = m_selfRef;
//		return m_impl->create_transaction<write_access, composed_transaction>(auto_submit_transaction, selfRef.get_ref(), m_rawWritable)->set_size(sz);
//	}
//
//	static rcref<synchronized_file<write_access, file_size_t> > create(const rcref<file<write_access, file_size_t> >& rawWritable, const rcref<synchronized_file_impl<file_size_t> >& impl = rcnew(synchronized_file_impl<file_size_t>))
//	{
//		rcref<this_t> syncFile = rcnew(this_t, rawWritable, impl);
//		syncFile->set_self_ref(syncFile);
//		return syncFile;
//	}
//
//	template <file_transaction_type type> // write-only transaction
//	class transaction : public file<write_access, file_size_t>
//	{
//	private:
//		typedef transaction<type> this_t;
//
//		transaction(const this_t&);
//		this_t& operator=(const this_t&);
//
//		typename synchronized_file_impl<file_size_t>::transaction<type> m_transaction;
//
//	protected:
//		explicit transaction(const rcref<typename synchronized_file_impl<file_size_t>::transaction<type> >& t)
//			: m_transaction(t)
//		{ }
//
//		virtual rcref<typename file<write_access, file_size_t>::writer> begin_write(const rcref<segment_buffer_map<file_size_t> >& sbm) { return m_transaction->write(sbm); }
//
//	public:
//		void submit() { m_transaction->submit(); }
//		void abort() { m_transaction->abort(); }
//
//		bool was_submitted() const { return m_transaction->was_submitted(); }
//		bool was_submitted_or_aborted() const { return m_transaction->was_submitted_or_aborted(); }
//	};
//
//	template <file_transaction_type type>
//	rcref<transaction<type> > create_transaction(transaction_submit_type submitType = auto_submit_transaction)
//	{
//		return rcnew(transaction<type>, m_impl->create_transaction<write_access, type>(submitType, m_impl, m_rawWritable));
//	}
//};
//
//template <typename file_size_t>
//class synchronized_file<read_write_access, file_size_t> : public synchronized_file<read_access, file_size_t>, public synchronized_file<write_access, file_size_t>, public file<read_write_access, file_size_t>
//{
//private:
//	typedef synchronized_file<read_write_access, file_size_t> this_t;
//
//	synchronized_file(const this_t&);
//	this_t& operator=(const this_t&);
//
//	virtual rcref<typename file<read_access, file_size_t>::reader> begin_read(const rcref<segment_map<file_size_t> >& sm) { return synchronized_file<read_access, file_size_t>::begin_read(sm); }
//	virtual rcref<typename file<write_access, file_size_t>::writer> begin_write(const rcref<segment_buffer_map<file_size_t> >& sbm) { return synchronized_file<write_access, file_size_t>::begin_write(sbm); }
//
//	explicit synchronized_file(const rcref<file<read_write_access, file_size_t> >& rawFile,
//		const rcref<synchronized_file_impl<file_size_t> >& impl = rcnew(synchronized_file_impl<file_size_t>))
//		: synchronized_file<read_access, file_size_t>(rawFile, impl),
//		synchronized_file<write_access, file_size_t>(rawFile, impl)
//	{ }
//
//	explicit synchronized_file(const rcref<file<read_access, file_size_t> >& rawReadable,
//		const rcref<file<write_access, file_size_t> >& rawWritable,
//		const rcref<synchronized_file_impl<file_size_t> >& impl = rcnew(synchronized_file_impl<file_size_t>))
//		: synchronized_file<read_access, file_size_t>(rawReadable, impl),
//		synchronized_file<write_access, file_size_t>(rawWritable, impl)
//	{ }
//
//	void set_self_ref(const rcref<synchronized_file<read_write_access, file_size_t> >& selfRef)
//	{
//		synchronized_file<read_access, file_size_t>::set_self_ref(selfRef);
//		synchronized_file<write_access, file_size_t>::set_self_ref(selfRef);
//	}
//
//public:
//	virtual rcref<typename file<read_access, file_size_t>::size_reader> get_size() { return synchronized_file<read_access, file_size_t>::get_size(); }
//	virtual rcref<typename file<write_access, file_size_t>::size_writer> set_size(const file_size_t& sz) { return synchronized_file<write_access, file_size_t>::set_size(sz); }
//
//	static rcref<synchronized_file<read_write_access, file_size_t> > create(const rcref<file<read_access, file_size_t> >& rawReadable, const rcref<file<write_access, file_size_t> >& rawWritable, const rcref<synchronized_file_impl<file_size_t> >& impl = rcnew(synchronized_file_impl<file_size_t>))
//	{
//		rcref<this_t> syncFile = rcnew(this_t, rawReadable, rawWritable, impl);
//		syncFile->set_self_ref(syncFile);
//		return syncFile;
//	}
//
//	static rcref<synchronized_file<read_write_access, file_size_t> > create(const rcref<file<read_write_access, file_size_t> >& rawFile, const rcref<synchronized_file_impl<file_size_t> >& impl = rcnew(synchronized_file_impl<file_size_t>))
//	{
//		return create(rawFile, rawFile, impl);
//	}
//
//	template <file_transaction_type type> // read-write transaction
//	class transaction : public file<read_access, file_size_t>::transaction<type>, public file<write_access, file_size_t>::transaction<type>, public file<read_write_access, file_size_t>
//	{
//	private:
//		typedef transaction<type> this_t;
//
//		transaction(const this_t&);
//		this_t& operator=(const this_t&);
//
//	protected:
//		explicit transaction(const rcref<typename synchronized_file_impl<file_size_t>::transaction<type> >& t)
//			: file<read_access, file_size_t>::transaction<type>(t),
//			file<write_access, file_size_t>::transaction<type>(t)
//		{ }
//
//	public:
//		void submit() { m_transaction->submit(); }
//		void abort() { m_transaction->abort(); }
//
//		bool was_submitted() const { return m_transaction->was_submitted(); }
//		bool was_submitted_or_aborted() const { return m_transaction->was_submitted_or_aborted(); }
//	};
//
//	template <file_transaction_type type>
//	rcref<transaction<type> > create_transaction(transaction_submit_type submitType = auto_submit_transaction)
//	{
//		return rcnew(transaction<type>, m_impl->create_transaction<read_write_access, type>(submitType, m_impl, m_rawReadable, m_rawWritable));
//	}
//
//	rcref<transaction<locking_transaction> > create_lock(const file_mask<file_size_t>& fileMask, transaction_submit_type submitType = auto_submit_transaction)
//	{
//		return rcnew(transaction<locking_transaction>, m_impl->create_lock(fileMask, submitType, m_impl, m_rawReadable, m_rawWritable));
//	}
//
//	// redefined here to resolve multiple base class ambiguity
//	rcref<writer> write(file_size_t offset, void* b, size_t n) { return file<read_write_access, file_size_t>::write(offset, b, n); }
//	rcref<writer> write(file_size_t offset, const const_buffer& b) { return file<read_write_access, file_size_t>::write(offset, b); }
//	rcref<writer> write(file_size_t offset, const composite_buffer& b) { return file<read_write_access, file_size_t>::write(offset, b); }
//	rcref<writer> write(const rcref<segment_buffer_map<file_size_t> >& sbm) { return file<read_write_access, file_size_t>::write(sbm); }
//
//	rcref<reader> read(file_size_t offset, typename file<read_write_access, file_size_t>::buffer_size_t n)
//	{ return file<read_write_access, file_size_t>::read(offset, n); }
//
//	rcref<reader> read(const segment<file_size_t>& s) { return file<read_write_access, file_size_t>::read(s); }
//	rcref<reader> read(const rcref<segment_map<file_size_t> >& sm) { return file<read_write_access, file_size_t>::read(sm); }
//};
//
//
//*/
//
///*
//	class cursor : public datasource, public object
//	{
//	protected:
//		const weak_rcptr<file<read_access, file_size_t> > m_source;
//		file_size_t m_curPos;
//
//		const rcref<queue>& get_io_queue() const { return m_ioQueue; }
//
//	private:
//		class cursor_reader : public datasource::reader
//		{
//		protected:
//			const weak_rcptr<cursor> m_cursor;
//
//		public:
//			cursor_reader(const rcref<cursor>& c)
//				: m_cursor(c),
//				datasource::reader(c)
//			{ }
//
//			virtual void reading()
//			{
//				rcptr<cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					rcptr<file<read_access> > f = c->m_source;
//					if (!f)
//						close();
//					else
//						f->read(c->m_curPos, get_requested_size())->dispatch(file<read_access>::reader::dispatch_t(&cursor_reader::read_done, this_rcref));
//				}
//			}
//
//			void read_done(const rcref<const file<read_access>::reader>& r)
//			{
//				m_bufferList = r->get_composite_buffer();
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					c->m_curPos += m_bufferList.size();
//					complete();
//				}
//			}
//		};
//		friend class cursor_reader;
//
//		virtual rcref<datasource::reader> create_reader()
//		{
//			return rcnew(cursor_reader, this_rcref);
//		}
//
//	public:
//		class seeker : public queue::operation
//		{
//		protected:
//			friend class read_only_cursor;
//
//			const weak_rcptr<read_only_cursor> m_cursor;
//			file_size_t m_position;
//			bool m_succeeded;
//
//			seeker(file_size_t pos, const rcref<read_only_cursor>& c)
//				: queue::operation(c->get_io_queue()),
//				m_cursor(c),
//				m_position(pos),
//				m_succeeded(false)
//			{ }
//
//			void complete() { m_succeeded = true; queue::operation::complete(); }
//			void close() { queue::operation::close(); }
//			void enqueue() { queue::operation::enqueue(); }
//
//			virtual void execute()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					c->m_curPos = m_position;
//					complete();
//				}
//			}
//
//		public:
//			bool succeeded() const { return m_succeeded; }
//		};
//
//		class teller : public queue::operation
//		{
//		protected:
//			friend class file<read_access>;
//			friend class read_only_cursor;
//
//			const weak_rcptr<read_only_cursor> m_cursor;
//			file_size_t m_position;
//			bool m_succeeded;
//
//			teller(const rcref<read_only_cursor>& c)
//				: queue::operation(c->get_io_queue()),
//				m_cursor(c),
//				m_succeeded(false)
//			{ }
//
//			void complete() { m_succeeded = true; queue::operation::complete(); }
//			void close() { queue::operation::close(); }
//			void enqueue() { queue::operation::enqueue(); }
//
//			virtual void execute()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					m_position = c->m_curPos;
//					complete();
//				}
//			}
//
//		public:
//			// Returns false on failure.  (Should only fail if file was closed).
//			bool get_position(uint64_t& dst) const
//			{
//				if (m_position == (file_size_t)-1)
//					return false;
//				dst = m_position;
//				return true;
//			}
//		};
//
//	protected:
//		class eof_seeker : public seeker
//		{
//		public:
//			eof_seeker(const rcref<read_only_cursor>& c)
//				: seeker(0, c)
//			{ }
//
//			virtual void execute()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					rcptr<file<read_access> > f = c->m_file;
//					if (!f)
//						close();
//					else
//					{
//						m_position = f->eof();
//						c->m_curPos = m_position;
//						complete();
//					}
//				}
//			}
//		};
//		
//		class eof_teller : public teller
//		{
//		public:
//			eof_teller(const rcref<read_only_cursor>& c)
//				: teller(c)
//			{ }
//
//			virtual void execute()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					rcptr<file<read_access> > f = c->m_file;
//					if (!f)
//						close();
//					else
//					{
//						m_position = f->eof();
//						complete();
//					}
//				}
//			}
//		};
//
//	public:
//		rcref<seeker> seek(file_size_t pos)
//		{
//			rcref<seeker> skr = rcnew(seeker, pos, this_rcref);
//			skr->enqueue();
//			return skr;
//		}
//
//		rcref<seeker> seek_end()
//		{
//			rcref<seeker> skr = rcnew(eof_seeker, this_rcref);
//			skr->enqueue();
//			return skr;
//		}
//
//		rcref<teller> tell()
//		{
//			rcref<teller> tlr = rcnew(teller, this_rcref);
//			tlr->enqueue();
//			return tlr;
//		}
//
//		rcref<teller> get_eof()
//		{
//			rcref<teller> tlr =  rcnew(eof_teller, this_rcref);
//			tlr->enqueue();
//			return tlr;
//		}
//
//	protected:
//		read_only_cursor(const rcref<file>& f, const rcref<queue>& ioQueue = queue::create())
//			: datasource(ioQueue),
//			m_file(f)
//		{ }
//	};
//
//
//
//
//template <typename file_size_t>
//class file_mask
//{
//private:
//	// Note: If a locking_transaction needs to shrink a file, its mask must include
//	// the area between the new EOF, the old EOF, as well as past the EOF.
//
//	typedef file_mask<file_size_t> this_t;
//
//	file_mask(const this_t&);
//	this_t& operator=(const this_t&);
//	
//	// A file mask can be either inclusize or exclusive.  In other words, a file mask
//	// can either identify a portion of a file, or everything but a portion of the file.
//	// 
//	// When in inclusion mode, an exclusion will remove a range from the inclusion list,
//	// and a inclusion will add a range to the inclusion list.
//	//
//	// When in exclusion mode, an exclusion will add a range to the exclusion list,
//	// and a inclusion will remove the range from the exclusion list.
//	// 
//	// The only way to switch modes between inclusion and exclusion mode, is to call either
//	// set_to_all(), or clear().
//
//	segment_map<file_size_t> m_segments;
//	bool m_excludeMode; // otherwise in include mode
//	bool m_pastEof; // true if mask considers the (unknown) EOF position
//
//	template <bool exclude>
//	void add_remove(const segment<file_size_t>& s)
//	{
//		if (m_excludeMode == exclude)
//			m_segments.add(s);
//		else
//			m_segments.remove(s);
//	}
//
//	template <bool exclude>
//	void add_remove(const segment_map<file_size_t>& sm)
//	{
//		if (m_excludeMode == exclude)
//		{
//			segment_map<file_size_t>::iterator itor = sm.get_first();
//			while (!!itor)
//			{
//				m_segments.add(*itor);
//				++itor;
//			}
//		}
//		else
//		{
//			segment_map<file_size_t>::iterator itor = sm.get_first();
//			while (!!itor)
//			{
//				m_segments.remove(*itor);
//				++itor;
//			}
//		}
//	}
//
//	file_mask(bool excludeMode, bool spanPastEof)
//		: m_excludeMode(excludeMode),
//		m_pastEof(spanPastEof)
//	{ }
//
//public:
//	file_mask()
//		: m_excludeMode(false),
//		m_pastEof(false)
//	{ }
//
//	static this_t all(bool spanPastEof = true) { return this_t(true, !spanPastEof); }
//	static this_t contents() { return this_t(true, true); }
//	static this_t eof() { return this_t(false, true); }
//		
//	void clear()
//	{
//		m_excludeMode = true;
//		m_pastEof = false;
//		m_segments.clear();
//	}
//
//	void set_to_all(bool spanPastEof = true)
//	{
//		m_excludeMode = true;
//		m_pastEof = !includePastEof;
//		m_segments.clear();
//	}
//
//	void set_to_contents()
//	{
//		m_excludeMode = true;
//		m_pastEof = true;
//		m_segments.clear();
//	}
//
//	void set_to_eof()
//	{
//		m_excludeMode = false;
//		m_pastEof = true;
//		m_segments.clear();
//	}
//
//	void include(const segment<file_size_t>& s) { add_remove<false>(s); }
//	void exclude(const segment<file_size_t>& s) { add_remove<true>(s); }
//		
//	void include(const segment_map<file_size_t>& sm) { add_remove<false>(sm); }
//	void exclude(const segment_map<file_size_t>& sm) { add_remove<true>(sm); }
//
//	void include_past_eof() { m_pastEof = !m_excludeMode; }
//	void exclude_past_eof() { m_pastEof = m_excludeMode; }
//
//	// does_overlap assumes both masks are of the same file.
//	bool does_overlap(const this_t& fm, const file_size_t& currentEof) const
//	{
//		if (m_excludeMode && !fm.m_excludeMode)
//			return fm.does_overlap(*this, currentEof); // Flip to ensures rest of algoritm doesn't need to worry about (m_excludeMode && !fm.m_excludeMode)
//		
//		if (!m_excludeMode)
//		{
//			if (!fm.m_excludeMode) // && (!m_excludeMode) 
//			{ // both in include mode.
//				if (!!m_pastEof)
//				{
//					if (!!fm.m_pastEof)
//						return true;
//					segment_map<file_size_t>::iterator lastItor = fm.m_segments.get_last();
//					if (!lastItor)
//						return false; // no overlap if nothing is in there.
//					if (lastItor->get_end() > currentEof)
//						return true;
//				}
//				else if (!!fm.m_pastEof)
//				{
//					segment_map<file_size_t>::iterator lastItor = m_segments.get_last();
//					if (!lastItor)
//						return false; // no overlap if nothing is in there.
//					if (lastItor->get_end() > currentEof)
//						return true;
//				}
//
//				return m_segments.does_overlap(fm.m_segments);
//			}
//			else // if ((!m_excludeMode) && (!!fm.m_excludeMode)) // If one is in include mode and the other is in exclude mode.
//			{
//				if (!fm.m_pastEof) // if excluding one does not exclude past EOF, there is overlap.
//				{
//					if (!!m_pastEof)
//						return true;
//					segment_map<file_size_t>::iterator lastItor = m_segments.get_last();
//					if (!lastItor) // nothing in list and doesn't care about pastEof, so empty.  No overlap.
//						return false;
//					if (lastItor->get_end() > currentEof)
//						return true;
//				}
//				// Check if everything up until the EOF in the include list is fully contained in exclude list.
//				segment_map<file_size_t>::iterator itor = m_segments.get_first();
//				while (!!itor)
//				{
//					segment<file_size_t> curSegment = *itor;
//					if (curSegment.get_start() >= currentEof)
//						return false;
//					
//					file_size_t curSegmentEnd = curSegment.get_end(); // Clip end if necessary
//					if (curSegmentEnd > currentEof)
//						curSegment.m_length -= (curSegmentEnd - currentEof);
//
//					if (!fm.m_segments.does_fully_contain(curSegment))
//						return true;
//					++itor;
//				}
//				return false;
//			}
//		}
//		else // if ((!!m_excludeMode) && (!!fm.m_excludeMode))
//		{ // Both masks are in exclude mode.
//			if (!m_pastEof && !fm.m_pastEof) // If both don't exclude past the EOF, they overlap.
//				return true;
//			if (!currentEof) // empty file, don't care about segments
//				true false;
//
//			file_size_t curPos = 0;
//			segment_map<file_size_t>::iterator itor = m_segments.get_first(); // Overlaps are fine.  Adjacent are fine.
//			segment_map<file_size_t>::iterator itor2 = fm.m_segments.get_first(); // But any gaps imply an overlap.
//			if (!itor) // Empty itor means nothing is excluded, so any gaps in itor2 before EOF implies an overlap.		
//				return ((!itor2) // If no itor2 either, nothing is excluded from either.  So they overlap.
//					|| (itor2->get_start() > curPos) // gap found before.
//					|| (itor2->get_end() < currentEof)); // gap found after
//
//			if (!itor2) // Empty itor2 means nothing is excluded, so any gaps in itor before EOF implies an overlap.
//				return ((itor->get_start() > curPos) // gap found before.
//					|| (itor->get_end() < currentEof)); // gap found after
//
//			for (;;)
//			{
//				if (itor->get_start() <= itor2->get_start()) // itor starts first
//				{
//					if (itor->get_start() > curPos) // found a gap
//						return true;
//					file_size_t itorEnd = itor->get_end();
//					if (itorEnd > curPos)
//					{
//						curPos = itorEnd;
//						if (curPos >= currentEof)
//							return false;
//					}
//					if (!++itor)
//						return (itor2->get_end() < currentEof);
//				}
//				else // if (itor2->get_start() < itor->get_start()) // itor2 starts first
//				{
//					if (itor2->get_start() > curPos) // found a gap
//						return true;
//					file_size_t itor2End = itor2->get_end();
//					if (itor2End > curPos)
//					{
//						curPos = itor2End;
//						if (curPos >= currentEof)
//							return false;
//					}
//					if (!++itor2)
//						return (itor->get_end() < currentEof);
//				}
//				//continue;
//			}
//		}
//	}
//};
//
//
//template <typename file_size_t>
//class file<read_write_access> : public file<read_access, file_size_t>, public file<write_access, file_size_t>
//{
//public:
//	typedef file<read_access>::reader reader;
//	typedef file<read_access>::writer writer;
////	typedef file<read_access>::locking_transaction locking_transaction;
////	typedef file<read_access>::failable_transaction failable_transaction;
//
//	class composed_transaction : public file<read_access>::composed_transaction
//	{
//	protected:
//		friend class file<read_write_access>;
//
//		composed_transaction(const rcref<file<read_write_access> >& f)
//			: file<read_access>::composed_transaction(f)
//		{ }
//
//	public:
//		rcref<writer> write(file_size_t offset, size_t n)
//		{
//			rcptr<file<read_write_access> > f = m_file;
//			rcref<writer> w = (!f) ? rcnew(writer) : f->create_writer();
//			m_transaction->m_writerList.insert(w);
//			return w;
//		}
//	};
//
//	rcref<composed_transaction> create_read_write_composed_transaction()
//	{
//		return rcnew(composed_transaction, this_rcref);
//	}
//
//	class failable_transaction : public file<read_access>::failable_transaction
//	{
//	protected:
//		friend class file<read_write_access>;
//
//		failable_transaction(const rcref<file<read_write_access> >& f)
//			: file<read_access>::failable_transaction(f)
//		{ }
//
//	public:
//		rcref<writer> write(file_size_t offset, size_t n)
//		{
//			rcptr<file<read_write_access> > f = m_file;
//			rcref<writer> w = (!f) ? rcnew(writer) : f->create_writer();
//			m_transaction->m_writerList.insert(w);
//			return w;
//		}
//	};
//
//	rcref<failable_transaction> create_read_write_failable_transaction()
//	{
//		return rcnew(failable_transaction, this_rcref);
//	}
//
//	class locking_transaction : public file<read_access>::locking_transaction
//	{
//	protected:
//		friend class file<read_write_access>;
//
//		locking_transaction(const rcref<file<read_write_access> >& f)
//			: file<read_access>::locking_transaction(f)
//		{ }
//
//	public:
//		rcref<writer> write(file_size_t offset, size_t n)
//		{
//			rcptr<file<read_write_access> > f = m_file;
//			rcref<writer> w = (!f) ? rcnew(writer) : f->create_writer();
//			m_transaction->m_writerList.insert(w);
//			return w;
//		}
//	};
//
//	rcref<locking_transaction> create_read_write_locking_transaction()
//	{
//		return rcnew(locking_transaction, this_rcref);
//	}
//
//	enum class create_mode
//	{
//		open_if_exists = 0x01,
//		create_only = 0x02,
//		open_or_create = 0x03,
//		open_truncate_or_create = 0x07,
//
////		open_mask = 0x01,
////		create_mask = 0x02,
////		truncate_mask = 0x04,
//	};
//
//	// Implemented at cogs::os level, to use os::file derived class.
//	static rcptr<file<read_write_access> > open(const string& location, create_mode mode = open_if_exists);
//	
//protected:
//	virtual rcref<writer> create_writer() = 0;
//};
//
//
//
//template <typename file_size_t>
//class file<read_write_access>
//	
//
////------------------------
//
//	class segment_map_base
//	{
//	private:
//		segment_map_base(const segment_map_base&);
//		segment_map_base& operator=(const segment_map_base&);
//
//		typedef state_map_node node;
//		typedef sorted_list<file_size_t, true, node> list_t;
//
//		list_t	m_list;
//
//		node* split_off_after_at(node& n, file_size_t p)
//		{
//		}
//
//		void clear_inner()
//		{
//			node* n = m_list.get_first_postorder();
//			while (!!n)
//			{
//				node* n2 = node::get_next_postorder(n);
//				default_allocator::destruct_deallocate_type(n.get());
//				n = n2;
//			}
//		}
//
//	protected:
//		segment_map_base()
//		{ }
//
//		segment_map_base(const segment_buffer& sb)
//		{
//			m_list.insert(new (default_allocator::get()) node(sb));
//		}
//
//		~segment_list_base() { clear_inner(); }
//
//		void clear()
//		{
//			clear_inner();
//			m_list.clear();
//		}
//
//		bool is_empty() const { return !m_list; }
//		bool operator!() const { return !m_list; }
//
//		// An iterator is guaranteed to remain valid until the element reference to had been removed,
//		// completely overwritten (segment_buffer_map), or if a segment is added (to a segment_map) that
//		// causes coalescing to occuring removing the element referenced by the iterator.
//		class iterator
//		{
//		private:
//			ptr<node> m_node;
//
//		protected:
//			iterator(const ptr<node>& n) : m_node(n) { }
//
//			friend class segment_map_base;
//
//		public:
//			iterator() { }
//
//			iterator(const iterator& i) : m_node(i.m_node) { }
//
//			iterator& operator++() { if (!!m_node) m_node = node::get_next(m_node); return *this; }
//			iterator& operator--() { if (!!m_node) m_node = node::get_prev(m_node); return *this; }
//
//			bool operator!() const { return !m_node; }
//
//			bool operator==(const iterator& i) const { return m_node == i.m_node; }
//			bool operator!=(const iterator& i) const { return m_node != i.m_node; }
//
//			segment_buffer* get() const { return (!m_node) ? (segment_t*)0 : &(m_node->m_contents); }
//			segment_buffer& operator*() const { return m_node->m_contents; }
//			segment_buffer* operator->() const { return &(m_node->m_contents); }
//			
//			void clear() { m_node = 0; }
//
//			iterator& operator=(const iterator& i) { m_node = i.m_node; return *this; }
//		};
//		
//		void add_segment(const segment& s) const // Only called from segment_map.  Ignores buffers
//		{
//			file_size_t trailingEnd;
//
//			// Find first one that starts before or at the end of the new segment.
//			node* trailingNode = m_list.find_any_equal_or_nearest_less_than(s.get_end());
//			bool overlapping = false;
//			if (!!trailingNode)
//			{
//				trailingEnd = trailingNode->get_end();
//				overlapping = (trailingEnd >= s.m_start);
//			}
//			if (!overlapping)
//			{
////				++m_count;
//				m_list.insert(new (default_allocator::get()) node(s));
//			}
//			else // trailingEnd will be set
//			{
//				bool doneMerge = false;
//				if ((trailingEnd != s.m_start) && (s.m_start < trailingNode->m_start))
//				{
//					node* leadingNode = m_list.find_any_equal_or_nearest_less_than(s.m_start); // We know leadingNode != trailingNode
//					if (!leadingNode)
//						leadingNode = m_list.get_first(); // Might be some cruft overlapping, so start at beginning cleaning stuff out.
//					else if (leadingNode->get_end() < s.m_start) // If leadingNode is well before us.
//						++leadingNode;
//					else // If leadingNode overlaps or is adjacent
//					{
//						leadingNode->m_segment.merge(s);
//						trailingNode->m_segment.merge(leadingNode);
//						doneMerge = true;
//					}
//					// Delete all the junk starting at leadingNode, up to and not including trailingNode.
//					while (leadingNode != trailingNode)
//					{
//						node* next = node::get_next(*leadingNode);
//						m_list.remove(leadingNode);
//						default_allocator::destruct_deallocate_type(leadingNode);
////						--m_count;
//						leadingNode = next;
//					}
//				}
//				if (!doneMerge)
//					trailingNode->m_segment.merge(s);
//			}
//		}
//
//		// add_buffer() 
//		iterator add_buffer(const segment_buffer& sb) const // Only called from segment_buffer_map
//		{
//			;
//		}
//
//		// split_create() ensures there are blocks encompassing the entirety of the specified segment.
//		// If any blocks are found that span the borders, they are split such that the start and end
//		// positions of the specified segment will corespond with the start and end of blocks.
//		iterator split_create(const segment& s) const // Only called from file_state_map
//		{
//			;
//		}
//
//		void remove(const iterator& itor)
//		{
//			m_list.remove(itor.m_node);
//			default_allocator::destruct_deallocate_type(itor.m_node.get());
//			itor.m_node = 0;
//			//--m_count;
//		}
//
//		iterator get_first() const { return iterator(m_list.get_first()); }
//		iterator get_last() const { return iterator(m_list.get_last()); }
//
//		iterator find(const file_size_t& n) const { return iterator(m_list.find_any_equal(n)); }
//		iterator find_any_before(const file_size_t& n) const { return iterator(m_list.find_any_less_than(n)); }
//		iterator find_any_after(const file_size_t& n) const { return iterator(m_list.find_any_greater_than(n)); }
//		iterator find_equal_or_any_before(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_less_than(n)); }
//		iterator find_equal_or_any_after(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_greater_than(n)); }
//		iterator find_nearest_before(const file_size_t& n) const { return iterator(m_list.find_nearest_less_than(n)); }
//		iterator find_nearest_after(const file_size_t& n) const { return iterator(m_list.find_nearest_greater_than(n)); }
//		iterator find_equal_or_nearest_before(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_nearest_less_than(n)); }
//		iterator find_equal_or_nearest_after(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_nearest_greater_than(n)); }
//	};
//
//public:
//	// segment_map will coalesc segments as they are added.
//	class segment_map : public segment_map_base
//	{
//	private:
//		segment_map(const segment_map&);
//		segment_map& operator=(const segment_map&);
//
//	public:
//		segment_map()
//		{ }
//
//		class iterator
//		{
//		private:
//			segment_map_base::iterator m_itor;
//
//		protected:
//			friend class segment_map;
//
//			iterator(const segment_map_base::iterator& i) : m_itor(i) { }
//
//		public:
//			iterator() { }
//			iterator(const iterator& i) : m_itor(i.m_itor) { }
//
//			iterator& operator=(const iterator& i) { m_itor = i.m_itor; return *this; }
//
//			iterator& operator++() { ++m_itor; return *this; }
//			iterator operator++(int) { return m_itor++; }
//
//			iterator operator--() { return --m_itor; }
//			iterator operator--(int) { return m_itor--; }
//
//			bool operator!() const { return !m_itor; }
//		
//			bool operator==(const iterator& i) const { return m_itor == i.m_itor; }
//			bool operator!=(const iterator& i) const { return !operator==(i); }
//
//			const segment* get() const { return &(m_itor->get_segment()); }
//			const segment& operator*() const { return (m_itor->get_segment()); }
//			const segment* operator->() const { return &(m_itor->get_segment()); }
//
//			void clear() { m_itor.clear(); }
//		};
//	};
//
//	// segment_map will not do any coalescing of segments, only dividing.
//	class segment_buffer_map : public segment_map_base
//	{
//	private:
//		segment_buffer_map(const segment_buffer_map&);
//		segment_buffer_map& operator=(const segment_buffer_map&);
//
//	public:
//		segment_buffer_map()
//		{ }
//
//		class iterator
//		{
//		private:
//			segment_map_base::iterator m_itor;
//
//		protected:
//			friend class segment_buffer_map;
//
//			iterator(const segment_map_base::iterator& i) : m_itor(i) { }
//
//		public:
//			iterator() { }
//			iterator(const iterator& i) : m_itor(i.m_itor) { }
//
//			iterator& operator=(const iterator& i) { m_itor = i.m_itor; return *this; }
//
//			iterator& operator++() { ++m_itor; return *this; }
//			iterator operator++(int) { return m_itor++; }
//
//			iterator operator--() { return --m_itor; }
//			iterator operator--(int) { return m_itor--; }
//
//			bool operator!() const { return !m_itor; }
//		
//			bool operator==(const iterator& i) const { return m_itor == i.m_itor; }
//			bool operator!=(const iterator& i) const { return !operator==(i); }
//
//			const segment_buffer* get() const { return m_itor->get(); }
//			const segment_buffer& operator*() const { return *m_itor; }
//			const segment_buffer* operator->() const { return m_itor->get(); }
//
//			void clear() { m_itor.clear(); }
//		};
//	};
//};
//*/
///*
//template <unsigned int file_size_bits>
//class file<read_access, file_size_bits> : public object
//{
//public:
//	typedef file<read_access, file_size_bits> this_t;
//
//	typedef bits_to_uint_t<file_size_bits> file_size_t;
//
//	class reader;
//	class read_only_cursor;
//	class composed_transaction;
//	class locking_transaction;
//	class failable_transaction;
//
//	//class segment;
//	//class segment_buffer;
//	//class segment_map;
//	//class segment_buffer_map;
//
//private:
//	// A segment represents a start position and end/length.
//	// segment_base implements logic common to various segment types, which store their lengths in
//	// differnet associated object such as buffer or composite_buffer.  This avoids storing a the length
//	// redundantly.
//	template <typename aux_t = file_size_t>
//	class segment_base
//	{
//	private:
//		typedef segment_base<aux_t> this_t;
//
//		// Wrapper class for file_size_t to give it the same interface as a buffer or composite_buffer.
//		class length_aux_t
//		{
//		private:
//			file_size_t m_length;
//			
//		private:
//			length_aux_t(const file_size_t& length)
//				: m_length(length)
//			{ }
//
//			// The following defines the minimum interface for any classes used with segment_base<>.
//
//			length_aux_t(const length_aux_t& src) // copy constructor
//				: m_length(src.m_length)
//			{ }
//
//			length_aux_t& operator=(const length_aux_t& src) { m_length = src.m_length; }
//
//			file_size_t get_length() const { return m_length; }
//
//			length_aux_t split_off_before(file_size_t n) { return length_aux_t(n); }
//
//			length_aux_t split_off_after(file_size_t n)
//			{
//				length_aux_t result(m_length - n);
//				m_length = n;
//				return result;
//			}
//
//			void advance(file_size_t n) { m_length -= n; }
//		};
//
//		typedef std::conditional_t<std::is_same_v<aux_t, file_size_t>, length_aux_t, aux_t>	internal_t;
//
//	public:
//		file_size_t m_start;
//		internal_t m_aux;
//
//		segment_base()
//		{ }
//
//		segment_base(const this_t& s)
//			: m_start(s.m_start),
//			m_aux(s.m_aux)
//		{ }
//
//		segment_base(const file_size_t& start, const aux_t& aux)
//			: m_start(start),
//			m_aux(aux)
//		{ }
//		
//		const file_size_t get_start() const { return m_start; }
//		const file_size_t get_length() const { return m_aux.get_length(); }
//		const file_size_t get_end() const { return m_start + get_length(); }
//
//		const segment<file_size_t> get_segment() const { return segment<file_size_t>(get_start(), get_length()); }
//
//		this_t split_off_after(file_size_t n)
//		{
//			if (n >= m_length)
//			{
//				this_t result;
//				return result;
//			}
//
//			this_t result(m_start + n, m_aux.split_off_after(n));
//			return result;
//		}
//
//		this_t split_off_before(file_size_t n)
//		{
//			if (n > m_length)
//				n = m_length;
//
//			this_t result(m_start, m_aux.split_off_before(n));
//			m_start += n;
//			m_length -= n;
//			return result;
//		}
//		
//		// merge() stores the union of both segments.  It's likely an error to use merge() with
//		// segments that are not overlaping or adjacent
//		void merge(const this_t& s)
//		{
//			if (s.m_start <= m_start)
//			{
//				file_size_t gap = m_start - s.m_start;
//				if (!!gap) // At this point, s starts before this.  We need to add the portion of s that starts before us.
//				{
//					;
//
//					m_length += gap;
//					m_start = s.m_start;
//				}
//				if (m_length < s.m_length)
//					m_length = s.m_length;
//			}
//			else // if (m_start < s.m_start)
//			{
//				file_size_t sEnd = s.get_end();
//				if (sEnd > get_end())
//					m_length = sEnd - m_start;
//			}
//		}
//
//		bool operator<(const segment& s2) const
//		{
//			return m_start < s2.m_start;
//		}
//	};
//	
//	template <typename aux_t> // aux_t can be buffer, composite_buffer, or void
//	class segment_base
//	{
//	public:
//		file_size_t m_start;
//		composite_buffer m_buffer;
//
//		segment_buffer()
//		{ }
//
//		segment_buffer(const segment_buffer& sb)
//			: m_start(sb.m_start),
//			m_buffer(sb.m_buffer)
//		{ }
//
//		segment_buffer(const file_size_t start, const composite_buffer& buffer)
//			: m_start(start),
//			m_buffer(buffer)
//		{ }
//
//		const segment get_segment() const { return segment(m_start, m_buffer.size()); }
//		const file_size_t get_end() const { return m_start + m_buffer.size(); }
//
//		// merge() stores the union of both segment_buffers.  It's an error to use merge() with
//		// segment_buffers that are not overlaping or adjacent.
//		void merge(const segment_buffer& s)
//		{
//			composite_buffer newBuffer;
//			if (s.m_start <= m_start)
//			{
//				file_size_t gap = m_start - s.m_start; // will be <= s.m_buffer.size()
//				m_start = s.m_start;
//				newBuffer = s.m_buffer;
//				if (s.m_buffer.size() < gap + m_buffer.size())
//				{
//					m_buffer.advance(s.m_buffer.size() - gap);
//					newBuffer.append(m_buffer);
//				}
//				m_buffer = newBuffer;
//			}
//			else // if (m_start < s.m_start)
//			{
//				newBuffer = m_buffer;
//				newBuffer.truncate_to(s.m_start - m_start);
//				newBuffer.append(s.m_buffer);
//				file_size_t sEnd = s.get_end();
//				if (sEnd < get_end())
//				{
//					m_buffer.advance(sEnd - m_start);
//					newBuffer.append(m_buffer);
//				}
//			}
//			m_buffer = newBuffer;
//		}
//
//
//		segment_buffer split_off_after(size_t n)
//		{
//			segment_buffer result;
//			result.m_start = m_start + n;
//			result.m_buffer = m_buffer.split_back(n);
//			return result;
//		}
//
//		segment_buffer split_off_before(size_t n)
//		{
//			segment_buffer result;
//			result.m_start = m_start;
//			result.m_buffer = m_buffer.split_front(n);
//			m_start += n;
//			return result;
//		}
//
//		bool operator<(const segment_buffer& s2) const
//		{
//			return m_start < s2.m_start;
//		}
//	};
//
//	// Since segment_list and vector_buffer are so similar, segment_list_base combines them
//	// into a common utility class.
//	template <bool hasBufferList = false>
//	class segment_list_base
//	{
//	private:
//		segment_list_base(const segment_list_base<hasBufferList>&);
//		segment_list_base<hasBufferList>& operator=(const segment_list_base<hasBufferList>&);
//
//		typedef conditional<hasBufferList, segment_buffer, segment>	segment_t;
//
//		class node : public sorted_list_node<false, node>
//		{
//		public:
//			segment_t m_segment;
//
//			node(const segment_t& s)
//				: m_segment(s)
//			{ }
//		};
//
//		typedef sorted_list<file_size_t, false, node> list_t;
//		list_t m_list;
//		size_t m_count;
//
//		void clear_inner()
//		{
//			node* n = m_list.get_first_postorder();
//			while (!!n)
//			{
//				node* n2 = node::get_next_postorder(n);
//				default_allocator::destruct_deallocate_type(n.get());
//				n = n2;
//			}
//		}
//
//	public:
//		// An iterator is only guaranteed to remain valid until the next write to the list.
//		// This is because the contents of the list may be modified (nodes coalesced) on write.
//		class iterator
//		{
//		private:
//			ptr<node> m_node;
//
//		protected:
//			iterator(const ptr<node>& n) : m_node(n) { }
//
//			friend class segment_list_base<hasBufferList>;
//
//		public:
//			iterator() { }
//
//			iterator(const iterator& i) : m_node(i.m_node) { }
//
//			iterator& operator++() { if (!!m_node) m_node = node::get_next(m_node); return *this; }
//			iterator& operator--() { if (!!m_node) m_node = node::get_prev(m_node); return *this; }
//
//			bool operator!() const { return !m_node; }
//
//			bool operator==(const iterator& i) const { return m_node == i.m_node; }
//			bool operator!=(const iterator& i) const { return m_node != i.m_node; }
//
//			segment_t* get() const { return (!m_node) ? (segment_t*)0 : &(m_node->m_contents); }
//			segment_t& operator*() const { return m_node->m_contents; }
//			segment_t* operator->() const { return &(m_node->m_contents); }
//
//			void clear() { m_node = 0; }
//
//			iterator& operator=(const iterator& i) { m_node = i.m_node; return *this; }
//		};
//
//		segment_list_base() : m_count(0) { }
//		segment_list_base(const segment_t& s) : m_count(1) { m_list.insert(new (default_allocator::get()) node(s)); }
//
//		~segment_list_base() { clear_inner(); }
//
//		const size_t count() const { return m_count; }
//		bool is_empty() const { return !m_count; }
//
//		void clear()
//		{
//			clear_inner();
//			m_list.clear();
//			m_count = 0;
//		}
//
//		bool does_overlap(const segment& s) const
//		{
//			node* n = m_list.find_nearest_less_than(s.get_end());
//			// If nothing starts before the end of this segment, then there is no overlap.
//			// Otherwise, it overlaps if it ends after we start.
//			return (!!n) && (n->m_segment.get_end() > s.m_start);
//		}
//
//		template <bool hasBufferList2>
//		bool does_overlap(const segment_list_base<hasBufferList2>& slh) const
//		{
//			// The shorter list should iterate through all blocks.
//			// The longer list should be doing binary lookups, in inner loop.
//			if (m_count > slh.m_count)
//				return slh.does_overlap(*this);
//
//			node* n = m_list.get_first();
//			while (!!n)
//			{
//				if (slh.does_overlap(n->m_segment.get_segment()))
//					return true;
//				n = node::get_next(n);
//			}
//			return false;
//		}
//		
//		void add(const segment_t& s)
//		{
//			file_size_t trailingEnd;
//
//			// Find first one that starts before or at the end of the new segment.
//			node* trailingNode = m_list.find_any_equal_or_nearest_less_than(s.get_end());
//			bool overlapping = false;
//			if (!!trailingNode)
//			{
//				trailingEnd = trailingNode->get_end();
//				overlapping = (trailingEnd >= s.m_start);
//			}
//			if (!overlapping)
//			{
//				++m_count;
//				m_list.insert(new (default_allocator::get()) node(s));
//			}
//			else // trailingEnd will be set
//			{
//				bool doneMerge = false;
//				if ((trailingEnd != s.m_start) && (s.m_start < trailingNode->m_start))
//				{
//					node* leadingNode = m_list.find_any_equal_or_nearest_less_than(s.m_start); // We know leadingNode != trailingNode
//					if (!leadingNode)
//						leadingNode = m_list.get_first(); // Might be some cruft overlapping, so start at beginning cleaning stuff out.
//					else if (leadingNode->get_end() < s.m_start) // If leadingNode is well before us.
//						++leadingNode;
//					else // If leadingNode overlaps or is adjacent
//					{
//						leadingNode->m_segment.merge(s);
//						trailingNode->m_segment.merge(leadingNode);
//						doneMerge = true;
//					}
//					// Delete all the junk starting at leadingNode, up to and not including trailingNode.
//					while (leadingNode != trailingNode)
//					{
//						node* next = node::get_next(*leadingNode);
//						m_list.remove(leadingNode);
//						default_allocator::destruct_deallocate_type(leadingNode);
//						--m_count;
//						leadingNode = next;
//					}
//				}
//				if (!doneMerge)
//					trailingNode->m_segment.merge(s);
//			}
//		}
//
//		void remove(const iterator& itor)
//		{
//			m_list.remove(itor.m_node);
//			default_allocator::destruct_deallocate_type(itor.m_node.get());
//			itor.m_node = 0;
//			--m_count;
//		}
//
//		iterator get_first() const { return iterator(m_list.get_first()); }
//		iterator get_last() const { return iterator(m_list.get_last()); }
//
//		iterator find(const file_size_t& n) const { return iterator(m_list.find_any_equal(n)); }
//		iterator find_any_before(const file_size_t& n) const { return iterator(m_list.find_any_less_than(n)); }
//		iterator find_any_after(const file_size_t& n) const { return iterator(m_list.find_any_greater_than(n)); }
//		iterator find_equal_or_any_before(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_less_than(n)); }
//		iterator find_equal_or_any_after(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_greater_than(n)); }
//		iterator find_nearest_before(const file_size_t& n) const { return iterator(m_list.find_nearest_less_than(n)); }
//		iterator find_nearest_after(const file_size_t& n) const { return iterator(m_list.find_nearest_greater_than(n)); }
//		iterator find_equal_or_nearest_before(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_nearest_less_than(n)); }
//		iterator find_equal_or_nearest_after(const file_size_t& n) const { return iterator(m_list.find_any_equal_or_nearest_greater_than(n)); }
//	};
//
//	template <typename content_t>
//	class segment_map_base
//	{
//	public:
//		class map_segment : public segment, public sorted_list_node<true, map_segment>
//		{
//		public:
//			content_t m_contents;
//
//		protected:
//			friend class segment_map_base<content_t>;
//
//			map_segment(const segment& s, const content_t& c)
//				: segment(s),
//				m_contents(c)
//			{ }
//
//			map_segment(const segment& s)
//				: segment(s)
//			{ }
//		};
//
//	private:
//		typedef sorted_list<file_size_t, true, map_segment> list_t;
//		list_t m_list;
//
//		map_segment* split_off_after_at(map_segment& ss, file_size_t p)
//		{
//			map_segment* result = new (default_allocator::get()) map_segment(segment(ss.m_end, p), ss.m_contents.split_off_after(p - ss.m_start));
//			ss.m_end = p;
//			m_list.insert(result);
//			return result;
//		}
//
//		map_segment* split_off_before_at(map_segment& ss, file_size_t p)
//		{
//			map_segment* result = new (default_allocator::get()) map_segment(segment(ss.m_start, p), ss.m_contents.split_off_before(p - ss.m_start));
//			ss.m_start = p;
//			m_list.insert(result);
//			return result;
//		}
//
//	public:
//		map_segment* get_first()
//		{
//			return m_list.get_first();
//		}
//
//		map_segment* get_first(const segment& s)
//		{
//			map_segment* result = find_first_equal_or_nearest_greater_than(s.m_start);
//			if (!!result)
//			{
//				if (result->m_begin >= s.m_end)
//					result = 0;
//			}
//			return result;
//		}
//
//		map_segment* get_next(map_segment& ss, const segment& s)
//		{
//			map_segment* result = segment::get_next(&ss);
//			if (!!result)
//			{
//				if (result->m_begin >= s.m_end)
//					result = 0;
//			}
//			return result;
//		}
//
//		map_segment* add(const segment& s) // returns first in new range
//		{
//			map_segment* ss = m_list.find_nearest_less_than(s.m_end);
//			if ((!ss) || (ss->m_end <= s.m_start))
//			{
//				ss = new (default_allocator::get()) map_segment(s);
//				m_list.insert(ss);
//			}
//			else
//			{
//				if (ss->m_end < s.m_end) // If extending past the new range, create a new block to bridge the gap
//					m_list.insert(new (default_allocator::get()) map_segment(segment(ss->m_end, s.m_end)));
//				else if (s.m_end < ss->m_end) // If splitting the block at the end
//					split_off_after_at(ss, s.m_end);
//				for (;;) // just to use break/continue as goto labels
//				{
//					if (ss->m_start < s.m_start) // trim leading, then done.
//						ss = split_off_after_at(ss, s.m_start);
//					else if (s.m_start < ss->m_start) // block starts before curSubrange.  We need to look for prev blocks.
//					{
//						map_segment* prev = map_segment::get_prev(ss);
//						if ((!prev) || (prev->m_end <= s.m_start)) // Nothing prior within this segment, create 1 new segment
//						{
//							ss = new (default_allocator::get()) map_segment(segment(s.m_start, ss->m_start));
//							m_list.insert(ss);
//						}
//						else // (s.m_start < prev->m_end) // If prev ends within our block
//						{
//							if (prev->m_end < ss->m_start) // If prev ends before subsequent block starts, fill the gap
//								m_list.insert(new (default_allocator::get()) map_segment(segment(prev->m_end, ss->m_start)));
//							ss = prev;
//							continue;
//						}
//					}
//					break;
//				}
//			}
//			return ss;
//		}
//
//		void remove_segment(map_segment& s)
//		{
//			m_list.remove(s);
//			default_allocator::destruct_deallocate_type(&s);
//		}
//	};
//
//public:
//	typedef segment_base<void> segment;
//
//	typedef segment_base<buffer> segment_buffer;
//
//	typedef segment_base<composite_buffer> segment_composite_buffer;
//
//	typedef segment_list_base<false> segment_list;
//
//	// A vector_buffer is a segment_list, but of segment_buffer's instead of segments.
//	// A vector_buffer is useful for specifying buffers to a vectored write operation.
//	// Vectored read operations return their results in a vector_buffer.
//	typedef segment_list_base<true> vector_buffer;
//
//private:
//	
//	class transaction_internals
//	{
//	public:
//		rcptr<reader> m_firstReader;
//		rcptr<writer> m_firstWriter;
//		segment_list m_writeMask;
//		rcptr<transaction_internals> m_nextTransaction;
//	};
//
//public:
//
//	class reader : public waitable
//	{
//	private:
//		friend class locking_transaction;
//		friend class failable_transaction;
//
//		reader(const reader&);
//		reader& operator=(const reader&);
//
//	protected:
//		friend class file<read_access>;
//
//		event m_event;
//		segment_list m_requestedSegments;
//		vector_buffer m_vectorBuffer;
//		rcptr<reader> m_nextReader;
//		size_t m_numInnerReaders;
//		weak_rcptr<transaction_internals> m_transaction;
//
//		reader() : m_numInnerReaders(0) { self_acquire(); }
//
//		void complete() { m_event.set(); self_release(); }
//		virtual void reading() { complete(); }
//
//	public:
//		const segment_list& get_segments() const { return m_requestedSegments; }
//		const vector_buffer& get_buffers() const { return m_vectorBuffer; }
//
//		typedef delegate_t<void, const rcref<const reader>&> dispatch_t;
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_event.timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d, size_t n = 1) const volatile { m_event.dispatch(d, n); }
//		void dispatch(const dispatch_t& d, size_t n = 1) const { m_event.dispatch(delegate(d, this_rcref), n); }
//	};
//
//	class read_only_cursor : public datasource, public object
//	{
//	protected:
//		friend class file<read_access>;
//
//		const weak_rcptr<file<read_access> > m_file;
//		file_size_t m_curPos;
//
//		const rcref<queue>& get_io_queue() const { return m_ioQueue; }
//
//		class cursor_reader : public datasource::reader
//		{
//		protected:
//			const weak_rcptr<read_only_cursor> m_cursor;
//
//		public:
//			cursor_reader(const rcref<read_only_cursor>& c)
//				: m_cursor(c),
//				datasource::reader(c)
//			{ }
//
//			virtual void reading()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					rcptr<file<read_access> > f = c->m_file;
//					if (!f)
//						close();
//					else
//						f->read(c->m_curPos, get_requested_size())->dispatch(file<read_access>::reader::dispatch_t(&cursor_reader::read_done, this_rcref));
//				}
//			}
//
//			void read_done(const rcref<const file<read_access>::reader>& r)
//			{
//				m_bufferList = r->get_composite_buffer();
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					c->m_curPos += m_bufferList.size();
//					complete();
//				}
//			}
//		};
//		friend class cursor_reader;
//
//		virtual rcref<datasource::reader> create_reader()
//		{
//			return rcnew(cursor_reader, this_rcref);
//		}
//
//	public:
//		class seeker : public queue::operation
//		{
//		protected:
//			friend class read_only_cursor;
//
//			const weak_rcptr<read_only_cursor> m_cursor;
//			file_size_t m_position;
//			bool m_succeeded;
//
//			seeker(file_size_t pos, const rcref<read_only_cursor>& c)
//				: queue::operation(c->get_io_queue()),
//				m_cursor(c),
//				m_position(pos),
//				m_succeeded(false)
//			{ }
//
//			void complete() { m_succeeded = true; queue::operation::complete(); }
//			void close() { queue::operation::close(); }
//			void enqueue() { queue::operation::enqueue(); }
//
//			virtual void execute()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					c->m_curPos = m_position;
//					complete();
//				}
//			}
//
//		public:
//			bool succeeded() const { return m_succeeded; }
//		};
//
//		class teller : public queue::operation
//		{
//		protected:
//			friend class file<read_access>;
//			friend class read_only_cursor;
//
//			const weak_rcptr<read_only_cursor> m_cursor;
//			file_size_t m_position;
//			bool m_succeeded;
//
//			teller(const rcref<read_only_cursor>& c)
//				: queue::operation(c->get_io_queue()),
//				m_cursor(c),
//				m_succeeded(false)
//			{ }
//
//			void complete() { m_succeeded = true; queue::operation::complete(); }
//			void close() { queue::operation::close(); }
//			void enqueue() { queue::operation::enqueue(); }
//
//			virtual void execute()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					m_position = c->m_curPos;
//					complete();
//				}
//			}
//
//		public:
//			// Returns false on failure.  (Should only fail if file was closed).
//			bool get_position(uint64_t& dst) const
//			{
//				if (m_position == (file_size_t)-1)
//					return false;
//				dst = m_position;
//				return true;
//			}
//		};
//
//	protected:
//		class eof_seeker : public seeker
//		{
//		public:
//			eof_seeker(const rcref<read_only_cursor>& c)
//				: seeker(0, c)
//			{ }
//
//			virtual void execute()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					rcptr<file<read_access> > f = c->m_file;
//					if (!f)
//						close();
//					else
//					{
//						m_position = f->eof();
//						c->m_curPos = m_position;
//						complete();
//					}
//				}
//			}
//		};
//		
//		class eof_teller : public teller
//		{
//		public:
//			eof_teller(const rcref<read_only_cursor>& c)
//				: teller(c)
//			{ }
//
//			virtual void execute()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					rcptr<file<read_access> > f = c->m_file;
//					if (!f)
//						close();
//					else
//					{
//						m_position = f->eof();
//						complete();
//					}
//				}
//			}
//		};
//
//	public:
//		rcref<seeker> seek(file_size_t pos)
//		{
//			rcref<seeker> skr = rcnew(seeker, pos, this_rcref);
//			skr->enqueue();
//			return skr;
//		}
//
//		rcref<seeker> seek_end()
//		{
//			rcref<seeker> skr = rcnew(eof_seeker, this_rcref);
//			skr->enqueue();
//			return skr;
//		}
//
//		rcref<teller> tell()
//		{
//			rcref<teller> tlr = rcnew(teller, this_rcref);
//			tlr->enqueue();
//			return tlr;
//		}
//
//		rcref<teller> get_eof()
//		{
//			rcref<teller> tlr =  rcnew(eof_teller, this_rcref);
//			tlr->enqueue();
//			return tlr;
//		}
//
//	protected:
//		read_only_cursor(const rcref<file>& f, const rcref<queue>& ioQueue = queue::create())
//			: datasource(ioQueue),
//			m_file(f)
//		{ }
//	};
//
//protected:
//	class writer : public waitable
//	{
//	private:
//		writer(const writer&);
//		writer& operator=(const writer&);
//
//	protected:
//		friend class file<read_access>;
//		friend class file<read_write_access>;
//
//		event m_event;
//		vector_buffer m_unwrittenBuffers;
//		rcptr<writer> m_nextWriter;
//		size_t m_numInnerWriters;
//		weak_rcptr<transaction_internals> m_transaction;
//
//		writer() : m_numInnerWriters(0) { self_acquire(); }
//
//		void complete() { m_event.set(); self_release(); }
//		virtual void writing() { complete(); }
//
//	public:
//		const composite_buffer& get_unwritten_buffers() const { return m_unwrittenBuffers; }
//
//		typedef delegate_t<void, const rcref<const writer>&> dispatch_t;
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_event.timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d) const volatile { m_event.dispatch(d, n); }
//		void dispatch(const dispatch_t& d) const { m_event.dispatch(delegate(d, this_rcref), n); }
//	};
//
//	class inner_reader : public reader
//	{
//	public:
//		rcptr<reader> m_firstWaitingOuterReader;
//	};
//
//	class inner_writer : public writer
//	{
//	public:
//		rcptr<writer> m_firstWaitingOuterWriter;
//	};
//
//	// segment_state_map_t is a helper class representing N ranges, which may potentially overlap.
//	// Overlapping ranging will be divided, such that unique state can be tracked for each
//	// unique segment.
//	template <typename state_t>
//	class segment_state_map_t
//	{
//	public:
//		class state_segment : public segment, public sorted_list_node<true, state_segment>
//		{
//		public:
//			state_t m_state;
//			composite_buffer m_buffer;
//
//		protected:
//			friend class segment_state_map_t<state_t>;
//
//			state_segment(const state_segment& ss, const segment& s, const composite_buffer& b)
//				: segment(s),
//				m_state(ss.m_state)
//			{ }
//
//			state_segment(const segment& s)
//				: segment(s)
//			{ }
//		};
//
//	private:
//		typedef sorted_list<file_size_t, true, state_segment> state_list_t;
//		state_list_t m_stateList;
//
//		state_segment* split_off_after_at(state_segment& ss, file_size_t p)
//		{
//			state_segment* result = new (default_allocator::get()) state_segment(ss, segment(ss.m_end, p), ss.m_buffer.split_back(p - ss.m_start));
//			ss.m_end = p;
//			m_subrangeList.insert(result);
//			return result;
//		}
//
//		state_segment* split_off_before_at(state_segment& ss, file_size_t p)
//		{
//			state_segment* result = new (default_allocator::get()) state_segment(ss, segment(ss.m_start, p), ss.m_buffer.split_front(p - ss.m_start));
//			ss.m_start = p;
//			m_subrangeList.insert(result);
//			return result;
//		}
//
//	public:
//		state_segment* get_first_state()
//		{
//			return m_stateList.get_first();
//		}
//
//		state_segment* get_first_state(const segment& s)
//		{
//			state_segment* result = find_first_equal_or_nearest_greater_than(s.m_start);
//			if (!!result)
//			{
//				if (result->m_begin >= s.m_end)
//					result = 0;
//			}
//			return result;
//		}
//
//		state_segment* get_next_state(state_segment& ss, const segment& s)
//		{
//			state_segment* result = segment::get_next(&ss);
//			if (!!result)
//			{
//				if (result->m_begin >= s.m_end)
//					result = 0;
//			}
//			return result;
//		}
//
//		state_segment* create_state(const segment& s) // returns first in new range
//		{
//			state_segment* ss = m_subrangeList.find_nearest_less_than(s.m_end);
//			if ((!ss) || (ss->m_end <= s.m_start))
//			{
//				ss = new (default_allocator::get()) state_segment(s);
//				m_subrangeList.insert(ss);
//			}
//			else
//			{
//				if (ss->m_end < s.m_end) // If extending past the new range, create a new block to bridge the gap
//					m_subrangeList.insert(new (default_allocator::get()) state_segment(segment(ss->m_end, s.m_end)));
//				else if (s.m_end < ss->m_end) // If splitting the block at the end
//					split_off_after_at(ss, s.m_end);
//				for (;;) // just to use break/continue as goto labels
//				{
//					if (ss->m_start < s.m_start) // trim leading, then done.
//						ss = split_off_after_at(ss, s.m_start);
//					else if (s.m_start < ss->m_start) // block starts before curSubrange.  We need to look for prev blocks.
//					{
//						state_segment* prev = state_segment::get_prev(ss);
//						if ((!prev) || (prev->m_end <= s.m_start)) // Nothing prior within this segment, create 1 new segment
//						{
//							ss = new (default_allocator::get()) state_segment(segment(s.m_start, ss->m_start));
//							m_subrangeList.insert(ss);
//						}
//						else // (s.m_start < prev->m_end) // If prev ends within our block
//						{
//							if (prev->m_end < ss->m_start) // If prev ends before subsequent block starts, fill the gap
//								m_subrangeList.insert(new (default_allocator::get()) state_segment(segment(prev->m_end, ss->m_start)));
//							ss = prev;
//							continue;
//						}
//					}
//					break;
//				}
//			}
//			return ss;
//		}
//
//		void remove_segment(state_segment& s)
//		{
//			m_stateList.remove(s);
//			default_allocator::destruct_deallocate_type(&s);
//		}
//	};
//
//	// issued-map
//	//
//	//	The issued_map is the lowest-most layer, and reflects the IO operations that have been issued.
//	class issued_segment_state_t
//	{
//	public:
//		rcptr<inner_reader> m_issuedReader; // Set if a reader is issued in this segment.  Removed when it completes.
//		rcptr<inner_writer> m_issuedWriter; // Set if a writer is issued in this segment.  Removed when it completes.
//	};
//
//	typedef segment_state_map_t<issued_segment_state_t> issued_state_map_t;
//
//	issued_state_map_t m_issuedStateMap;
//
//	void issue_read(const rcref<reader>& r)
//	{
//		rcptr<inner_reader> innerReader = 0;
//		segment_list::iterator itor = r->m_requestedSegments.get_first();
//		while (!!itor)
//		{
//			do {
//				issued_state_map_t::state_segment* ss = m_issuedStateMap.create_state(*itor);
//				if (!!ss->m_buffer)
//					r->m_vectorBuffer.add(segment_buffer(ss->m_start, ss->m_buffer));
//				else if (!!ss->m_state.m_issuedReader)
//				{
//					if (ss->m_state.m_issuedReader->m_firstWaitingOuterReader)
//						r->m_firstWaitingOuterReader = ss->m_state.m_issuedReader->m_firstWaitingOuterReader;
//					ss->m_state.m_issuedReader->m_firstWaitingOuterReader = r;
//				}
//				else
//				{
//					if (!innerReader)
//						innerReader = create_reader();
//					innerReader->m_requestedSegments.add(ss.get_segment());
//				}
//				ss = m_issuedStateMap.get_next_state(*ss);
//			} while (!!ss);
//			++itor;
//		}
//		if (!!innerReader)
//		{
//			innerReader->reading();
//			// TODO: Notify when read is complete, so issued_state can be cleaned up, other reads completed, etc.
//		}
//		// else // TODO: Complete outer reader?
//	}
//
//	void issue_write(const rcref<writer>& w)
//	{
//		rcptr<inner_writer> innerWriter;
//		segment_list::iterator itor = w->m_unwrittenBuffers.get_first();
//		while (!!itor)
//		{
//			do {
//				issued_state_map_t::state_segment* ss = m_issuedStateMap.create_state(*itor);
//				ss->m_buffer = itor->m_buffer;
//
//				if (!!ss->m_start.m_issuedWriter) // If there is another writer, then this is a collision.
//				{ // It's valid to err in favor of the existing write, so, the new write
//					if (ss->m_state.m_issuedWriter->m_firstWaitingOuterWriter) // need only wait for completion.
//						w->m_firstWaitingOuterWriter = ss->m_state.m_issuedWriter->m_firstWaitingOuterWriter;
//					ss->m_state.m_issuedWriter->m_firstWaitingOuterWriter = w;
//				}
//				else
//				{
//					if (!innerWriter)
//						innerWriter = create_writer();
//					innerWriter->m_unwrittenBuffers.add(*itor);
//				}
//				ss = m_issuedStateMap.get_next_state(*ss);
//			} while (!!ss);
//			++itor;
//		}
//		if (!!innerWriter)
//		{
//			innerWriter->writing();
//			// TODO: Notify when write is complete, so issued_state can be cleaned up
//		}
//	}
//
//	void issue_reads(const rcref<reader>& firstReader)
//	{
//		rcptr<reader> r = firstReader;
//		for (;;)
//		{
//			rcptr<reader> next = r->m_nextReader;
//			issue_read(r);
//			if (!next)
//				break;
//			r = next;
//		}
//	}
//
//	void issue_writes(const rcref<writer>& firstWriter)
//	{
//		rcptr<writer> w = firstWriter;
//		for (;;)
//		{
//			rcptr<writer> next = w->m_nextWriter;
//			issue_write(w);
//			if (!next)
//				break;
//			w = next;
//		}
//	}
//
//	void issue_transaction(const rcref<transaction_internals>& t)
//	{
//		issue_reads(t->m_firstRead);
//		issue_writes(t->m_firstWriter);
//	}
//
//	//	If any of these segments are written to before the failable_transaction completes, it fails.
//	collection<rcref<class failable_transaction_internals> > m_failableTransactions;
//
//	class failable_transaction_internals : public transaction_internals
//	{
//	public:
//		typename collection<rcref<failable_transaction_internals> >::remove_token m_removeToken;
//		segment_list m_readRanges;
//		bool m_aborted;
//		
//		void abort()
//		{
//			COGS_ASSERT(!m_aborted);
//			m_aborted = true;
//			m_failableTransactions.remove(m_removeToken);
//		}
//	};
//
//	void run_write_notifications(const segment_list& rm)
//	{
//		collection<rcref<failable_transaction_internals> >::iterator itor = m_failableTransactions.get_first();
//		while (!!itor)
//		{
//			rcref<failable_transaction_internals> t = *itor;
//			++itor;
//			if (t->m_readRanges.does_overlap(rm))
//			{
//				m_failableTransactions.remove(t->m_removeToken);
//				t->abort();
//			}
//		}
//	}
//
//	class locking_transaction_internals : public transaction_internals
//	{
//	public:
//		collection<rcref<reader> > m_deferredReaders; // Will be populated if this locking_transaction is not the active one.
//	};
//
//	class coalesced_transaction_state
//	{
//	public:
//		ptr<coalesced_transaction_state> m_nextCoalescedTransactionState;
//		ptr<coalesced_transaction_state> m_prevCoalescedTransactionState;
//
//		rcptr<reader> m_outerReader;
//
//		coalesced_transaction_state()
//		{
//		}
//
//		coalesced_transaction_state(const coalesced_transaction_state& src)
//			: m_outerReader(src.m_outerReader),
//			m_prevCoalescedTransactionState(src),
//			m_nextCoalescedTransactionState(src.m_nextCoalescedTransactionState)
//		{
//			src.m_nextCoalescedTransactionState = this;
//			if (m_nextCoalescedTransactionState)
//				m_nextCoalescedTransactionState->m_prevCoalescedTransactionState = this;
//		}
//	};
//
//	typedef segment_state_map_t<coalesced_transaction_state> coalesced_transaction_map;
//
//	coalesced_transaction_map m_coalescedTransactionMap;
//
//	void coalesc_deferred_reader(const rcref<reader>& r)
//	{
//		segment_list::iterator itor = r->m_requestedSegments.get_first();
//		while (!!itor)
//		{
//			do {
//				coalesced_transaction_map::state_segment* ss = m_coalescedTransactionMap.create_state(*itor);
//				if (!!ss->m_buffer)
//					r->m_vectorBuffer.add(segment_buffer(ss->m_start, ss->m_buffer));
//				else if (!!ss->m_state.m_outerReader)
//					ss->m_state.m_outerReader = r;
//				else
//				{
//#error
//					// ????
//				}
//				// ????
//				ss = m_coalescedTransactionMap.get_next_state(*ss);
//			} while (!!ss);
//			++itor;
//		}
//	}
//
//	void coalesc_deferred_writer(const rcref<writer>& r)
//	{
//	}
//
//	void coalesc_deferred_readers(const rcref<reader>& r)
//	{
//		rcptr<reader> r = firstReader;
//		for (;;)
//		{
//			rcptr<reader> next = r->m_nextReader;
//			coalesc_deferred_reader(r);
//			if (!next)
//				break;
//			r = next;
//		}
//	}
//
//	void coalesc_deferred_writers(const rcref<writer>& w)
//	{
//		rcptr<writer> w = firstWriter;
//		for (;;)
//		{
//			rcptr<writer> next = w->m_nextWriter;
//			coalesc_deferred_writer(w);
//			if (!next)
//				break;
//			w = next;
//		}
//	}
//
//	void coalesc_deferred_transaction(const rcref<transaction_internals>& t)
//	{
//		coalesc_deferred_readers(t->m_firstReader);
//		coalesc_deferred_writers(t->m_firstWriter);
//	}
//
//	// The locking_transaction currently in progress
//	rcptr<locking_transaction_internals> m_currentLockingTransaction;
//
//	// A link-list of deferred locking transactions
//	rcptr<locking_transaction_internals> m_deferredLockingTransactions;
//
//	// A link-list of composed_transactions and/or failable_transactions containg writes that overlapped
//	// with the current locking_transaction.  This list is processed between locking_transaction's.
//	rcptr<transaction_internals> m_deferredComposedTransactions;
//	rcptr<failable_transaction_internals> m_deferredFailableTransactions;
//
//	// lock-mask
//	//
//	//	The lock-mask keeps track of any segments read by the locking_transaction that is in progress.
//	//	Any transaction that contains writes to a segment that has been read by the locking_transaction,
//	//	will be blocked until the locking_transaction is complete (and issued before the next locking_transaction
//	//	is issued).
//	segment_list m_lockMask;
//
//	delegate_serial_defer_guard m_deferGuard;
//
//	void commit_failable_read(rcref<reader>& r)
//	{
//		rcptr<failable_transaction_internals> t = r->m_transaction;
//		if (!t) || (t->m_aborted)
//			r->reading()
//		else
//		{
//			t->m_readRange.add(m_firstReader->m_requestedSegments); // Register the segment in the write_notification_map
//			issue_read(r); //Issue read directly to issued_map
//		}
//	};
//
//	bool commit_transaction(const rcref<transaction_internals>& t)
//	{
//		if (m_lockMask.does_overlap(t->m_writeMask))
//			return false;
//		run_write_notifications(t->m_writeMask); // Next check for overlaps with the write notifications, aborting failable_transactions if necessary.
//		issue_transaction(t); //Issue both reads and writers directly to issued-map
//		return true;
//	};
//
//	void commit_composed(const rcref<transaction_internals>& t)
//	{
//		if (!commit_transaction(t))
//		{
//			t->m_nextDeferredTransaction = m_deferredComposedTransactions;
//			m_deferredComposedTransactions = t;
//		}
//	}
//
//	void commit_failable(const rcref<failable_transaction_internals>& t)
//	{
//		if (t->m_aborted) //check if transaction was interrupted. 
//		{                 // If so, complete the writes without issuing them and return.
//			// TODO: Abort all writers
//		}
//		m_failableTransactions.remove(t->m_removeToken);
//
//		//Commit the writes as if they are a composed transaction containing only writes.
//		if (!commit_transaction(t))
//		{
//			t->m_nextDeferredTransaction = m_deferredFailableTransactions;
//			m_deferredFailableTransactions = t;
//		}
//	}
//
//	void commit_locking(const rcref<locking_transaction_internals>& t)
//	{
//		// Checks writes against write_notification_map, but NOT the lock_map
//		run_write_notifications(t->m_writeMask);
//		
//		issue_writes(t); // Issue the writes.
//		m_lockMask.clear();
//
//		// issue everything in m_waitingComposedTransactions and clean it out.
//		while (!!m_deferredFailableTransactions)
//		{
//			m_deferredFailableTransactions->commit_failable(m_deferredFailableTransactions);
//			m_deferredFailableTransactions = m_deferredFailableTransactions->m_nextDeferredTransaction;
//		}
//		while (!!m_deferredComposedTransactions)
//		{
//			m_deferredComposedTransactions->commit_composed(m_deferredComposedTransactions);
//			m_deferredComposedTransactions = m_deferredComposedTransactions->m_nextDeferredTransaction;
//		}
//
//		// Start the next m_waitingLockingTransactions
//		m_currentLockingTransaction = m_waitingLockingTransactions;
//		if (!m_currentLockingTransaction)
//			m_waitingLockingTransactions = 0;
//		else
//			issue_reads(m_currentLockingTransaction);
//	}
//
//	class locking_transaction
//	{
//	private:
//		locking_transaction();
//		locking_transaction(const locking_transaction&);
//
//	protected:
//		friend class file<read_access>;
//		friend class file<read_write_access>;
//
//		weak_rcptr<file<read_access> > m_file;
//		rcref<locking_transaction_internals> m_transaction;
//
//		locking_transaction(const rcref<file<read_access> >& f)
//			: m_file(f),
//			m_transaction(rcnew(locking_transaction_internals))
//		{ }
//
//	public:
//		~locking_transaction()
//		{
//			rcptr<file<read_access> > f = m_file;
//			if (!!f)
//			{
//				typedef const delegate_t<void, const rcref<locking_transaction_internals>&> rider_delegate_t;
//				f->m_deferGuard.submit(delegate(rider_delegate_t(&file<read_access>::commit_locking, f.get_ref()), m_transaction));
//			}
//			else
//			{
//				writer_map_t::iterator itor = m_transaction->m_writerList.get_first();
//				while (!!itor)
//				{
//					(*itor)->complete();
//					++itor;
//				}
//			}
//		}
//			
//		rcref<reader> read(const segment_list& sl)
//		{
//			rcptr<file<read_access> > f = m_file;
//			rcref<reader> r = (!f) ? rcnew(reader) : f->create_reader(sl);
//			if (!f)
//				r->complete();
//			else
//			{
//				// TODO
//			}
//			return r;
//		}
//		rcref<reader> read(const segment& s) { return read(segment_list(s)); }
//		rcref<reader> read(file_size_t offset, size_t n) { return read(segment(offset, n)); }
//
//
//	};
//	
//	class failable_transaction
//	{
//	private:
//		failable_transaction();
//		failable_transaction(const failable_transaction&);
//
//	protected:
//		friend class file<read_access>;
//		friend class file<read_write_access>;
//
//		weak_rcptr<file<read_access> > m_file;
//		rcref<failable_transaction_internals> m_transaction;
//
//		failable_transaction(const rcref<file<read_access> >& f)
//			: m_file(f),
//			m_transaction(rcnew(failable_transaction_internals))
//		{ }
//
//	public:
//
//		~failable_transaction()
//		{
//			rcptr<file<read_access> > f = m_file;
//			if (!!f)
//			{
//				typedef const delegate_t<void, const rcref<failable_transaction_internals>&> rider_delegate_t;
//				f->m_deferGuard.submit(delegate(rider_delegate_t(&file<read_access>::commit_failable, f.get_ref()), m_transaction));
//			}
//			else
//			{
//				writer_map_t::iterator itor = m_transaction->m_writerList.get_first();
//				while (!!itor)
//				{
//					(*itor)->complete();
//					++itor;
//				}
//			}
//		}
//
//		rcref<reader> read(file_size_t offset, size_t n)
//		{
//			rcptr<file<read_access> > f = m_file;
//			rcref<reader> r = (!f) ? rcnew(reader) : f->create_reader();
//			if (!f)
//				r->complete();
//			else
//			{
//				// TODO
//			}
//			return r;
//		}
//	};
//	
//public:
//	class composed_transaction
//	{
//	private:
//		composed_transaction();
//		composed_transaction(const composed_transaction&);
//
//	protected:
//		friend class file<read_access>;
//		friend class file<read_write_access>;
//
//		weak_rcptr<file<read_access> > m_file;
//		rcref<transaction_internals> m_transaction;
//
//		composed_transaction(const rcref<file<read_access> >& f)
//			: m_file(f),
//			m_transaction(rcnew(transaction_internals))
//		{ }
//
//	public:
//		~composed_transaction()
//		{
//			rcptr<file<read_access> > f = m_file;
//			if (!!f)
//			{
//				typedef const delegate_t<void, const rcref<transaction_internals>&> rider_delegate_t;
//				f->m_deferGuard.submit(delegate(rider_delegate_t(&file<read_access>::commit_composed, f.get_ref()), m_transaction));
//			}
//			else
//			{
//				reader_map_t::iterator itor = m_transaction->m_readerList.get_first();
//				while (!!itor)
//				{
//					(*itor)->complete();
//					++itor;
//				}
//				
//				writer_map_t::iterator itor2 = m_transaction->m_writerList.get_first();
//				while (!!itor2)
//				{
//					(*itor2)->complete();
//					++itor2;
//				}
//			}
//		}
//
//		rcref<reader> read(file_size_t offset, size_t n)
//		{
//			rcptr<file<read_access> > f = m_file;
//			rcref<reader> r = (!f) ? rcnew(reader) : f->create_reader();
//			r->m_offset = offset;
//			r->m_requestedSize = n;
//			m_transaction->m_readerList.insert(r);
//			return r;
//		}
//	};
//
//	virtual const file_size_t eof() = 0;
//
//	rcref<reader> read(file_size_t offset, size_t n)
//	{
//		return create_read_only_composed_transaction()->read(offset, n);
//	}
//
//	static rcptr<file<read_access> > open(const string& location);
//
//	rcref<read_only_cursor> create_read_only_cursor()
//	{
//		return rcnew(read_only_cursor, this_rcref);
//	}
//
//	rcref<composed_transaction> create_read_only_composed_transaction()
//	{
//		return rcnew(composed_transaction, this_rcref);
//	}
//
//protected:
//	virtual rcref<reader> create_reader() = 0;
//};
//
//
//template <>
//class file<read_write_access> : public file<read_access>
//{
//public:
//	typedef file<read_access>::reader reader;
//	typedef file<read_access>::writer writer;
////	typedef file<read_access>::locking_transaction locking_transaction;
////	typedef file<read_access>::failable_transaction failable_transaction;
//
//	class composed_transaction : public file<read_access>::composed_transaction
//	{
//	protected:
//		friend class file<read_write_access>;
//
//		composed_transaction(const rcref<file<read_write_access> >& f)
//			: file<read_access>::composed_transaction(f)
//		{ }
//
//	public:
//		rcref<writer> write(file_size_t offset, size_t n)
//		{
//			rcptr<file<read_write_access> > f = m_file;
//			rcref<writer> w = (!f) ? rcnew(writer) : f->create_writer();
//			m_transaction->m_writerList.insert(w);
//			return w;
//		}
//	};
//
//	rcref<composed_transaction> create_read_write_composed_transaction()
//	{
//		return rcnew(composed_transaction, this_rcref);
//	}
//
//	class failable_transaction : public file<read_access>::failable_transaction
//	{
//	protected:
//		friend class file<read_write_access>;
//
//		failable_transaction(const rcref<file<read_write_access> >& f)
//			: file<read_access>::failable_transaction(f)
//		{ }
//
//	public:
//		rcref<writer> write(file_size_t offset, size_t n)
//		{
//			rcptr<file<read_write_access> > f = m_file;
//			rcref<writer> w = (!f) ? rcnew(writer) : f->create_writer();
//			m_transaction->m_writerList.insert(w);
//			return w;
//		}
//	};
//
//	rcref<failable_transaction> create_read_write_failable_transaction()
//	{
//		return rcnew(failable_transaction, this_rcref);
//	}
//
//	class locking_transaction : public file<read_access>::locking_transaction
//	{
//	protected:
//		friend class file<read_write_access>;
//
//		locking_transaction(const rcref<file<read_write_access> >& f)
//			: file<read_access>::locking_transaction(f)
//		{ }
//
//	public:
//		rcref<writer> write(file_size_t offset, size_t n)
//		{
//			rcptr<file<read_write_access> > f = m_file;
//			rcref<writer> w = (!f) ? rcnew(writer) : f->create_writer();
//			m_transaction->m_writerList.insert(w);
//			return w;
//		}
//	};
//
//	rcref<locking_transaction> create_read_write_locking_transaction()
//	{
//		return rcnew(locking_transaction, this_rcref);
//	}
//
//	enum class create_mode
//	{
//		open_if_exists = 0x01,
//		create_only = 0x02,
//		open_or_create = 0x03,
//		open_truncate_or_create = 0x07,
//
////		open_mask = 0x01,
////		create_mask = 0x02,
////		truncate_mask = 0x04,
//	};
//
//	// Implemented at cogs::os level, to use os::file derived class.
//	static rcptr<file<read_write_access> > open(const string& location, create_mode mode = open_if_exists);
//	
//protected:
//	virtual rcref<writer> create_writer() = 0;
//};
//
//
//typedef file<read_access> file_ro;
//typedef file<read_write_access> file_wr;
//*/
///*
//
//class byte_segment_t
//{
//protected:
//	uint64_t m_start;
//	uint64_t m_end;
//
//public:
//	byte_segment_t()
//		: m_start(0),
//		m_end(0)
//	{ }
//
//	byte_segment_t(const byte_segment_t& src)
//		: m_start(src.m_start),
//		m_end(src.m_end)
//	{ }
//	
//	byte_segment_t(uint64_t start, size_t n)
//		: m_start(start),
//		m_end(start + n)
//	{ }
//
//	byte_segment_t& operator=(const byte_segment_t& src)
//	{
//		m_start = src.m_start;
//		m_end = src.m_end;
//		return *this;
//	}
//
//	const uint64_t start() const { return m_start; }
//	const uint64_t end() const { return m_end; }
//
//	byte_segment_t split_off_after_at(uint64_t midpoint)
//	{
//		byte_segment_t result;
//		result.m_start = midpoint;
//		result.m_end = m_end;
//		m_end = midpoint;
//		return result;
//	}
//
//	byte_segment_t split_off_before_at(uint64_t midpoint)
//	{
//		byte_segment_t result;
//		result.m_start = m_start;
//		result.m_end = midpoint;
//		m_start = midpoint;
//		return result;
//	}
//
//	byte_segment_t split_off_after(uint64_t n) { return split_off_after_at(m_start + n); }
//	byte_segment_t split_off_before(uint64_t n) { return split_off_before_at(m_start + n); }
//};
//
//
//// A byte_range_t is a utilize class for representing a range of bytes within a file.
//// A byte_range_t may contain non-contiguous sets of bytes.
//class byte_range_t
//{
//private:
//	byte_range_t(const byte_range_t&);
//	byte_range_t& operator=(const byte_range_t&);
//
//public:
//	class block_t : public byte_segment_t, public sorted_list_node<true, block_t>
//	{
//	private:
//		block_t();
//		block_t& operator=(const block_t&);
//
//	protected:
//		friend class byte_range_t;
//
//		block_t(const byte_segment_t& src)
//			: byte_segment_t(src)
//		{ }
//
//	public:
//
//		uint64_t& start() { return m_start; }
//		const uint64_t start() const { return m_start; }
//
//		uint64_t& end() { return m_end; }
//		const uint64_t end() const { return m_end; }
//
//		// sorted_list_node interface
//		uint64_t get_key() const { return start(); }
//	};
//		
//private:
//	typedef sorted_list<uint64_t, true, block_t> block_list_t; 
//	block_list_t m_blockList;
//
//public:
//	class iterator
//	{
//	private:
//		block_list_t::iterator m_itor;
//
//	protected:
//		friend class byte_range_t;
//
//		iterator(const block_list_t::iterator& i) : m_itor(i) { }
//
//	public:
//		iterator() { }
//		iterator(const iterator& i) : m_itor(i.m_itor) { }
//		iterator(const volatile iterator& i) : m_itor(i.m_itor) { }
//
//		iterator& operator=(const iterator& i) { m_itor = i.m_itor; return *this; }
//		iterator& operator=(const volatile iterator& i) { m_itor = i.m_itor; return *this; }
//		volatile iterator& operator=(const iterator& i) volatile { m_itor = i.m_itor; return *this; }
//		volatile iterator& operator=(const volatile iterator& i) volatile { m_itor = i.m_itor; return *this; }
//
//		iterator& operator++() { ++m_itor; return *this; }
//		iterator operator++(int) { return m_itor++; }
//
//		iterator operator--() { return --m_itor; }
//		iterator operator--(int) { return m_itor--; }
//
//		bool operator!() const { return !m_itor; }
//		bool operator!() const volatile { return !m_itor; }
//		
//		bool operator==(const iterator& i) const { return m_itor == i.m_itor; }
//		bool operator==(const volatile iterator& i) const { return m_itor == i.m_itor; }
//		bool operator==(const iterator& i) const volatile { return m_itor == i.m_itor; }
//		bool operator==(const volatile iterator& i) const volatile { return m_itor == i.m_itor; }
//		
//		bool operator!=(const iterator& i) const { return !operator==(i); }
//		bool operator!=(const volatile iterator& i) const { return !operator==(i); }
//		bool operator!=(const iterator& i) const volatile { return !operator==(i); }
//		bool operator!=(const volatile iterator& i) const volatile { return !operator==(i); }
//
//		block_t* get() const { return m_itor.get(); }
//		block_t* get() const volatile { return m_itor.get(); }
//		block_t& operator*() const { return *m_itor; }
//		block_t& operator*() const volatile { return *m_itor; }
//		block_t* operator->() const { return m_itor.get(); }
//		block_t* operator->() const volatile { return m_itor.get(); }
//
//		void clear() { m_itor.clear(); }
//		void clear() volatile { m_itor.clear(); }
//	};
//
//	byte_range_t()
//	{ }
//
//	~byte_range_t()
//	{
//		ptr<block_t> n = m_blockList.get_first_postorder();
//		while (!!n)
//		{
//			ptr<block_t> n2 = block_t::get_next_postorder(n);
//			default_allocator::destruct_deallocate_type(n.get());
//			n = n2;
//		}
//	}
//
//	iterator get_first() const { return m_blockList.get_first(); }
//
//	void add_range(const byte_segment_t& s)
//	{
//		block_t* trailingBlock = m_blockList.find_any_equal_or_nearest_less_than(s.end());
//		for (;;) // just for a goto label using break.
//		{
//			if (!!trailingBlock)
//			{
//				uint64_t trailingEnd = trailingBlock->end();
//				if (trailingEnd >= s.start())
//				{
//					uint64_t tmpEnd = s.end();
//					if (trailingEnd != s.start())
//					{ // Here, we know trailingBlock ends after the start of this new range.  Use the later end.
//						if (tmpEnd < trailingEnd) // start is before the end of trailingEnd.
//							tmpEnd = trailingEnd;
//						if (s.start() < trailingBlock->start()) // if we start within it, just extend it.  Otherwise, if we started before it...
//						{
//							block_t* leadingBlock = m_blockList.find_any_equal_or_nearest_less_than(s.start()); // We know leadingBlock != trailingBlock
//							if (!!leadingBlock)
//								trailingBlock->start() = leadingBlock->start();
//							else
//							{
//								trailingBlock->start() = s.start();
//								leadingBlock = m_blockList.get_first().get();
//							}
//							while (leadingBlock != trailingBlock)
//							{
//								block_t* next = block_t::get_next(leadingBlock);
//								m_blockList.remove(leadingBlock);
//								default_allocator::destruct_deallocate_type(leadingBlock);
//								leadingBlock = next;
//							}
//						}
//					}
//					trailingBlock->end() = tmpEnd;
//					break;
//				}
//			}
//			m_blockList.insert(new (default_allocator::get()) block_t(s));
//			break;
//		}
//	}
//};
//
//template <>
//class file<read_access> : public object
//{
//private:
//	class transaction_operation;
//
//protected:
//	class file_writer : public waitable
//	{
//	private:
//		file_writer(const file_writer&);
//		file_writer& operator=(const file_writer&);
//
//	protected:
//		friend class file<read_write_access>;
//		friend class transaction_operation;
//
//		rcptr<transaction_operation> m_transactionOperation;
//		event m_event;
//		uint64_t m_offset;
//		size_t m_requestedSize; // Just to remind caller how much their original request was for.
//		composite_buffer m_unwrittenBufferList; // On Entry: Buffer to write.  On Exit: whatever couldn't be written
//
//		file_writer()
//		{
//			self_acquire();
//		}
//
//		void complete()
//		{
//			m_transactionOperation->release();
//			m_event.set();
//			self_release();
//		}
//
//		virtual void writing()
//		{
//			complete();
//		}
//
//	public:
//		const size_t get_requested_size() const { return m_requestedSize; }
//		const composite_buffer& get_unwritten_buffer() const { return m_unwrittenBufferList; }
//		const size_t get_write_size() const { return m_requestedSize - m_unwrittenBufferList.size(); } // m_writeSize; }
//		const uint64_t get_offset() const { return m_offset; }
//		bool was_any_written() const { return m_requestedSize != m_unwrittenBufferList.size(); }
//
//		typedef delegate_t<void, const rcref<const file_writer>&> dispatch_t;
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_event.timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d) const volatile { m_event.dispatch(d, n); }
//		void dispatch(const dispatch_t& d) const { m_event.dispatch(delegate(d, this_rcref), n); }
//	};
//	
//public:
//	class file_reader : public waitable
//	{
//	private:
//		file_reader(const file_reader&);
//		file_reader& operator=(const file_reader&);
//
//	protected:
//		friend class file<read_access>;
//		friend class transaction_operation;
//
//		rcptr<transaction_operation> m_transactionOperation;
//		event m_event;
//		uint64_t m_offset;
//		size_t m_requestedSize;
//		composite_buffer m_bufferList; // all read results.
//
//		file_reader()
//		{
//			self_acquire();
//		}
//
//		void complete()
//		{
//			m_transactionOperation->release();
//			m_event.set();
//			self_release();
//		}
//
//		virtual void reading()
//		{
//			complete();
//		}
//
//	public:
//		const size_t get_requested_size() const { return m_requestedSize; }
//		const size_t get_read_size() const { return m_bufferList.size(); }
//		const uint64_t get_offset() const { return m_offset; }
//		const composite_buffer& get_composite_buffer() const { return m_bufferList; }
//
//		typedef delegate_t<void, const rcref<const file_reader>&> dispatch_t;
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_event.timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d, size_t n = 1) const volatile { m_event.dispatch(d, n); }
//		void dispatch(const dispatch_t& d, size_t n = 1) const { m_event.dispatch(delegate(d, this_rcref), n); }
//	};
//
//private:
//	class transaction_operation : public dlink_t<transaction_operation>, public sorted_list_node<true, transaction_operation>, public object
//	{
//	private:
//		class deferred_add_range_t
//		{
//		public:
//			byte_segment_t m_segment;
//			uint64_t m_lastWritePosition; // 0 means it's a read operation
//			file_writer* m_writer;
//			file_reader* m_reader;
//
//			deferred_add_range_t()
//			{ }
//
//			deferred_add_range_t(file_writer* w, const byte_segment_t& s)
//				: m_segment(s),
//				m_writer(w),
//				m_lastWritePosition(s.end())
//			{ }
//
//			deferred_add_range_t(file_reader* r, const byte_segment_t& s)
//				: m_segment(s),
//				m_reader(r),
//				m_lastWritePosition(0)
//			{ }
//
//		};
//
//		// Makes transaction object thread-safe.  Multiple threads may add operations
//		// to a transaction.
////		volatile container_serial_defer_guard<deferred_add_range_t> m_deferGuard;
//		size_t m_numOperations;
//
//		void add_range(const deferred_add_range_t& s)
//		{
//			m_deferGuard.begin_guard();
//			m_deferGuard.add(s);
//			deferred_add_range_t curSegment;
//			while (m_deferGuard.release(curSegment))
//			{
//				++m_numOperations;
//				m_rangeMask.add_range(curSegment.m_segment);
//				if (!curSegment.m_lastWritePosition)
//					m_myReaders.append(curSegment.m_reader);
//				else
//				{
//					if (m_lastWritePosition < curSegment.m_lastWritePosition)
//						m_lastWritePosition = curSegment.m_lastWritePosition;
//					m_myWriters.append(curSegment.m_writer);
//				}
//			}
//		}
//
//	public:
//		weak_rcptr<file<read_access> > m_file;
//
//		// Range of the transaction currently being composed (not yet committed, not immediate)
//		byte_range_t m_rangeMask;
//		size_t m_countDown; // count-down until issued
//		uint64_t m_lastWritePosition;
//
//		// When the transaction completes, it issues these readers and writers.
//		// These are operations not yet issued.
//		vector<rcptr<transaction_operation> > m_waitingWriters;
//		vector<rcptr<transaction_operation> > m_waitingReaders; // empty if a read transaction
//			
//		collection<file_reader*> m_myReaders;
//		collection<file_writer*> m_myWriters;
//
//		transaction_operation(const weak_rcptr<file<read_access> >& f)
//			: m_file(f),
//			m_numOperations(0),
//			m_countDown(0),
//			m_lastWritePosition(0)
//		{ }
//
//		void add_reader(file_reader* r, const byte_segment_t& s)
//		{
//			add_range(deferred_add_range_t(r, s));
//		}
//
//		void add_writer(file_writer* w, const byte_segment_t& s)
//		{
//			add_range(deferred_add_range_t(w, s));
//		}
//
//		bool is_write_transaction() const { return m_lastWritePosition != 0; }
//
//		void issue()
//		{
//			collection<file_reader*>::iterator i = m_myReaders.get_first();
//			while (!!i)
//			{
//				(*i)->reading();
//				++i;
//			}
//			collection<file_writer*>::iterator i2 = m_myWriters.get_first();
//			while (!!i2)
//			{
//				(*i2)->writing();
//				++i2;
//			}
//		}
//
//		void release()
//		{
//			size_t n = pre_assign_prev(m_numOperations);
//			if (!n)
//			{
//				rcptr<file<read_access> > f = m_file;
//				if (!!f)
//					f->complete_transaction(is_write_transaction(), this_rcref); 
//			}
//		}
//
//		uint64_t get_key() const { return m_lastWritePosition; }
//		typedef dlink_t<transaction_operation>::ref_t ref_t;
//	};
//
//public:
//	class read_only_transaction : public object
//	{
//	protected:
//		friend class file<read_access>;
//
//		const weak_rcptr<file<read_access> > m_file;
//
//		rcref<transaction_operation> m_transactionOperation;
//
//		explicit read_only_transaction(const rcref<file<read_access> >& f)
//			: m_transactionOperation(rcnew(transaction_operation)(f)),
//			m_file(f)
//		{ }
//
//	public:
//		~read_only_transaction()
//		{
//			rcptr<file> f = m_file;
//			if (!!f)
//				f->add_transaction(m_transactionOperation);
//		}
//
//		rcref<file_reader> read(uint64_t offset, size_t n)
//		{
//			rcptr<file> f = m_file;
//			rcref<file_reader> r = (!f) ? rcnew(file_reader) : f->create_reader();
//			r->m_transactionOperation = m_transactionOperation;
//			r->m_offset = offset;
//			r->m_requestedSize = n;
//			if (!!f)
//				m_transactionOperation->add_reader(r.get(), byte_segment_t(offset, n));
//			return r;
//		}
//	};
//
//protected:
//	uint64_t m_eof; // only used when writing.  read-only access will use API each time eof is requested.
//
//private:
//	// A file manages concurrent and pending read/write operations against sets of ranges.
//
//	// Any operations that change the EOF need to be serialized.  That could be any write past the
//	// current EOF.  As the EOF is extended, other pending EOF operations may no longer be past the
//	// new EOF and need to be released.  If the file size is reduced, non-EOF pending writes may need
//	// to be added to EOF queue.  Unfortunately, if the EOF is changed by another process, that is not
//	// currently supported (it may(?) function properly, but not perform optimally).
//	sorted_list<uint64_t, true, transaction_operation> m_allWritesByEndPos; // All pending writes, sorted by m_lastWritePosition.
//	ptr<transaction_operation> m_allWritesInPostedOrder; // circular double-link list of writes, in posted order.
//
//	class subrange_t : public byte_segment_t, public sorted_list_node<true, subrange_t>
//	{
//	public:
//		size_t m_readCount; // number of reads concurrently executing
//		size_t m_pendingReadCount; // 
//		rcptr<transaction_operation> m_lastWaitingWriter; // Tail of the queue of pending writers.
//		vector<rcref<transaction_operation> > m_waitingReaders; // waiting readers not yet issued.
//
//		subrange_t(uint64_t startPos, uint64_t endPos, bool writeMode, const rcref<transaction_operation>& t)
//			: m_readCount(0)
//		{
//			start() = startPos;
//			end() = endPos;
//			if (!writeMode)
//				m_pendingReadCount = 1;
//			else
//			{
//				m_pendingReadCount = 0;
//				m_lastWaitingWriter = t;
//			}
//		}
//
//		subrange_t(const byte_segment_t& s, const subrange_t& src)
//			: byte_segment_t(s),
//			m_readCount(src.m_readCount),
//			m_pendingReadCount(src.m_pendingReadCount),
//			m_lastWaitingWriter(src.m_lastWaitingWriter),
//			m_waitingReaders(src.m_waitingReaders)
//		{
//			for (size_t i = 0; i < m_waitingReaders.size(); i++)
//				++(m_waitingReaders[i]->m_countDown);
//		}
//				
//		bool operator<(const subrange_t& cmp) const { return start() < cmp.start(); }
//
//		void register_waiter(bool writeMode, const rcref<transaction_operation>& t)
//		{
//			if (!writeMode) // read mode
//				++m_pendingReadCount;
//			else // if (writeMode) // write mode
//			{
//				if (!m_lastWaitingWriter)
//					m_lastWaitingWriter = t;
//				else
//				{
//					m_lastWaitingWriter->m_waitingWriters.append(t);
//					++(t->m_countDown);
//				}
//			}
//		}
//
//		uint64_t& start() { return m_start; }
//		const uint64_t start() const { return m_start; }
//
//		uint64_t& end() { return m_end; }
//		const uint64_t end() const { return m_end; }
//
//		// sorted_list_node interface
//		uint64_t get_key() const { return start(); }
//	};
//
//	typedef sorted_list<uint64_t, true, subrange_t> subrange_list_t; 
//	subrange_list_t m_subrangeList;
//
//	subrange_t* insert_new_subrange(uint64_t startPos, uint64_t endPos, bool writeMode, const rcref<transaction_operation>& t)
//	{
//		subrange_t* result = new (default_allocator::get()) subrange_t(startPos, endPos, writeMode, t);
//		m_subrangeList.insert(result);
//		return result;
//	}
//
//	void split_off_after_at(subrange_t* from, uint64_t midpoint)
//	{
//		m_subrangeList.insert(new (default_allocator::get()) subrange_t(from->split_off_after_at(midpoint), *from));
//	}
//
//	void split_off_before_at(subrange_t* from, uint64_t midpoint)
//	{
//		m_subrangeList.insert(new (default_allocator::get()) subrange_t(from->split_off_before_at(midpoint), *from));
//	}
//
//	void add_transaction(const rcref<transaction_operation>& t)
//	{
//		bool writeMode = t->is_write_transaction();
//		if (writeMode)
//		{
//			m_allWritesByEndPos.insert(t.get());
//			if (!m_allWritesInPostedOrder)
//			{
//				m_allWritesInPostedOrder = t.get();
//				t->dlink_t<transaction_operation>::set_next_link(t.get());
//				t->dlink_t<transaction_operation>::set_prev_link(t.get());
//			}
//			else
//			{
//				transaction_operation* last = m_allWritesInPostedOrder->dlink_t<transaction_operation>::get_prev_link();
//				last->dlink_t<transaction_operation>::set_next_link(t.get());
//				t->dlink_t<transaction_operation>::set_prev_link(last);
//				m_allWritesInPostedOrder->dlink_t<transaction_operation>::set_prev_link(t.get());
//				t->dlink_t<transaction_operation>::set_next_link(m_allWritesInPostedOrder);
//			}
//		}
//		byte_range_t::iterator itor = t->m_rangeMask.get_first();
//		while (!!itor)
//		{
//			uint64_t blockStart = itor->start();
//			uint64_t blockEnd = itor->end();
//
//			subrange_t* curSubrange = m_subrangeList.find_nearest_less_than(blockEnd);
//			for (;;) // just to use break/continue as goto labels
//			{
//				if (!!curSubrange)
//				{
//					uint64_t curEnd = curSubrange->end();
//					if (blockStart < curEnd)
//					{
//						if (curEnd < blockEnd) // If extending past the curSubrange, need to create a new block from curEnd to blockEnd
//							insert_new_subrange(curEnd, blockEnd, writeMode, t);
//						else if (blockEnd < curEnd) // If splitting the curSubrange at the end
//							split_off_after_at(curSubrange, curEnd);
//
//						uint64_t curStart = curSubrange->start();
//						for (;;) // just to use break/continue as goto labels
//						{
//							if (curStart < blockStart) // trim leading, then done.
//								split_off_before_at(curSubrange, blockStart);
//							else if (blockStart < curStart) // block starts before curSubrange.  We need to look for prev blocks.
//							{
//								subrange_t* prev = subrange_t::get_prev(curSubrange);
//								if (!prev) // Everything prior is free, create 1 new block
//									insert_new_subrange(blockStart, curStart, writeMode, t);
//								else // if (!!prev)
//								{
//									uint64_t prevStart = prev->start();
//									uint64_t prevEnd = prev->end();
//									if (prevEnd <= blockStart) // If only prev ends before we start, create 1 new block
//										insert_new_subrange(blockStart, curStart, writeMode, t);
//									else // if (prevEnd > pos) // If prev ends within our block
//									{
//										if (prevEnd < curStart) // If prev ends before subsequent block starts
//											insert_new_subrange(prevEnd, curStart, writeMode, t);
//
//										curSubrange->register_waiter(writeMode, t);
//										curSubrange = prev;
//										curStart = prevStart;
//										continue;
//									}
//								}
//							}
//							// else // if (pos == curStart) // Done
//							curSubrange->register_waiter(writeMode, t);
//							break;
//						}
//						break;
//					}
//				}
//				insert_new_subrange(blockStart, blockEnd, writeMode, t);
//				break;
//			}
//			++itor;
//		}
//
//		if (t->m_countDown == 0)
//		{
//			if (!writeMode) // read mode
//				issue_read(t);
//			else
//				t->issue();
//		}
//	}
//
//	void issue_read(const rcref<transaction_operation>& t)
//	{
//		byte_range_t::iterator itor = t->m_rangeMask.get_first();
//		while (!!itor)
//		{
//			uint64_t i = itor->start();
//			subrange_t* curSubrange = m_subrangeList.find_nearest_less_than(itor->end());
//			for (;;) // just to use break/continue as goto labels
//			{
//				curSubrange->m_pendingReadCount--;
//				curSubrange->m_readCount++;
//				if (curSubrange->start() == i)
//					break;
//				curSubrange = subrange_t::get_prev(curSubrange);
//			}
//			++itor;
//		}
//		t->issue();
//	}
//
//	void complete_transaction(bool writeMode, const rcref<transaction_operation>& t)
//	{
//		byte_range_t::iterator itor = t->m_rangeMask.get_first();
//		while (!!itor)
//		{
//			uint64_t blockStart = itor->start();
//			uint64_t blockEnd = itor->end();
//
//			// There should already be blocks for the entirety of the transaction.
//
//			subrange_t* sr = m_subrangeList.find_any_equal(itor->start());
//			uint64_t curEnd;
//			if (!writeMode) // read mode
//			{
//				for (;;)
//				{
//					curEnd = sr->end();
//					subrange_t* next = subrange_t::get_next(sr);
//					if (!--(sr->m_readCount)) // Last reader to release this block.
//					{
//						if (!sr->m_lastWaitingWriter) // If no waiting writers, remove the block
//						{
//							m_subrangeList.remove(sr);
//							default_allocator::destruct_deallocate_type(sr);
//						}
//					}
//					if (blockEnd == curEnd)
//						break; // this will be hit
//					sr = next;
//				}
//			}
//			else // if (writeMode) // write mode
//			{
//				for (;;)
//				{
//					curEnd = sr->end();
//					subrange_t* next = subrange_t::get_next(sr);
//					if (sr->m_lastWaitingWriter == t)
//					{
//						if (!sr->m_waitingReaders)
//						{
//							m_subrangeList.remove(sr);
//							default_allocator::destruct_deallocate_type(sr);
//						}
//						else
//						{
//							for (size_t i = 0; i < sr->m_waitingReaders.size(); i++)
//							{
//								rcref<transaction_operation>& t2 = sr->m_waitingReaders[i];
//								if (!--(t2->m_countDown))
//									issue_read(t2.get_ref());
//							}
//						}
//					}
//					if (blockEnd == curEnd)
//						break; // this will be hit
//					sr = next;
//				}
//			}
//			++itor;
//		}
//		size_t i;
//		for (i = 0; i < t->m_waitingWriters.size(); i++)
//		{
//			const rcptr<transaction_operation>& t2 = t->m_waitingWriters[i];
//			if (!--(t2->m_countDown))
//				t2->issue();
//		}
//		for (i = 0; i < t->m_waitingReaders.size(); i++)
//		{
//			const rcptr<transaction_operation>& t2 = t->m_waitingReaders[i];
//			if (!--(t2->m_countDown))
//				issue_read(t2.get_ref());
//		}
//	}
//
//public:
//	class read_only_cursor : public datasource, public object
//	{
//	protected:
//		friend class file<read_access>;
//
//		const weak_rcptr<file<read_access> > m_file;
//		uint64_t m_curPos;
//
//		const rcref<queue>& get_io_queue() const { return m_ioQueue; }
//
//		class cursor_reader : public datasource::reader
//		{
//		protected:
//			const weak_rcptr<read_only_cursor> m_cursor;
//
//		public:
//			cursor_reader(const rcref<read_only_cursor>& c)
//				: m_cursor(c),
//				datasource::reader(c)
//			{ }
//
//			virtual void reading()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					rcptr<file<read_access> > f = c->m_file;
//					if (!f)
//						close();
//					else
//						f->read(c->m_curPos, get_requested_size())->dispatch(file_reader::dispatch_t(&cursor_reader::read_done, this_rcref));
//				}
//			}
//
//			void read_done(const rcref<const file_reader>& r)
//			{
//				m_bufferList = r->get_composite_buffer();
//				complete();
//			}
//		};
//		friend class cursor_reader;
//
//		virtual rcref<datasource::reader> create_reader()
//		{
//			return rcnew(cursor_reader, this_rcref);
//		}
//
//	public:
//		class seeker : public queue::operation
//		{
//		protected:
//			friend class read_only_cursor;
//
//			const weak_rcptr<read_only_cursor> m_cursor;
//			uint64_t m_position;
//			bool m_succeeded;
//
//			seeker(uint64_t pos, const rcref<read_only_cursor>& c)
//				: queue::operation(c->get_io_queue()),
//				m_cursor(c),
//				m_position(pos),
//				m_succeeded(false)
//			{ }
//
//			void complete() { m_succeeded = true; queue::operation::complete(); }
//			void close() { queue::operation::close(); }
//			void enqueue() { queue::operation::enqueue(); }
//
//			virtual void execute()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					c->m_curPos = m_position;
//					complete();
//				}
//			}
//
//		public:
//			bool succeeded() const { return m_succeeded; }
//		};
//
//		class teller : public queue::operation
//		{
//		protected:
//			friend class file<read_access>;
//			friend class read_only_cursor;
//
//			const weak_rcptr<read_only_cursor> m_cursor;
//			uint64_t m_position;
//			bool m_succeeded;
//
//			teller(const rcref<read_only_cursor>& c)
//				: queue::operation(c->get_io_queue()),
//				m_cursor(c),
//				m_succeeded(false)
//			{ }
//
//			void complete() { m_succeeded = true; queue::operation::complete(); }
//			void close() { queue::operation::close(); }
//			void enqueue() { queue::operation::enqueue(); }
//
//			virtual void execute()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					m_position = c->m_curPos;
//					complete();
//				}
//			}
//
//		public:
//			// Returns false on failure.  (Should only fail if file was closed).
//			bool get_position(uint64_t& dst) const
//			{
//				if (m_position == (uint64_t)-1)
//					return false;
//				dst = m_position;
//				return true;
//			}
//		};
//
//	protected:
//		class eof_seeker : public seeker
//		{
//		public:
//			eof_seeker(const rcref<read_only_cursor>& c)
//				: seeker(0, c)
//			{ }
//
//			virtual void execute()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					rcptr<file<read_access> > f = c->m_file;
//					if (!f)
//						close();
//					else
//					{
//						m_position = f->eof();
//						c->m_curPos = m_position;
//						complete();
//					}
//				}
//			}
//		};
//		
//		class eof_teller : public teller
//		{
//		public:
//			eof_teller(const rcref<read_only_cursor>& c)
//				: teller(c)
//			{ }
//
//			virtual void execute()
//			{
//				rcptr<read_only_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					rcptr<file<read_access> > f = c->m_file;
//					if (!f)
//						close();
//					else
//					{
//						m_position = f->eof();
//						complete();
//					}
//				}
//			}
//		};
//
//	public:
//		rcref<seeker> seek(uint64_t pos)
//		{
//			rcref<seeker> skr = rcnew(seeker, pos, this_rcref);
//			skr->enqueue();
//			return skr;
//		}
//
//		rcref<seeker> seek_end()
//		{
//			rcref<seeker> skr = rcnew(eof_seeker, this_rcref);
//			skr->enqueue();
//			return skr;
//		}
//
//		rcref<teller> tell()
//		{
//			rcref<teller> tlr = rcnew(teller, this_rcref);
//			tlr->enqueue();
//			return tlr;
//		}
//
//		rcref<teller> get_eof()
//		{
//			rcref<teller> tlr =  rcnew(eof_teller, this_rcref);
//			tlr->enqueue();
//			return tlr;
//		}
//
//	protected:
//		read_only_cursor(const rcref<file>& f, const rcref<queue>& ioQueue = queue::create())
//			: datasource(ioQueue),
//			m_file(f)
//		{ }
//	};
//
//	virtual uint64_t eof()
//	{
//		return get_eof();
//	}
//
//	rcref<file_reader> read(uint64_t offset, size_t n)
//	{
//		return create_read_only_transaction()->read(offset, n);
//	}
//
//	static rcptr<file<read_access> > open(const string& location);
//
//	rcref<read_only_cursor> create_read_only_cursor()
//	{
//		return rcnew(read_only_cursor, this_rcref);
//	}
//
//	rcref<read_only_transaction> create_read_only_transaction()
//	{
//		return rcnew(read_only_transaction, this_rcref);
//	}
//
//
//protected:
//	virtual rcref<file_reader> create_reader() = 0;
//	virtual uint64_t get_eof() = 0;
//};
//
//
//template <>
//class file<read_write_access> : public file<read_access>
//{
//public:
//	class read_write_cursor : public read_only_cursor, public datasink
//	{
//	protected:
//		class cursor_writer : public datasink::writer
//		{
//		protected:
//			const weak_rcptr<read_write_cursor> m_cursor;
//
//		public:
//			cursor_writer(const rcref<read_write_cursor>& c)
//				: m_cursor(c),
//				datasink::writer(c)
//			{ }
//				
//			virtual void writing()
//			{
//				rcptr<read_write_cursor> c = m_cursor;
//				if (!c)
//					close();
//				else
//				{
//					rcptr<file<read_write_access> > f = c->m_file;
//					if (!f)
//						close();
//					else
//						f->write(c->m_curPos, get_unwritten_buffer())->dispatch(file_writer::dispatch_t(&cursor_writer::write_done, this_rcref));
//				}
//			}
//
//			void write_done(const rcref<const file_writer>& w)
//			{
//				size_t n = w->get_write_size();
//				if (!n)
//					close();
//				else
//				{
//					m_unwrittenBufferList.advance(n);
//					rcptr<read_write_cursor> c = m_cursor;
//					if (!c)
//						close();
//					else
//					{
//						c->m_curPos += n;
//						if (!m_unwrittenBufferList)
//							complete();
//						else
//							writing();
//					}
//				}
//			}
//		};
//		friend class cursor_writer;
//
//		virtual rcref<datasink::writer> create_writer()
//		{
//			return rcnew(cursor_writer, this_rcref);
//		}
//
//	public:
//		read_write_cursor(const rcref<file>& f, const rcref<queue>& ioQueue = queue::create())
//			: datasink(ioQueue),
//			read_only_cursor(f, ioQueue)
//		{ }
//	};
//
//	class read_write_transaction : public read_only_transaction
//	{
//	protected:
//		friend class file<read_write_access>;
//
//		explicit read_write_transaction(const rcref<file<read_write_access> >& f)
//			: read_only_transaction(f)
//		{ }
//
//	public:
//		rcref<file_writer> write(uint64_t offset, const composite_buffer& compBuf)
//		{
//			rcptr<file> f = m_file;
//			rcref<file_writer> w = (!f) ? rcnew(file_writer) : f->create_writer();
//			w->m_transactionOperation = m_transactionOperation;
//			w->m_offset = offset;
//			w->m_unwrittenBufferList = compBuf;
//			w->m_requestedSize = compBuf.size();
//			if (!!f)
//				m_transactionOperation->add_writer(w.get(), byte_segment_t(offset, compBuf.size()));
//			return w;
//		}
//	};
//
//	enum class create_mode
//	{
//		open_if_exists = 0x01,
//		create_only = 0x02,
//		open_or_create = 0x03,
//		open_truncate_or_create = 0x07,
//
////		open_mask = 0x01,
////		create_mask = 0x02,
////		truncate_mask = 0x04,
//	};
//
//	// Implemented at cogs::os level, to use os::file derived class.
//	static rcptr<file<read_write_access> > open(const string& location, create_mode mode = open_if_exists);
//	
//	rcref<file_writer> write(uint64_t offset, const composite_buffer& compBuf)
//	{
//		return create_read_write_transaction()->write(offset, compBuf);
//	}
//
//	virtual uint64_t eof()
//	{
//		return m_eof;
//	}
//
//	rcref<read_write_cursor> create_read_write_cursor()
//	{
//		return rcnew(read_write_cursor, this_rcref);
//	}
//
//	rcref<read_write_transaction> create_read_write_transaction()
//	{
//		return rcnew(read_write_transaction, this_rcref);
//	}
//
//protected:
//	virtual rcref<file_writer> create_writer() = 0;
//};
//
//
//*/
//
//
//};
//};
//
//
//#endif
//
//
//
//#include "cogs/os/io/file.hpp"
//
//#endif
//
