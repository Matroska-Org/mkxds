/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2003 Christophe Paris.  All rights reserved.
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
	\version \$Id: MatroskaReader.cpp,v 1.10 2003/08/01 22:26:40 robux4 Exp $
	\author Christophe Paris     <toffparis @ users.sf.net>
*/

#include "MatroskaReader.h"
#include <dshow.h>
#include <streams.h>
#include "ebml/EbmlVoid.h"
#include "ebml/MemIOCallback.h"
#include "matroska/KaxCuesData.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MatroskaReader::MatroskaReader()
 :bReadDummyElements(true)
 ,m_pStream(NULL)
 ,m_pKaxIO(NULL)
 ,m_qwDuration(0)
 ,m_TimeCodeScale(1000000)
 ,m_Tracks(NULL)
 ,m_CueEntries(NULL)
 ,m_Chapters(NULL)
 ,m_MetaSeeks(NULL)
 ,m_Tags(NULL)
 ,m_Title(L"")
 ,bLookForBlock(false)
 ,m_CurrCluster(NULL)
 ,bTrustCRC(TRUE)
{
}

// ----------------------------------------------------------------------------

MatroskaReader::~MatroskaReader()
{
	NOTE("MatroskaReader::~MatroskaReader");
	delete m_pStream;

//	if (m_CurrCluster != NULL)
//		delete m_CurrCluster;

	if (m_Tracks != NULL)
		delete m_Tracks;

	if (m_CueEntries != NULL)
		delete m_CueEntries;

	if (m_MetaSeeks != NULL)
		delete m_MetaSeeks;

	if (m_Chapters != NULL)
		delete m_Chapters;

	if (m_pKaxIO != NULL)
		m_pKaxIO->close();

	while(mTrackInfos.size())
	{
		delete mTrackInfos.back();
		mTrackInfos.pop_back();
	}
}

// ----------------------------------------------------------------------------

