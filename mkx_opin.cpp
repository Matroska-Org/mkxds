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
	\version \$Id: mkx_opin.cpp,v 1.12 2003/08/02 08:33:37 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Christophe Paris <toffparis @ users.sf.net>
*/


#include <streams.h>
#include <initguid.h> // for Vorbis UIDs
#include <dvdmedia.h> // for VIDEOINFOHEADER2

#include "mkx_opin.h"
#include "Subtitles.h"
#include "codecs.h"
#include "OggDS.h"
#include "CoreVorbisGUID.h"
#include "matroska/KaxBlockData.h"

void BuildAACDecoderSpecificData(const char* CodecID, int SampleRate,
								 int Channels, BYTE* CodeData);

// ***********************************************************
// 
//  The MkxOutPin methods
//

MkxOutPin::MkxOutPin(TCHAR *pObjectName, HRESULT *phr, CSource *pms, LPCWSTR pName, KaxTrackInfoStruct & TrackInfos)
:CSourceStream(pObjectName, phr, pms, pName)
 ,m_TrackInfos(TrackInfos)
 ,m_bSendHeader(false)
 ,mBlockQueue(
	60, // aMaxWriteThreshold
	4, // aMaxReadThreshold
	58, // aMinWriteThreshold
	2) // aMinReadThreshold
 ,mCachedBlockG(NULL)
 ,m_rtStart(0)
 ,m_bDiscontinuity(false)
 ,m_bIsProcessing(false) 
{
	unsigned int i;

#ifndef _UNICODE
	int cch = lstrlenW(pName) + 1;
	CHAR *lpszName = new char[cch * 2];
	WideCharToMultiByte(GetACP(), 0, pName, -1, lpszName, cch, NULL, NULL);
	mBlockQueue.mName = lpszName;
#else // _UNICODE
	mBlockQueue.mName = pName;
#endif // _UNICODE

	switch(TrackInfos.Type)
	{
	case track_video:
	{		
		m_mt.InitMediaType();
		m_mt.SetType(&MEDIATYPE_Video);
		m_mt.SetFormatType(&FORMAT_VideoInfo);

		REFERENCE_TIME AvgTimePerFrame = 0;
		if(TrackInfos.VideoInfo.FrameRate > 0)
			AvgTimePerFrame = (REFERENCE_TIME)(10000000i64 / TrackInfos.VideoInfo.FrameRate);
		else
			AvgTimePerFrame = (REFERENCE_TIME)TrackInfos.DefaultDuration / 100;
		
		if (AvgTimePerFrame == 0)
			AvgTimePerFrame = 400000; // 40ms in case we don't know
			
		if(TrackInfos.CodecID.substr(0, 8) == "V_MPEG4/")
		{
			VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)m_mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
			memset(pvih, 0, m_mt.FormatLength());
			pvih->bmiHeader.biSize = sizeof(pvih->bmiHeader);
			pvih->bmiHeader.biCompression = FOURCC_mp4v;
			pvih->bmiHeader.biWidth = TrackInfos.VideoInfo.PixelWidth;
			pvih->bmiHeader.biHeight = TrackInfos.VideoInfo.PixelHeight;
			pvih->AvgTimePerFrame = AvgTimePerFrame;
			
			m_mt.SetSubtype(&CLSID_mp4v);
			m_mt.SetSampleSize(1);
			m_mts.push_back(m_mt);

			if(TrackInfos.VideoInfo.DisplayWidth != 0 && TrackInfos.VideoInfo.DisplayHeight != 0)
			{
				BITMAPINFOHEADER tmp = pvih->bmiHeader;
				m_mt.formattype = FORMAT_VideoInfo2;
				VIDEOINFOHEADER2* pvih2 = (VIDEOINFOHEADER2*)m_mt.ReallocFormatBuffer(
					sizeof(VIDEOINFOHEADER2) + TrackInfos.CodecPrivateLen);
				memset(pvih2, 0, m_mt.FormatLength());
				pvih2->bmiHeader = tmp;
				pvih2->dwPictAspectRatioX = (DWORD)TrackInfos.VideoInfo.DisplayWidth;
				pvih2->dwPictAspectRatioY = (DWORD)TrackInfos.VideoInfo.DisplayHeight;
				m_mts.push_front(m_mt);
			}
		}
		else if(TrackInfos.CodecID == "V_MS/VFW/FOURCC")
		{
			VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)m_mt.AllocFormatBuffer(
				sizeof(VIDEOINFOHEADER) + TrackInfos.CodecPrivateLen - 
				sizeof(BITMAPINFOHEADER));
			memset(pvih, 0, m_mt.FormatLength());
			memcpy(&pvih->bmiHeader, TrackInfos.CodecPrivate, TrackInfos.CodecPrivateLen);
			pvih->AvgTimePerFrame = AvgTimePerFrame;
			
			FOURCCMap	fcc(pvih->bmiHeader.biCompression);
			m_mt.SetSubtype((GUID*)&fcc);
			m_mt.SetSampleSize(pvih->bmiHeader.biWidth * pvih->bmiHeader.biHeight * 4);
			m_mts.push_back(m_mt);

			if(TrackInfos.VideoInfo.DisplayWidth != 0 && TrackInfos.VideoInfo.DisplayHeight != 0)
			{
				BITMAPINFOHEADER tmp = pvih->bmiHeader;
				m_mt.formattype = FORMAT_VideoInfo2;
				VIDEOINFOHEADER2* pvih2 = (VIDEOINFOHEADER2*)m_mt.ReallocFormatBuffer(
					sizeof(VIDEOINFOHEADER2) + TrackInfos.CodecPrivateLen - 
					sizeof(BITMAPINFOHEADER));
				memset(pvih2, 0, m_mt.FormatLength());
				pvih2->bmiHeader = tmp;
				pvih2->dwPictAspectRatioX = (DWORD)TrackInfos.VideoInfo.DisplayWidth;
				pvih2->dwPictAspectRatioY = (DWORD)TrackInfos.VideoInfo.DisplayHeight;
				m_mts.push_front(m_mt);
			}
		}
		else if(TrackInfos.CodecID.substr(0, 9) == "V_REAL/RV")
		{
			m_mt.subtype = FOURCCMap('00VR' + ((TrackInfos.CodecID[9]-0x30)<<16));
			VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)m_mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER) + TrackInfos.CodecPrivateLen);
			memset(m_mt.Format(), 0, m_mt.FormatLength());
			memcpy(m_mt.Format() + sizeof(VIDEOINFOHEADER), TrackInfos.CodecPrivate, TrackInfos.CodecPrivateLen);
			pvih->bmiHeader.biSize = sizeof(pvih->bmiHeader);
			pvih->bmiHeader.biWidth = (LONG)TrackInfos.VideoInfo.PixelWidth;
			pvih->bmiHeader.biHeight = (LONG)TrackInfos.VideoInfo.PixelHeight;
			pvih->bmiHeader.biCompression = m_mt.subtype.Data1;
			pvih->AvgTimePerFrame = AvgTimePerFrame;
			m_mt.SetSampleSize(pvih->bmiHeader.biWidth*pvih->bmiHeader.biHeight*4);
			m_mts.push_front(m_mt);

			if(TrackInfos.VideoInfo.DisplayWidth != 0 && TrackInfos.VideoInfo.DisplayHeight != 0)
			{
				BITMAPINFOHEADER tmp = pvih->bmiHeader;
				m_mt.formattype = FORMAT_VideoInfo2;
				VIDEOINFOHEADER2* pvih2 = (VIDEOINFOHEADER2*)m_mt.ReallocFormatBuffer(
					sizeof(VIDEOINFOHEADER2) + TrackInfos.CodecPrivateLen);
				memset(m_mt.Format(), 0, m_mt.FormatLength());
				memcpy(m_mt.Format() + sizeof(VIDEOINFOHEADER2), TrackInfos.CodecPrivate, TrackInfos.CodecPrivateLen);
				pvih2->bmiHeader = tmp;
				pvih2->AvgTimePerFrame = AvgTimePerFrame;
				pvih2->dwPictAspectRatioX = (DWORD)TrackInfos.VideoInfo.DisplayWidth;
				pvih2->dwPictAspectRatioY = (DWORD)TrackInfos.VideoInfo.DisplayHeight;
				m_mts.push_front(m_mt);
			}
		}
 
		mBlockQueue.SetMaxWriteThreshold(60); //not too large to avoid reading too much data when seeking
		mBlockQueue.SetMinWriteThreshold(54);
	}
	break;

	case track_audio:
	{	
		m_mt.InitMediaType();
		m_mt.SetType(&MEDIATYPE_Audio);
		m_mt.SetFormatType(&FORMAT_WaveFormatEx);		
		WAVEFORMATEX* pwfe = (WAVEFORMATEX*)m_mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
		memset(pwfe, 0, sizeof(WAVEFORMATEX));
		pwfe->nChannels = (WORD)TrackInfos.AudioInfo.Channels;
		pwfe->nSamplesPerSec = (DWORD)TrackInfos.AudioInfo.SamplingFreq;
		pwfe->wBitsPerSample = (WORD)TrackInfos.AudioInfo.BitDepth;
		pwfe->nBlockAlign = (WORD)((pwfe->nChannels * pwfe->wBitsPerSample) / 8);
		pwfe->nAvgBytesPerSec = pwfe->nSamplesPerSec * pwfe->nBlockAlign;
		m_mt.SetSampleSize(pwfe->nChannels * pwfe->nSamplesPerSec * 32 >> 3);
 
		if(TrackInfos.CodecID == "A_VORBIS")
		{
			// ----- OggDS -----			
			m_mt.SetType(&MEDIATYPE_Audio);
			m_mt.SetSubtype(&MEDIASUBTYPE_Vorbis);
			m_mt.SetFormatType(&FORMAT_VorbisFormat);
			VORBISFORMAT* pvf = (VORBISFORMAT*)m_mt.AllocFormatBuffer(sizeof(VORBISFORMAT));
			pvf->nChannels = TrackInfos.AudioInfo.Channels;
			pvf->nSamplesPerSec = TrackInfos.AudioInfo.SamplingFreq;
			pvf->nMaxBitsPerSec = pvf->nMinBitsPerSec = pvf->nAvgBitsPerSec = -1;
			pvf->fQuality = 0;
			m_mt.SetSampleSize(1);
			m_mts.push_back(m_mt);

			// ----- CoreVorbis -----			
			m_mt.SetType(&MEDIATYPE_Audio);
			m_mt.SetSubtype(&MEDIASUBTYPE_Vorbis2);
			m_mt.SetFormatType(&FORMAT_VorbisFormat2);
			VORBISFORMAT2* pvf2 = (VORBISFORMAT2*)m_mt.AllocFormatBuffer(sizeof(VORBISFORMAT2) + TrackInfos.CodecPrivateLen - 3);
			memcpy((BYTE*)pvf2+sizeof(VORBISFORMAT2), TrackInfos.CodecPrivate+3, TrackInfos.CodecPrivateLen - 3);
			
			binary *codecPrivate = TrackInfos.CodecPrivate;
			uint32 lastHeaderSize = TrackInfos.CodecPrivateLen - 1;
			uint8 nbHeaders = *((uint8 *)codecPrivate);
			codecPrivate++;			
			// 3 headers for vorbis
			while(nbHeaders)
			{
				uint32 currentHeaderSize = 0;
				do{
					currentHeaderSize += *(uint8 *)codecPrivate;
					lastHeaderSize--;
				} while((*codecPrivate++) == 0xFF);
				lastHeaderSize -= currentHeaderSize;
				pvf2->HeaderSize[2-nbHeaders] = currentHeaderSize;
				nbHeaders--;
			}			
			pvf2->HeaderSize[2-nbHeaders] = lastHeaderSize;
			pvf2->Channels = TrackInfos.AudioInfo.Channels;
			pvf2->SamplesPerSec = TrackInfos.AudioInfo.SamplingFreq;
			pvf2->BitsPerSample = TrackInfos.AudioInfo.BitDepth;			
			m_mt.SetSampleSize(1);
			m_mts.push_back(m_mt);	
			
			m_bSendHeader = true;
		}
		else if(TrackInfos.CodecID == "A_MS/ACM")
		{
			WAVEFORMATEX* pwfx = (WAVEFORMATEX*)TrackInfos.CodecPrivate;
			CreateAudioMediaType(pwfx, &m_mt, TRUE);
			m_mts.push_front(m_mt);
		}
		else if(TrackInfos.CodecID.substr(0, 6) == "A_AAC/")
		{
			m_mt.subtype = FOURCCMap(pwfe->wFormatTag = WAVE_FORMAT_AAC);
			BYTE* pExtra = m_mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX)+2) + sizeof(WAVEFORMATEX);
			((WAVEFORMATEX*)m_mt.pbFormat)->cbSize = 2;
			
			BuildAACDecoderSpecificData(TrackInfos.CodecID.c_str(),
				TrackInfos.AudioInfo.SamplingFreq, TrackInfos.AudioInfo.Channels, pExtra);

			m_mts.push_front(m_mt);
		}
		else if(TrackInfos.CodecID.substr(0,7) == "A_REAL/")
		{
			m_mt.subtype = FOURCCMap( (DWORD)TrackInfos.CodecID[7] |
				((DWORD)TrackInfos.CodecID[8] << 8) |
				((DWORD)TrackInfos.CodecID[9] << 16)|
				((DWORD)TrackInfos.CodecID[10] << 24));
			
			BYTE* pExtra = m_mt.ReallocFormatBuffer(sizeof(WAVEFORMATEX) + TrackInfos.CodecPrivateLen);
			memcpy(pExtra + sizeof(WAVEFORMATEX), TrackInfos.CodecPrivate, TrackInfos.CodecPrivateLen);
			
			m_mts.push_front(m_mt);
		}
		else
		{
			GUID *pGuid = NULL;
			for (i=0; i<ACM_CODEC_LIST_LEN; i++)
			{
				if (TrackInfos.CodecID == ACM_CODEC_LIST[i].name)
				{
					pwfe->wFormatTag = ACM_CODEC_LIST[i].id;
					if (ACM_CODEC_LIST[i].id == 0)
					{
						pGuid = (GUID*)&ACM_CODEC_LIST[i].guid;
					}
					break;
				}
			}

			if (pGuid != NULL)
			{
				m_mt.SetSubtype(pGuid);					
			}
			m_mts.push_front(m_mt);
		}
	}
		break;

	case track_subtitle:

		m_mt.SetType(&MEDIATYPE_Subtitle);
		m_mt.SetFormatType(&FORMAT_SubtitleInfo);		
		SUBTITLEINFO* pfSubInfo = (SUBTITLEINFO*)m_mt.AllocFormatBuffer(sizeof(SUBTITLEINFO) + TrackInfos.CodecPrivateLen);
		memset(pfSubInfo, 0, m_mt.FormatLength());
		pfSubInfo->dwOffset = sizeof(SUBTITLEINFO);
		strncpy(pfSubInfo->IsoLang, TrackInfos.Language.empty() ?
			"eng" : TrackInfos.Language.c_str(), 3);
		if (TrackInfos.CodecPrivateLen)
		{
			memcpy(pfSubInfo + pfSubInfo->dwOffset, TrackInfos.CodecPrivate, TrackInfos.CodecPrivateLen);
		}		

		if(TrackInfos.CodecID == "S_TEXT/UTF8" ||
			TrackInfos.CodecID == "S_TEXT/ASCII")
		{
			m_mt.SetSubtype(&MEDIASUBTYPE_UTF8);
		}
		else if(TrackInfos.CodecID == "S_SSA")
		{
			m_mt.SetSubtype(&MEDIASUBTYPE_SSA);
		}
		else if(TrackInfos.CodecID == "S_ASS")
		{
			m_mt.SetSubtype(&MEDIASUBTYPE_ASS);
		}
		else if(TrackInfos.CodecID == "S_USF")
		{
			m_mt.SetSubtype(&MEDIASUBTYPE_USF);
		}		

		m_mts.push_front(m_mt);

