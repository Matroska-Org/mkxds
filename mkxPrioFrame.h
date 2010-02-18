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
        \version \$Id: mkxPrioFrame.h,v 1.7 2003/08/01 21:15:47 robux4 Exp $
        \author Steve Lhomme     <robux4 @ users.sf.net>
*/

#ifndef MKXDS_MKX_PRIO_FRAME_H
#define MKXDS_MKX_PRIO_FRAME_H

#include "matroska/KaxBlock.h"
#include "matroska/KaxBlockData.h"

#include <map>
#include <deque>

using namespace LIBMATROSKA_NAMESPACE;

/*!
	\class MkxPrioBuffer
*/
class MkxPrioBuffer {
public:
	MkxPrioBuffer(KaxBlockGroup & aBlock)
		:Block(&aBlock)
		,Displayed(false)
		,CanDelete(true)
	{
		KaxReferencePriority & RefPrio = GetChild<KaxReferencePriority>(aBlock);
		Priority = uint8(RefPrio);
		Timecode = aBlock.GlobalTimecode();
	}

	MkxPrioBuffer & operator=(const MkxPrioBuffer & aBuffer)
	{
		Block     = aBuffer.Block;
		Priority  = aBuffer.Priority;
		Timecode  = aBuffer.Timecode;
		Displayed = aBuffer.Displayed;
		return *this;
	}

	KaxBlockGroup * Block;
	uint8           Priority; ///< used to speed access to this value
	uint64          Timecode; ///< used to speed access to this value
	bool            Displayed;
	bool            CanDelete;
};

class MkxPrioKey {
public:
	MkxPrioKey(MkxPrioBuffer & aBuf)
		:Elt(aBuf)
	{}

	bool operator<(const MkxPrioKey &aKey) const
	{
		return (Elt.Timecode < aKey.Elt.Timecode);
	}
	
	MkxPrioBuffer & Elt;
};

/*!
	\class MkxPrioCache
*/
class MkxPrioCache : public CCritSec {
public:
	MkxPrioCache(UINT MinCache, UINT aMaxWriteThreshold, UINT aMaxReadThreshold, 
	         UINT aMinWriteThreshold = 1);
	~MkxPrioCache();

	void PushBlock(KaxBlockGroup & aBlock);

	KaxBlockGroup * GetElementInTimeOrder();

	KaxBlockGroup * GetNextInTimeOrder(KaxBlockGroup & aBlock);

	void UsedBlock(KaxBlockGroup & aBlock);

	void StartFlush();
	void EndFlush();

	void Reset();

	BOOL IsEnding() const {return Ending;}
	BOOL IsFlushing() const {return Flushing;}

	/*!
		\brief Retrieve a Block from the queue
		\note Block until data is coming, unless it's being flushed or at the end of stream
		\note the reader becomes the "owner" of the Block (memory freeing)
		\return a Block or NULL if the queue is being flushed or ending
	*/
	KaxBlockGroup * GetFrontBlock();

	/*!
		\brief Put a Block in the queue
		\param bForce pushing an element will not block the caller in this case
		\note the queue becomes the "owner" of the Block (memory freeing)
	*/
//	void PushBlockBack(KaxBlockGroup * aBlock);

	/*!
		\brief trigger the Pause mode of the queue (reduced queue size)
	*/
	void SetPauseMode(bool bPauseMode);

	/*!
		\brief Signal that the queue will not have any further blocks
	*/
	void EndOfStream();

#ifdef _UNICODE
	std::wstring                        mName;
#else // _UNICODE
	std::string                         mName;
#endif // _UNICODE

	void SetMaxReadThreshold(UINT Threashold);
	void SetMaxWriteThreshold(UINT Threashold);
	void SetMinWriteThreshold(UINT Threashold);

	void DisableWriteBlock();
	void EnableWriteBlock();

protected:
	std::map<MkxPrioKey, MkxPrioBuffer*> DisplayOrderList;
	std::deque<MkxPrioBuffer*>           CodingOrderList;
	UINT                                 MinCache;
	UINT                                 MinWriteThreshold;
	UINT                                 MaxWriteThreshold;
	UINT                                 MaxReadThreshold;
	BOOL                                 Ending;
	BOOL                                 Flushing;
	CAMEvent                             mWriteUnBlock;
	BOOL                                 bWriteCanBlock;
	CAMEvent                             mReadUnBlock;

	/*!
		Erase blocks that are not needed anymore
		\warning use only when locked
	*/
	void StripList();
};

#endif // MKXDS_MKX_PRIO_FRAME_H

