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
	\version \$Id: mkxread.h,v 1.6 2003/08/01 21:15:47 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/

#ifndef MKXDS_MKXREAD_H
#define MKXDS_MKXREAD_H

#include "MatroskaReader.h"

class MkxFilter;
class MkxOutPin;

class MkxReadThread : public CAMThread {
public:
	MkxReadThread(MkxFilter * aFilter, IOCallback * aFileHandle);
	virtual ~MkxReadThread();
	virtual BOOL Create(void);
	void Stop();
	const uint64 GetDuration() { return m_MKReader.GetDuration(); }
	const UTFstring& GetTitle() { return m_MKReader.GetTitle(); }	
	inline const KaxChapters *GetChapters() const {return m_MKReader.GetChapters();}

	void SeekToTimecode(uint64 aTimecode, bool bPauseMode);
	void BlockProc();
	void UnBlockProc(bool bPauseMode);

protected: 
	enum WorkState {
		CMD_STOP,
		CMD_SLEEP,
		CMD_WAKEUP,
	};

	virtual DWORD ThreadProc();
	HRESULT CreateOutputPin();

	MkxFilter *mFilter;
	MatroskaReader  m_MKReader;

	struct TrackPin {
		UINT       TrackNum;
		MkxOutPin *Pin;
	};

	TrackPin *TrackPinTable;

	MkxOutPin *PinForTrack(UINT aTrackNum) const;

	CAMEvent SeekProtect;
	HANDLE   SeekProtection;
	BOOL     bTrustCRC;
};

#endif // MKXDS_MKXREAD_H
