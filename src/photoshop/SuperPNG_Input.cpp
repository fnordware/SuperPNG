
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


#include "SuperPNG.h"

#include "lcms2_internal.h"

#include <stdlib.h>
#include <math.h>

#ifdef __PIWin__
#include "PIDLLInstance.h"
#endif


extern SPBasicSuite		*sSPBasic;
extern SPPluginRef		gPlugInRef;


void SuperPNG_VerifyFile(GPtr globals)
{
	// Start at the top and read 4 bytes.
#define TEST_BYTES	4

	char buf[TEST_BYTES];

#ifdef __PIMac__
	gResult = FSSetForkPosition(gStuff->dataFork, fsFromStart, 0);

	ByteCount readCount = TEST_BYTES;
	
	gResult = FSReadFork(gStuff->dataFork, fsAtMark, 0, readCount, (void *)buf, &readCount);
#else // __PIWin__
	LARGE_INTEGER lpos;

	lpos.QuadPart = 0;

#if _MSC_VER < 1300
	DWORD pos = SetFilePointer((HANDLE)gStuff->dataFork, lpos.u.LowPart, &lpos.u.HighPart, FILE_BEGIN);

	BOOL result = (pos != 0xFFFFFFFF || NO_ERROR == GetLastError());
#else
	BOOL result = SetFilePointerEx((HANDLE)gStuff->dataFork, lpos, NULL, FILE_BEGIN);
#endif
	// read test bytes
	DWORD count = TEST_BYTES, readCount = 0;
	
	result = ReadFile((HANDLE)gStuff->dataFork, (LPVOID)buf, count, &readCount, NULL);
#endif
	
	if(gResult != noErr)
		return;
	
	// Check the identifier
	if(readCount != TEST_BYTES ||
		png_sig_cmp((png_bytep)buf, (png_size_t)0, (png_size_t)TEST_BYTES) != 0)
	{
		gResult = formatCannotRead;
	}
}


#pragma mark-


static void png_replace_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	FormatRecord *fmt_rec = (FormatRecord *)png_get_io_ptr(png_ptr);

#ifdef __PIMac__
	ByteCount readCount = length;
	
	OSErr result = FSReadFork(fmt_rec->dataFork, fsAtMark, 0, readCount, (void *)data, &readCount);
#else
	DWORD count = length, readCount = 0;
	
	BOOL result = ReadFile((HANDLE)fmt_rec->dataFork, (LPVOID)data, count, &readCount, NULL);
#endif
}


#pragma mark-

#define PF_MAX_CHAN16			32768

static inline unsigned16 Demote(unsigned16 val)
{
	return (val > PF_MAX_CHAN16 ? ( (val - 1) >> 1 ) + 1 : val >> 1);
}

template<typename T>
static inline T minimum(T a, T b)
{
	return (a < b ? a : b);
}


typedef struct {
	unsigned8	r;
	unsigned8	g;
	unsigned8	b;
	unsigned8	a;
} RGBApixel8;

static void Premultiply(RGBApixel8 *buf, int64 len)
{
	while(len--)
	{
		if(buf->a != 255)
		{	
			float mult = (float)buf->a / 255.f;
			
			buf->r = ((float)buf->r * mult) + 0.5f;
			buf->g = ((float)buf->g * mult) + 0.5f;
			buf->b = ((float)buf->b * mult) + 0.5f;
		}
		
		buf++;
	}
}

typedef struct {
	unsigned16	r;
	unsigned16	g;
	unsigned16	b;
	unsigned16	a;
} RGBApixel16;

static void Premultiply(RGBApixel16 *buf, int64 len)
{
	while(len--)
	{
		if(buf->a != PF_MAX_CHAN16)
		{	
			float mult = (float)buf->a / (float)PF_MAX_CHAN16;
			
			buf->r = ((float)buf->r * mult) + 0.5f;
			buf->g = ((float)buf->g * mult) + 0.5f;
			buf->b = ((float)buf->b * mult) + 0.5f;
		}
		
		buf++;
	}
}

typedef struct {
	unsigned8	y;
	unsigned8	a;
} YApixel8;

