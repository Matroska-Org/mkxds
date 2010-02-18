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
	\version \$Id: mkx_opin.h,v 1.3 2003/07/20 18:22:13 jcsston Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Christophe Paris     <toffparis @ users.sf.net>
*/

#ifndef MKXDS_OUTPUTPIN_H
#define MKXDS_OUTPUTPIN_H

#include <tchar.h>

#include "global.h"
#include "mkxPrioFrame.h"
#include "matroska/KaxBlock.h"
using namespace LIBMATROSKA_NAMESPACE;


class MkxOutPin : public CSourceStream {
public:
	MkxOutPin(TCHAR *pObjectName, HRESULT *phr, CSource *pms, LPCWSTR pName, KaxTrackInfoStruct & TrackInfos);
	virtual ~MkxOutPin();
//	MkxQueue<KaxBlockGroup> & BlockQueue() {return mBlockQueue;}
//	const MkxQueue<KaxBlockGroup> & BlockQueue() const {return mBlockQueue;}

	void SetNewStartTime(REFERENCE_TIME aTime);
	bool IsProcessing(void) { return m_bIsProcessing; }
	void SetProcessingFlag()  { m_bIsProcessing = true; }

	STDMETHODIMP NonDelegatingQueryInterface( REFIID riid, void ** ppv )
	{
		if(riid == IID_IMediaSeeking)
		{			
			return m_pFilter->NonDelegatingQueryInterface(riid,ppv);
		}
		return CSourceStream::NonDelegatingQueryInterface(riid,ppv);
	}

	void PushBlock(KaxBlockGroup & aBlock);

	HRESULT DeliverBeginFlush();
	HRESULT DeliverEndFlush();

	void EndStream();
	void Reset();
	void DisableWriteBlock();
	void EnableWriteBlock();
	void SetPauseMode(bool bPauseMode);

protected:
	virtual HRESULT FillBuffer(IMediaSample *pSamp);
	virtual HRESULT GetMediaType(int iPosition, CMediaType* pMediaType);
	virtual HRESULT CheckMediaType(const CMediaType* pMediaType);

    // called from CBaseOutputPin during connection to ask for
    // the count and size of buffers we need.
	virtual HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);

	virtual HRESULT OnThreadStartPlay(void);
	virtual HRESULT OnThreadDestroy(void);

	virtual HRESULT DoBufferProcessingLoop(void);

	void SendOneHeaderPerSample(binary* CodecPrivateData, int DataLen);
	
	void UpdateFromSeek();

private:
	KaxTrackInfoStruct    & m_TrackInfos;
	MkxPrioCache            mBlockQueue;
	KaxBlockGroup *         mCachedBlockG;
	KaxBlock *              mCachedBlock;
	UINT                    mBlockIndex;
	REFERENCE_TIME          mPrevStartTime;
	REFERENCE_TIME          m_rtStart;
	bool                    m_bSendHeader;
	bool					m_bDiscontinuity;

	std::deque<CMediaType>	m_mts;
	bool					m_bIsProcessing;
};

#endif // MKXDS_OUTPUTPIN_H