int MatroskaReader::InitKaxFile(IOCallback *pIOCb, BOOL TrustCRC) 
{
	bool	bTracksLoaded = false;
	int		UpperElementLevel = 0;
	bool    bKeepElement1 = false;
	unsigned int Index0;
	
    NOTE("MatroskaReader::InitKaxFile");
	bTrustCRC = TrustCRC;
	// initialize Element array
	for (int i=0; i<6; i++)
		m_pElems[i] = NULL;
	
	// initialize EBML Stream
	if (pIOCb == NULL)
		return -1;
	
	if (m_pStream != NULL)
		return -2;	// for now we dont support this
	
	m_iUpElLev = 0;
	m_Tracks = NULL;
	m_CueEntries = NULL;
	m_Chapters = NULL;
	m_MetaSeeks = NULL;
	m_CurrCluster = NULL;

	if ((m_pStream = new EbmlStream(*pIOCb)) == NULL)
		return -3;
	
	m_pKaxIO = pIOCb;
	
	// is this a valid EBML file ?
	if ((m_pElems[0] = m_pStream->FindNextID(EbmlHead::ClassInfos, 0xFFFFFFFFL)) != NULL)
	{
		// Make sure we have the expected element		
		if (!(EbmlId(*m_pElems[0]) == EbmlHead::ClassInfos.GlobalId))
		{
			return -1;
		}
		
		m_pElems[1] = m_pStream->FindNextElement(m_pElems[0]->Generic().Context, UpperElementLevel, m_pElems[0]->ElementSize(), bReadDummyElements);
		while (m_pElems[1] != NULL) 
		{
			if (UpperElementLevel > 0)
				break;
			if (UpperElementLevel < 0)
				UpperElementLevel = 0;
			
			if (EbmlId(*m_pElems[1]) == EDocType::ClassInfos.GlobalId)
			{
				EDocType & DocType = *static_cast<EDocType*>(m_pElems[1]);
				DocType.ReadData(m_pStream->I_O());
				if (std::string(DocType) != "matroska")
					return -5;
				break; // we are finished with the EbmlHead for now
			}

			if (UpperElementLevel > 0) 
			{
				UpperElementLevel--;
				delete m_pElems[1];
				m_pElems[1] = m_pElems[2];
				if (UpperElementLevel > 0)
					break;
			}
			else
			{
				m_pElems[1]->SkipData(*m_pStream, m_pElems[1]->Generic().Context);
				delete m_pElems[1];
				
				m_pElems[1] = m_pStream->FindNextElement(m_pElems[0]->Generic().Context, UpperElementLevel, m_pElems[0]->ElementSize(), bReadDummyElements);
			}
		}
		
		m_pElems[0]->SkipData(*m_pStream, EbmlHead_Context);
		if (m_pElems[0] != NULL)
		{
			delete m_pElems[0];
			m_pElems[0] = NULL;
		}
	}
	else
	{
		return -4;
	}
	
	NOTE("MatroskaReader::InitKaxFile matroska ebml header OK");

	// find a matroska segment
	if ((m_pElems[0] = m_pStream->FindNextID(KaxSegment::ClassInfos, 0xFFFFFFFFL)) != NULL)
		if (!(EbmlId(*m_pElems[0]) == KaxSegment::ClassInfos.GlobalId))
			return -5;
		
		NOTE("MatroskaReader::InitKaxFile found KaxSegment");

		mCurSegment = m_Segments.AddSegment(*static_cast<KaxSegment *>(m_pElems[0]));

		// parse all sub-elements
		m_pElems[1] = m_pStream->FindNextElement(m_pElems[0]->Generic().Context, UpperElementLevel, m_pElems[0]->ElementSize(), bReadDummyElements);
		while (m_pElems[1] != NULL) 
		{
			if (UpperElementLevel > 0)
				break;
			if (UpperElementLevel < 0)
				UpperElementLevel = 0;
			
			if (EbmlId(*m_pElems[1]) == KaxTracks::ClassInfos.GlobalId)
			{
				NOTE("MatroskaReader::InitKaxFile found KaxTracks");
				if (m_Tracks != NULL) {
					///< should we merge the two ?
				} else {
					m_Tracks = static_cast<KaxTracks*>(m_pElems[1]);
					m_Tracks->Read(*m_pStream, KaxTracks::ClassInfos.Context, m_iUpElLev, m_pElems[2], bReadDummyElements);
					for (Index0 = 0; Index0 < m_Tracks->ListSize(); Index0++) {
						if (EbmlId(*((*m_Tracks)[Index0])) == KaxTrackEntry::ClassInfos.GlobalId)
						{
							NOTE("MatroskaReader::InitKaxFile found KaxTrackEntry");

							// and fill it with those values					
							KaxTrackInfoStruct *TrackInfo = new KaxTrackInfoStruct;
							memset(TrackInfo, 0, sizeof(KaxTrackInfoStruct));
							InitTrack(*static_cast<KaxTrackEntry *>((*m_Tracks)[Index0]), UpperElementLevel, TrackInfo);
							mTrackInfos.push_back(TrackInfo);
						}
					}
				}

				bTracksLoaded = true;
				bKeepElement1 = true;
			} else if (EbmlId(*m_pElems[1]) == KaxCues::ClassInfos.GlobalId) {
				if (m_CueEntries != NULL) {
					///< should we merge the two ?
				} else {
					NOTE("MatroskaReader::InitKaxFile found KaxCues");
					m_CueEntries = static_cast<KaxCues*>(m_pElems[1]);
					m_CueEntries->SetGlobalTimecodeScale(m_TimeCodeScale);
					m_CueEntries->Read(*m_pStream, KaxCues::ClassInfos.Context, m_iUpElLev, m_pElems[2], bReadDummyElements);
					if (!bTrustCRC || !m_CueEntries->HasChecksum() || m_CueEntries->VerifyChecksum()) {
						m_CueEntries->Sort();
						bKeepElement1 = true;
					} else {
						m_CueEntries = NULL;
						bKeepElement1 = false;
					}
				}
			} else if (EbmlId(*m_pElems[1]) == KaxSeekHead::ClassInfos.GlobalId) {
				if (m_MetaSeeks != NULL) {
					///< should we merge the two ?
				} else {
					NOTE("MatroskaReader::InitKaxFile found KaxSeekHead");
					m_MetaSeeks = static_cast<KaxSeekHead*>(m_pElems[1]);
					m_MetaSeeks->Read(*m_pStream, KaxSeekHead::ClassInfos.Context, m_iUpElLev, m_pElems[2], bReadDummyElements);
					if (!bTrustCRC || !m_MetaSeeks->HasChecksum() || m_MetaSeeks->VerifyChecksum()) {
						m_CueEntries = GetCueEntry(*m_pStream, *m_MetaSeeks, *static_cast<KaxSegment *>(m_pElems[0]));
						if (m_CueEntries != NULL) {
							if (bTrustCRC && m_CueEntries->HasChecksum() && !m_CueEntries->VerifyChecksum()) {
								m_CueEntries = NULL;
							} else {
								m_CueEntries->Sort();
							}
						}
						m_Chapters = GetChaptersEntry(*m_pStream, *m_MetaSeeks, *static_cast<KaxSegment *>(m_pElems[0]));
						if (m_Chapters != NULL) {
							if (bTrustCRC && m_Chapters->HasChecksum() && !m_Chapters->VerifyChecksum()) {
								m_Chapters = NULL;
							}
						}
						bKeepElement1 = true;
					} else {
						m_MetaSeeks = NULL;
						bKeepElement1 = false;
					}
				}
			} else if (EbmlId(*m_pElems[1]) == KaxChapters::ClassInfos.GlobalId) {
				if (m_Chapters != NULL) {
					///< should we merge the two ?
				} else {
					NOTE("MatroskaReader::InitKaxFile found KaxChapters");
					m_Chapters = static_cast<KaxChapters*>(m_pElems[1]);
					m_Chapters->Read(*m_pStream, KaxChapters::ClassInfos.Context, m_iUpElLev, m_pElems[2], bReadDummyElements);
					if (!bTrustCRC || !m_Chapters->HasChecksum() || m_Chapters->VerifyChecksum()) {
						bKeepElement1 = true;
					} else {
						m_Chapters = NULL;
						bKeepElement1 = false;
					}
				}
			}
			else if (EbmlId(*m_pElems[1]) == KaxCluster::ClassInfos.GlobalId) 
			{
				NOTE("MatroskaReader::InitKaxFile found first KaxCluster");
				// store position
//				m_qwKaxSegStart = m_pKaxIO->getFilePointer();
				
				m_pTmpElems[0] = m_pElems[0];
				m_pTmpElems[1] = m_pElems[1];
				
				// and exit if we already parsed the TrackEntry
				m_iLevel = 1;
				m_iTmpUpElLev = m_iUpElLev = UpperElementLevel;
				
				if (bTracksLoaded)
					return 1;
			}
			else if (EbmlId(*m_pElems[1]) == KaxInfo::ClassInfos.GlobalId) 
			{
				KaxInfo *pInfos = static_cast<KaxInfo *>(m_pElems[1]);
				pInfos->Read(*m_pStream, KaxInfo::ClassInfos.Context, UpperElementLevel, m_pElems[2], bReadDummyElements);
				if (!bTrustCRC || !pInfos->HasChecksum() || pInfos->VerifyChecksum()) {
					for (Index0 = 0; Index0<pInfos->ListSize() ;Index0++) {
						if ((*pInfos)[Index0]->Generic().GlobalId == KaxTimecodeScale::ClassInfos.GlobalId) 
						{
							NOTE("MatroskaReader::InitKaxFile found KaxTimecodeScale");
							KaxTimecodeScale & TimecodeScale = *static_cast<KaxTimecodeScale*>((*pInfos)[Index0]);
							m_TimeCodeScale = TimecodeScale;
						}
						else if ((*pInfos)[Index0]->Generic().GlobalId == KaxDuration::ClassInfos.GlobalId) 
						{
							NOTE("MatroskaReader::InitKaxFile found KaxDuration");
							KaxDuration & Duration = *static_cast<KaxDuration*>((*pInfos)[Index0]);
							double TimeScale = (double)uint32(m_TimeCodeScale);
							TimeScale /= 100.0f;	// multiplicator (convert 1ns -> 100ns)
							m_qwDuration = uint64(((float)Duration) * TimeScale);
						}
						else if ((*pInfos)[Index0]->Generic().GlobalId == KaxTitle::ClassInfos.GlobalId) 
						{
							NOTE("MatroskaReader::InitKaxFile found KaxTitle");
							KaxTitle & Title = *static_cast<KaxTitle*>((*pInfos)[Index0]);
							m_Title = Title;
						}
					}
				}
				bKeepElement1 = false;
			} else {
				bKeepElement1 = false;
			}
			
			if (UpperElementLevel > 0) 
			{
				UpperElementLevel--;
				if (!bKeepElement1)
					delete m_pElems[1];
				m_pElems[1] = m_pElems[2];
				if (UpperElementLevel > 0)
					break;
			}
			else
			{
				m_pElems[1]->SkipData(*m_pStream, m_pElems[1]->Generic().Context);
				if (!bKeepElement1)
					delete m_pElems[1];
				
				m_pElems[1] = m_pStream->FindNextElement(m_pElems[0]->Generic().Context, UpperElementLevel, m_pElems[0]->ElementSize(), bReadDummyElements);
			}
	}
	
	
	NOTE("MatroskaReader::InitKaxFile leave");
	return 0;
}