static void Premultiply(YApixel8 *buf, int64 len)
{
	while(len--)
	{
		if(buf->a != 255)
		{	
			float mult = (float)buf->a / 255.f;
			
			buf->y = ((float)buf->y * mult) + 0.5f;
		}
		
		buf++;
	}
}

typedef struct {
	unsigned16	y;
	unsigned16	a;
} YApixel16;

static void Premultiply(YApixel16 *buf, int64 len)
{
	while(len--)
	{
		if(buf->a != PF_MAX_CHAN16)
		{	
			float mult = (float)buf->a / (float)PF_MAX_CHAN16;
			
			buf->y = ((float)buf->y * mult) + 0.5f;
		}
		
		buf++;
	}
}

#pragma mark-

static inline double RoundDPI(double dpi)
{
	// round to the nearest 0.1
#ifdef __GNUC__
	return round(dpi * 10.0) / 10.0;
#else
	return floor((dpi * 10.0) + 0.5) / 10.0;		
#endif
}


// these stolen from an earlier version of Little CMS and private parts of the current version

static cmsBool
SetTextTags(cmsHPROFILE hProfile, const wchar_t* Description)
{
    cmsMLU *DescriptionMLU, *CopyrightMLU;
    cmsBool  rc = FALSE;
    cmsContext ContextID = cmsGetProfileContextID(hProfile);

    DescriptionMLU  = cmsMLUalloc(ContextID, 1);
    CopyrightMLU    = cmsMLUalloc(ContextID, 1);

    if (DescriptionMLU == NULL || CopyrightMLU == NULL) goto Error;

    if (!cmsMLUsetWide(DescriptionMLU,  "en", "US", Description)) goto Error;
    if (!cmsMLUsetWide(CopyrightMLU,    "en", "US", L"fnord software")) goto Error;

    if (!cmsWriteTag(hProfile, cmsSigProfileDescriptionTag,  DescriptionMLU)) goto Error;
    if (!cmsWriteTag(hProfile, cmsSigCopyrightTag,           CopyrightMLU)) goto Error;

    rc = TRUE;

Error:

    if (DescriptionMLU)
        cmsMLUfree(DescriptionMLU);
    if (CopyrightMLU)
        cmsMLUfree(CopyrightMLU);
    return rc;
}