//		mBlockQueue.SetMinReadThreshold(1);
		mBlockQueue.SetMaxReadThreshold(1);
		break;
	}
}

MkxOutPin::~MkxOutPin()
{	
	NOTE("MkxOutPin::~MkxOutPin");
	m_mts.clear();
/* Let the mBlockQueue handle that		delete mCachedBlockG; */
}

HRESULT MkxOutPin::GetMediaType(int iPosition, CMediaType* pMediaType)
{
	if(iPosition < 0)
		return E_INVALIDARG;
	
	if(iPosition >= m_mts.size())
		return VFW_S_NO_MORE_ITEMS;
	
	*pMediaType = m_mts[iPosition];
	
	return S_OK;
}

HRESULT MkxOutPin::CheckMediaType(const CMediaType* pMediaType)
{
	for(UINT i = 0; i < m_mts.size(); i++)
	{
		if (pMediaType->majortype == m_mts[i].majortype &&
			pMediaType->subtype == m_mts[i].subtype)
		{
			return S_OK;
		}
	}
	
	return E_INVALIDARG;
}

HRESULT MkxOutPin::DeliverBeginFlush()
{
//	mCachedBlockG = NULL;
NOTE("MkxOutPin::DeliverBeginFlush");
	m_bIsProcessing = false;
	mBlockQueue.StartFlush();
NOTE("MkxOutPin::DeliverBeginFlush A");
	HRESULT result = CBaseOutputPin::DeliverBeginFlush();
//	Stop(); // make sure the FillBuffer has ended
NOTE("MkxOutPin::DeliverBeginFlush B");
	return result;
}

