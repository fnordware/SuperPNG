//
// SuperPNG
//
// by Brendan Bolles
//

#ifndef __SuperPNG_H__
#define __SuperPNG_H__

#include "PIDefines.h"
#include "PIFormat.h"
#include "PIUtilities.h"

#include "SuperPNG_Terminology.h"


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
// compression strategy; see deflateInit2() below for details
#endif


typedef long A_long;
typedef char A_Boolean;
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
	
} SuperPNG_outData;


typedef struct Globals
{ // This is our structure that we use to pass globals between routines:

	short				*result;			// Must always be first in Globals.
	FormatRecord		*formatParamBlock;	// Must always be second in Globals.

	Ptr					pixelData;
	int32				rowBytes;
	
	SuperPNG_inData		in_options;
	SuperPNG_outData	options;
	
} Globals, *GPtr, **GHdl;				// *GPtr = global pointer; **GHdl = global handle
	
// The routines that are dispatched to from the jump list should all be
// defined as
//		void RoutineName (GPtr globals);
// And this typedef will be used as the type to create a jump list:
typedef void (* FProc)(GPtr globals);


//-------------------------------------------------------------------------------
//	Globals -- definitions and macros
//-------------------------------------------------------------------------------

#define gResult				(*(globals->result))
#define gStuff				(globals->formatParamBlock)

#define gPixelBuffer		(globals->pixelBuffer)
#define gPixelData			(globals->pixelData)
#define gRowBytes			(globals->rowBytes)

#define gInOptions			(globals->in_options)
#define gOptions			(globals->options)

//-------------------------------------------------------------------------------
//	Prototypes
//-------------------------------------------------------------------------------

#ifdef PS_CS4_SDK
typedef intptr_t entryData;
typedef void * allocateGlobalsPointer;
typedef intptr_t simpleProp;
#else
typedef long entryData;
typedef uint32 allocateGlobalsPointer;
typedef int32 simpleProp;
#endif

// Everything comes in and out of PluginMain. It must be first routine in source:
DLLExport MACPASCAL void PluginMain (const short selector,
					  	             FormatRecord *formatParamBlock,
						             entryData *data,
						             short *result);

// funcs living in other files
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


// During write phase:
Boolean ReadScriptParamsOnWrite (GPtr globals);	// Read any scripting params.
OSErr WriteScriptParamsOnWrite (GPtr globals);	// Write any scripting params.

//-------------------------------------------------------------------------------

#endif // __SuperPNG_H__
