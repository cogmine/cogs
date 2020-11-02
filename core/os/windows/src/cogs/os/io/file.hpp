//
//  Copyright (C) 2000-2020 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//
//
//// Status: Placeholder
//
//#ifndef COGS_HEADER_OS_IO_FILE
//#define COGS_HEADER_OS_IO_FILE
//
//
//#include "cogs/env.hpp"
//#include "cogs/collections/map.hpp"
//#include "cogs/collections/set.hpp"
//#include "cogs/function.hpp"
//#include "cogs/io/datasource.hpp"
//#include "cogs/io/datasink.hpp"
//#include "cogs/io/file.hpp"
//#include "cogs/mem/placement.hpp"
//#include "cogs/mem/rcnew.hpp"
//#include "cogs/os/io/completion_port.hpp"
//#include "cogs/sync/delegate_guard.hpp"
//
//
//namespace cogs {
//namespace os {
//
//
//// os::file<> is actually not intended to be instantiated by externals calls.
//// Rather os::file<>::open() returns a class derived from synchronized_file<> that
//// utilized os::file<> instances internally.
//template <io::access_mode accessMode>
//class file;
//
//
//class file_globals : public object
//{
//protected:
//	class file_info;
//
//private:
//	// On Win32, the best way to tell if two paths refer to the same file is to open both files with CreateFile,
//	// call GetFileInformationByHandle for both, and compare dwVolumeSerialNumber, nFileIndexLow, nFileIndexHigh.
//	// If all three are equal they both point to the same file.
//	class file_id
//	{
//	public:
//		DWORD m_dwVolumeSerialNumber;
//		DWORD m_nFileIndexHigh;
//		DWORD m_nFileIndexLow;
//
//		rcptr<file<io::read_access> > m_readable;
//		rcptr<file<io::write_access> > m_writable;
//
//		bool operator<(const file_id& cmp) const
//		{
//			if (m_nFileIndexLow < cmp.m_nFileIndexLow) // First compare field most likely to be different.
//				return true;
//			if (m_nFileIndexLow > cmp.m_nFileIndexLow)
//				return false;
//
//			if (m_nFileIndexHigh < cmp.m_nFileIndexHigh)
//				return true;
//			if (m_nFileIndexHigh > cmp.m_nFileIndexHigh)
//				return false;
//
//			if (m_dwVolumeSerialNumber < cmp.m_dwVolumeSerialNumber)
//				return true;
//			//if (m_dwVolumeSerialNumber > cmp.m_dwVolumeSerialNumber)
//				return false;
//		}
//	};
//
//
//	typedef map<file_id, weak_rcptr<class file_info>, true> file_id_map_t;
//	typedef io::default_file_size_t file_size_t;
//
//	rcref<delegate_chain> m_serialQueue;
//	file_id_map_t m_fileIdMap;
//
//	class file_release_info
//	{
//	public:
//		file_id_map_t::iterator m_itor;
//		weak_rcptr<class file_info> m_fileInfo;
//	};
//
//	inline static placement<rcptr<file_globals> > s_globals;
//
//
//	file_globals()
//		: m_serialQueue(delegate_chain::create())
//	{ }
//
//protected:
//	friend class file<io::read_access>;
//	friend class file<io::write_access>;
//	friend class file<io::read_write_access>;
//
//	class file_info : public object
//	{
//	public:
//		weak_rcptr<file<io::read_access> > m_rawReadFile;
//		weak_rcptr<file<io::write_access> > m_rawWriteFile;
//		file_id_map_t::iterator m_itor;
//
//		rcptr<io::synchronized_file_impl<file_size_t> > m_syncImpl;
//
//		~file_info()
//		{
//			if (!!m_syncImpl)
//			{
//				file_release_info releaseInfo;
//				releaseInfo.m_itor = m_itor;
//				releaseInfo.m_fileInfo = this_weak_rcptr;
//
//				rcptr<file_globals> g = get_globals();
//				typedef function<void(const file_release_info&)> temp_delegate_t;
//				g->m_serialQueue->submit(delegate(temp_delegate_t(&file_globals::release_file_info, g.get_ref()), releaseInfo));
//			}
//		}
//	};
//
//public:
//	class opener : public waitable
//	{
//	private:
//		opener(const opener&) = delete;
//		opener& operator=(const opener&) = delete;
//
//		single_fire_condition m_condition;
//
//		rcptr<auto_HANDLE> m_handle;
//		file_id m_fileId;
//
//		template <io::access_mode accessMode>
//		friend class file_opener;
//
//	protected:
//		friend class file_globals;
//
//		rcptr<io::synchronized_file<io::read_access, io::default_file_size_t> > m_readFile;
//		rcptr<io::synchronized_file<io::write_access, io::default_file_size_t> > m_writeFile;
//		rcptr<io::synchronized_file<io::read_write_access, io::default_file_size_t> > m_readWriteFile;
//
//		void complete() { m_condition.signal(); self_release(); }
//
//		opener() { self_acquire(); }
//
//		void set_file(const rcref<io::synchronized_file<io::read_access, io::default_file_size_t> >& f) { m_readFile = f; }
//		void set_file(const rcref<io::synchronized_file<io::write_access, io::default_file_size_t> >& f) { m_writeFile = f; }
//		void set_file(const rcref<io::synchronized_file<io::read_write_access, io::default_file_size_t> >& f) { m_readWriteFile = f; m_readFile = f; m_writeFile = f; }
//
//	public:
//		const rcptr<io::synchronized_file<io::read_access, io::default_file_size_t> >& get_read_file() const { return m_readFile; }
//		const rcptr<io::synchronized_file<io::write_access, io::default_file_size_t> >& get_write_file() const { return m_writeFile; }
//		const rcptr<io::synchronized_file<io::read_write_access, io::default_file_size_t> >& get_read_write_file() const { return m_readWriteFile; }
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_condition.timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d) const volatile { m_condition.dispatch(d); }
//	};
//
//	template <io::access_mode accessMode>
//	rcref<opener> open(const string& location, bool createIfNotPresent)
//	{
//		rcref<opener> o = rcnew(opener);
//
//		// We always use shared file access.  Multiple references to the same file should be synchronized using
//		// transactions.  They will share the same synchronized_file<>.  Also writing to a file from another
//		// process is generally not supported, though it's possible to use a single reader without caching.
//		DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
//
//		// On Win32, the best way to tell if two paths refer to the same file is to open both files with CreateFile,
//		// call GetFileInformationByHandle for both, and compare dwVolumeSerialNumber, nFileIndexLow, nFileIndexHigh.
//		// If all three are equal they both point to the same file.
//
//		DWORD dwDesiredAccess = GENERIC_READ;
//		if (accessMode == io::write_access) // If opening for read_write_access, open read separately, first.
//			dwDesiredAccess = GENERIC_WRITE;
//
//		DWORD dwCreationDisposition = OPEN_EXISTING;
//		if (createIfNotPresent)
//			dwCreationDisposition = OPEN_ALWAYS;
//
//		for (;;)
//		{
//			// Unfortunately, Win32 doesn't currently offer a way to open a file asynchronously.
//			HANDLE h = CreateFile(location.cstr(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, FILE_FLAG_OVERLAPPED, NULL);
//			if (h != INVALID_HANDLE_VALUE)
//			{
//				o->m_handle = rcnew(auto_HANDLE)(h);
//
//				BY_HANDLE_FILE_INFORMATION fileInfo;
//				BOOL b = GetFileInformationByHandle(h, &fileInfo);
//				if (!!b)
//				{
//					o->m_fileId.m_dwVolumeSerialNumber = fileInfo.dwVolumeSerialNumber;
//					o->m_fileId.m_nFileIndexHigh = fileInfo.nFileIndexHigh;
//					o->m_fileId.m_nFileIndexLow = fileInfo.nFileIndexLow;
//
//					// TBD - Windows 8 - need for deal with 16-byte File IDs.  GetFileInformationByHandleEx w/ FILE_ID_INFO
//
//					typedef function<void(const rcref<opener>&)> temp_delegate_t;
//					m_serialQueue->submit(delegate(temp_delegate_t(&file_globals::find_match<accessMode>, this_rcref), o));
//					break;
//				}
//				o->m_handle = 0;
//			}
//			o->complete();
//			break;
//		}
//
//		return o;
//	}
//
//private:
//	template <io::access_mode accessMode>
//	void find_match(const rcref<opener>& o)
//	{
//		bool collision;
//		rcptr<file<io::read_access> > rawReadFile;
//		rcptr<file<io::write_access> > rawWriteFile;
//
//		rcptr<file_info> fileInfo = rcnew(file_info);
//		file_id_map_t::iterator itor = m_fileIdMap.insert_unique(o->m_fileId, fileInfo, collision);
//		for (;;) // just for a goto using break.
//		{
//			if (collision)
//			{
//				rcptr<file_info> tmp = *itor;
//				if (!tmp)
//					*itor = fileInfo;
//				else
//				{
//					fileInfo = tmp;
//					if ((accessMode & io::read_access) != 0)
//						rawReadFile = fileInfo->m_rawReadFile;
//					if ((accessMode & io::write_access) != 0)
//						rawWriteFile = fileInfo->m_rawWriteFile;
//					break;
//				}
//			}
//			fileInfo->m_itor = itor;
//			fileInfo->m_syncImpl = rcnew(io::synchronized_file_impl<file_size_t>);
//			break;
//		}
//
//		for (;;) // just for a goto using break.
//		{
//			if (((accessMode & io::write_access) != 0) && !rawWriteFile)
//			{
//				rcptr<auto_HANDLE> writeHandle;
//				if (accessMode == io::write_access)
//					writeHandle = o->m_handle;
//				else // open write channel now
//				{
//					FILE_ID_DESCRIPTOR fileIdDesc;
//					fileIdDesc.dwSize = sizeof(FILE_ID_DESCRIPTOR);
//					fileIdDesc.Type = FileIdType; // 0
//					fileIdDesc.FileId.HighPart = o->m_fileId.m_nFileIndexHigh;
//					fileIdDesc.FileId.LowPart = o->m_fileId.m_nFileIndexLow;
//					HANDLE h = OpenFileById(o->m_handle->get(), &fileIdDesc, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, FILE_FLAG_OVERLAPPED);
//					if (h == INVALID_HANDLE_VALUE) // Couldn't open for write access, gotta abort.
//					{
//						if (!collision) // If we just added it, remove it.
//							m_fileIdMap.remove(itor);
//						break; // go to end
//					}
//					writeHandle = rcnew(auto_HANDLE)(h);
//				}
//				FILE_STANDARD_INFO fsi;
//				BOOL b = GetFileInformationByHandleEx(writeHandle->get(), FileStandardInfo, &fsi, sizeof(FILE_STANDARD_INFO));
//				COGS_ASSERT(b);
//				rawWriteFile = rcnew(file<io::write_access>)(writeHandle.get_ref(), fileInfo.get_ref(), fsi.EndOfFile.QuadPart);
//				fileInfo->m_rawWriteFile = rawWriteFile;
//			}
//
//			if (((accessMode & io::read_access) != 0) && !rawReadFile)
//			{
//				rawReadFile = rcnew(file<io::read_access>)(o->m_handle.get_ref(), fileInfo.get_ref());
//				fileInfo->m_rawReadFile = rawReadFile;
//			}
//
//			if (accessMode == io::read_access)
//				o->set_file(io::synchronized_file<io::read_access, file_size_t>::create(rawReadFile.get_ref(), fileInfo->m_syncImpl.get_ref()));
//			else if (accessMode == io::write_access)
//				o->set_file(io::synchronized_file<io::write_access, file_size_t>::create(rawWriteFile.get_ref(), fileInfo->m_syncImpl.get_ref()));
//			else if (accessMode == io::read_write_access)
//				o->set_file(io::synchronized_file<io::read_write_access, file_size_t>::create(rawReadFile.get_ref(), rawWriteFile.get_ref(), fileInfo->m_syncImpl.get_ref()));
//			break;
//		}
//		// cleanup
//		o->m_handle = 0;
//		o->complete();
//		m_serialQueue->run_next();
//	}
//
//	void release_file_info(const file_release_info& releaseInfo)
//	{
//		if (releaseInfo.m_fileInfo == *(releaseInfo.m_itor))
//			m_fileIdMap.remove(releaseInfo.m_itor);
//		m_serialQueue->run_next();
//	}
//
//	static rcptr<file_globals> get_globals()
//	{
//		volatile rcptr<file_globals>& globalsPtr = s_globals.get();
//		rcptr<file_globals> myGlobals = globalsPtr;
//		if (!myGlobals)
//		{
//			rcptr<file_globals> newGlobals = rcnew(file_globals);
//			if (globalsPtr.compare_exchange(newGlobals, myGlobals, myGlobals))
//				myGlobals = newGlobals;
//		}
//		return myGlobals;
//	}
//
//public:
//	static void shutdown()
//	{
//		// os::file::shutdown() happens right before the memory manager is shut down.  This should be the
//		// only thread running.  If files are still around or being opened at this point, something must
//		// have gone horribly wrong.
//		volatile rcptr<file_globals>& globalsPtr = s_globals.get();
//		//rcptr<file_globals> tmp = get_globals();
//		globalsPtr = 0;
//	}
//};
//
//
//template <>
//class file<io::read_access> : public io::file<io::read_access, io::default_file_size_t>, public object
//{
//private:
//	typedef file<io::read_access> this_t;
//
//	file(const this_t&) = delete;
//	this_t& operator=(const this_t&) = delete;
//
//	typedef io::default_file_size_t file_size_t;
//
//	class reader : public io::file<io::read_access, file_size_t>::reader
//	{
//	public:
//		rcref<file<io::read_access> > m_file;
//
//		reader(const rcref<io::segment_map<file_size_t> >& sm, const rcref<file<io::read_access> >& f)
//			: io::file<io::read_access, file_size_t>::reader(sm),
//			m_file(f)
//		{ }
//	};
//
//	rcref<auto_HANDLE> m_readHandle;
//	rcref<file_globals::file_info> m_fileInfo;
//
// 	virtual rcref<io::file<io::read_access, file_size_t>::reader> begin_read(const rcref<io::segment_map<file_size_t> >& sm)
//	{
//		return rcnew(reader)(sm, this_rcref);
//	}
//
//	class size_reader : public io::file<io::read_access, file_size_t>::size_reader
//	{
//	public:
//		size_reader(io::default_file_size_t sz)
//			: io::file<io::read_access, file_size_t>::size_reader(sz)
//		{ }
//
//		void complete() { io::file<io::read_access, file_size_t>::size_reader::complete(); }
//	};
//
//	virtual rcref<io::file<io::read_access, file_size_t>::size_reader> get_size()
//	{
//		FILE_STANDARD_INFO fsi;
//		BOOL b = GetFileInformationByHandleEx(m_readHandle->get(), FileStandardInfo, &fsi, sizeof(FILE_STANDARD_INFO));
//
//		rcref<size_reader> result = rcnew(size_reader)(fsi.EndOfFile.QuadPart);
//		result->complete();
//		return result;
//	}
//
//protected:
//	friend class file_globals;
//
//	file(const rcref<auto_HANDLE>& ah, const rcref<file_globals::file_info>& fileInfo)
//		: m_readHandle(ah),
//		m_fileInfo(fileInfo)
//	{ }
//
//public:
//	class opener : public waitable
//	{
//	private:
//		opener(const opener&) = delete;
//		opener& operator=(const opener&) = delete;
//
//		rcref<file_globals::opener> m_openerInternals;
//
//	protected:
//		friend class file;
//
//		opener(const rcref<file_globals::opener>& o)
//			: m_openerInternals(o)
//		{ }
//
//	public:
//		const rcptr<io::synchronized_file<io::read_access, io::default_file_size_t> >& get_file() const { return m_openerInternals->get_read_file(); }
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_openerInternals->timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d) const volatile { m_openerInternals->dispatch(d); }
//	};
//
//	static rcref<opener> open(const string& location)
//	{
//		rcptr<file_globals> g = file_globals::get_globals();
//		return rcnew(opener)(g->open<io::read_access>(location, false));
//	}
//};
//
//template <>
//class file<io::write_access> : public io::file<io::write_access, io::default_file_size_t>, public object
//{
//private:
//	typedef file<io::read_access> this_t;
//
//	file(const this_t&) = delete;
//	this_t& operator=(const this_t&) = delete;
//
//	typedef io::default_file_size_t file_size_t;
//
//	rcref<auto_HANDLE> m_writeHandle;
//	rcref<file_globals::file_info> m_fileInfo;
//
//	volatile number<int_to_fixed_integer_t<file_size_t> > m_eof;
//
//	class writer : public io::file<io::write_access, file_size_t>::writer
//	{
//	public:
//		rcref<file<io::write_access> > m_file;
//
//		writer(const rcref<io::segment_buffer_map<file_size_t> >& sbm, const rcref<file<io::write_access> >& f)
//			: io::file<io::write_access, file_size_t>::writer(sbm),
//			m_file(f)
//		{ }
//	};
//
// 	virtual rcref<io::file<io::write_access, file_size_t>::writer> begin_write(const rcref<io::segment_buffer_map<file_size_t> >& sbm)
//	{
//		return rcnew(writer)(sbm, this_rcref);
//	}
//
//	class size_writer : public io::file<io::write_access, file_size_t>::size_writer
//	{
//	public:
//		size_writer(io::default_file_size_t sz)
//			: io::file<io::write_access, file_size_t>::size_writer(sz)
//		{ }
//
//		void complete() { io::file<io::write_access, file_size_t>::size_writer::complete(); }
//	};
//
//	virtual rcref<io::file<io::write_access, file_size_t>::size_writer> set_size(const io::default_file_size_t& sz)
//	{
//		FILE_END_OF_FILE_INFO feofi;
//		feofi.EndOfFile.QuadPart = sz;
//		BOOL b = SetFileInformationByHandle(m_writeHandle->get(), FileEndOfFileInfo, &feofi, sizeof(FILE_END_OF_FILE_INFO));
//		COGS_ASSERT(b);
//
//		rcref<size_writer> result = rcnew(size_writer)(sz);
//		result->complete();
//		return result;
//	}
//
//protected:
//	friend class file_globals;
//
//	file(const rcref<auto_HANDLE>& ah, const rcref<file_globals::file_info>& fileInfo, const file_size_t& eof)
//		: m_writeHandle(ah),
//		m_fileInfo(fileInfo),
//		m_eof(eof)
//	{ }
//
//public:
//	class opener : public waitable
//	{
//	private:
//		opener(const opener&) = delete;
//		opener& operator=(const opener&) = delete;
//
//		rcref<file_globals::opener> m_openerInternals;
//
//	protected:
//		friend class file;
//
//		opener(const rcref<file_globals::opener>& o)
//			: m_openerInternals(o)
//		{ }
//
//	public:
//		const rcptr<io::synchronized_file<io::write_access, io::default_file_size_t> >& get_file() const { return m_openerInternals->get_write_file(); }
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_openerInternals->timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d) const volatile { m_openerInternals->dispatch(d); }
//	};
//
//	static rcref<opener> open(const string& location, bool createIfNotPresent = false)
//	{
//		rcptr<file_globals> g = file_globals::get_globals();
//		return rcnew(opener)(g->open<io::write_access>(location, createIfNotPresent));
//	}
//};
//
//template <>
//class file<io::read_write_access>
//{
//private:
//	typedef file<io::read_access> this_t;
//
//	file(const this_t&) = delete;
//	this_t& operator=(const this_t&) = delete;
//
//	typedef io::default_file_size_t file_size_t;
//
//	file() = delete;
//
//public:
//	class opener : public file<io::read_access>::opener, public file<io::write_access>::opener
//	{
//	private:
//		opener(const opener&) = delete;
//		opener& operator=(const opener&) = delete;
//
//		rcref<file_globals::opener> m_openerInternals;
//
//	protected:
//		friend class file;
//
//		opener(const rcref<file_globals::opener>& o)
//			: file<io::read_access>::opener(o),
//			file<io::write_access>::opener(o),
//			m_openerInternals(o)
//		{ }
//
//	public:
//		const rcptr<io::synchronized_file<io::read_write_access, io::default_file_size_t> >& get_file() const { return m_openerInternals->get_read_write_file(); }
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_openerInternals->timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d) const volatile { m_openerInternals->dispatch(d); }
//
//		bool test() const { return timed_wait(timeout_t::none()); }
//		bool operator!() const { return !test(); }
//		void wait() const { timed_wait(timeout_t::infinite()); }
//	};
//
//	static rcref<opener> open(const string& location, bool createIfNotPresent = true)
//	{
//		rcptr<file_globals> g = file_globals::get_globals();
//		return rcnew(opener)(g->open<io::read_write_access>(location, createIfNotPresent));
//	}
//};
//
//
//	/*
//class file : public io::file<io::read_write_access, io::default_file_size_t>, public object
//{
//private:
//	file(const file&) = delete;
//	file& operator=(const file&) = delete;
//
//	typedef io::default_file_size_t file_size_t;
//
//	class reader : public io::file<io::read_access, io::default_file_size_t>::reader
//	{
//	public:
//		rcref<file> m_file;
//
//		reader(const rcref<io::segment_map<file_size_t> >& sm, const rcref<file>& f)
//			: io::file<io::read_access, io::default_file_size_t>::reader(sm),
//			m_file(f)
//		{ }
//	};
//
//	class writer : public io::file<io::write_access, file_size_t>::writer
//	{
//	public:
//		rcref<file> m_file;
//
//		writer(const rcref<io::segment_buffer_map<file_size_t> >& sbm, const rcref<file>& f)
//			: io::file<io::write_access, file_size_t>::writer(sbm),
//			m_file(f)
//		{ }
//	};
//
//	rcref<auto_HANDLE> m_readHandle;
//	weak_rcptr<auto_HANDLE> m_writeHandle;
//
//	size_t m_writeCount;
//
//	file(const rcref<auto_HANDLE>& readHandle)
//		: m_readHandle(readHandle),
//		m_writeCount(0)
//	{ }
//
// 	virtual rcref<io::file<io::read_access, file_size_t>::reader> begin_read(const rcref<io::segment_map<file_size_t> >& sm)
//	{
//		return rcnew(reader)(sm, this_rcref);
//	}
//
//	virtual rcref<io::file<io::write_access, file_size_t>::writer> begin_write(const rcref<io::segment_buffer_map<file_size_t> >& sbm)
//	{
//		return rcnew(writer)(sbm, this_rcref);
//	}
//
//	// On Win32, the best way to tell if two paths refer to the same file is to open both files with CreateFile,
//	// call GetFileInformationByHandle for both, and compare dwVolumeSerialNumber, nFileIndexLow, nFileIndexHigh.
//	// If all three are equal they both point to the same file.
//	class file_id
//	{
//	public:
//		DWORD m_dwVolumeSerialNumber;
//		DWORD m_nFileIndexHigh;
//		DWORD m_nFileIndexLow;
//
//		rcptr<file> m_rawFile;
//		weak_rcptr<io::synchronized_file_impl<file_size_t> > m_synchronizedFileImpl;
//
//		bool operator<(const file_id& cmp) const
//		{
//			if (m_nFileIndexLow < cmp.m_nFileIndexLow) // First compare field most likely to be different.
//				return true;
//			if (m_nFileIndexLow > cmp.m_nFileIndexLow)
//				return false;
//
//			if (m_nFileIndexHigh < cmp.m_nFileIndexHigh)
//				return true;
//			if (m_nFileIndexHigh > cmp.m_nFileIndexHigh)
//				return false;
//
//			if (m_dwVolumeSerialNumber < cmp.m_dwVolumeSerialNumber)
//				return true;
//			//if (m_dwVolumeSerialNumber > cmp.m_dwVolumeSerialNumber)
//				return false;
//		}
//	};
//
//	typedef set<file_id, true> file_id_map_t;
//
//	class globals
//	{
//	public:
//		file_id_map_t m_fileIdMap;
//		rcref<delegate_chain> m_serialQueue;
//
//		globals()
//			: m_serialQueue(delegate_chain::create())
//		{ }
//	};
//
//	static placement<rcptr<globals> > s_globals;
//
//	template<io::access_mode accessMode>
//	class synchronized_file : public io::synchronized_file<accessMode, io::default_file_size_t>
//	{
//	public:
//		struct content_with_handle
//		{
//			file_id_map_t::iterator m_itor;
//			rcref<auto_HANDLE> m_writeHandle;
//
//#error still a problem.  If all high level references go away, we're still disposing of the m_writeHandle will still in use.
//#error need a way for the write handle to life as long as all write operations.
//
//			void set_write_handle(const rcref<m_writeHandle>& ah) { m_writeHandle = ah; }
//			rcptr<m_writeHandle> get_write_handle() const { return m_writeHandle; }
//		};
//
//		struct content_without_handle
//		{
//			file_id_map_t::iterator m_itor;
//
//			void set_write_handle(const rcref<m_writeHandle>& ah) { }
//			rcptr<m_writeHandle> get_write_handle() const { return 0; }
//		};
//		typedef conditional<((accessMode & write_access) != 0), content_with_handle, content_without_handle> m_contents;
//
//		synchronized_file(const rcref<io::synchronized_file_impl<io::default_file_size_t> >& impl, const file_id_map_t::iterator& itor)
//			: io::synchronized_file<io::read_write_access, io::default_file_size_t>(impl),
//			m_itor(itor)
//		{ }
//
//		void set_write_handle(const rcref<m_writeHandle>& ah) { m_contents.set_write_handle(ah); }
//		rcptr<m_writeHandle> get_write_handle() const { return m_contents.get_write_handle(); }
//	};
//
//	class synchronized_file_impl : public io::synchronized_file_impl<io::default_file_size_t>
//	{
//	public:
//		~synchronized_file_impl()
//		{
//#error
//			rcptr<globals> g = get_globals();
//			typedef delegate_t<void, const file_id_map_t::iterator&> itor_delegate_t;
//			g->m_serialQueue->submit(delegate(itor_delegate_t(&file::release_write_reference), m_itor));
//		}
//	};
//
//public:
//	virtual file_size_t get_eof() const
//	{
//		file_size_t result = 0;
//
//		// TBD
//
//		return result;
//	}
//
//	virtual void set_eof(file_size_t newEof)
//	{
//		// TBD
//	}
//
//	template <io::access_mode mode>
//	class opener : public waitable
//	{
//	private:
//		opener(const opener&) = delete;
//		opener& operator=(const opener&) = delete;
//
//		single_fire_condition m_condition;
//		rcptr<io::synchronized_file<mode, io::default_file_size_t> > m_file;
//
//	protected:
//		friend class file;
//
//		opener() { self_acquire(); }
//
//		file_id m_fileId;
//		rcptr<auto_HANDLE> m_handle;
//
//		void complete() { m_condition.signal(); self_release(); }
//
//	public:
//		const rcptr<io::synchronized_file<mode, io::default_file_size_t> >& get_file() const { return m_file; }
//
//		virtual bool timed_wait(const timeout_t& timeout, unsigned int spinCount = 0) const volatile { return m_condition.timed_wait(timeout, spinCount); }
//		virtual void dispatch(const delegate& d) const volatile { m_condition.dispatch(d); }
//
//		typedef delegate_t<void, const rcref<const reader>&> dispatch_t;
//		void dispatch(const dispatch_t& d) const { dispatch(delegate(d, this_rcref)); }
//	};
//
//	enum class create_mode
//	{
//		open_if_exists = 0x01,
//		create_only = 0x02,
//		open_or_create = 0x03,
//
//	//	open_mask = 0x01,
//	//	create_mask = 0x02,
//	};
//
//
//	template <io::access_mode accessMode>
//	static rcref<opener<accessMode> > open(const string& location)
//	{
//		return open<accessMode, ((accessMode & io::write_access) != 0) ? open_or_create : open_if_exists>(location);
//	}
//
//	template <io::access_mode accessMode, create_mode createMode>
//	static rcref<opener<accessMode> > open(const string& location)
//	{
//		// Can't use create_only if opening for only read access.
//		static_assert((createMode != create_only) || ((accessMode & write_access) != 0));
//
//		rcref<opener<accessMode> > o = rcnew(opener<accessMode>);
//
//		// On Win32, the best way to tell if two paths refer to the same file is to open both files with CreateFile,
//		// call GetFileInformationByHandle for both, and compare dwVolumeSerialNumber, nFileIndexLow, nFileIndexHigh.
//		// If all three are equal they both point to the same file.
//		//
//		// If we have already opened the file, it's possible we may already have write access, and not sharing
//		// write access.  For now, request only read access.
//		DWORD dwDesiredAccess = GENERIC_READ;
//		DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
//
//		// Use of a synchronized_file<> means that writing to the file concurrently from another process, is not supported.
//		// However, if we don't open read handles with FILE_SHARE_WRITE permission, we'll be unable to add a write-capabale
//		// handle later.  A write-handle will remove FILE_SHARE_WRITE.
//
//		DWORD dwCreationDisposition = OPEN_EXISTING;
//		switch (createMode)
//		{
//		case create_only:
//			dwCreationDisposition = CREATE_NEW;
//			break;
//		case open_or_create:
//			dwCreationDisposition = OPEN_ALWAYS;
//			break;
//		default:
//			break;
//		};
//
//		for (;;)
//		{
//			// Unfortunately, Win32 doesn't currently offer a way to open a file asynchronously.
//			HANDLE h = CreateFile(location.cstr(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, FILE_FLAG_OVERLAPPED, NULL);
//			if (h != INVALID_HANDLE_VALUE)
//			{
//				o->m_handle = rcnew(auto_HANDLE)(h);
//
//				// Even if we had created it with create_only, it's still possible another thread just now opened
//				// the same file and already created a map entry for it.  Gotta check.
//				BY_HANDLE_FILE_INFORMATION fileInfo;
//				BOOL b = GetFileInformationByHandle(h, &fileInfo);
//				if (!!b)
//				{
//					o->m_fileId.m_dwVolumeSerialNumber = fileInfo.dwVolumeSerialNumber;
//					o->m_fileId.m_nFileIndexHigh = fileInfo.nFileIndexHigh;
//					o->m_fileId.m_nFileIndexLow = fileInfo.nFileIndexLow;
//
//					rcptr<globals> g = get_globals();
//					typedef delegate_t<void, const rcref<opener<accessMode> >&> temp_delegate_t;
//					g->m_serialQueue->submit(delegate(temp_delegate_t(&file::find_match<accessMode>), o));
//					break;
//				}
//				o->m_handle = 0;
//			}
//			o->complete();
//			break;
//		}
//
//		return o;
//	}
//
//	template <io::access_mode accessMode>
//	static void find_match(const rcref<opener<accessMode> >& o)
//	{
//		rcptr<io::synchronized_file_impl<file_size_t> > synchronizedFileImpl;
//		rcptr<file> rawFile;
//		bool readAlreadyOpen = false;
//		bool writeAlreadyOpen = false;
//		bool collision;
//		HANDLE writeHandle = INVALID_HANDLE_VALUE;
//		rcptr<globals> g = get_globals();
//		file_id_map_t::iterator itor = g->m_fileIdMap.insert_unique(o->m_fileId, collision);
//		if (collision) // Update existing file_id in the map to include write access, in case we just added that.
//		{
//			synchronizedFileImpl = itor->m_synchronizedFileImpl;
//			if (!!synchronizedFileImpl)
//			{
//				readAlreadyOpen = true;
//				rawFile = itor->m_rawFile;
//				if ((accessMode & io::write_access) != 0)
//					writeAlreadyOpen = !!(rawFile->m_writeHandle);
//			}
//			// else // If it has been released (there would be an operation pending to remove this, but we opened the file first.)
//		}           // That pending operation will only remove it, if it doesn't point to a new synchronizedFile
//		for (;;)
//		{
//			if ((accessMode & io::write_access) != 0) // If we just opened for write access ...
//			{
//				if (!writeAlreadyOpen) // ... and it doesn't have a write handle yet, open one.
//				{ // At least we can use OpenFileById() to avoid hitting the directory structure again.
//					FILE_ID_DESCRIPTOR fileIdDesc;
//					fileIdDesc.dwSize = sizeof(FILE_ID_DESCRIPTOR);
//					fileIdDesc.Type = FileIdType; // 0
//					fileIdDesc.FileId.HighPart = o->m_fileId.m_nFileIndexHigh;
//					fileIdDesc.FileId.LowPart = o->m_fileId.m_nFileIndexLow;
//					writeHandle = OpenFileById(o->m_handle->get(), &fileIdDesc, GENERIC_WRITE, FILE_SHARE_READ, NULL, FILE_FLAG_OVERLAPPED);
//					if (writeHandle == INVALID_HANDLE_VALUE) // Couldn't open for write access, gotta abort.
//					{
//						if (!collision) // If we just added it, remove it.
//							g->m_fileIdMap.remove(itor);
//						break;
//					}
//				}
//			}
//			if (!readAlreadyOpen) // We need to add a new rawFile and synchronizedFile
//			{
//				rawFile = rcnew(file)(o->m_handle.get_ref());
//				synchronizedFileImpl = rcnew(io::synchronized_file_impl<file_size_t>)(rawFile.get_ref(), rawFile.get_ref());
//				itor->m_synchronizedFileImpl = synchronizedFileImpl;
//				itor->m_rawFile = rawFile;
//			}
//			if ((accessMode & io::write_access) != 0)
//			{
//				++(rawFile->m_writeCount);
//				if (!writeAlreadyOpen)
//					rawFile->m_writeHandle = rcnew(auto_HANDLE)(writeHandle);
//			}
//			o->m_file = rcnew(synchronized_file<accessMode>)(synchronizedFileImpl.get_ref());
//			break;
//		}
//		o->m_handle = 0;
//		o->complete();
//		g->m_serialQueue->run_next();
//	}
//
//	static void release_write_reference(const file_id_map_t::iterator& itor)
//	{
//		rcptr<globals> g = get_globals();
//		rcptr<file> f = itor->m_rawFile;
//		if (!--(f->m_writeCount))
//			f->m_writeHandle = 0; // if the last write closing, close write handle.
//		g->m_serialQueue->run_next();
//	}
//
//	template <io::access_mode accessMode>
//	static void release_file_id(const file_id_map_t::iterator& itor)
//	{
//		rcptr<globals> g = get_globals();
//		rcptr<file> f = itor->m_rawFile;
//		if (!((accessMode & io::write_access) != 0) && !--(f->m_writeCount))
//			m_rawFile->m_writeHandle = 0; // if the last write closing, close write handle.
//		g->m_serialQueue->run_next();
//	}
//
//	static rcptr<globals> get_globals()
//	{
//		volatile rcptr<globals>& globalsPtr = s_globals.get();
//		rcptr<globals> myGlobals = globalsPtr;
//		if (!myGlobals)
//		{
//			rcptr<globals> newGlobals = rcnew(globals);
//			if (globalsPtr.compare_exchange(newGlobals, myGlobals, myGlobals))
//				myGlobals = newGlobals;
//		}
//		return myGlobals;
//	}
//
//	static void shutdown()
//	{
//		// os::file::shutdown() happens right before the memory manager is shut down.  This should be the
//		// only thread running.  If files are still around or being opened at this point, something must
//		// have gone horribly wrong.
//		volatile rcptr<globals>& globalsPtr = s_globals.get();
//		//rcptr<globals> tmp = get_globals();
//		//COGS_ASSERT(tmp->m_fileIdMap.is_empty());
//		globalsPtr = 0;
//	}
//};
//*/
//
///*
//inline rcptr<io::file<io::read_access> > io::file<io::read_access>::open(const string& location)
//{
//	HANDLE h = CreateFile(location.cstr(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//	if (h == INVALID_HANDLE_VALUE)
//		return rcptr<io::file<io::read_access> >();
//	return rcnew(os::file)(h);
//}
//
//inline rcptr<io::file<io::read_write_access> > io::file<io::read_write_access>::open(const string& location, io::file<io::read_write_access>::create_mode mode)
//{
//	DWORD dwCreationDisposition = 0;
//	switch (mode)
//	{
//	case open_if_exists:
//		dwCreationDisposition = OPEN_EXISTING;
//		break;
//	case create_only:
//		dwCreationDisposition = CREATE_NEW;
//		break;
//	case open_or_create:
//		dwCreationDisposition = OPEN_ALWAYS;
//		break;
//	case open_truncate_or_create:
//		dwCreationDisposition = CREATE_ALWAYS;
//		break;
//	default:
//		COGS_ASSERT(false);
//	};
//
//	// Lack of FILE_SHARE_WRITE ensures there are no writes occuring to this file elsewhere.
//	HANDLE h = CreateFile(location.cstr(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
//	if (h == INVALID_HANDLE_VALUE)
//		return rcptr<io::file<io::read_write_access> >();
//
//	rcref<os::file> f = rcnew(os::file)(h);
//	f->m_eof = f->get_eof();
//	return f;
//}
//*/
//
//
//
//	/*
//class file : public io::file<io::read_write_access>
//{
//protected:
//	file();
//	file(const file&);
//
//	class file_reader : public io::file<io::read_access>::reader
//	{
//	protected:
//		weak_rcptr<auto_HANDLE> m_handle;
//		rcref<completion_port> m_completionPort;
//		buffer m_currentBuffer;
//		completion_port::overlapped_t* m_overlapped;
//
//	public:
//		file_reader(const weak_rcptr<auto_HANDLE>& h, const rcref<completion_port>& cp = completion_port::get())
//			: m_handle(h),
//			m_overlapped(0),
//			m_completionPort(cp)
//		{ }
//
//		virtual void reading()
//		{
//			// Defer initiating the actual read to the completion port thread.
//			// This is because, on Windows, if the thread initiating asynchronous IO
//			// terminates, so does the IO request.
//			m_completionPort->dispatch(COGS_CONST_DELEGATE_FROM_RC_MEMBER(&file_reader::execute_in_completion_port_thread, this_rcref, reference_strength::strong));
//		}
//
//		void execute_in_completion_port_thread()
//		{
//			rcptr<auto_HANDLE> ah = m_handle;
//			if (!ah)
//				complete();
//			else
//			{
//				m_overlapped = new (default_memory_manager::get()) completion_port::overlapped_t(COGS_CONST_DELEGATE_FROM_RC_MEMBER(&file_reader::read_done, this_rcref, reference_strength::strong));
//				LARGE_INTEGER offset;
//				offset.QuadPart = m_offset;
//				m_overlapped->Offset = offset.LowPart;
//				m_overlapped->OffsetHigh = offset.HighPart;
//				m_currentBuffer.set(m_requestedSize);
//				BOOL b = ReadFileEx(ah->m_handle, m_currentBuffer.get_ptr(), m_requestedSize, m_overlapped, NULL);
//				if (!b)
//				{
//					complete();
//					default_memory_manager::destruct_deallocate_type(m_overlapped);
//				}
//			}
//		}
//
//		void read_done()
//		{
//			size_t n = m_overlapped->m_transferCount;
//			m_currentBuffer.truncate_to(n);
//			get_buffer().append(m_currentBuffer);
//			complete();
//			default_memory_manager::destruct_deallocate_type(m_overlapped);
//		}
//	};
//
//	class file_writer : public io::file<io::read_write_access>::writer
//	{
//	protected:
//		weak_rcptr<auto_HANDLE> m_handle;
//		rcref<completion_port> m_completionPort;
//		completion_port::overlapped_t* m_overlapped;
//		buffer m_currentBuffer;
//		uint64_t m_curOffset;
//
//	public:
//		file_writer(const weak_rcptr<auto_HANDLE>& h, const rcref<completion_port>& cp = completion_port::get())
//			: m_handle(h),
//			m_overlapped(0),
//			m_completionPort(cp)
//		{ }
//
//		virtual void writing()
//		{
//			// Defer initiating the actual read to the completion port thread.
//			// This is because, on Windows, if the thread initiating asynchronous IO
//			// terminates, so does the IO request.
//			m_curOffset = m_offset;
//			m_completionPort->dispatch(COGS_CONST_DELEGATE_FROM_RC_MEMBER(&file_writer::execute_in_completion_port_thread, this_rcref, reference_strength::strong));
//		}
//
//		void execute_in_completion_port_thread()
//		{
//			rcptr<auto_HANDLE> ah = m_handle;
//			if (!ah)
//				complete();
//			else
//			{
//				if (!m_overlapped)
//					m_overlapped = new (default_memory_manager::get()) completion_port::overlapped_t(COGS_CONST_DELEGATE_FROM_RC_MEMBER(&file_writer::write_done, this_rcref, reference_strength::strong));
//				else
//					m_overlapped->clear();
//				LARGE_INTEGER offset;
//				offset.QuadPart = m_curOffset;
//				m_overlapped->Offset = offset.LowPart;
//				m_overlapped->OffsetHigh = offset.HighPart;
//				if (!m_currentBuffer)
//					m_currentBuffer = get_buffer().remove_first();
//				BOOL b = WriteFileEx(ah->m_handle, m_currentBuffer.get_ptr(), m_currentBuffer.size(), m_overlapped, NULL);
//				if (!b)
//				{
//					complete();
//					default_memory_manager::destruct_deallocate_type(m_overlapped);
//				}
//			}
//		}
//
//		void write_done()
//		{
//			size_t n = m_overlapped->m_transferCount;
//			if (!n)
//			{
//				get_buffer().prepend(m_currentBuffer);
//				complete();
//				default_memory_manager::destruct_deallocate_type(m_overlapped);
//			}
//			else
//			{
//				m_currentBuffer.advance(n);
//				if (!m_currentBuffer && !get_buffer())
//				{
//					complete();
//					default_memory_manager::destruct_deallocate_type(m_overlapped);
//				}
//				else
//				{
//					m_curOffset += n;
//					m_completionPort->dispatch(COGS_CONST_DELEGATE_FROM_RC_MEMBER(&file_writer::execute_in_completion_port_thread, this_rcref, reference_strength::strong));
//				}
//			}
//		}
//	};
//
//	file(HANDLE h)
//		: m_handle(rcnew(auto_HANDLE)(h))
//	{ }
//
//	virtual rcref<io::file<io::read_write_access>::reader> create_reader()
//	{
//		return rcnew(file_reader)(m_handle);
//	}
//
//	virtual rcref<io::file<io::read_write_access>::writer> create_writer()
//	{
//		return rcnew(file_writer)(m_handle);
//	}
//
//	virtual uint64_t eof()
//	{
//		LARGE_INTEGER fileSize;
//		BOOL b = GetFileSizeEx(m_handle->m_handle, &fileSize);
//		return fileSize.QuadPart;
//	}
//
//private:
//	rcref<auto_HANDLE> m_handle;
//
//	friend inline rcptr<io::file<io::read_access> > io::file<io::read_access>::open(const string& location);
//	friend inline rcptr<io::file<io::read_write_access> > io::file<io::read_write_access>::open(const string& location, io::file<io::read_write_access>::create_mode mode);
//};
//
//};
//
//
//inline rcptr<io::file<io::read_access> > io::file<io::read_access>::open(const string& location)
//{
//	HANDLE h = CreateFile(location.cstr(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//	if (h == INVALID_HANDLE_VALUE)
//		return rcptr<io::file<io::read_access> >();
//	return rcnew(os::file)(h);
//}
//*/
///*
//inline rcptr<io::file<io::read_write_access> > io::file<io::read_write_access>::open(const string& location, io::file<io::read_write_access>::create_mode mode)
//{
//	DWORD dwCreationDisposition = 0;
//	switch (mode)
//	{
//	case open_if_exists:
//		dwCreationDisposition = OPEN_EXISTING;
//		break;
//	case create_only:
//		dwCreationDisposition = CREATE_NEW;
//		break;
//	case open_or_create:
//		dwCreationDisposition = OPEN_ALWAYS;
//		break;
//	case open_truncate_or_create:
//		dwCreationDisposition = CREATE_ALWAYS;
//		break;
//	default:
//		COGS_ASSERT(false);
//	};
//
//	// Lack of FILE_SHARE_WRITE ensures there are no writes occuring to this file elsewhere.
//	HANDLE h = CreateFile(location.cstr(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
//	if (h == INVALID_HANDLE_VALUE)
//		return rcptr<io::file<io::read_write_access> >();
//
//	rcref<os::file> f = rcnew(os::file)(h);
//	f->m_eof = f->get_eof();
//	return f;
//}
//*/
//
//
//}
//}
//
//
//#endif
//