// ----------------------------------------------------------------------------

// parsing KaxTrackEntry SubElements
int MatroskaReader::InitTrack(KaxTrackEntry & aTrack, int &UEL, KaxTrackInfoStruct *pTrackInfos)
{
    NOTE("MatroskaReader::InitTrack");
	
	// PrE - computer video player profile
	aTrack.SetGlobalTimecodeScale(m_TimeCodeScale);
	unsigned int i,j;
	for (i=0; i<aTrack.ListSize(); i++)
	{
		if (EbmlId(*aTrack[i]) == KaxTrackNumber::ClassInfos.GlobalId)
		{
			KaxTrackNumber &TrackNum = *static_cast<KaxTrackNumber*>(aTrack[i]);
			pTrackInfos->Number = TrackNum;
		}
		else if (EbmlId(*aTrack[i]) == KaxTrackType::ClassInfos.GlobalId)
		{
			KaxTrackType &TrackType = *static_cast<KaxTrackType*>(aTrack[i]);
			pTrackInfos->Type = TrackType;
		}
		else if (EbmlId(*aTrack[i]) == KaxTrackFlagEnabled::ClassInfos.GlobalId)
		{
			KaxTrackFlagEnabled &FlagEnabled = *static_cast<KaxTrackFlagEnabled*>(aTrack[i]);
			pTrackInfos->Enabled = FlagEnabled;
		}
		else if (EbmlId(*aTrack[i]) == KaxTrackFlagDefault::ClassInfos.GlobalId) 
		{
			KaxTrackFlagDefault &FlagDefault = *static_cast<KaxTrackFlagDefault*>(aTrack[i]);
			pTrackInfos->Default = FlagDefault;
		}
		else if (EbmlId(*aTrack[i]) == KaxTrackFlagLacing::ClassInfos.GlobalId)
		{
			KaxTrackFlagLacing &FlagLacing= *static_cast<KaxTrackFlagLacing*>(aTrack[i]);
			pTrackInfos->Lacing = FlagLacing;
		}
		else if (EbmlId(*aTrack[i]) == KaxTrackMinCache::ClassInfos.GlobalId) 
		{
			KaxTrackMinCache &MinCache = *static_cast<KaxTrackMinCache*>(aTrack[i]);
			pTrackInfos->MinCache = MinCache;
		}
		else if (EbmlId(*aTrack[i]) == KaxTrackMaxCache::ClassInfos.GlobalId) 
		{
			KaxTrackMaxCache &MaxCache = *static_cast<KaxTrackMaxCache*>(aTrack[i]);
			pTrackInfos->MaxCache = MaxCache;
		}
		else if (EbmlId(*aTrack[i]) == KaxTrackName::ClassInfos.GlobalId) 
		{
			KaxTrackName &Name = *static_cast<KaxTrackName*>(aTrack[i]);
			pTrackInfos->Name = Name;
		}
		else if (EbmlId(*aTrack[i]) == KaxTrackLanguage::ClassInfos.GlobalId) 
		{
			KaxTrackLanguage &Language = *static_cast<KaxTrackLanguage*>(aTrack[i]);
			pTrackInfos->Language = Language;
		}
		else if (EbmlId(*aTrack[i]) == KaxCodecID::ClassInfos.GlobalId) 
		{
			KaxCodecID &CodecID = *static_cast<KaxCodecID*>(aTrack[i]);
			pTrackInfos->CodecID = CodecID;
		}
		else if (EbmlId(*aTrack[i]) == KaxCodecPrivate::ClassInfos.GlobalId)
		{
			KaxCodecPrivate &CodecPrivate = *static_cast<KaxCodecPrivate*>(aTrack[i]);
			pTrackInfos->CodecPrivateLen = CodecPrivate.GetSize();
			pTrackInfos->CodecPrivate = (binary*)malloc(CodecPrivate.GetSize() * sizeof(binary));
			memcpy(pTrackInfos->CodecPrivate, CodecPrivate.GetBuffer(), CodecPrivate.GetSize());			
		}
		else if (EbmlId(*aTrack[i]) == KaxCodecName::ClassInfos.GlobalId) 
		{
			KaxCodecName &CodecName = *static_cast<KaxCodecName*>(aTrack[i]);
			pTrackInfos->CodecName = CodecName;
		}
		else if (EbmlId(*aTrack[i]) == KaxCodecDecodeAll::ClassInfos.GlobalId)
		{
			KaxCodecDecodeAll &CodecDecodeAll = *static_cast<KaxCodecDecodeAll*>(aTrack[i]);
			pTrackInfos->CodecDecodeAll = CodecDecodeAll;
		}
		else if (EbmlId(*aTrack[i]) == KaxTrackOverlay::ClassInfos.GlobalId) 
		{
			KaxTrackOverlay &Overlay = *static_cast<KaxTrackOverlay*>(aTrack[i]);
			pTrackInfos->Overlay = Overlay;
		}
		else if (EbmlId(*aTrack[i]) == KaxTrackDefaultDuration::ClassInfos.GlobalId) 
		{
			KaxTrackDefaultDuration &DefaultDuration = *static_cast<KaxTrackDefaultDuration*>(aTrack[i]);
			pTrackInfos->DefaultDuration = DefaultDuration;
		}
		else if (EbmlId(*aTrack[i]) == KaxTrackVideo::ClassInfos.GlobalId) 
		{
			KaxTrackVideo & TrackVideo = *static_cast<KaxTrackVideo*>(aTrack[i]);
			for (j = 0; j<TrackVideo.ListSize(); j++)
			{
				if (EbmlId(*TrackVideo[j]) == KaxVideoFlagInterlaced::ClassInfos.GlobalId) 
				{
					KaxVideoFlagInterlaced &Interlaced = *static_cast<KaxVideoFlagInterlaced*>(TrackVideo[j]);
					pTrackInfos->VideoInfo.Interlaced = Interlaced;
				}
				else if (EbmlId(*TrackVideo[j]) == KaxVideoStereoMode::ClassInfos.GlobalId)
				{
					KaxVideoStereoMode &StereoMode = *static_cast<KaxVideoStereoMode*>(TrackVideo[j]);
					pTrackInfos->VideoInfo.StereoMode = StereoMode;
				}
				else if (EbmlId(*TrackVideo[j]) == KaxVideoPixelWidth::ClassInfos.GlobalId)
				{
					KaxVideoPixelWidth &PixelWidth = *static_cast<KaxVideoPixelWidth*>(TrackVideo[j]);
					pTrackInfos->VideoInfo.PixelWidth = PixelWidth;
				}
				else if (EbmlId(*TrackVideo[j]) == KaxVideoPixelHeight::ClassInfos.GlobalId)
				{
					KaxVideoPixelHeight &PixelHeight = *static_cast<KaxVideoPixelHeight*>(TrackVideo[j]);
					pTrackInfos->VideoInfo.PixelHeight = PixelHeight;
				}
				else if (EbmlId(*TrackVideo[j]) == KaxVideoDisplayWidth::ClassInfos.GlobalId)
				{
					KaxVideoDisplayWidth &DisplayWidth = *static_cast<KaxVideoDisplayWidth*>(TrackVideo[j]);
					pTrackInfos->VideoInfo.DisplayWidth = DisplayWidth;
				}
				else if (EbmlId(*TrackVideo[j]) == KaxVideoDisplayHeight::ClassInfos.GlobalId)
				{
					KaxVideoDisplayHeight &DisplayHeight = *static_cast<KaxVideoDisplayHeight*>(TrackVideo[j]);
					pTrackInfos->VideoInfo.DisplayHeight = DisplayHeight;
				}
				else if (EbmlId(*TrackVideo[j]) == KaxVideoFrameRate::ClassInfos.GlobalId)
				{
					KaxVideoFrameRate &FrameRate = *static_cast<KaxVideoFrameRate*>(TrackVideo[j]);
					pTrackInfos->VideoInfo.FrameRate = FrameRate;
				}				
			}
		}
		else if (EbmlId(*aTrack[i]) == KaxTrackAudio::ClassInfos.GlobalId)
		{
			KaxTrackAudio & TrackAudio = *static_cast<KaxTrackAudio*>(aTrack[i]);
			for (j = 0; j<TrackAudio.ListSize(); j++)
			{
				if (EbmlId(*TrackAudio[j]) == KaxAudioSamplingFreq::ClassInfos.GlobalId) 
				{
					KaxAudioSamplingFreq *SampFreq = static_cast<KaxAudioSamplingFreq*>(TrackAudio[j]);
					if (SampFreq == NULL) {
						// default value
						pTrackInfos->AudioInfo.SamplingFreq = 8000;
					} else {
						pTrackInfos->AudioInfo.SamplingFreq = *SampFreq;
					}
				}
				else if (EbmlId(*TrackAudio[j]) == KaxAudioChannels::ClassInfos.GlobalId)
				{
					KaxAudioChannels *Channels = static_cast<KaxAudioChannels*>(TrackAudio[j]);
					if (Channels == NULL) {
						pTrackInfos->AudioInfo.Channels = 1;
					} else {
						pTrackInfos->AudioInfo.Channels = *Channels;
					}
				}
				else if (EbmlId(*TrackAudio[j]) == KaxAudioPosition::ClassInfos.GlobalId)
				{
					KaxAudioPosition &ChanPos = *static_cast<KaxAudioPosition*>(TrackAudio[j]);
					pTrackInfos->AudioInfo.Position = ChanPos.GetBuffer();
				}
				else if (EbmlId(*TrackAudio[j]) == KaxAudioBitDepth::ClassInfos.GlobalId) 
				{
					KaxAudioBitDepth *BitDepth = static_cast<KaxAudioBitDepth*>(TrackAudio[j]);
					if (BitDepth == NULL) {
						// default value
						pTrackInfos->AudioInfo.BitDepth = 16;
					} else {
						pTrackInfos->AudioInfo.BitDepth = *BitDepth;
					}
				}/*
				 else if (EbmlId(*plev4) == KaxAudioSubTrack::ClassInfos.GlobalId) {
				 
				}*/
			}
		}
	}

	return 0;
}