HRESULT MkxOutPin::DeliverEndFlush()
{
	NOTE("MkxOutPin::DeliverEndFlush");
	mBlockQueue.EndFlush();
	mBlockQueue.Reset();
NOTE("MkxOutPin::DeliverEndFlush A");
	HRESULT result = CBaseOutputPin::DeliverEndFlush();
//	Pause();
NOTE("MkxOutPin::DeliverEndFlush C");
	return result;
}

void MkxOutPin::EndStream()
{
	NOTE("MkxOutPin::EndStream");
	mBlockQueue.EndOfStream();
}

void MkxOutPin::Reset()
{
	NOTE("MkxOutPin::Reset");
	mBlockQueue.Reset();
}
void MkxOutPin::DisableWriteBlock()
{
	NOTE("MkxOutPin::DisableWriteBlock");
	mBlockQueue.DisableWriteBlock();
}
void MkxOutPin::EnableWriteBlock()
{
	NOTE("MkxOutPin::EnableWriteBlock");
	mBlockQueue.EnableWriteBlock();
}
void MkxOutPin::SetPauseMode(bool bPauseMode)
{
	NOTE1("MkxOutPin::SetPauseMode %d", bPauseMode);
	mBlockQueue.SetPauseMode(bPauseMode);
}

void MkxOutPin::PushBlock(KaxBlockGroup & aBlock)
{
	mBlockQueue.PushBlock(aBlock);
//	mBlockQueue.PushBlockBack(&aBlock);
}

