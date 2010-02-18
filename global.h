/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2003 Jan-D. Schlenker.  All rights reserved.
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
	\version \$Id: global.h,v 1.5 2003/07/28 22:47:22 robux4 Exp $
	\author Jan-D. Schlenker <myfun @ users.corecodec.org>
	\author Christophe Paris <toffparis @ users.sf.net>
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

// libebml includes
#include "ebml/EbmlTypes.h"
#include "ebml/EbmlHead.h"
#include "ebml/EbmlSubHead.h"
#include "ebml/EbmlStream.h"
#include "ebml/EbmlContexts.h"

// libmatroska includes
#include "matroska/KaxConfig.h"
#include "matroska/KaxBlock.h"
#include "matroska/KaxSegment.h"
#include "matroska/KaxContexts.h"
#include "matroska/KaxTracks.h"
#include "matroska/KaxInfo.h"
#include "matroska/KaxInfoData.h"
#include "matroska/KaxCluster.h"
#include "matroska/KaxClusterData.h"
#include "matroska/KaxTrackAudio.h"
#include "matroska/KaxTrackVideo.h"
#include "matroska/KaxSeekHead.h"
#include "matroska/KaxTags.h"
#include "matroska/KaxChapters.h"


struct KaxVideoTrackInfoStruct {
	uint8		Interlaced;
	uint8		StereoMode;
	uint32		PixelWidth;
	uint32		PixelHeight;
	uint32		DisplayWidth;
	uint32		DisplayHeight;
	uint32		DisplayUnit;
	uint32		AspectRatio;
	float		FrameRate;
};

struct KaxAudioTrackInfoStruct {
	float		SamplingFreq;
	uint32		Channels;
	void		*Position;
	uint32		BitDepth;			// optional for PrE
	uint32		SubTrackName;
	uint32		SubTrackISRC;
};

struct KaxTrackInfoStruct {
	uint8		 Number;
	uint8		 Type;
	uint8		 Enabled;
	uint8		 Default;
	uint8		 Lacing;
	uint32		 MinCache;				// possible range ??
	uint32		 MaxCache;				// possible range ??
	UTFstring    Name;
	std::string	 Language;
	std::string	 CodecID;
	binary		*CodecPrivate;
	uint32		 CodecPrivateLen;
	UTFstring    CodecName;
	uint8		 CodecDecodeAll;
	uint32		 Overlay;				// possible range ??
	uint32		 DefaultDuration;
	union {
		KaxVideoTrackInfoStruct	VideoInfo;
		KaxAudioTrackInfoStruct	AudioInfo;
	};
	~KaxTrackInfoStruct()
	{
		if (CodecPrivate != NULL) 
			free(CodecPrivate);
		if (Type == track_audio)
			delete AudioInfo.Position;
	}
};

#endif __GLOBAL_H__