// ----------------------------------------------------------------------------

int MatroskaReader::GetTrackNumber() const
{
	return mTrackInfos.size();
}

// ----------------------------------------------------------------------------

KaxTrackInfoStruct& MatroskaReader::GetTrackInfo(UINT i)
{
	assert(i<mTrackInfos.size());
	return *(mTrackInfos[i]);
}

// ----------------------------------------------------------------------------

/*!
	\todo put element in a home made Cue entry if none is found in the file
	\todo use KaxCluster::Read() to get block to enable CheckSum checking
*/
KaxBlockGroup *MatroskaReader::GetNextBlockG() 
{
//	KaxCluster	  *SegCluster;
	KaxBlockGroup *pBlockG;
	BOOL bKeepElement0 = TRUE;
	BOOL bKeepElement1 = FALSE;
	BOOL bKeepElement2 = TRUE;
	UINT Index0;
	BOOL bChecksumPassed;

	switch (m_iLevel) 
	{
//	case -2: goto bgroup;
	case 0: break;
	case 1: goto level1;
	case 2: goto level2;
	case 10:goto levelSeek;
	default: return NULL;
	}
	
	m_iLevel = 0;
	if ((m_pElems[0] = m_pStream->FindNextID(KaxSegment::ClassInfos, 0xFFFFFFFFL)) == NULL)
		return NULL;
	
	while (1) 
	{
		if (m_iUpElLev > 0)
			break;
		if (m_iUpElLev < 0)
			m_iUpElLev = 0;
		bKeepElement0 = false;
		
		// segment (level 0)
		if (EbmlId(*m_pElems[0]) == KaxSegment::ClassInfos.GlobalId)
		{
			mCurSegment = m_Segments.AddSegment(*static_cast<KaxSegment *>(m_pElems[0]));
			bKeepElement0 = true;

			m_iLevel = 1;
levelSeek:
			if ((m_pElems[1] = m_pStream->FindNextElement(KaxSegment::ClassInfos.Context, m_iUpElLev, m_pElems[0]->ElementSize(), bReadDummyElements)) == NULL)
				break;

			while (1)
			{
				if (m_iUpElLev > 0)
					break;
				if (m_iUpElLev < 0)
					m_iUpElLev = 0;
level1:
				// cluster (level 1)
				if (EbmlId(*m_pElems[1]) == KaxCluster::ClassInfos.GlobalId) 
				{
					m_CurrCluster = static_cast<KaxCluster *>(m_pElems[1]);
					if (bTrustCRC)
					{
						MemIOCallback aReadBuffer(m_CurrCluster->GetSize());
						aReadBuffer.write(m_pStream->I_O(), m_CurrCluster->GetSize());
						EbmlStream MemStream(aReadBuffer);
						m_CurrCluster->Read(MemStream, KaxCluster::ClassInfos.Context, m_iUpElLev, m_pElems[2], bReadDummyElements);
						if (m_CurrCluster->HasChecksum()) {
							EbmlCrc32 aChecksum;
							// VERY UGLY ! assume the timecode was the first 6 octets
							aChecksum.FillCRC32(aReadBuffer.GetDataBuffer() + 6, m_CurrCluster->GetSize() - 6);
							bChecksumPassed = (aChecksum.GetCrc32() == m_CurrCluster->GetCrc32());
						} else {
							bChecksumPassed = TRUE;
						}
					} else {
						m_CurrCluster->Read(*m_pStream, KaxCluster::ClassInfos.Context, m_iUpElLev, m_pElems[2], bReadDummyElements);
						bChecksumPassed = TRUE;
					}

					if (bChecksumPassed) {
						CurrClusterIndex = 0;
						// look for the KaxClusterTimecode
						for (Index0 = 0; Index0 < m_CurrCluster->ListSize(); Index0++) {
							if (EbmlId(*((*m_CurrCluster)[Index0])) == KaxClusterTimecode::ClassInfos.GlobalId) {
								KaxClusterTimecode &ClusterTime = *static_cast<KaxClusterTimecode*>((*m_CurrCluster)[Index0]);
								m_CurrCluster->InitTimecode(uint32(ClusterTime), m_TimeCodeScale);
								assert(mCurSegment != NULL);
								mCurSegment->ContainsTimecode(m_CurrCluster->GlobalTimecode() / 100);
								if (Index0 == 0)
									CurrClusterIndex = 1;
								break;
							}
						}
						
						m_iLevel = 2;
level2:
						for (Index0 = CurrClusterIndex; Index0 < m_CurrCluster->ListSize(); Index0++) {
							if (EbmlId(*((*m_CurrCluster)[Index0])) == KaxBlockGroup::ClassInfos.GlobalId)
							{						
	//							bKeepElement2 = false;
								pBlockG = static_cast<KaxBlockGroup*>((*m_CurrCluster)[Index0]);
								pBlockG->SetParent(*m_CurrCluster);
								KaxBlock *TheBlock = static_cast<KaxBlock *>(pBlockG->FindElt(KaxBlock::ClassInfos));
								if (TheBlock != NULL) {
									TheBlock->SetParent(*m_CurrCluster);
									pBlockG->SetParentTrack(*static_cast<KaxTrackEntry *>((*m_Tracks)[IndexForTrack(TheBlock->TrackNum())]));
									if (bLookForBlock) {
										// make some checks to know if we discard this Block or not
										if (BlockTrack == TheBlock->TrackNum() && BlockTimecode == TheBlock->GlobalTimecode())
										{
											bLookForBlock = false;
										}
									}
									if (!bLookForBlock) {
	//									bKeepElement2 = true;
	//									m_iLevel = -2;	// means we're in blockgroup
										// remove the BlockGroup from this cluster, the reader should not know about it anymore
										m_CurrCluster->Remove(Index0);
										return pBlockG;
									}
								}
							}
							CurrClusterIndex++; // next time, start further
						}
					}
					// no more blocks in this cluster, delete the Cluster
					bKeepElement1 = false;
//					delete m_CurrCluster;
//					m_CurrCluster = NULL;
				} else if (EbmlId(*m_pElems[1]) == KaxCues::ClassInfos.GlobalId) {
					if (m_CueEntries != NULL) {
						///< should we merge the two ?
					} else {
						m_CueEntries = static_cast<KaxCues*>(m_pElems[1]);
						m_CueEntries->SetGlobalTimecodeScale(m_TimeCodeScale);
						m_CueEntries->Read(*m_pStream, KaxCues::ClassInfos.Context, m_iUpElLev, m_pElems[2], bReadDummyElements);
						m_CueEntries->Sort();
						bKeepElement1 = true;
					}
				} else if (EbmlId(*m_pElems[1]) == KaxSeekHead::ClassInfos.GlobalId) {
					if (m_MetaSeeks != NULL) {
						///< should we merge the two ?
					} else {
						m_MetaSeeks = static_cast<KaxSeekHead*>(m_pElems[1]);
						m_MetaSeeks->Read(*m_pStream, KaxSeekHead::ClassInfos.Context, m_iUpElLev, m_pElems[2], bReadDummyElements);
						m_CueEntries = GetCueEntry(*m_pStream, *m_MetaSeeks, *static_cast<KaxSegment *>(m_pElems[0]));						
						if (m_CueEntries != NULL)
							m_CueEntries->Sort();
						///< \todo : read tags and use them to display description in mplayer and add an inteface for other tags
						//m_Tags = GetTagsEntry(*m_pStream, *m_MetaSeeks, *static_cast<KaxSegment *>(m_pElems[0]));
						bKeepElement1 = true;
					}
				} else {
					bKeepElement1 = false;
				}

				/*
				else if (m_pElems[1]->IsDummy())//EbmlId(*m_pElems[1]) == DummyRawElement::ClassInfos.GlobalId)
				break;//continue;	// skip() = assert() !!!
				*/		
				if (m_iUpElLev > 0) 
				{
					m_iUpElLev--;
					if (!bKeepElement1)
						delete m_pElems[1];
					m_pElems[1] = m_pElems[2];
					if (m_iUpElLev > 0)
						break;
				}
				else 
				{
					///< \todo modify FindNextElement to disable using global elements instead, in seeking mode
					if (!(EbmlId(*m_pElems[1]) == EbmlVoid::ClassInfos.GlobalId)) {
						m_pElems[1]->SkipData(*m_pStream, m_pElems[1]->Generic().Context);
					}
					if (!bKeepElement1)
						delete m_pElems[1];
					
					if ((m_pElems[1] = m_pStream->FindNextElement(m_pElems[0]->Generic().Context, m_iUpElLev, m_pElems[0]->ElementSize(), bReadDummyElements)) == NULL)
						break;
				}
			}
		} else {
			bKeepElement0 = false;
		}
		
		if (m_iUpElLev > 0)
		{
			m_iUpElLev--;
			if (!bKeepElement0)
				delete m_pElems[0];
			m_pElems[0] = m_pElems[1];
			if (m_iUpElLev > 0)
				break;
		}
		else {
			m_pElems[0]->SkipData(*m_pStream, m_pElems[0]->Generic().Context);
			if (!bKeepElement0)
				delete m_pElems[0];
			
			if ((m_pElems[0] = m_pStream->FindNextID(KaxSegment::ClassInfos, 0xFFFFFFFFL)) == NULL) {
				// no more readable data in this file
				m_iLevel = 0;
				break;
			}
		}
	}
	
	return NULL;
}