// called from CBaseOutputPin during connection to ask for
// the count and size of buffers we need.
HRESULT MkxOutPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pAllocProps)
{
	ALLOCATOR_PROPERTIES	Actual;
	HRESULT	hr;
	
	if (pAlloc == NULL) return E_POINTER;
	if (pAllocProps == NULL) return E_POINTER;
			
	///< \todo check buffer number
	pAllocProps->cBuffers = 8;
	
	// compute buffer size
	switch(m_TrackInfos.Type)
	{
	case track_video:
		if (pAllocProps->cbBuffer < m_TrackInfos.VideoInfo.PixelHeight *
			m_TrackInfos.VideoInfo.PixelWidth * 4)
		{
			pAllocProps->cbBuffer = m_TrackInfos.VideoInfo.PixelHeight *
				m_TrackInfos.VideoInfo.PixelWidth * 4;
		}
		break;
	case track_audio:
	case track_subtitle:
		///< \todo check buffer size
		pAllocProps->cbBuffer = 0xFFFF;
		break;
	}
	
	pAllocProps->cbAlign = 4;
	
	// set properties
	hr = pAlloc->SetProperties(pAllocProps, &Actual);
	if (FAILED(hr))
		return hr;
	
	return NO_ERROR;
}

/*!
	\todo ensure the whole Block is deleted when not used anymore
*/
HRESULT MkxOutPin::FillBuffer(IMediaSample *pSamp)
{
	CheckPointer(pSamp,E_POINTER);
	BYTE *pData;
	long cbData;
	REFERENCE_TIME	tBlockDuration, tSmpStart, tSmpEnd;	
	uint64 aDuration;
bool TTT = (m_TrackInfos.Type == track_video);

    // Access the sample's data buffer
    pSamp->GetPointer(&pData);
    cbData = pSamp->GetSize();
	
	// Get a block for this track
	if (mCachedBlockG == NULL) 
	{
		while (1) {
NOTE("MkxOutPin::FillBuffer");
			///> \todo GetFrontBlock may remove the block from the queue ?
			mCachedBlockG = mBlockQueue.GetFrontBlock();
			if(mCachedBlockG == NULL)
			{
				if (!mBlockQueue.IsEnding() && !mBlockQueue.IsFlushing())
					continue;
				else {
NOTE1("MkxOutPin::FillBuffer %c ending/flushing !", (m_TrackInfos.Type == track_audio)?'A':'V');
					return S_FALSE; // ONLY when this is the end of this stream !!!
				}
			}
			mCachedBlock = static_cast<KaxBlock *>(mCachedBlockG->FindFirstElt(KaxBlock::ClassInfos, false));
			if (mCachedBlock == NULL) {
				mBlockQueue.UsedBlock(*mCachedBlockG);
// DONE in MkxPrioCache::StripList	delete mCachedBlockG; // continue the search
				mCachedBlockG = NULL;
			} else {
				mPrevStartTime = mCachedBlock->GlobalTimecode() / 100;
				mBlockIndex = 0;
				break;
			}
		}
	}

	// Set sample timestamps
	if (mCachedBlockG->GetBlockDuration(aDuration)) {
		tBlockDuration = aDuration / 100;
	} else { 
		// get the next block in display order
		KaxBlockGroup * aNext = mBlockQueue.GetNextInTimeOrder(*mCachedBlockG);
		if (aNext != NULL) {
			tBlockDuration = (aNext->GlobalTimecode() / 100) - mPrevStartTime;
		} else {
			///< \todo default duration of the track if it exists
			tBlockDuration = 400000 * mCachedBlock->NumberFrames(); // + 40ms
		}
	}

	if (mBlockIndex + 1 == mCachedBlock->NumberFrames()) {
		tSmpEnd = mCachedBlock->GlobalTimecode() / 100 + tBlockDuration;
		tBlockDuration /= mCachedBlock->NumberFrames();
		tSmpStart = mPrevStartTime + mBlockIndex * tBlockDuration;
	} else {
		tBlockDuration /= mCachedBlock->NumberFrames();
		tSmpStart = mPrevStartTime + mBlockIndex * tBlockDuration;
		tSmpEnd = tSmpStart + tBlockDuration;
	}

	tSmpStart -= m_rtStart;
	tSmpEnd -= m_rtStart;
	pSamp->SetTime(&tSmpStart, &tSmpEnd);
	pSamp->SetMediaTime(NULL,NULL);

	// Fill the pData buffer with the sample data
	DataBuffer & aBuf = mCachedBlock->GetBuffer(mBlockIndex);
	CopyMemory(pData, aBuf.Buffer(), aBuf.Size());
	pSamp->SetActualDataLength(aBuf.Size());	
	UINT BufSize = aBuf.Size();

	/**
	if(m_TrackInfos.Type == track_subtitle)
	{
		NOTE4("Sub start:%8I64d, end:%I864d dur:%I864d, %s", tSmpStart, tSmpEnd, tBlockDuration, pData);
	}
	/**/

//	NOTE5("LOGGING %s start:%8I64d, end:%8I64d dur:%8I64d size:%d", m_TrackInfos.Type == track_video ? L"VIDEO" : L"AUDIO",
//		tSmpStart, tSmpEnd, tBlockDuration, BufSize);
	
	mBlockIndex++;
assert(mCachedBlockG != NULL);
	if (mBlockIndex != 1 || (NULL == static_cast<KaxReferenceBlock *>(mCachedBlockG->FindFirstElt(KaxReferenceBlock::ClassInfos, false))))
	{ 
		// Set this to true for key frame
		pSamp->SetSyncPoint(TRUE);
	}

	if (mBlockIndex == mCachedBlock->NumberFrames()) {
		mBlockQueue.UsedBlock(*mCachedBlockG);
NOTE1("MkxOutPin::FillBuffer done with 0x%08x", mCachedBlockG);
/* Let the mBlockQueue handle that		delete mCachedBlockG; */
		mCachedBlockG = NULL;
		mPrevStartTime = tSmpEnd;
	}

	if(m_bDiscontinuity)
	{
		/**
		NOTE3("SetDiscontinuity : %s, KF: %s (%s)", m_bDiscontinuity ? L"true" : L"false",
			toto ? L"true" : L"false", m_TrackInfos.Type == track_video ? L"video" : L"audio");
		/**/
		pSamp->SetDiscontinuity(m_bDiscontinuity);
		m_bDiscontinuity = false;
	}

	pSamp->SetPreroll(tSmpStart < 0);	

	NOTE5("MkxOutPin::FillBuffer out %c 0x%08x %I64d/%I64d buffer size %d", (m_TrackInfos.Type == track_audio)?'A':'V', this, tSmpStart, tSmpEnd, BufSize);
    return NOERROR;
}

