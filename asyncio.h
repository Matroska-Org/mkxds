/*
** CAsyncIOCallback
**
** implements IOCallback derived class to enable libmatroska to read from
** file via IAsyncReader interface.
**
*/

#ifndef _ASYNCIOCALLBACK_H_
#define _ASYNCIOCALLBACK_H_

#include <streams.h>

#include "KaxConfig.h"
#include "IOCallback.h"

using namespace LIBEBML_NAMESPACE;
START_LIBMATROSKA_NAMESPACE


class CAsyncIOCb : public IOCallback
{
private:
	int64			m_offs, m_size;
	BOOL			m_open;
	IAsyncReader	*m_pReader;

public:
	CAsyncIOCb(IAsyncReader *pReader);

	virtual uint32 read(void*Buffer,size_t Size);
	virtual size_t write(const void*Buffer,size_t Size);

//	virtual void readFully(void*Buffer,size_t Size);
	void writeFully(const void*Buffer,size_t Size);

	virtual void close();

	virtual void setFilePointer(int64 Offset, seek_mode Mode=seek_beginning);
	virtual uint64 getFilePointer();
};


END_LIBMATROSKA_NAMESPACE

#endif _ASYNCIOCALLBACK_H_