// ----------------------------------------------------------------------------

void  MatroskaReader::Reset()
{
NOTE("MatroskaReader::Reset");
	m_pElems[0] = 0;
	m_pElems[1] = 0;
	m_iUpElLev = 0;
	m_iLevel = 0;
	m_pKaxIO->setFilePointer(0);	
}

// ----------------------------------------------------------------------------

void MatroskaReader::EndOfStream()
{
NOTE1("MatroskaReader::EndOfStream %d", m_iLevel);
	if (m_iLevel != 10) {
		// reset all read tags
		for (INT i=1; i<=m_iLevel; i++) {
			if (m_pElems[i] != NULL) {
//				delete m_pElems[i]; // can't delete an element if it's a saved one Block, 
				m_pElems[i] = NULL;
			}
		}
	}
	m_pElems[2] = NULL;
	m_iLevel = 10; // levelSeek
	m_iUpElLev = 0;
}

// ----------------------------------------------------------------------------

void MatroskaReader::JumpToTimecode(uint64 aTimecode, bool bSeekToTheCorrectBlock)
{
	uint64 approx_location = 0; ///< \todo position of the first Cluster
	bool bLocationFound = false;
	KaxSegment *aSegment = NULL;
	SegmentInfo *tmp = m_Segments.FindTimecode(aTimecode);
	if (tmp != NULL)
	{
		mCurSegment = tmp;
	}
	assert(mCurSegment != NULL);
	aSegment = mCurSegment->Segment();

	// cue mode
	if (m_CueEntries != NULL)
	{
		const KaxCuePoint * aPoint = m_CueEntries->GetTimecodePoint(aTimecode * 100);
		if (aPoint != NULL)
		{
			const KaxCueTrackPositions * aPos = aPoint->GetSeekPosition();
			if (aPos != NULL)
			{
				approx_location = aPos->ClusterPosition();
				bLocationFound = true;
				if (bSeekToTheCorrectBlock)
				{
					bLookForBlock = aPoint->Timecode(BlockTimecode, m_TimeCodeScale);
					BlockTrack = aPos->TrackNumber();
				}
			}
		}
	}

	// blind mode
	if (!bLocationFound)
	{
		///< \todo alternative version with the length of the Segment
		int64 SegLength;
		int64 CurLocation = m_pKaxIO->getFilePointer();
		if (aSegment != NULL && aSegment->IsFiniteSize()) {
			SegLength = aSegment->GetSize();
		} else {
			m_pKaxIO->setFilePointer(0, seek_end);
			SegLength = m_pKaxIO->getFilePointer();
		}
		
		if (m_qwDuration != 0) {
			// dirty
			int64 duration = m_qwDuration, timecode = aTimecode;
			approx_location = uint64(double(SegLength) * double(timecode) / double(duration));
		}

		// meta seek mode
		if (m_MetaSeeks != NULL)
		{
			// find the Cluster closer to this location
			approx_location = FindClusterLocated(approx_location, *m_MetaSeeks);
		}
	}

	if (m_iLevel == -1) {
		m_iLevel = 3;
	}
	if (m_iLevel == -2) {
		m_iLevel = 2;
	}
	if (m_iLevel != 10) {
		// reset all read tags
		for (INT i=1; i<=m_iLevel; i++) {
			if (m_pElems[i] != NULL) {
//				delete m_pElems[i]; // can't delete an element if it's a saved one Block, 
				m_pElems[i] = NULL;
			}
		}
	}
	m_iLevel = 10; // levelSeek
	m_iUpElLev = 0;

	// set the file pointer to the most appropriate location
	if (aSegment != NULL) {
		m_pElems[0] = aSegment;
		NOTE2("Seek at location %I64d in the file (%I64d)", aSegment->GetGlobalPosition(approx_location), approx_location);
		m_pKaxIO->setFilePointer(aSegment->GetGlobalPosition(approx_location));
	} else {
		NOTE1("Seek to timecode %I64d but no Segment Found !!! (we are probably at the end of the file)", aTimecode);
		m_pKaxIO->setFilePointer(approx_location);
	}
}

