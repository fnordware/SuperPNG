//////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2002-2014, Brendan Bolles
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// 
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////////


#ifndef __SuperPNG_H__
#define __SuperPNG_H__

#include "PIFormat.h"
#include "PIUtilities.h"


#include "png.h"


#ifndef ZLIB_H
// some useful macros from zlib.h
#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION  (-1)
// compression levels

#define Z_FILTERED            1
#define Z_HUFFMAN_ONLY        2
#define Z_RLE                 3
#define Z_FIXED               4
#define Z_DEFAULT_STRATEGY    0
// compression strategy
#endif


typedef int A_long;
typedef unsigned char A_Boolean;
typedef unsigned char A_u_char;

enum {
	PNG_ALPHA_NONE = 0,
	PNG_ALPHA_TRANSPARENCY,
	PNG_ALPHA_CHANNEL
};
typedef A_u_char PNG_Alpha;

typedef struct {
	PNG_Alpha	alpha;
	A_Boolean	mult;
	
} SuperPNG_inData;

typedef struct {
	A_long		compression;
	A_long		filter;
	A_long		strategy;
	A_Boolean	interlace;
	A_Boolean	metadata;
	PNG_Alpha	alpha;
	A_Boolean	clean_transparent;
	A_long		quant_quality;
	A_Boolean	pngquant;
	
} SuperPNG_outData;


typedef struct Globals
{
	short				*result;			// Must always be first in Globals.
	FormatRecord		*formatParamBlock;	// Must always be second in Globals.

	Ptr					pixelData;
	int32				rowBytes;
	
	SuperPNG_inData		in_options;
	SuperPNG_outData	options;
	
} Globals, *GPtr, **GHdl;


// The Photoshop SDK relies on this stuff

#define gResult				(*(globals->result))
#define gStuff				(globals->formatParamBlock)

#define gPixelBuffer		(globals->pixelBuffer)
#define gPixelData			(globals->pixelData)
#define gRowBytes			(globals->rowBytes)

#define gInOptions			(globals->in_options)
#define gOptions			(globals->options)


// functions living in other files
void SuperPNG_VerifyFile(GPtr globals);
void SuperPNG_FileInfo(GPtr globals);
void SuperPNG_ReadFile(GPtr globals);
void SuperPNG_WriteFile(GPtr globals);

// my backward compatible buffer and handle routines
Handle myNewHandle(GPtr globals, const int32 inSize);
Ptr myLockHandle(GPtr globals, Handle h);
void myUnlockHandle(GPtr globals, Handle h);
int32 myGetHandleSize(GPtr globals, Handle h);
void mySetHandleSize(GPtr globals, Handle h, const int32 inSize);
void myDisposeHandle(GPtr globals, Handle h);

OSErr myAllocateBuffer(GPtr globals, const int32 inSize, BufferID *outBufferID);
Ptr myLockBuffer(GPtr globals, const BufferID inBufferID, Boolean inMoveHigh);
void myFreeBuffer(GPtr globals, const BufferID inBufferID);


// scripting functions
Boolean ReadScriptParamsOnWrite (GPtr globals);	// Read any scripting params.
OSErr WriteScriptParamsOnWrite (GPtr globals);	// Write any scripting params.


// Our entry point, the one function exported
DLLExport MACPASCAL void PluginMain (const short selector,
					  	             FormatRecord *formatParamBlock,
						             intptr_t *data,
						             short *result);

#endif // __SuperPNG_H__
