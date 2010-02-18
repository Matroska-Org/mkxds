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
	\version \$Id: mkxPrioFrame.cpp,v 1.8 2003/08/01 21:15:47 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/

/*=====================================================================

  If we have in input the data in coding order from matroska, we need
  to be able to tell which frame is next in the stream.

  In input we may have IBP frames in that order :
  I1 P2 P5 B3 B4 P6 I7

  In output we need to be able to give the frames in this order :
  I1 P2 B3 B4 P5 P6 I7

======================================================================*/

#include <streams.h>

#include "mkxPrioFrame.h"

MkxPrioCache::MkxPrioCache(UINT aMinCache, UINT aMaxWriteThreshold,
						   UINT aMaxReadThreshold, UINT aMinWriteThreshold)
 :MinCache(aMinCache)
 ,MinWriteThreshold(aMaxWriteThreshold)
 ,MaxWriteThreshold(aMaxReadThreshold)
 ,MaxReadThreshold(aMinWriteThreshold)
 ,Ending(FALSE)
 ,Flushing(FALSE)
 ,mWriteUnBlock(TRUE)
 ,bWriteCanBlock(TRUE)
 ,mReadUnBlock(TRUE)
{
}

MkxPrioCache::~MkxPrioCache()
{
}

void MkxPrioCache::SetMaxReadThreshold(UINT Threshold)
{
	Lock();
	MaxReadThreshold = Threshold;
	Unlock();
}
void MkxPrioCache::SetMaxWriteThreshold(UINT Threshold)
{
	Lock();
	MaxWriteThreshold = Threshold;
	Unlock();
}
void MkxPrioCache::SetMinWriteThreshold(UINT Threshold)
{
	Lock();
	MinWriteThreshold = Threshold;
	Unlock();
}

void MkxPrioCache::DisableWriteBlock()
{
	NOTE1("MkxPrioCache::WriteBlock Disable (0x%08x)", this);
	bWriteCanBlock = FALSE;
	mWriteUnBlock.Set();
}
void MkxPrioCache::EnableWriteBlock()
{
	NOTE1("MkxPrioCache::WriteBlock Enable (0x%08x)", this);
	bWriteCanBlock = TRUE;
}

void MkxPrioCache::Reset()
{
	NOTE("MkxPrioCache::Reset");
	mWriteUnBlock.Reset();
	mReadUnBlock.Reset();
	Ending = FALSE;
	Flushing = FALSE;
NOTE1("MkxPrioCache::WriteBlock Reset (0x%08x)", this);
	bWriteCanBlock = TRUE;
}

void MkxPrioCache::EndOfStream()
{
	NOTE("MkxPrioCache::EndOfStream");
	Ending = TRUE;
	mReadUnBlock.Set();
}

void MkxPrioCache::SetPauseMode(bool bPauseMode)
{
	NOTE1("MkxPrioCache::SetPauseMode %d", bPauseMode);
}

void MkxPrioCache::PushBlock(KaxBlockGroup & aBlock)
{
	///< \todo delete an element that has been "displayed"
	MkxPrioBuffer & EltToAdd = *(new MkxPrioBuffer(aBlock));
	MkxPrioKey    & EltKey   = *(new MkxPrioKey(EltToAdd));
	Lock();
	while (MaxWriteThreshold <= DisplayOrderList.size() && bWriteCanBlock) {
		mWriteUnBlock.Reset();
NOTE1("MkxPrioCache::PushBlock blocking (0x%08x)", this);
		Unlock();
		mWriteUnBlock.Wait();
		Lock();
NOTE1("MkxPrioCache::PushBlock UNblocking WRITE (0x%08x)", this);
	}
	DisplayOrderList.insert( std::make_pair(EltKey, &EltToAdd) );
	CodingOrderList.push_back( &EltToAdd );
NOTE4("MkxPrioCache::PushBlock (0x%08x) 0x%08x (%d/%d)", this, &aBlock, DisplayOrderList.size(), MaxWriteThreshold);
	if (DisplayOrderList.size() >= MaxReadThreshold && !mReadUnBlock.Check()) {
NOTE1("MkxPrioCache::PushBlock UNblocking READ (0x%08x)", this);
		mReadUnBlock.Set(); // unblock the reader
	}
	Unlock();
}