/*!
	\brief find a Cue entry in the meta seek and read it in memory
*/
KaxCues * MatroskaReader::GetCueEntry(EbmlStream & inDataStream, const KaxSeekHead & aMetaSeek, const KaxSegment & FirstSegment)
{
	KaxCues * result = NULL;
	KaxSeek * aElt = aMetaSeek.FindFirstOf(KaxCues::ClassInfos);
	
	if (aElt != NULL)
	{
		// get the location and read it
		KaxSeekPosition * aPosition = static_cast<KaxSeekPosition *>(aElt->FindFirstElt(KaxSeekPosition::ClassInfos, false));
		if (aPosition != NULL)
		{
			uint64 CuePosition = FirstSegment.GetGlobalPosition(uint64(*aPosition));
			uint64 CurrentLocation = inDataStream.I_O().getFilePointer();
			int upper = 0;
			EbmlElement *fake;

			inDataStream.I_O().setFilePointer(CuePosition);
			fake = inDataStream.FindNextElement(KaxSegment::ClassInfos.Context, upper, m_pElems[0]->ElementSize(), false);
			if (fake != NULL && EbmlId(*fake) == KaxCues::ClassInfos.GlobalId) {
				result = static_cast<KaxCues *>(fake);
				upper=0;
				result->SetGlobalTimecodeScale(m_TimeCodeScale);
				result->Read(inDataStream, KaxCues::ClassInfos.Context, upper, fake, false);
			}
			inDataStream.I_O().setFilePointer(CurrentLocation);
		}
	}

	return result;
}

