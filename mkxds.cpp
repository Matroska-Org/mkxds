/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2003 Steve Lhomme.  All rights reserved.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding an other license may use this file in accordance with 
** the Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.matroska.org/license/qpl/ for QPL licensing information.
** See http://www.matroska.org/license/gpl/ for GPL licensing information.
**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/
/*!
	\file
	\version \$Id: mkxds.cpp,v 1.9 2003/07/31 19:44:01 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Christophe Paris <toffparis @ users.sf.net>
*/

#include <streams.h>
#include <initguid.h>    // declares DEFINE_GUID to declare an EXTERN_C const.
                         // Use once per project.
#include <qnetwork.h> // for IAMMediaContent

#include <tchar.h>
#include "mkxds.h"

#include "WinIOCallback.h"
#include "MatroskaReader.h"
#include "mkx_opin.h"
#include "mkxPrioFrame.h"
#include "mkxread.h"
#include "mkxdsProperty.h"
#include "IChapterInfo.h"

// ----------------------------------------------------------------------------
// Data used to register the filter
// ----------------------------------------------------------------------------

// Object table - all com objects in this DLL
CFactoryTemplate g_Templates[2]=
{
	{ L"Matroska Filter"
		, &CLSID_MKXDEMUX
		, MkxFilter::CreateInstance
		, NULL
		, &MkxFilter::sudFilter 
	},
	{ L"Matroska Filter Property Page"
		, &CLSID_MKXFILTERPROP
		, MkxFilterProperty::CreateInstance
	}
};
int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);

const AMOVIESETUP_MEDIATYPE sudOutPinTypes = {
	&MEDIATYPE_NULL,
	&MEDIASUBTYPE_NULL
};

const AMOVIESETUP_PIN sudPins =
{
		L"Output",              // Obsolete
		FALSE,                  // Is this pin rendered ?
		TRUE,                   // Is it an output pin ?
		FALSE,                  // Can the filter create zero instance ?
		FALSE,                  // Does the filter create multiple instances ?
		&CLSID_NULL,            // Obsolete
		NULL,                   // Obsolete
		1,                      // Number of types
    &sudOutPinTypes };      // Pin details

// setup data - allows the self-registration to work.
const AMOVIESETUP_FILTER MkxFilter::sudFilter = {
    g_Templates[0].m_ClsID		// clsID
  , g_Templates[0].m_Name		// strName
  , MERIT_NORMAL				// dwMerit
  , 1							// nPins
  , &sudPins					// lpPin
};

// The streams.h DLL entrypoint.
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

// The entrypoint required by the MSVC runtimes. This is used instead
// of DllEntryPoint directly to ensure global C++ classes get initialised.
BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved) {
	
    return DllEntryPoint(reinterpret_cast<HINSTANCE>(hDllHandle), dwReason, lpreserved);
}

STDAPI DllRegisterServer()
{
	HKEY key;
	DWORD disp;

    // TODO : use StringFromGUID2 on CLSID instead of hardcoded GUID string

	// Identification by extension
	// HKEY_CLASSES_ROOT\Media Type\Extensions\.ext
	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT,
			_T("Media Type\\Extensions\\.mkv"), 0, _T("REG_SZ"),
			REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &disp))
	{
		// We are the source filter
		const BYTE guidSource[] = "{34293064-02F2-41d5-9D75-CC5967ACA1AB}";
		RegSetValueExA(key, "Source Filter", 0, REG_SZ, guidSource, countof(guidSource));
		RegCloseKey(key);
	}

	if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT,
		_T("Media Type\\Extensions\\.mka"), 0, _T("REG_SZ"),
		REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, &disp))
	{
		// We are the source filter
		const BYTE guidSource[] = "{34293064-02F2-41d5-9D75-CC5967ACA1AB}";
		RegSetValueExA(key, "Source Filter", 0, REG_SZ, guidSource, countof(guidSource));
		RegCloseKey(key);
	}

	return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
	RegDeleteKey(HKEY_CLASSES_ROOT, _T("Media Type\\Extensions\\.mkv"));
	RegDeleteKey(HKEY_CLASSES_ROOT, _T("Media Type\\Extensions\\.mka"));

    return AMovieDllRegisterServer2(FALSE);
}

// ***********************************************************
// 
//  The MkxDs methods
//