HRESULT MkxOutPin::OnThreadStartPlay(void)
{
	NOTE("MkxOutPin::OnThreadStartPlay");
	SendOneHeaderPerSample(m_TrackInfos.CodecPrivate, m_TrackInfos.CodecPrivateLen);
	// DeliverNewSegment( m_rtStart, m_rtStop, 1.0 );
/* Let the mBlockQueue handle that		delete mCachedBlockG; */
	mCachedBlockG = NULL;
	m_bDiscontinuity = true;	
	return NOERROR;
}

HRESULT MkxOutPin::OnThreadDestroy(void)
{
	NOTE("MkxOutPin::OnThreadDestroy");
/* Let the mBlockQueue handle that		delete mCachedBlockG; */
	mCachedBlockG = NULL;
	m_bIsProcessing = false;
	return NOERROR;
}

void MkxOutPin::SendOneHeaderPerSample(binary* CodecPrivateData, int DataLen)
{
	if (!m_bSendHeader ||
		!CBasePin::IsConnected() ||
		m_mt.subtype != MEDIASUBTYPE_Vorbis)
	{
		return;
	}

	IMediaSample	*pSample;
	BYTE			*pData;
	REFERENCE_TIME	tTime = 0;
	binary *codecPrivate = CodecPrivateData;
	uint32 lastHeaderSize = DataLen - 1;
	uint16 nbHeaders = (*(uint8 *)codecPrivate);
	CGenericList<uint32> headerSizeList(_T("Headers"));
	codecPrivate++;

	while(nbHeaders--)
	{
		uint32 currentHeaderSize = 0;
		do{
			currentHeaderSize += *(uint8 *)codecPrivate;
			lastHeaderSize--;
		} while((*codecPrivate++) == 0xFF);
		lastHeaderSize -= currentHeaderSize;
		headerSizeList.AddTail((uint32*)currentHeaderSize);
	}
	headerSizeList.AddTail((uint32*)lastHeaderSize);
	
	while(headerSizeList.GetCount()) 
	{
		uint32 currentHeaderSize = (uint32)headerSizeList.GetHead();
		headerSizeList.RemoveHead();
		
		if (FAILED(GetDeliveryBuffer(&pSample, NULL, NULL, 0)))
			return;
		
		pSample->GetPointer(&pData);
		memcpy(pData, codecPrivate, currentHeaderSize);
		pSample->SetActualDataLength(currentHeaderSize);
		pSample->SetTime(&tTime,&tTime);
		pSample->SetMediaTime(&tTime,&tTime);
		Deliver(pSample);
		pSample->Release();
		
		codecPrivate += currentHeaderSize;
	}
}