/*!
	\brief find a Tag entry in the meta seek and read it in memory
*/
KaxTags * MatroskaReader::GetTagsEntry(EbmlStream & inDataStream, const KaxSeekHead & aMetaSeek, const KaxSegment & FirstSegment)
{
	KaxTags * result = NULL;
	KaxSeek * aElt = aMetaSeek.FindFirstOf(KaxTags::ClassInfos);
	
	if (aElt != NULL)
	{
		// get the location and read it
		KaxSeekPosition * aPosition = static_cast<KaxSeekPosition *>(aElt->FindFirstElt(KaxSeekPosition::ClassInfos, false));
		if (aPosition != NULL)
		{
			uint64 TagsPosition = FirstSegment.GetGlobalPosition(uint64(*aPosition));
			uint64 CurrentLocation = inDataStream.I_O().getFilePointer();
			int upper = 0;
			EbmlElement *fake;
			
			inDataStream.I_O().setFilePointer(TagsPosition);
			fake = inDataStream.FindNextElement(KaxSegment::ClassInfos.Context, upper, m_pElems[0]->ElementSize(), false);
			if (fake != NULL && EbmlId(*fake) == KaxTags::ClassInfos.GlobalId) {
				result = static_cast<KaxTags *>(fake);
				upper = 0;
				result->Read(inDataStream, KaxTags::ClassInfos.Context, upper, fake, false);
			}
			inDataStream.I_O().setFilePointer(CurrentLocation);
		}
	}
	
	return result;
}