// Provide the way for the COM support routines in <streams.h>
// to create a CKaxDemuxFilter object
CUnknown * WINAPI MkxFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	DbgSetModuleLevel(LOG_ERROR,5);
    DbgSetModuleLevel(LOG_TRACE,5); // comment this to remove trace
	//DbgSetModuleLevel(LOG_MEMORY,2);
	DbgSetModuleLevel(LOG_LOCKING,2);
	//DbgSetModuleLevel(LOG_TIMING,5);
	
    NOTE("MkxFilter::CreateInstance");

    CUnknown *pNewObject = new MkxFilter(NAME("MkxDs Object"), punk, phr );
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return pNewObject;
}

MkxFilter::MkxFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr)
 :CSource(tszName, punk, *sudFilter.clsID, phr)
// ,m_pMKReader(NULL)
 ,m_FileHandle(NULL)
 ,m_rtStart((long)0)
 ,m_pFileName(NULL)
{
    m_rtStop = _I64_MAX / 2;
    m_rtDuration = m_rtStop;
    m_dRateSeeking = 1.0;
	
    m_dwSeekingCaps = AM_SEEKING_CanGetDuration		
		| AM_SEEKING_CanGetStopPos
		| AM_SEEKING_CanSeekForwards
        | AM_SEEKING_CanSeekBackwards
        | AM_SEEKING_CanSeekAbsolute;
}

MkxFilter::~MkxFilter()
{
	NOTE("MkxFilter::~MkxFilter");
	delete m_pRunningThread;
	delete m_FileHandle;
	delete [] m_pFileName;
}

// ============================================================================
// MkxFilter IFileSourceFilter interface
// ============================================================================

STDMETHODIMP MkxFilter::Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
	NOTE("MkxFilter::Load in...");
	CheckPointer(lpwszFileName, E_POINTER);	
	
	// lstrlenW is one of the few Unicode functions that works on win95
	int cch = lstrlenW(lpwszFileName) + 1;
	
	delete [] m_pFileName;
	m_pFileName = new WCHAR[cch];
	
	if (m_pFileName!=NULL)
	{
		CopyMemory(m_pFileName, lpwszFileName, cch*sizeof(WCHAR));	
	} else {
		NOTE("CCAudioSource::Load filename is empty !");
		return VFW_E_CANNOT_RENDER;
	}

#ifndef _UNICODE
	TCHAR *lpszFileName=0;
	lpszFileName = new char[cch * 2];
	if (!lpszFileName) {
		NOTE("MkxFilter::Load new lpszFileName failed E_OUTOFMEMORY");
		return E_OUTOFMEMORY;
	}
	WideCharToMultiByte(GetACP(), 0, lpwszFileName, -1,
		lpszFileName, cch, NULL, NULL);
	NOTE1("MkxFilter::Load Loading %s", lpszFileName);
#else
	TCHAR lpszFileName[MAX_PATH]={0};
	lstrcpy(lpszFileName, lpwszFileName);
#endif
	
	// Open the file & create pin
	m_FileHandle = new WinIOCallback(lpszFileName, MODE_READ);

#ifndef _UNICODE
	delete[] lpszFileName;
#endif

	if (m_FileHandle == NULL)
		return S_FALSE;

	// create the thread that will output data
	m_pRunningThread = new MkxReadThread(this, m_FileHandle);
	if (m_pRunningThread == NULL)
		return S_FALSE;
	m_pRunningThread->Create();
	m_rtDuration = m_rtStop = m_pRunningThread->GetDuration();
	
	return S_OK;
}

// ----------------------------------------------------------------------------

STDMETHODIMP MkxFilter::GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *pmt)
{
	NOTE("MkxFilter::GetCurFile");
	
	CheckPointer(ppszFileName, E_POINTER);
	*ppszFileName = NULL;
	
	if (m_pFileName!=NULL)
	{
		DWORD n = sizeof(WCHAR)*(1+lstrlenW(m_pFileName));
		
		*ppszFileName = (LPOLESTR) CoTaskMemAlloc( n );
		if (*ppszFileName!=NULL)
		{
			CopyMemory(*ppszFileName, m_pFileName, n);
		}
	}
	
	if (pmt!=NULL)
	{
		// TODO
		NOTE("MkxFilter::GetCurFile TODO");
	}
	
	return NOERROR;
}

// ============================================================================

STDMETHODIMP MkxFilter::Pause()
{
	NOTE("MkxFilter::Pause");
	
	if (m_pRunningThread == NULL)
		return S_FALSE;
	
	if(m_State == State_Stopped)
	{
		m_pRunningThread->UnBlockProc(false);
	}
	else if(m_State == State_Running)
	{
		m_pRunningThread->BlockProc();
	}

	return CSource::Pause();
}