static void ReadMetadataPre(GPtr globals, png_structp png_ptr, png_infop info_ptr)
{
	// ICC profiles
	if(gStuff->canUseICCProfiles)
	{
		png_charp icc_name = NULL;
		png_bytep icc_profile = NULL;
		png_uint_32 icc_proflen = 0;
		int icc_compression;	

		const bool is_color = (gStuff->imageMode == plugInModeRGBColor ||
								gStuff->imageMode == plugInModeRGB48 ||
								gStuff->imageMode == plugInModeIndexedColor);
			
		if(png_get_iCCP(png_ptr, info_ptr,
					&icc_name, &icc_compression, &icc_profile, &icc_proflen) )
		{
			gStuff->iCCprofileSize = icc_proflen;
			gStuff->iCCprofileData = myNewHandle(globals, icc_proflen);
			
			Ptr icc_ptr = myLockHandle(globals, gStuff->iCCprofileData);
			
			memcpy(icc_ptr, icc_profile, icc_proflen);
			
			myUnlockHandle(globals, gStuff->iCCprofileData);
		}
		else
		{
			// do we have other color space chunks?	 if so, build a profile.
			cmsHPROFILE iccH = NULL;
			
			int sRGB_intent;
			double png_gamma;

			if(png_get_sRGB(png_ptr, info_ptr, &sRGB_intent))
			{
				if(is_color)
				{
					iccH = cmsCreate_sRGBProfile();
					
					SetTextTags(iccH, L"PNG sRGB");
				}
				else
				{
					cmsCIExyY D65 = { 0.3127, 0.3290, 1.0 };

					cmsFloat64Number Parameters[5];
					Parameters[0] = 2.4;
					Parameters[1] = 1. / 1.055;
					Parameters[2] = 0.055 / 1.055;
					Parameters[3] = 1. / 12.92;
					Parameters[4] = 0.04045;	// d

					cmsToneCurve *gamma = cmsBuildParametricToneCurve(NULL, 4, Parameters);

					iccH = cmsCreateGrayProfile(&D65, gamma);
					
					SetTextTags(iccH, L"PNG sLUM");
					
					cmsFreeToneCurve(gamma);
				}
			}
			else if(png_get_gAMA(png_ptr, info_ptr, &png_gamma) )
			{
				// use sRGB for defaults
				cmsCIExyY white_point = { 0.3127, 0.3290, 1.0 };

				cmsCIExyYTRIPLE colorants = {
									   {0.6400, 0.3300, 1.0},
									   {0.3000, 0.6000, 1.0},
									   {0.1500, 0.0600, 1.0}
									   };
									   
				cmsToneCurve *gamma = cmsBuildGamma(NULL, 1.0 / png_gamma);
				
				// do we have color information too?
				double w_x, w_y, r_x, r_y,
						g_x, g_y, b_x, b_y;
			
				if( png_get_cHRM(png_ptr, info_ptr,
							&w_x, &w_y, &r_x, &r_y,
							&g_x, &g_y, &b_x, &b_y ) )
				{
					assert(is_color);
					
					cmsCIExyY white_point;
					cmsCIExyYTRIPLE colorants;
					
					// values from the file														   
					colorants.Red.x = r_x;		colorants.Red.y = r_y;
					colorants.Green.x = g_x;	colorants.Green.y = g_y;
					colorants.Blue.x = b_x;		colorants.Blue.y = b_y;
					white_point.x = w_x;		white_point.y = w_y;

					colorants.Red.Y = colorants.Green.Y = colorants.Blue.Y = white_point.Y = 1.0;
				}
				
				if(is_color)
				{
					cmsToneCurve *color_gamma[3] = {gamma, gamma, gamma};
					
					iccH = cmsCreateRGBProfile(&white_point, &colorants, color_gamma);
				}
				else
				{
					iccH = cmsCreateGrayProfile(&white_point, gamma);
				}
				
				if(iccH)
				{
					char prof_name[128];
					sprintf(prof_name, "gamma %1.1f (SuperPNG Fabricated Profile)", 1.0 / png_gamma);
					
					// poor-man's wchar copy
					wchar_t wprof_name[128];
					
					const char *c = prof_name;
					wchar_t *w = wprof_name;
					
					do{
						*w++ = *c;
						
					}while(*c++ != '\0');
					
					SetTextTags(iccH, wprof_name);
				}
				
				cmsFreeToneCurve(gamma);
			}
				
			if(iccH)
			{
				cmsUInt32Number profile_len = 0;
				
				if( cmsSaveProfileToMem(iccH, NULL, &profile_len) )
				{
					gStuff->iCCprofileSize = profile_len;
					gStuff->iCCprofileData = myNewHandle(globals, profile_len);
					
					Ptr icc_ptr = myLockHandle(globals, gStuff->iCCprofileData);
					
					cmsSaveProfileToMem(iccH, icc_ptr, &profile_len);
										
					myUnlockHandle(globals, gStuff->iCCprofileData);
				
					cmsCloseProfile(iccH);
				}
			}
		}
	}
	
	if( png_get_pixels_per_meter(png_ptr, info_ptr) )
	{
		// PNG uses pixels per meter
		const png_uint_32 ppm = png_get_pixels_per_meter(png_ptr, info_ptr);
		const png_uint_32 ppm_x = png_get_x_pixels_per_meter(png_ptr, info_ptr);
		const png_uint_32 ppm_y = png_get_y_pixels_per_meter(png_ptr, info_ptr);
		
		// Photoshop uses dots per inch
		// 1 inch = 25.4 mm (exactly)
		double dpi_x = (double)ppm_x * 0.0254;
		double dpi_y = (double)ppm_y * 0.0254;
		
		// round the DPI to the nearest 0.1
		// necessary because PNG's (int)pixels/meter doesn't have enough precision
		// to capture all of Photoshop's (Fixed)dots/inch
		// 72.0 DPI becomes 72.01 DPI
		dpi_x = RoundDPI(dpi_x);
		dpi_y = RoundDPI(dpi_y);
			
		gStuff->imageHRes = (dpi_x * 65536.0) + 0.5;
		gStuff->imageVRes = (dpi_y * 65536.0) + 0.5;
	}
}


	
static void ReadMetadataPost(GPtr globals, png_structp png_ptr, png_infop info_ptr)
{
	if(gStuff->propertyProcs)
	{
		png_textp text_blocks = NULL;
		int num_blocks = 0;
		
		if( png_get_text(png_ptr, info_ptr, &text_blocks, &num_blocks) )
		{
			for(int i=0; i < num_blocks; i++)
			{	
				const png_text &text_block = text_blocks[i];
				
				const png_size_t text_len = (text_block.compression >= PNG_ITXT_COMPRESSION_NONE ?
												text_block.itxt_length : text_block.text_length);
				
				if(!strcmp(text_block.key, "Copyright"))
				{
					PISetProp(kPhotoshopSignature, 'cpyr', 0, true, NULL);
				}
				else if(!strcmp(text_block.key, "URL") && text_len > 0 && text_block.text != NULL)
				{
					Handle url_handle;
					char *url_ptr;
					
					url_handle = myNewHandle(globals, text_len);
					url_ptr = (char *)myLockHandle(globals, url_handle);
					
					memcpy(url_ptr, text_block.text, text_len);
					
					myUnlockHandle(globals, url_handle);
					
					PISetProp(kPhotoshopSignature, 'URL ', 0, NULL, url_handle);
					
					//PIDisposeHandle(url_handle);
				}
				else if(!strcmp(text_block.key, "XML:com.adobe.xmp") && text_len > 0 && text_block.text != NULL)
				{
					Handle xmp_handle;
					char *xmp_ptr;
					
					xmp_handle = myNewHandle(globals, text_len);
					xmp_ptr = (char *)myLockHandle(globals, xmp_handle);
					
					memcpy(xmp_ptr, text_block.text, text_len);
					
					myUnlockHandle(globals, xmp_handle);
					
					PISetProp(kPhotoshopSignature, 'xmpd', 0, NULL, xmp_handle);
					
					//PIDisposeHandle(xmp_handle);
				}
			}
		}
	}
}