KaxBlockGroup * MkxPrioCache::GetFrontBlock()
{
	KaxBlockGroup * result = NULL;
	Lock();
NOTE1("MkxPrioCache::GetFrontBlock (0x%08x)", this);
	while (result == NULL) {
		for (size_t i=0;i<CodingOrderList.size(); i++)
		{
			MkxPrioBuffer *tst = CodingOrderList[i];
			// find the first frame not displayed in coding order
			if (!CodingOrderList[i]->Displayed) {
	//			CodingOrderList[i].Displayed = true;
				result = CodingOrderList[i]->Block;
				CodingOrderList[i]->CanDelete = false;
NOTE1("MkxPrioCache::GetFrontBlock CanDelete NOT 0x%08x", result);
				break;
			}
		}
		if (Ending || Flushing)
			break; // leave the while
		if (result == NULL) {
			// block reading until something occurs
			mReadUnBlock.Reset();
NOTE1("MkxPrioCache::GetFrontBlock blocked (0x%08x)", this);
			Unlock();
			mReadUnBlock.Wait();
NOTE1("MkxPrioCache::GetFrontBlock UNblocked (0x%08x) AAA", this);
			Lock();
NOTE1("MkxPrioCache::GetFrontBlock UNblocked (0x%08x)", this);
		}
		if (Ending || Flushing)
			break; // leave the while
	}
NOTE1("MkxPrioCache::GetFrontBlock 0x%08x", result);
	Unlock();
	return result;
}

KaxBlockGroup * MkxPrioCache::GetNextInTimeOrder(KaxBlockGroup & aBlock)
{
	KaxBlockGroup * result = NULL;
	// find the right buffer to output
	MkxPrioBuffer TmpBuf(aBlock);
	MkxPrioKey    TmpKey(TmpBuf);
	Lock();
	std::map<MkxPrioKey, MkxPrioBuffer*>::iterator & TheElt = DisplayOrderList.find( TmpKey );
	if (!Flushing && !Ending && TheElt != DisplayOrderList.end()) {
		TheElt++;
		if (TheElt != DisplayOrderList.end()) {
			result = (*TheElt).second->Block;
NOTE1("MkxPrioCache::GetNextInTimeOrder found 0x%08x", result);
		}
	}
	Unlock();
	return result;
}

void MkxPrioCache::UsedBlock(KaxBlockGroup & aBlock)
{
	Lock();
	MkxPrioBuffer TmpBuf(aBlock);
	MkxPrioKey    TmpKey(TmpBuf);
	std::map<MkxPrioKey, MkxPrioBuffer*>::iterator & TheElt = DisplayOrderList.find( TmpKey );
	if (TheElt != DisplayOrderList.end()) {
		MkxPrioBuffer *tst = (*TheElt).second;
		tst->CanDelete = true;
NOTE1("MkxPrioCache::PushBlock CanDelete (0x%08x)", (*TheElt).second->Block);
		(*TheElt).second->Displayed = true;
		KaxBlock & tmpBlk = GetChild<KaxBlock>(*(*TheElt).second->Block);
		tmpBlk.ReleaseFrames();
		StripList();
		// Unblock writing if needed+possible
		if (MinWriteThreshold >= DisplayOrderList.size() && !mWriteUnBlock.Check()) {
NOTE1("MkxPrioCache::PushBlock Unblocking (0x%08x)", this);
			mWriteUnBlock.Set();	
		}
	}
	Unlock();
}

void MkxPrioCache::StartFlush()
{
	Lock();
NOTE2("MkxPrioCache::StartFlush (0x%08x) start %d", this, DisplayOrderList.size());
	Flushing = TRUE;
	mReadUnBlock.Set(); // in case it's blocked, deblock it
	Unlock();
}

void MkxPrioCache::EndFlush()
{
	Lock();
NOTE2("MkxPrioCache::EndFlush (0x%08x) start %d", this, DisplayOrderList.size());
	std::map<MkxPrioKey, MkxPrioBuffer*>::iterator & TheElt = DisplayOrderList.begin();
	for (; TheElt != DisplayOrderList.end(); TheElt++) {
		if ((*TheElt).second->CanDelete) {
			KaxBlockGroup *tmp = (*TheElt).second->Block;
//(*TheElt).second->Block = NULL;
//NOTE1("MkxPrioCache::Flush delete 0x%08x", tmp);
			delete tmp;
		}
//NOTE("MkxPrioCache::Flush delete done");
	}
	DisplayOrderList.clear();
	CodingOrderList.clear();
NOTE2("MkxPrioCache::EndFlush (0x%08x) done %d", this, DisplayOrderList.size());
	Unlock();
}

/*!
	\todo handle MinCache & ReferencePriorities (for forward frames)
*/
void MkxPrioCache::StripList()
{
	NOTE2("MkxPrioCache::StripList %d/%d", DisplayOrderList.size(), CodingOrderList.size());
	// only delete elements that are displayed and not "needed in the cache" according to the MinCache
	std::map<MkxPrioKey, MkxPrioBuffer*>::iterator TheElt = DisplayOrderList.begin();
	for (; TheElt != DisplayOrderList.end(); ) {
		if ((*TheElt).second->Displayed && (*TheElt).second->CanDelete) {
			KaxBlockGroup *tmp = (*TheElt).second->Block;
//(*TheElt).second->Block = NULL;
NOTE1("MkxPrioCache::StripList delete 0x%08x", tmp);
			TheElt = DisplayOrderList.erase(TheElt);
			delete tmp;
		} else {
			TheElt++;
		}
	}
NOTE1("MkxPrioCache::StripList done %d", DisplayOrderList.size());
}
