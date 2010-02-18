// ----------------------------------------------------------------------------
#ifndef _COREVORBIS_H_
#define _COREVORBIS_H_
// ----------------------------------------------------------------------------


// {0835DC4B-AA01-48c3-A42D-FD62C530A3E1}
DEFINE_GUID(CLSID_CoreVorbisDecoder, 
			0x835dc4b, 0xaa01, 0x48c3, 0xa4, 0x2d, 0xfd, 0x62, 0xc5, 0x30, 0xa3, 0xe1);

// {8D2FD10B-5841-4a6b-8905-588FEC1ADED9}
DEFINE_GUID(MEDIASUBTYPE_Vorbis2, 
			0x8d2fd10b, 0x5841, 0x4a6b, 0x89, 0x5, 0x58, 0x8f, 0xec, 0x1a, 0xde, 0xd9);

// {B36E107F-A938-4387-93C7-55E966757473}
DEFINE_GUID(FORMAT_VorbisFormat2, 
			0xb36e107f, 0xa938, 0x4387, 0x93, 0xc7, 0x55, 0xe9, 0x66, 0x75, 0x74, 0x73);

typedef struct tagVORBISFORMAT2
{
	DWORD Channels;
	DWORD SamplesPerSec;
	DWORD BitsPerSample;	
	DWORD HeaderSize[3]; // 0: Identification, 1: Comment, 2: Setup
} VORBISFORMAT2, *PVORBISFORMAT2, FAR *LPVORBISFORMAT2;


// ----------------------------------------------------------------------------
#endif // _COREVORBIS_H_
// ----------------------------------------------------------------------------