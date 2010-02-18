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
	\version \$Id: mkxds.h,v 1.4 2003/07/28 22:47:22 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Christophe Paris <toffparis @ users.sf.net>
*/

#ifndef MKXDS_FILTER_H
#define MKXDS_FILTER_H

#include "ebml/IOCallback.h"

using namespace LIBEBML_NAMESPACE;

// {34293064-02F2-41d5-9D75-CC5967ACA1AB}
DEFINE_GUID(CLSID_MKXDEMUX, 
0x34293064, 0x2f2, 0x41d5, 0x9d, 0x75, 0xcc, 0x59, 0x67, 0xac, 0xa1, 0xab);

#include "IChapterInfo.h"

// {1AC0BEBD-4D2B-45ad-BCEB-F2C41C5E3788}
//DEFINE_GUID(MEDIASUBTYPE_Matroska, 
//0x1ac0bebd, 0x4d2b, 0x45ad, 0xbc, 0xeb, 0xf2, 0xc4, 0x1c, 0x5e, 0x37, 0x88);

#ifdef __cplusplus
extern "C" {
#endif

// {36A2372A-8697-45ae-903C-6F1C8009F750}
DEFINE_GUID(IID_IMkxFilter, 
0x36a2372a, 0x8697, 0x45ae, 0x90, 0x3c, 0x6f, 0x1c, 0x80, 0x9, 0xf7, 0x50);

DECLARE_INTERFACE_(IMkxFilter, IUnknown)
{};

#ifdef __cplusplus
}
#endif

// forward declaration
class MatroskaReader;
class MkxReadThread;