#pragma mark-

static void png_init_read(GPtr globals, png_structp *png_ptr, png_infop *info_ptr)
{
	// Set the file reading back to the beginning
#ifdef __PIMac__
	gResult = FSSetForkPosition(gStuff->dataFork, fsFromStart, 0);
#else
	// set pointer to beginning of file
	LARGE_INTEGER lpos;

	lpos.QuadPart = 0;

#if _MSC_VER < 1300
	DWORD pos = SetFilePointer((HANDLE)gStuff->dataFork, lpos.u.LowPart, &lpos.u.HighPart, FILE_BEGIN);

	BOOL result = (pos != 0xFFFFFFFF || NO_ERROR == GetLastError());
#else
	BOOL result = SetFilePointerEx((HANDLE)gStuff->dataFork, lpos, NULL, FILE_BEGIN);
#endif
#endif
	
	
	// Set up PNG pointers
	*png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		
	*info_ptr = png_create_info_struct(*png_ptr);
	
	
	png_set_read_fn(*png_ptr, (void *)gStuff, png_replace_read_data);
}



void SuperPNG_FileInfo(GPtr globals)
{
	png_structp png_ptr;
	png_infop info_ptr;


	// set up read struct
	png_init_read(globals, &png_ptr, &info_ptr);

	if(gResult != noErr)
		return;


	// this is how libpng handles errors
#ifdef PNG_SETJMP_SUPPORTED
	if(setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		
		gStuff->data = NULL;

#ifdef __PIMac__		
		gResult = errReportString;
		Str255 errString = "\p\rBrendan's an idiot";
		PIReportError(errString);
#else
		gResult = -1;
#endif

		return;
	}
#endif
	

	// Read PNG header
	
	png_read_info(png_ptr, info_ptr);
	
		
	int color_type, bit_depth, interlace_type;
	png_uint_32 width, height;
	
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		&interlace_type, NULL, NULL);


	if( bit_depth < 8 )
	{
		if( color_type & PNG_COLOR_MASK_PALETTE )
		{
			png_set_packing(png_ptr);
			//png_set_packswap(png_ptr);
			bit_depth = 8;
		}
		else if ( (bit_depth != 1) || (gStuff->hostSig == 'FXTC') )
		{
			png_set_expand_gray_1_2_4_to_8(png_ptr);
			bit_depth = 8;
		}
	}
	else if( bit_depth == 16 )
	{
	#ifndef __PIMacPPC__
		png_set_swap(png_ptr);
	#endif
	}
	
	if(width > SHRT_MAX || height > SHRT_MAX)
	{
		if(gStuff->HostSupports32BitCoordinates)
		{
			gStuff->PluginUsing32BitCoordinates = TRUE;
		}
		else
		{
			gResult = errReportString;
			Str255 errString = "\pPNG file too big";
			PIReportError(errString);
		}
	}
	
	gStuff->imageSize.h = gStuff->imageSize32.h = width;
	gStuff->imageSize.v = gStuff->imageSize32.v = height;
	
	gStuff->depth = bit_depth;
	
	
	// Translate from PNG color types to Photoshop image modes & planes
	
	if(color_type & PNG_COLOR_MASK_PALETTE)
	{
		int trans_counter = 0;
		
		png_bytep trans;
		int num_trans = 0;
		png_color_16p trans_values;
				
		if( png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &trans_values) )
		{
			for(int j=0; j < num_trans && trans_counter < 2; j++)
			{
				if(trans[j] != 255)
				{
					if(trans[j] == 0)
						trans_counter += 1;
					else
						trans_counter += 2; // so any partially transparent entries force RGBA
				}
			}
		}
		
		
		if( trans_counter > 1 || (gStuff->hostSig == 'FXTC') ) //!(gStuff->hostModes & plugInModeIndexedColor)
		{
			png_set_palette_to_rgb(png_ptr);
			
			gStuff->imageMode = plugInModeRGBColor;
			
			if(num_trans)
				gStuff->planes = 4;
			else
				gStuff->planes = 3;
		}
		else
		{
			png_colorp palette;
			int num_palette;
			int32 counter=0;
			
			gStuff->imageMode = plugInModeIndexedColor;
			gStuff->planes = 1;
			
			png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
			
			for(int i=0; i < num_palette; i++)
			{
				gStuff->redLUT[i]	= palette[i].red;
				gStuff->greenLUT[i] = palette[i].green;
				gStuff->blueLUT[i]	= palette[i].blue;
			}
			
			gStuff->lutCount = num_palette;
			
			if(num_trans)
			{
				for(int i=0, found_one=FALSE; i < num_trans && i < num_palette && !found_one; i++)
				{
					if(trans[i] != 255)
					{
						gStuff->transparentIndex = i;
						
						found_one = TRUE;
					}
				}
			}
		}
	}	 
	else if(color_type & PNG_COLOR_MASK_COLOR)
	{
		if( gStuff->depth == 16 )
		{
			gStuff->imageMode = plugInModeRGB48;
		}
		else
			gStuff->imageMode = plugInModeRGBColor;

		if( color_type & PNG_COLOR_MASK_ALPHA )
		{
			gStuff->planes = 4;
		}
		else if( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) )
		{
			gStuff->planes = 4;
			
			png_set_tRNS_to_alpha(png_ptr);
		}
		else		
			gStuff->planes = 3;
	}
	else
	{
		// greyscale
		if(gStuff->depth == 16)
		{
			gStuff->imageMode = plugInModeGray16;
		}
		else if(gStuff->depth == 1)
		{
			gStuff->imageMode = plugInModeBitmap;
		}
		else
			gStuff->imageMode = plugInModeGrayScale;
		
		
		if( color_type & PNG_COLOR_MASK_ALPHA )
		{
			gStuff->planes = 2;
		}
		else if( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) )
		{
			gStuff->planes = 2;
			
			png_set_tRNS_to_alpha(png_ptr);
		}
		else
			gStuff->planes = 1;
	}

	if(gInOptions.alpha == PNG_ALPHA_TRANSPARENCY && (gStuff->planes == 4 || gStuff->planes == 2))
	{
		gStuff->transparencyPlane = gStuff->planes - 1;
		gStuff->transparencyMatting = 0;
	}

	if(gStuff->hostSig != 'FXTC')
	{
		ReadMetadataPre(globals, png_ptr, info_ptr);
		ReadMetadataPost(globals, png_ptr, info_ptr);
	}
	
	
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}