STDMETHODIMP MkxFilter::Run(REFERENCE_TIME tStart)
{
	NOTE("MkxFilter::Run");
	m_pRunningThread->UnBlockProc(false);
	
	return CSource::Run(tStart);
}

STDMETHODIMP MkxFilter::Stop()
{
	NOTE("MkxFilter::Stop");

	if(m_State != State_Stopped)
	{
		m_pRunningThread->Stop();
	}	

	return CSource::Stop();
}

// ============================================================================
// MkxFilter IMediaSeeking (code from CSourceSeeking in BaseClasses\ctlutil.cpp)
// ============================================================================

HRESULT MkxFilter::ChangeStart()
{
	m_pRunningThread->SeekToTimecode(m_rtStart, m_State == State_Stopped);
	return S_OK;
}

HRESULT MkxFilter::ChangeStop()
{
	return S_OK;
}

HRESULT MkxFilter::ChangeRate()
{
	return S_OK;
}

HRESULT MkxFilter::IsFormatSupported(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    // only seeking in time (REFERENCE_TIME units) is supported
    return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT MkxFilter::QueryPreferredFormat(GUID *pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT MkxFilter::SetTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);

    // nothing to set; just check that it's TIME_FORMAT_TIME
    return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : E_INVALIDARG;
}

HRESULT MkxFilter::IsUsingTimeFormat(const GUID * pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    return *pFormat == TIME_FORMAT_MEDIA_TIME ? S_OK : S_FALSE;
}

HRESULT MkxFilter::GetTimeFormat(GUID *pFormat)
{
    CheckPointer(pFormat, E_POINTER);
    *pFormat = TIME_FORMAT_MEDIA_TIME;
    return S_OK;
}

HRESULT MkxFilter::GetDuration(LONGLONG *pDuration)
{
    CheckPointer(pDuration, E_POINTER);
    CAutoLock lock(&m_SeekLock);
    *pDuration = m_rtDuration;
    return S_OK;
}

HRESULT MkxFilter::GetStopPosition(LONGLONG *pStop)
{
    CheckPointer(pStop, E_POINTER);
    CAutoLock lock(&m_SeekLock);
    *pStop = m_rtStop;
    return S_OK;
}

HRESULT MkxFilter::GetCurrentPosition(LONGLONG *pCurrent)
{
    // GetCurrentPosition is typically supported only in renderers and
    // not in source filters.
    return E_NOTIMPL;
}

HRESULT MkxFilter::GetCapabilities( DWORD * pCapabilities )
{
    CheckPointer(pCapabilities, E_POINTER);
    *pCapabilities = m_dwSeekingCaps;
    return S_OK;
}

HRESULT MkxFilter::CheckCapabilities( DWORD * pCapabilities )
{
    CheckPointer(pCapabilities, E_POINTER);

    // make sure all requested capabilities are in our mask
    return (~m_dwSeekingCaps & *pCapabilities) ? S_FALSE : S_OK;
}

HRESULT MkxFilter::ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat,
                           LONGLONG    Source, const GUID * pSourceFormat )
{
    CheckPointer(pTarget, E_POINTER);
    // format guids can be null to indicate current format

    // since we only support TIME_FORMAT_MEDIA_TIME, we don't really
    // offer any conversions.
    if(pTargetFormat == 0 || *pTargetFormat == TIME_FORMAT_MEDIA_TIME)
    {
        if(pSourceFormat == 0 || *pSourceFormat == TIME_FORMAT_MEDIA_TIME)
        {
            *pTarget = Source;
            return S_OK;
        }
    }

    return E_INVALIDARG;
}