class MkxFilter : public CSource
                , public IMediaSeeking
                , public IFileSourceFilter
                , public ISpecifyPropertyPages
				, public IMkxFilter
				, public IAMMediaContent
				, public IChapterInfo
{
public:

    // filter object registration information
    static const AMOVIESETUP_FILTER sudFilter;

	// called to create the COM filter oject
    static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
	
	DECLARE_IUNKNOWN;
		
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv)
    {	
		if (riid == IID_IFileSourceFilter) {
			return GetInterface((IFileSourceFilter *)this, ppv);
		} else if (riid == IID_IMediaSeeking) {
			return GetInterface((IMediaSeeking *)this, ppv);
		} else if (riid == IID_ISpecifyPropertyPages) {
			return GetInterface((ISpecifyPropertyPages *) this, ppv);
		} else if (riid == IID_IMkxFilter) {
			return GetInterface((IMkxFilter *) this, ppv);
		} else if (riid == IID_IAMMediaContent) {
			return GetInterface((IAMMediaContent *)this, ppv);			
		} else if (riid == IID_IChapterInfo) {
			return GetInterface((IChapterInfo *)this, ppv);			
		} else 	{
			return CSource::NonDelegatingQueryInterface(riid,ppv);
		}
    }	

	// --- IFileSourceFilter ---
	STDMETHODIMP Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt);
	STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *pmt);

    // --- IMediaSeeking ---	
    STDMETHODIMP IsFormatSupported(const GUID * pFormat);
    STDMETHODIMP QueryPreferredFormat(GUID *pFormat);
    STDMETHODIMP SetTimeFormat(const GUID * pFormat);
    STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);
    STDMETHODIMP GetTimeFormat(GUID *pFormat);
    STDMETHODIMP GetDuration(LONGLONG *pDuration);
    STDMETHODIMP GetStopPosition(LONGLONG *pStop);
    STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);
    STDMETHODIMP GetCapabilities( DWORD * pCapabilities );
    STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );
    STDMETHODIMP ConvertTimeFormat( LONGLONG * pTarget, const GUID * pTargetFormat,
		LONGLONG    Source, const GUID * pSourceFormat );	
    STDMETHODIMP SetPositions( LONGLONG * pCurrent,  DWORD CurrentFlags
			     , LONGLONG * pStop,  DWORD StopFlags );	
    STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );	
    STDMETHODIMP GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );
    STDMETHODIMP SetRate( double dRate);
    STDMETHODIMP GetRate( double * pdRate);
    STDMETHODIMP GetPreroll(LONGLONG *pPreroll);
	

    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME tStart);
	
    // ISpecifyPropertyPages method

    STDMETHODIMP GetPages(CAUUID *pPages);

    // --- IAMMediaContent methods ---	
    STDMETHODIMP GetTypeInfoCount(UINT*Count) { return S_OK; }
    STDMETHODIMP GetTypeInfo(UINT Index, LCID Language, ITypeInfo**TypeInfo) { return S_OK; }
    STDMETHODIMP GetIDsOfNames(const GUID&Interface, wchar_t**Names, UINT Count, LCID Language,DISPID*ID) { return S_OK; }
    STDMETHODIMP Invoke(DISPID ID,const GUID&Interface,LCID Language,WORD Flags,DISPPARAMS*Params,VARIANT*Result,EXCEPINFO*Exception,UINT*Error) { return S_OK; }
	STDMETHODIMP get_Title(BSTR FAR* pbstrTitle);
	STDMETHODIMP get_AuthorName(BSTR FAR* pbstrAuthorName) { return E_NOTIMPL; }
    STDMETHODIMP get_Rating(BSTR FAR* pbstrRating) { return E_NOTIMPL; }
    STDMETHODIMP get_Description(BSTR FAR* pbstrDescription) { return E_NOTIMPL; }
    STDMETHODIMP get_Copyright(BSTR FAR* pbstrCopyright) { return E_NOTIMPL; }
    STDMETHODIMP get_BaseURL(BSTR FAR* pbstrBaseURL) { return E_NOTIMPL; }
    STDMETHODIMP get_LogoURL(BSTR FAR* pbstrLogoURL) { return E_NOTIMPL; }
    STDMETHODIMP get_LogoIconURL(BSTR FAR* pbstrLogoURL) { return E_NOTIMPL; }
    STDMETHODIMP get_WatermarkURL(BSTR FAR* pbstrWatermarkURL) { return E_NOTIMPL; }
    STDMETHODIMP get_MoreInfoURL(BSTR FAR* pbstrMoreInfoURL) { return E_NOTIMPL; }
    STDMETHODIMP get_MoreInfoBannerImage(BSTR FAR* pbstrMoreInfoBannerImage) { return E_NOTIMPL; }
    STDMETHODIMP get_MoreInfoBannerURL(BSTR FAR* pbstrMoreInfoBannerURL) { return E_NOTIMPL; }
    STDMETHODIMP get_MoreInfoText(BSTR FAR* pbstrMoreInfoText) { return E_NOTIMPL; }
	
    // --- IChapterInfo methods ---	
	STDMETHODIMP_(UINT) GetChapterCount(UINT aChapterID);
	STDMETHODIMP_(UINT) GetChapterId(UINT aParentChapterId, UINT aIndex);
	STDMETHODIMP_(UINT) GetChapterCurrentId();
	STDMETHODIMP_(BOOL) GetChapterInfo(UINT aChapterID, ChapterElement* pStructureToFill);
	STDMETHODIMP_(BSTR) GetChapterStringInfo(UINT aChapterID, const CHAR PreferredLanguage[3], const CHAR CountryCode[2]);

protected:
	IOCallback * m_FileHandle;
	DWORD ThreadProc( );

private:

	// Constructor - just calls the base class constructor
    MkxFilter(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	virtual ~MkxFilter();

	HRESULT CreateOutputPin();

//	MatroskaReader *m_pMKReader;

	MkxReadThread *m_pRunningThread;

	// --- IFileSourceFilter ---
	LPWSTR m_pFileName;

	// --- IMediaSeeking ---
    // we call this to notify changes. Override to handle them
    HRESULT ChangeStart();
    HRESULT ChangeStop();
    HRESULT ChangeRate();
	
    CRefTime m_rtDuration;      // length of stream
    CRefTime m_rtStart;         // source will start here
    CRefTime m_rtStop;          // source will stop here
    double m_dRateSeeking;
	
    // seeking capabilities
    DWORD m_dwSeekingCaps;
	
    CCritSec m_SeekLock;	
};

#endif // MKXDS_FILTER_H