KaxChapters * MatroskaReader::GetChaptersEntry(EbmlStream & inDataStream, const KaxSeekHead & aMetaSeek, const KaxSegment & FirstSegment)
{
	KaxChapters * result = NULL;
	KaxSeek * aElt = aMetaSeek.FindFirstOf(KaxChapters::ClassInfos);
	
	if (aElt != NULL)
	{
		// get the location and read it
		KaxSeekPosition * aPosition = static_cast<KaxSeekPosition *>(aElt->FindFirstElt(KaxSeekPosition::ClassInfos, false));
		if (aPosition != NULL)
		{
			uint64 CurrentLocation = inDataStream.I_O().getFilePointer();
			uint64 ChapterPosition = FirstSegment.GetGlobalPosition(uint64(*aPosition));
			int upper = 0;
			EbmlElement *fake;

			inDataStream.I_O().setFilePointer(ChapterPosition);
			fake = inDataStream.FindNextElement(KaxSegment::ClassInfos.Context, upper, m_pElems[0]->ElementSize(), false);
			if (fake != NULL && EbmlId(*fake) == KaxChapters::ClassInfos.GlobalId) {
				result = static_cast<KaxChapters *>(fake);
				upper=0;
				result->Read(inDataStream, KaxChapters::ClassInfos.Context, upper, fake, false);
			}
			// return to the original point
			inDataStream.I_O().setFilePointer(CurrentLocation);
		}
	}

	return result;
}

int64 MatroskaReader::FindClusterLocated(int64 aLocation, const KaxSeekHead & aMetaSeek)
{
	int64 result = aLocation;
	KaxSeek *PrevClust = NULL, *NextClust = NULL;
	KaxSeek *aElt = aMetaSeek.FindFirstOf(KaxCluster::ClassInfos);

	while (aElt != NULL)
	{
		if (aElt->Location() < aLocation && (PrevClust == NULL || aElt->Location() > PrevClust->Location()))
		{
			PrevClust = aElt;
		}
		if (aElt->Location() > aLocation && (NextClust == NULL || aElt->Location() < NextClust->Location()))
		{
			NextClust = aElt;
		}
		aElt = aMetaSeek.FindNextOf(*aElt);
	}

	return (PrevClust == NULL)? result: PrevClust->Location();
}

// ----------------------------------------------------------------------------

//     SegmentInfo class

void SegmentInfo::ContainsTimecode(uint64 aTimecode)
{
	if (!bMinIsSet) {
		bMinIsSet = true;
		mMinTimecode = aTimecode;
	} else {
		if (mMinTimecode > aTimecode)
			mMinTimecode = aTimecode;
	}

	if (!bMaxIsSet) {
		bMaxIsSet = true;
		mMaxTimecode = aTimecode;
	} else {
		if (mMaxTimecode < aTimecode)
			mMaxTimecode = aTimecode;
	}
}

bool SegmentInfo::IsTimecodeInside(uint64 aTimecode) const
{
	return (mMinTimecode <= aTimecode && aTimecode <= mMaxTimecode);
}

// ----------------------------------------------------------------------------

//     SegmentList class

SegmentInfo * SegmentList::AddSegment(KaxSegment & aSegment)
{
	SegmentInfo *tmp = new SegmentInfo(aSegment);
	if (tmp != NULL)
		mList.push_back(tmp);

	return tmp;
}

SegmentInfo * SegmentList::FindTimecode(uint64 aTimecode) const
{
	for (unsigned int i=0; i<mList.size(); i++)
	{
		if (mList[i]->IsTimecodeInside(aTimecode))
			return mList[i];
	}
	return NULL;
}

SegmentList::~SegmentList()
{
	while (mList.size())
	{
		SegmentInfo *tmp = mList.back();
		mList.pop_back();
		delete tmp;
	}
}

int MatroskaReader::IndexForTrack(uint16 TrackNum)
{
	for (UINT result=0; result<mTrackInfos.size(); result++)
	{
		if (uint16(mTrackInfos[result]->Number) == TrackNum)
			return result;
	}
	return -1;
}

