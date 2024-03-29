#ifndef _SUBTITLES_GUID_H_
#define _SUBTITLES_GUID_H_

#pragma once

// {1AC0BEBD-4D2B-45ad-BCEB-F2C41C5E3788}
DEFINE_GUID(MEDIASUBTYPE_Matroska, 
			0x1ac0bebd, 0x4d2b, 0x45ad, 0xbc, 0xeb, 0xf2, 0xc4, 0x1c, 0x5e, 0x37, 0x88);

// {E487EB08-6B26-4be9-9DD3-993434D313FD}
DEFINE_GUID(MEDIATYPE_Subtitle, 
			0xe487eb08, 0x6b26, 0x4be9, 0x9d, 0xd3, 0x99, 0x34, 0x34, 0xd3, 0x13, 0xfd);

// {87C0B230-03A8-4fdf-8010-B27A5848200D}
DEFINE_GUID(MEDIASUBTYPE_UTF8, 
			0x87c0b230, 0x3a8, 0x4fdf, 0x80, 0x10, 0xb2, 0x7a, 0x58, 0x48, 0x20, 0xd);

// {3020560F-255A-4ddc-806E-6C5CC6DCD70A}
DEFINE_GUID(MEDIASUBTYPE_SSA, 
			0x3020560f, 0x255a, 0x4ddc, 0x80, 0x6e, 0x6c, 0x5c, 0xc6, 0xdc, 0xd7, 0xa);

// {326444F7-686F-47ff-A4B2-C8C96307B4C2}
DEFINE_GUID(MEDIASUBTYPE_ASS, 
			0x326444f7, 0x686f, 0x47ff, 0xa4, 0xb2, 0xc8, 0xc9, 0x63, 0x7, 0xb4, 0xc2);

// {B753B29A-0A96-45be-985F-68351D9CAB90}
DEFINE_GUID(MEDIASUBTYPE_USF, 
			0xb753b29a, 0xa96, 0x45be, 0x98, 0x5f, 0x68, 0x35, 0x1d, 0x9c, 0xab, 0x90);

// {A33D2F7D-96BC-4337-B23B-A8B9FBC295E9}
DEFINE_GUID(FORMAT_SubtitleInfo, 
			0xa33d2f7d, 0x96bc, 0x4337, 0xb2, 0x3b, 0xa8, 0xb9, 0xfb, 0xc2, 0x95, 0xe9);

#pragma pack(push, 1)
typedef struct {
	DWORD dwOffset;	
	CHAR IsoLang[4]; // three letter lang code + terminating zero
} SUBTITLEINFO;
#pragma pack(pop)

// SUBTITLEINFO structure content starting at dwOffset (also the content of CodecPrivate)
// --------------------------------------------------------------------------------------
//
// Here the text should start with the Byte Order Mark, even though 
// UTF-8 is prefered, it also helps identifying the encoding type.
//
// MEDIASUBTYPE_USF: 
//
// <?xml version="1.0" encoding="UTF-8"?>
// <!-- DOCTYPE USFSubtitles SYSTEM "USFV100.dtd" -->
// <?xml-stylesheet type="text/xsl" href="USFV100.xsl"?>
// 
// <USFSubtitles version="1.0">
// ... every element excluding <subtitles></subtitles> ...
// </USFSubtitles>
//
// MEDIASUBTYPE_SSA/ASS:
//
// The file header and all sub-sections except [Events]
//

// Data description of the media samples (everything is UTF-8 encoded here)
// ------------------------------------------------------------------------
//
// MEDIASUBTYPE_USF:
//
// The text _inside_ the <subtitle>..</subtitle> element. 
//
// Since timing is set on the sample, there is no need to put
// <subtitle start=".." stop=".." duration=".."> into the data.
//
// MEDIASUBTYPE_SSA/ASS:
//
// Comma separated values similar to the "Dialogue: ..." line with these fields:
// ReadOrder, Layer, Style, Name, MarginL, MarginR, MarginV, Effect, Text
//
// With the exception of ReadOrder every field can be found in ASS files. The
// ReadOrder field is needed for the decoder to be able to reorder the streamed 
// samples as they were placed originally in the file.
//
// If the source is only SSA, the Layer field can be left empty.
//

// Matroska CodecID mappings
// ------------------------
//
// S_TEXT/ASCII	<->	MEDIATYPE_Text		MEDIASUBTYPE_NULL	FORMAT_None
// S_TEXT/UTF8	<->	MEDIATYPE_Subtitle	MEDIASUBTYPE_UTF8	FORMAT_SubtitleInfo
// S_SSA		<->	MEDIATYPE_Subtitle	MEDIASUBTYPE_SSA	FORMAT_SubtitleInfo
// S_ASS		<->	MEDIATYPE_Subtitle	MEDIASUBTYPE_ASS	FORMAT_SubtitleInfo
// S_USF		<->	MEDIATYPE_Subtitle	MEDIASUBTYPE_USF	FORMAT_SubtitleInfo
//

#endif // _SUBTITLES_GUID_H_
