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
	\version \$Id: codecs.h,v 1.2 2003/07/20 18:22:13 jcsston Exp $
	\author Jan-D. Schlenker <myfun @ users.corecodec.org>
	\author Christophe Paris <toffparis @ users.sf.net>
*/

#ifndef __CODECS_H__
#define __CODECS_H__

#include <mmreg.h>

struct ACM_CODEC {
	char			*name;
	unsigned short	id;			// if "zero" the GUID is used
	GUID			guid;
};

struct VFW_CODEC {
	char			*name;
	union {
		unsigned long	dwfcc;
		char			fcc[4];
	};
};

// Borgtech
//#define WAVE_FORMAT_AAC 0xAAC0

// 3ivx
#define WAVE_FORMAT_AAC 0x00FF

#define ACM_CODEC_LIST_LEN		6
#define VFW_CODEC_LIST_LEN		1
static const ACM_CODEC ACM_CODEC_LIST[ACM_CODEC_LIST_LEN] = {
	{"A_NULL",				0x0000,						{0, 0, 0, 0}},
	{"A_MPEG/L3",			WAVE_FORMAT_MPEGLAYER3,		{0, 0, 0, 0}},
	{"A_AC3",				0x2000,						{0, 0, 0, 0}},
	{"A_PCM/INT/LIT",		WAVE_FORMAT_PCM,			{0, 0, 0, 0}},
	{"A_PCM/FLOAT/IEEE",	WAVE_FORMAT_IEEE_FLOAT,		{0, 0, 0, 0}},
	{"A_DTS",				0x2001,						{0, 0, 0, 0}}
};

#define FOURCC_mp4v mmioFOURCC('m','p','4','v')
DEFINE_GUID(CLSID_mp4v, FOURCC_mp4v, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71);

/*
static const VFW_CODEC VFW_CODEC_LIST[] = {
	{"MPEG4", {'X','V','I','D'}}
};
*/

#endif __CODECS_H__
