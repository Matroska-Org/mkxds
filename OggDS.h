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
	\version \$Id: OggDS.h,v 1.2 2003/07/20 18:22:13 jcsston Exp $
	\author Jan-D. Schlenker <myfun @ users.corecodec.org>
	\author Christophe Paris <toffparis @ users.sf.net>
*/

///< \todo this file is from OggDS, change the copyright of this file to T.W.

#ifndef _OggDS_h_
#define _OggDS_h_

// f07e245f-5a1f-4d1e-8bff-dc31d84a55ab
DEFINE_GUID(CLSID_OggSplitter, 0xf07e245f, 0x5a1f, 0x4d1e, 0x8b, 0xff, 0xdc, 0x31, 0xd8, 0x4a, 0x55, 0xab);

// {078C3DAA-9E58-4d42-9E1C-7C8EE79539C5}
DEFINE_GUID(CLSID_OggSplitPropPage, 0x78c3daa, 0x9e58, 0x4d42, 0x9e, 0x1c, 0x7c, 0x8e, 0xe7, 0x95, 0x39, 0xc5);

// 8cae96b7-85b1-4605-b23c-17ff5262b296 
DEFINE_GUID(CLSID_OggMux, 0x8cae96b7, 0x85b1, 0x4605, 0xb2, 0x3c, 0x17, 0xff, 0x52, 0x62, 0xb2, 0x96);

// {AB97AFC3-D08E-4e2d-98E0-AEE6D4634BA4}
DEFINE_GUID(CLSID_OggMuxPropPage, 0xab97afc3, 0xd08e, 0x4e2d, 0x98, 0xe0, 0xae, 0xe6, 0xd4, 0x63, 0x4b, 0xa4);

// {889EF574-0656-4B52-9091-072E52BB1B80}
DEFINE_GUID(CLSID_VorbisEnc, 0x889ef574, 0x0656, 0x4b52, 0x90, 0x91, 0x07, 0x2e, 0x52, 0xbb, 0x1b, 0x80);

// {c5379125-fd36-4277-a7cd-fab469ef3a2f}
DEFINE_GUID(CLSID_VorbisEncPropPage, 0xc5379125, 0xfd36, 0x4277, 0xa7, 0xcd, 0xfa, 0xb4, 0x69, 0xef, 0x3a, 0x2f);

// 02391f44-2767-4e6a-a484-9b47b506f3a4
DEFINE_GUID(CLSID_VorbisDec, 0x02391f44, 0x2767, 0x4e6a, 0xa4, 0x84, 0x9b, 0x47, 0xb5, 0x06, 0xf3, 0xa4);

// 77983549-ffda-4a88-b48f-b924e8d1f01c
DEFINE_GUID(CLSID_OggDSAboutPage, 0x77983549, 0xffda, 0x4a88, 0xb4, 0x8f, 0xb9, 0x24, 0xe8, 0xd1, 0xf0, 0x1c);

// {D2855FA9-61A7-4db0-B979-71F297C17A04}
DEFINE_GUID(MEDIASUBTYPE_Ogg,0xd2855fa9, 0x61a7, 0x4db0, 0xb9, 0x79, 0x71, 0xf2, 0x97, 0xc1, 0x7a, 0x4);

// cddca2d5-6d75-4f98-840e-737bedd5c63b
DEFINE_GUID(MEDIASUBTYPE_Vorbis, 0xcddca2d5, 0x6d75, 0x4f98, 0x84, 0x0e, 0x73, 0x7b, 0xed, 0xd5, 0xc6, 0x3b);

// 6bddfa7e-9f22-46a9-ab5e-884eff294d9f
DEFINE_GUID(FORMAT_VorbisFormat, 0x6bddfa7e, 0x9f22, 0x46a9, 0xab, 0x5e, 0x88, 0x4e, 0xff, 0x29, 0x4d, 0x9f);

typedef struct tagVORBISFORMAT
{
	WORD nChannels;
	DWORD nSamplesPerSec;
	DWORD nMinBitsPerSec;
	DWORD nAvgBitsPerSec;
	DWORD nMaxBitsPerSec;
	float fQuality;
} VORBISFORMAT, *PVORBISFORMAT, FAR *LPVORBISFORMAT;

#endif _OggDS_h_