HRESULT MkxFilter::SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags
                      , LONGLONG * pStop,  DWORD StopFlags )
{
    DWORD StopPosBits = StopFlags & AM_SEEKING_PositioningBitsMask;
    DWORD StartPosBits = CurrentFlags & AM_SEEKING_PositioningBitsMask;

    if(StopFlags) {
        CheckPointer(pStop, E_POINTER);

        // accept only relative, incremental, or absolute positioning
        if(StopPosBits != StopFlags) {
            return E_INVALIDARG;
        }
    }

    if(CurrentFlags) {
        CheckPointer(pCurrent, E_POINTER);
        if(StartPosBits != AM_SEEKING_AbsolutePositioning &&
           StartPosBits != AM_SEEKING_RelativePositioning) {
            return E_INVALIDARG;
        }
    }


    // scope for autolock
    {
        CAutoLock lock(&m_SeekLock);

        // set start position
        if(StartPosBits == AM_SEEKING_AbsolutePositioning)
        {
            m_rtStart = *pCurrent;
        }
        else if(StartPosBits == AM_SEEKING_RelativePositioning)
        {
            m_rtStart += *pCurrent;
        }

        // set stop position
        if(StopPosBits == AM_SEEKING_AbsolutePositioning)
        {
            m_rtStop = *pStop;
        }
        else if(StopPosBits == AM_SEEKING_IncrementalPositioning)
        {
            m_rtStop = m_rtStart + *pStop;
        }
        else if(StopPosBits == AM_SEEKING_RelativePositioning)
        {
            m_rtStop = m_rtStop + *pStop;
        }
    }


    HRESULT hr = S_OK;
    if(SUCCEEDED(hr) && StopPosBits) {
        hr = ChangeStop();
    }
    if(StartPosBits) {
        hr = ChangeStart();
    }

    return hr;
}


HRESULT MkxFilter::GetPositions( LONGLONG * pCurrent, LONGLONG * pStop )
{
    if(pCurrent) {
        *pCurrent = m_rtStart;
    }
    if(pStop) {
        *pStop = m_rtStop;
    }

    return S_OK;;
}


HRESULT MkxFilter::GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest )
{
    if(pEarliest) {
        *pEarliest = 0;
    }
    if(pLatest) {
        CAutoLock lock(&m_SeekLock);
        *pLatest = m_rtDuration;
    }
    return S_OK;
}

HRESULT MkxFilter::SetRate( double dRate)
{
    {
        CAutoLock lock(&m_SeekLock);
        m_dRateSeeking = dRate;
    }
    return ChangeRate();
}

HRESULT MkxFilter::GetRate( double * pdRate)
{
    CheckPointer(pdRate, E_POINTER);
    CAutoLock lock(&m_SeekLock);
    *pdRate = m_dRateSeeking;
    return S_OK;
}

HRESULT MkxFilter::GetPreroll(LONGLONG *pPreroll)
{
    CheckPointer(pPreroll, E_POINTER);
    *pPreroll = 0;
    return S_OK;
}

// ============================================================================
// ISpecifyPropertyPages
// ============================================================================