void SuperPNG_ReadFile(GPtr globals)
{
	png_structp png_ptr;
	png_infop info_ptr;
	
	// set up read struct again
	png_init_read(globals, &png_ptr, &info_ptr);

	if(gResult != noErr)
		return;


	// this is how libpng handles errors
#ifdef PNG_SETJMP_SUPPORTED
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		
		gStuff->data = NULL;

#ifdef __PIMac__		
		gResult = errReportString;
		Str255 errString = "\p\rBrendan's an idiot";
		PIReportError(errString);
#else
		gResult = -1;
#endif

		return;
	}
#endif

	// read the header (again)
	png_read_info(png_ptr, info_ptr);
	
	
	int color_type, bit_depth, interlace_type;
	png_uint_32 width, height;

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		&interlace_type, NULL, NULL);
	
	// transformations
	if( bit_depth < 8 )
	{
		if( color_type & PNG_COLOR_MASK_PALETTE )
		{
			png_set_packing(png_ptr);
			//png_set_packswap(png_ptr);
		}
		else if ( (bit_depth != 1) || (gStuff->hostSig == 'FXTC') )
		{
			png_set_expand_gray_1_2_4_to_8(png_ptr);
		}
	}
	else if( bit_depth == 16 )
	{
	#ifndef __PIMacPPC__
		png_set_swap(png_ptr);
	#endif
	}
	
	
	if(gStuff->imageMode != plugInModeIndexedColor)
	{
		if(color_type & PNG_COLOR_MASK_PALETTE)
		{
			png_set_palette_to_rgb(png_ptr);
		}
		
		if(!(color_type & PNG_COLOR_MASK_ALPHA) && png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		{
			png_set_tRNS_to_alpha(png_ptr);
		}
	}
	
 
	// Set the rest of the parameters
	gStuff->planeBytes = (gStuff->depth == 16 ? 2 : 1);
	gStuff->colBytes = gStuff->planeBytes * gStuff->planes;
	gStuff->rowBytes = gRowBytes = gStuff->colBytes * width;
	
	if(gStuff->depth == 16)
		gStuff->maxValue = 0x8000;
	
	gStuff->loPlane = 0;
	gStuff->hiPlane = gStuff->planes - 1;
			
	gStuff->theRect.left = gStuff->theRect32.left = 0;
	gStuff->theRect.right = gStuff->theRect32.right = width;
	

	if(png_get_interlace_type(png_ptr, info_ptr) == PNG_INTERLACE_ADAM7)
	{
		// Load the whole image into RAM at once
		BufferID bufferID = 0;
		
		gResult = myAllocateBuffer(globals, gRowBytes * height, &bufferID);
		
		if(gResult == noErr && bufferID != 0)
		{
			gStuff->data = gPixelData = myLockBuffer(globals, bufferID, TRUE);
		
			// read the image
			png_bytepp row_pointers = (png_bytepp)png_malloc(png_ptr, height * sizeof(png_bytep));
			
			for(int row=0; row < height; row++)
				row_pointers[row] = (png_bytep)( (char *)gPixelData + (row * gRowBytes) );
			
			png_read_image(png_ptr, row_pointers);

			// demote if necessary
			if(gStuff->depth == 16)
			{
				int64 samples = (int64)width * (int64)height * (int64)gStuff->planes;
				
				unsigned16 *pix = (unsigned16 *)gPixelData;
				
				while(samples--)
				{
					*pix = Demote(*pix);
					pix++;
				}
			}
			
			// premultiply if necessary
			if(gInOptions.alpha == PNG_ALPHA_CHANNEL && gInOptions.mult == TRUE)
			{
				int64 pixels = (int64)width * (int64)height;
				
				if(gStuff->planes == 4)
				{
					if(gStuff->depth == 16)
						Premultiply((RGBApixel16 *)gPixelData, pixels);
					else if(gStuff->depth == 8)
						Premultiply((RGBApixel8 *)gPixelData, pixels);
				}
				else if(gStuff->planes == 2)
				{
					if(gStuff->depth == 16)
						Premultiply((YApixel16 *)gPixelData, pixels);
					else if(gStuff->depth == 8)
						Premultiply((YApixel8 *)gPixelData, pixels);
				}
			}
			
			// read into Photoshop
			gStuff->theRect.top = gStuff->theRect32.top = 0;
			gStuff->theRect.bottom = gStuff->theRect32.bottom = height;

			gResult = AdvanceState();
			
			
			// free memory
			png_free(png_ptr, (void *)row_pointers);	
			
			myFreeBuffer(globals, bufferID);
			
			gStuff->data = gPixelData = NULL;
		}
	}
	else
	{
		// load some reasonable number of scanlines at a time
		const int num_scanlines = (gStuff->tileHeight == 0 ? 256 : gStuff->tileHeight);
		
		BufferID bufferID = 0;
		
		gResult = myAllocateBuffer(globals, gRowBytes * num_scanlines, &bufferID);
		
		if(gResult == noErr && bufferID != 0)
		{
			gStuff->data = gPixelData = myLockBuffer(globals, bufferID, TRUE);
		
			png_bytepp row_pointers = (png_bytepp)png_malloc(png_ptr, num_scanlines * sizeof(png_bytep));
			
			for (int row=0; row < num_scanlines; row++)
				row_pointers[row] = (png_bytep)( (char *)gPixelData + (row * gRowBytes) );
		
		
			int y = 0;
			
			while(y < height && gResult == noErr)
			{
				const int high_scanline = minimum<int>(y + num_scanlines - 1, height - 1);
				const int block_height = 1 + high_scanline - y;
				
				
				// read image
				png_read_rows(png_ptr, row_pointers, NULL, block_height);


				// demote if necessary
				if(gStuff->depth == 16)
				{
					int64 samples = (int64)width * (int64)block_height * (int64)gStuff->planes;
					
					unsigned16 *pix = (unsigned16 *)gPixelData;
					
					while(samples--)
					{
						*pix = Demote(*pix);
						pix++;
					}
				}

				// premultiply if necessary
				if(gInOptions.alpha == PNG_ALPHA_CHANNEL && gInOptions.mult == TRUE)
				{
					int64 pixels = (int64)width * (int64)block_height;
					
					if(gStuff->planes == 4)
					{
						if(gStuff->depth == 16)
							Premultiply((RGBApixel16 *)gPixelData, pixels);
						else if(gStuff->depth == 8)
							Premultiply((RGBApixel8 *)gPixelData, pixels);
					}
					else if(gStuff->planes == 2)
					{
						if(gStuff->depth == 16)
							Premultiply((YApixel16 *)gPixelData, pixels);
						else if(gStuff->depth == 8)
							Premultiply((YApixel8 *)gPixelData, pixels);
					}
				}
				
				
				// read into Photoshop
				gStuff->theRect.top = gStuff->theRect32.top = y;
				gStuff->theRect.bottom = gStuff->theRect32.bottom = high_scanline + 1;

				gResult = AdvanceState();
				
				PIUpdateProgress(y, height);
				
				
				y = high_scanline + 1;
			}
			
			// free memory
			png_free(png_ptr, (void *)row_pointers);	
			
			myFreeBuffer(globals, bufferID);
			
			gStuff->data = gPixelData = NULL;
		}
	}
	
	
	png_read_end(png_ptr, info_ptr);
	
	ReadMetadataPost(globals, png_ptr, info_ptr);
	
	// clean up
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	// very important!
	gStuff->data = NULL;
}