void MkxOutPin::SetNewStartTime(REFERENCE_TIME aTime)
{
//	CAutoLock(&);
	m_rtStart = aTime;
}

HRESULT MkxOutPin::DoBufferProcessingLoop(void)
{
	HRESULT hr = CSourceStream::DoBufferProcessingLoop();
	// We will get here earlier when a stream switcher block the stream
	
	// We must stop to fill the queue
	// the pin thread is still active, it's just waiting for a new command

	m_bIsProcessing = false;
NOTE("MkxOutPin::DoBufferProcessingLoop done");
	if(hr == S_OK)
	{
		mBlockQueue.StartFlush();
		mBlockQueue.EndFlush();
		mBlockQueue.Reset();
	}

	NOTE2("MkxOutPin::DoBufferProcessingLoop %s %0x08X",m_bIsProcessing ? L"true" : L"false", &mBlockQueue);

	return hr;
}

// ----------------------------------------------------------------------------
// Some helper functions
// ----------------------------------------------------------------------------

void BuildAACDecoderSpecificData(const char* CodecID, int SampleRate,
								 int Channels, BYTE* CodeData)
{
	const char* CodecIDProfileString = (const char*)&CodecID[12];
	char profile, srate_idx;
	
	// Recreate the 'private data' which faad2 uses in its initialization.
	// A_AAC/MPEG2/MAIN
	// 0123456789012345
	if (!strcmp(CodecIDProfileString, "MAIN"))
		profile = 0;
	else if (!strcmp(CodecIDProfileString, "LC"))
		profile = 1;
	else if (!strcmp(CodecIDProfileString, "SSR"))
		profile = 2;
	else
		profile = 3;
	
	if (92017 <= SampleRate)
		srate_idx = 0;
	else if (75132 <= SampleRate)
		srate_idx = 1;
	else if (55426 <= SampleRate)
		srate_idx = 2;
	else if (46009 <= SampleRate)
		srate_idx = 3;
	else if (37566 <= SampleRate)
		srate_idx = 4;
	else if (27713 <= SampleRate)
		srate_idx = 5;
	else if (23004 <= SampleRate)
		srate_idx = 6;
	else if (18783 <= SampleRate)
		srate_idx = 7;
	else if (13856 <= SampleRate)
		srate_idx = 8;
	else if (11502 <= SampleRate)
		srate_idx = 9;
	else if (9391 <= SampleRate)
		srate_idx = 10;
	else
		srate_idx = 11;
				
	CodeData[0] = ((profile + 1) << 3) | ((srate_idx & 0xe) >> 1);
	CodeData[1] = ((srate_idx & 0x1) << 7) | (Channels << 3);
}