STDMETHODIMP MkxFilter::GetPages(CAUUID *pPages)
{
    if (!pPages) return E_POINTER;

    pPages->cElems = 1;
    pPages->pElems = reinterpret_cast<GUID*>(CoTaskMemAlloc(sizeof(GUID)));
    if (pPages->pElems == NULL) 
    {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_MKXFILTERPROP;
    return NOERROR;
}

// ============================================================================
// IAMMediaContent
// ============================================================================

STDMETHODIMP MkxFilter::get_Title(BSTR FAR* pbstrTitle)
{	
	*pbstrTitle = SysAllocString(m_pRunningThread->GetTitle());
	return S_OK;
}

// ============================================================================
// IChapterInfo
// ============================================================================

const KaxChapterAtom * FindChapter(const KaxChapterAtom & Parent, UINT ChapterID)
{
	const KaxChapterAtom * result = NULL;
	const KaxChapterUID * _UID = static_cast<const KaxChapterUID *>(Parent.FindElt(KaxChapterUID::ClassInfos));
	if (_UID && uint64(*_UID) == ChapterID)
		result = &Parent;
	else {
		UINT Index0;
		for (Index0 = 0; Index0 < Parent.ListSize(); Index0++) {
			if (EbmlId(*(Parent[Index0])) == KaxChapterAtom::ClassInfos.GlobalId) {
				result = FindChapter(*static_cast<const KaxChapterAtom *>(Parent[Index0]), ChapterID);
				if (result != NULL) {
					break;
				}
			}
		}
	}
	return result;
}


STDMETHODIMP_(UINT) MkxFilter::GetChapterCount(UINT aChapterID)
{
	NOTE1("MkxFilter::GetChapterCount %d", aChapterID);
	const KaxChapters * Chapters = m_pRunningThread->GetChapters();
	if (Chapters != NULL) {
		// look for the specified chapter (0 is the 1st edition)
		const KaxEditionEntry * Edition = static_cast<const KaxEditionEntry *>((*Chapters)[0]);
		if (Edition != NULL) {
			if (aChapterID == 0)
				return Edition->ListSize();
			// recursively look for the specified chapter
			UINT Index0;
			const KaxChapterAtom *Found = NULL;
			for (Index0 = 0; Found != NULL && Index0 < Edition->ListSize(); Index0++) {
				Found = FindChapter(*static_cast<const KaxChapterAtom *>((*Edition)[Index0]), aChapterID);
			}
			if (Found != NULL)
				return Found->ListSize();
		}
	}
	return 0;
}

STDMETHODIMP_(UINT) MkxFilter::GetChapterId(UINT aParentChapterId, UINT aIndex)
{
	NOTE2("MkxFilter::GetChapterId %d/%d", aParentChapterId, aIndex);
	const KaxChapters * Chapters = m_pRunningThread->GetChapters();
	const KaxChapterAtom *Found = NULL;
	if (aIndex > 0 && Chapters != NULL) {
		// look for the specified chapter (0 is the 1st edition)
		const KaxEditionEntry * Edition = static_cast<const KaxEditionEntry *>((*Chapters)[0]);
		if (Edition != NULL) {
			if (aParentChapterId == 0) {
				if (aIndex <= Edition->ListSize())
					Found = static_cast<const KaxChapterAtom *>((*Edition)[aIndex - 1]);
			} else {
				// recursively look for the specified chapter
				UINT Index0;
				for (Index0 = 0; Found != NULL && Index0 < Edition->ListSize(); Index0++) {
					Found = FindChapter(*static_cast<const KaxChapterAtom *>((*Edition)[Index0]), aParentChapterId);
				}
			}
		}
	}
	if (Found != NULL) {
		const KaxChapterUID * _UID = static_cast<const KaxChapterUID *>(Found->FindElt(KaxChapterUID::ClassInfos));
		if (_UID)
			return uint32(*_UID);
	}
	return CHAPTER_BAD_ID;
}

STDMETHODIMP_(UINT) MkxFilter::GetChapterCurrentId()
{
	NOTE("MkxFilter::GetChapterCurrentId");
	const KaxChapters * Chapters = m_pRunningThread->GetChapters();
	if (Chapters != NULL) {
		assert(0); // not supported for the moment
	}
	return CHAPTER_BAD_ID;
}

STDMETHODIMP_(BOOL) MkxFilter::GetChapterInfo(UINT aChapterID, ChapterElement* pStructureToFill)
{
	NOTE("MkxFilter::GetChapterInfo");
	const KaxChapters * Chapters = m_pRunningThread->GetChapters();
	const KaxChapterAtom *Found = NULL;
	if (Chapters != NULL) {
		// look for the specified chapter (0 is the 1st edition)
		const KaxEditionEntry * Edition = static_cast<const KaxEditionEntry *>((*Chapters)[0]);
		if (Edition != NULL) {
			// recursively look for the specified chapter
			UINT Index0;
			for (Index0 = 0; Found == NULL && Index0 < Edition->ListSize(); Index0++) {
				Found = FindChapter(*static_cast<const KaxChapterAtom *>((*Edition)[Index0]), aChapterID);
			}
		}
	}
	if (Found != NULL) {
		pStructureToFill->Size = sizeof(ChapterElement); // 23
		if (Found->FindElt(KaxChapterAtom::ClassInfos) != NULL)
			pStructureToFill->Type = SubChapter;
		else 
			pStructureToFill->Type = AtomicChapter;

		const KaxChapterUID * _UID = static_cast<const KaxChapterUID *>(Found->FindElt(KaxChapterUID::ClassInfos));
		if (_UID != NULL)
			pStructureToFill->ChapterId = uint32(*_UID);
		else
			pStructureToFill->ChapterId = CHAPTER_BAD_ID;

		const KaxChapterTimeStart * _Start = static_cast<const KaxChapterTimeStart *>(Found->FindElt(KaxChapterTimeStart::ClassInfos));
		if (_Start != NULL)
			pStructureToFill->rtStart = uint64(*_Start) / 100;
		else
			pStructureToFill->rtStart = -1;

		const KaxChapterTimeStart * _Stop = static_cast<const KaxChapterTimeStart *>(Found->FindElt(KaxChapterTimeStart::ClassInfos));
		if (_Stop != NULL)
			pStructureToFill->rtStop = uint64(*_Stop) / 100;
		else
			pStructureToFill->rtStop = -1;

		return TRUE;
	}
	return FALSE;
}

STDMETHODIMP_(BSTR) MkxFilter::GetChapterStringInfo(UINT aChapterID, const CHAR PreferredLanguage[3], const CHAR CountryCode[2])
{
	NOTE("MkxFilter::GetChapterStringInfo");
	const KaxChapters * Chapters = m_pRunningThread->GetChapters();
	const KaxChapterAtom *Found = NULL;
	if (Chapters != NULL) {
		// look for the specified chapter (0 is the 1st edition)
		const KaxEditionEntry * Edition = static_cast<const KaxEditionEntry *>((*Chapters)[0]);
		if (Edition != NULL) {
			// recursively look for the specified chapter
			UINT Index0;
			for (Index0 = 0; Found == NULL && Index0 < Edition->ListSize(); Index0++) {
				Found = FindChapter(*static_cast<const KaxChapterAtom *>((*Edition)[Index0]), aChapterID);
			}
		}
	}
	if (Found != NULL) {
		char Lang[4], Country[3];
		Lang[0] = PreferredLanguage[0];
		Lang[1] = PreferredLanguage[1];
		Lang[2] = PreferredLanguage[2];
		Lang[3] = '\0';
		Country[0] = CountryCode[0];
		Country[1] = CountryCode[1];
		Country[2] = '\0';
		UINT Index0;
		for (Index0 = 0; Index0<Found->ListSize(); Index0++) {
			if (EbmlId(*((*Found)[Index0])) == KaxChapterDisplay::ClassInfos.GlobalId) {
				const KaxChapterDisplay * _Strings = static_cast<const KaxChapterDisplay *>((*Found)[Index0]);
				UINT Index1;
				bool bLangIsFound = false;
				bool bCountryIsFound = false;
				for (Index1=0; Index1 < _Strings->ListSize(); Index1++) {
					if (EbmlId(*((*_Strings)[Index1])) == KaxChapterLanguage::ClassInfos.GlobalId) {
						const KaxChapterLanguage * _Lang = static_cast<const KaxChapterLanguage *>((*_Strings)[Index1]);
						if (_Lang && std::string(*_Lang) == Lang) {
							bLangIsFound = true;
						}
					} else if (EbmlId(*((*_Strings)[Index1])) == KaxChapterCountry::ClassInfos.GlobalId) {
						const KaxChapterCountry * _Country = static_cast<const KaxChapterCountry *>((*_Strings)[Index1]);
						if (_Country && std::string(*_Country) == Country) {
							bCountryIsFound = true;
						}
					}
				}
				if (bLangIsFound && bCountryIsFound) {
					const KaxChapterString * _Str = static_cast<const KaxChapterString *>(_Strings->FindElt(KaxChapterString::ClassInfos));
					if (_Str) {
						const UTFstring tmp = *_Str;
						return SysAllocString(const_cast<wchar_t *>(tmp.c_str()));
					}
				}
			}
		}
		// dirty, don't care about the country
		for (Index0 = 0; Index0<Found->ListSize(); Index0++) {
			if (EbmlId(*((*Found)[Index0])) == KaxChapterDisplay::ClassInfos.GlobalId) {
				const KaxChapterDisplay * _Strings = static_cast<const KaxChapterDisplay *>((*Found)[Index0]);
				UINT Index1;
				bool bLangIsFound = false;
				bool bCountryIsFound = false;
				for (Index1=0; Index1 < _Strings->ListSize(); Index1++) {
					if (EbmlId(*((*_Strings)[Index1])) == KaxChapterLanguage::ClassInfos.GlobalId) {
						const KaxChapterLanguage * _Lang = static_cast<const KaxChapterLanguage *>((*_Strings)[Index1]);
						if (_Lang && std::string(*_Lang) == Lang) {
							bLangIsFound = true;
						}
					}
				}
				if (bLangIsFound) {
					const KaxChapterString * _Str = static_cast<const KaxChapterString *>(_Strings->FindElt(KaxChapterString::ClassInfos));
					if (_Str) {
						const UTFstring tmp = *_Str;
						return SysAllocString(const_cast<wchar_t *>(tmp.c_str()));
					}
				}
			}
		}
		// dirty, just give the first one
		for (Index0 = 0; Index0<Found->ListSize(); Index0++) {
			if (EbmlId(*((*Found)[Index0])) == KaxChapterDisplay::ClassInfos.GlobalId) {
				const KaxChapterDisplay * _Strings = static_cast<const KaxChapterDisplay *>((*Found)[Index0]);
				const KaxChapterString * _Str = static_cast<const KaxChapterString *>(_Strings->FindElt(KaxChapterString::ClassInfos));
				if (_Str) {
					const UTFstring tmp = *_Str;
					return SysAllocString(const_cast<wchar_t *>(tmp.c_str()));
				}
			}
		}
	}
	return NULL;
}

// ============================================================================

