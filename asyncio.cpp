#include "asyncio.h"

#define	DBGBOX(x)	MessageBox(NULL, x, "DbgMessage", MB_OK);

START_LIBMATROSKA_NAMESPACE

CAsyncIOCb::CAsyncIOCb(IAsyncReader *pReader):
 m_open(TRUE),
 m_pReader(pReader)
{
	if (m_pReader == NULL) return;
	m_pReader->Length(&m_size, &m_offs);
	m_offs = 0;
}

uint32 CAsyncIOCb::read(void*Buffer,size_t Size)
{
	BYTE	*buff;

	if (!m_open) return 0;
	buff = (BYTE*)Buffer;
	if (m_pReader->SyncRead(m_offs, Size, buff) != S_OK) return 0;
	m_offs += Size;
	return Size;	// assume we have read Size bytes
}

size_t CAsyncIOCb::write(const void*Buffer,size_t Size)
{
	return 0;
}
/*
void CAsyncIOCallback::readFully(void*Buffer,size_t Size)
{
    if(Buffer == NULL)
	{
		MessageBox(NULL, "error: empty input buffer", "ERR", MB_OK);
		return;
	}

    if(read(Buffer,Size) != Size)
    {
		MessageBox(NULL, "error: read error(EOF)", "ERR", MB_OK);
		return;
    }
}
*/
void CAsyncIOCb::writeFully(const void*Buffer,size_t Size)
{
    if (Buffer == NULL) MessageBox(NULL, "error: empty input buffer", "ERR (wr)", MB_OK);
}

void CAsyncIOCb::close()
{
	m_open = FALSE;
	m_pReader->Release();
}

void CAsyncIOCb::setFilePointer(int64 Offset, seek_mode Mode)
{
	switch (Mode)
	{
		case seek_beginning:
			m_offs = Offset;
			break;

		case seek_end:
			break;

		case seek_current:
			m_offs += Offset;
			break;
	}
}

uint64 CAsyncIOCb::getFilePointer()
{
	if (!m_open) return 0;
	return m_offs;
}

END_LIBMATROSKA_NAMESPACE
