
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

#include "libimagequant.h"

#include "lcms2_internal.h"

#include <stdlib.h>



static void png_replace_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    FormatRecord *fmt_rec = static_cast<FormatRecord *>(png_get_io_ptr(png_ptr));
   
#ifdef __PIMac__
	ByteCount writeCount = length;

	OSErr result = FSWriteFork(fmt_rec->dataFork, fsAtMark, 0, writeCount, (const void *)data, &writeCount);  
#else
	DWORD count = length, out = 0;
	
	BOOL result = WriteFile((HANDLE)fmt_rec->dataFork, (LPVOID)data, count, &out, NULL);	
#endif
}


// Replacement flush function also requred. This one does nothing

static void png_replace_flush(png_structp png_ptr)
{
	png_structp *pppng = &png_ptr;
}

#pragma mark-

#define PF_HALF_CHAN16			16384

static inline unsigned16 Promote(unsigned16 val)
{
	return (val > PF_HALF_CHAN16 ? ( (val - 1) << 1 ) + 1 : val << 1);
}

static inline unsigned16 ByteSwap(unsigned16 val)
{
#ifdef __PIMacPPC__
	return val;
#else
	return ((val >> 8) | (val << 8));
#endif
}

template<typename T>
static inline T minimum(T a, T b)
{
	return (a < b ? a : b);
}


#pragma mark-

static Boolean sRGBtest(cmsHPROFILE iccH)
{
	char name[256];
	cmsUInt32Number namelen = cmsGetProfileInfoASCII(iccH, cmsInfoDescription,
													"en", cmsNoCountry, name, 255);

	return (!strcmp(name, "sRGB IEC61966-2.1") ||
			!strcmp(name, "PNG sRGB") ||
			!strcmp(name, "PNG sLUM") );
}


// these stolen from an earlier version of Little CMS and private parts of the current version

static cmsBool
cmsReadMediaWhitePoint(cmsCIEXYZ* Dest, cmsHPROFILE hProfile)
{
    cmsCIEXYZ* Tag;

    _cmsAssert(Dest != NULL);

    Tag = (cmsCIEXYZ*) cmsReadTag(hProfile, cmsSigMediaWhitePointTag);

    // If no wp, take D50
    if (Tag == NULL) {
        *Dest = *cmsD50_XYZ();
        return TRUE;
    }

    // V2 display profiles should give D50
    if (cmsGetEncodedICCversion(hProfile) < 0x4000000) {

        if (cmsGetDeviceClass(hProfile) == cmsSigDisplayClass) {
            *Dest = *cmsD50_XYZ();
            return TRUE;
        }
    }

    // All seems ok
    *Dest = *Tag;
    return TRUE;
}


static int
ReadICCXYZ(cmsHPROFILE hProfile, cmsTagSignature sig, cmsCIEXYZ *Value, cmsBool lIsFatal)
{
	cmsCIEXYZ *Tag = (cmsCIEXYZ*)cmsReadTag(hProfile, sig);
	
	if(Tag)
	{
		*Value = *Tag;
		return TRUE;
	}
	else
		return -1;
}


static cmsBool
cmsTakeColorants(cmsCIEXYZTRIPLE *Dest, cmsHPROFILE hProfile)
{
	if (ReadICCXYZ(hProfile, cmsSigRedColorantTag, &Dest -> Red, TRUE) < 0) return FALSE;
	if (ReadICCXYZ(hProfile, cmsSigGreenColorantTag, &Dest -> Green, TRUE) < 0) return FALSE;
	if (ReadICCXYZ(hProfile, cmsSigBlueColorantTag, &Dest -> Blue, TRUE) < 0) return FALSE;

	return TRUE;
}


static
cmsBool ComputeChromaticAdaptation(cmsMAT3* Conversion,
                                const cmsCIEXYZ* SourceWhitePoint,
                                const cmsCIEXYZ* DestWhitePoint,
                                const cmsMAT3* Chad)
{

    cmsMAT3 Chad_Inv;
    cmsVEC3 ConeSourceXYZ, ConeSourceRGB;
    cmsVEC3 ConeDestXYZ, ConeDestRGB;
    cmsMAT3 Cone, Tmp;


    Tmp = *Chad;
    if (!_cmsMAT3inverse(&Tmp, &Chad_Inv)) return FALSE;

    _cmsVEC3init(&ConeSourceXYZ, SourceWhitePoint -> X,
                             SourceWhitePoint -> Y,
                             SourceWhitePoint -> Z);

    _cmsVEC3init(&ConeDestXYZ,   DestWhitePoint -> X,
                             DestWhitePoint -> Y,
                             DestWhitePoint -> Z);

    _cmsMAT3eval(&ConeSourceRGB, Chad, &ConeSourceXYZ);
    _cmsMAT3eval(&ConeDestRGB,   Chad, &ConeDestXYZ);

    // Build matrix
    _cmsVEC3init(&Cone.v[0], ConeDestRGB.n[0]/ConeSourceRGB.n[0],    0.0,  0.0);
    _cmsVEC3init(&Cone.v[1], 0.0,   ConeDestRGB.n[1]/ConeSourceRGB.n[1],   0.0);
    _cmsVEC3init(&Cone.v[2], 0.0,   0.0,   ConeDestRGB.n[2]/ConeSourceRGB.n[2]);


    // Normalize
    _cmsMAT3per(&Tmp, &Cone, Chad);
    _cmsMAT3per(Conversion, &Chad_Inv, &Tmp);

    return TRUE;
}


static cmsBool
cmsAdaptationMatrix(cmsMAT3* r, const cmsMAT3* ConeMatrix, const cmsCIEXYZ* FromIll, const cmsCIEXYZ* ToIll)
{
    cmsMAT3 LamRigg   = {{ // Bradford matrix
        {{  0.8951,  0.2664, -0.1614 }},
        {{ -0.7502,  1.7135,  0.0367 }},
        {{  0.0389, -0.0685,  1.0296 }}
    }};

    if (ConeMatrix == NULL)
        ConeMatrix = &LamRigg;

    return ComputeChromaticAdaptation(r, FromIll, ToIll, ConeMatrix);
}


static cmsBool
cmsAdaptMatrixFromD50(cmsMAT3 *r, cmsCIExyY *DestWhitePt)
{
        cmsCIEXYZ Dn;       
        cmsMAT3 Bradford;
        cmsMAT3 Tmp;

        cmsxyY2XYZ(&Dn, DestWhitePt);
        
        cmsAdaptationMatrix(&Bradford, NULL, cmsD50_XYZ(), &Dn);

        Tmp = *r;
        _cmsMAT3per(r, &Bradford, &Tmp);

        return TRUE;
}



static void WriteMetadata(GPtr globals, png_structp png_ptr, png_infop info_ptr)
{
	png_time pngTime;

#ifdef __PIMac__	
	// Write tIME chunk using Mac OS time structure (no time zone)
	time_t the_time = time(NULL);
	tm *local_time = localtime(&the_time);
	
	pngTime.year   = local_time->tm_year + 1900;
	pngTime.month  = local_time->tm_mon + 1;
	pngTime.day    = local_time->tm_mday;
	pngTime.hour   = local_time->tm_hour;
	pngTime.minute = local_time->tm_min;
	pngTime.second = local_time->tm_sec;
#else
	// Win time stuff
	SYSTEMTIME	dateTime;
	GetSystemTime(&dateTime);
	
	pngTime.year   = dateTime.wYear;
	pngTime.month  = dateTime.wMonth;
	pngTime.day    = dateTime.wDay;
	pngTime.hour   = dateTime.wHour;
	pngTime.minute = dateTime.wMinute;
	pngTime.second = dateTime.wSecond;
#endif
	
	png_set_tIME(png_ptr, info_ptr, &pngTime);

	
	if( (gStuff->hostSig != 'FXTC') && gStuff->imageHRes )
	{
		// pixels per inch
		const double dpi_x = (double)gStuff->imageHRes / 65536.0;
		const double dpi_y = (double)gStuff->imageVRes / 65536.0;
		
		// PNG uses pixels per meter
		// an inch is defined as exactly 25.4 mm
		const double ppm_x = dpi_x / 0.0254;
		const double ppm_y = dpi_y / 0.0254;
		
		// rounding
		const png_uint_32 i_ppm_x = ppm_x + 0.5;
		const png_uint_32 i_ppm_y = ppm_y + 0.5;
	
		png_set_pHYs(png_ptr, info_ptr, i_ppm_x, i_ppm_y, PNG_RESOLUTION_METER);
	}
	
	
	// Write ICC Profile - this will only happen if the user checks the box in Save As
	if( gStuff->canUseICCProfiles && (gStuff->iCCprofileSize > 0) && (gStuff->iCCprofileData != NULL) )
	{
		const Ptr icc = myLockHandle(globals, gStuff->iCCprofileData);
	
		if(icc)
		{
			cmsHPROFILE iccH = cmsOpenProfileFromMem(icc, gStuff->iCCprofileSize);
			
			if(iccH)
			{
				if( sRGBtest(iccH) )
				{
					// just setting the chunk if we have sRGB
					png_set_sRGB(png_ptr, info_ptr, PNG_sRGB_INTENT_PERCEPTUAL);
				}
				else
				{
					// store the profile, even if it's one we generated
					png_set_iCCP(png_ptr, info_ptr, "Photoshop ICC Profile",
								PNG_COMPRESSION_TYPE_BASE,
								(png_const_bytep)icc, gStuff->iCCprofileSize);
								
					// also store Gamma and Chrom information if possible
					cmsToneCurve *gammaTable = NULL;
					
					if( cmsIsTag(iccH, cmsSigGrayTRCTag) )
					{
						gammaTable = (cmsToneCurve *)cmsReadTag(iccH, cmsSigGrayTRCTag);
					}
					else if( cmsIsTag(iccH, cmsSigGreenTRCTag) )
					{
						gammaTable = (cmsToneCurve *)cmsReadTag(iccH, cmsSigGreenTRCTag);
					}
					
					if(gammaTable)
					{
						double gamma_est = cmsEstimateGamma(gammaTable, 0.1);
						
						if(gamma_est > 0.0)
						{
							png_set_gAMA(png_ptr, info_ptr, 1.0 / gamma_est);
							
							Boolean is_color = (gStuff->imageMode == plugInModeRGBColor ||
												gStuff->imageMode == plugInModeRGB48 ||
												gStuff->imageMode == plugInModeIndexedColor);

							if(is_color)
							{
								cmsCIEXYZ white_point;
								cmsCIEXYZTRIPLE colorants;

								if( cmsReadMediaWhitePoint(&white_point, iccH) && cmsTakeColorants(&colorants, iccH) )
								{
									cmsCIExyYTRIPLE xyColorants;
									cmsCIExyY xyWhitePoint;
									
									// make a big matrix so we can run cmsAdaptMatrixFromD50() on it
									// before grabbing the chromaticities 
									cmsMAT3 MColorants;
									
									MColorants.v[0].n[0] = colorants.Red.X;
									MColorants.v[1].n[0] = colorants.Red.Y;
									MColorants.v[2].n[0] = colorants.Red.Z;
									
									MColorants.v[0].n[1] = colorants.Green.X;
									MColorants.v[1].n[1] = colorants.Green.Y;
									MColorants.v[2].n[1] = colorants.Green.Z;
									
									MColorants.v[0].n[2] = colorants.Blue.X;
									MColorants.v[1].n[2] = colorants.Blue.Y;
									MColorants.v[2].n[2] = colorants.Blue.Z;
									
								
									cmsXYZ2xyY(&xyWhitePoint, &white_point);
									
									// apparently I have to do this to get the right YXZ values
									// for my chromaticities - anyone know why?
									cmsAdaptMatrixFromD50(&MColorants, &xyWhitePoint);
									
									// set the colrants back and convert to xyY
									colorants.Red.X = MColorants.v[0].n[0];
									colorants.Red.Y = MColorants.v[1].n[0];
									colorants.Red.Z = MColorants.v[2].n[0];

									colorants.Green.X = MColorants.v[0].n[1];
									colorants.Green.Y = MColorants.v[1].n[1];
									colorants.Green.Z = MColorants.v[2].n[1];

									colorants.Blue.X = MColorants.v[0].n[2];
									colorants.Blue.Y = MColorants.v[1].n[2];
									colorants.Blue.Z = MColorants.v[2].n[2];
									
									cmsXYZ2xyY(&xyColorants.Red, &colorants.Red);
									cmsXYZ2xyY(&xyColorants.Green, &colorants.Green);
									cmsXYZ2xyY(&xyColorants.Blue, &colorants.Blue);
									
									png_set_cHRM(png_ptr, info_ptr,
												xyWhitePoint.x, xyWhitePoint.y,
												xyColorants.Red.x, xyColorants.Red.y,
												xyColorants.Green.x, xyColorants.Green.y,
												xyColorants.Blue.x, xyColorants.Blue.y	);
								}
							}
						}
					}
				}
				
				cmsCloseProfile(iccH);
			}
			
			myUnlockHandle(globals, gStuff->iCCprofileData);
		}
	}

	if(gStuff->propertyProcs)
	{
		int32 handleSize[6] = {0,0,0,0,0,0}; // Max of 6 items for now
		intptr_t simpleProperty[6] = {0,0,0,0,0,0};
		Handle complexProperty[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
		png_text meta_text[7];
		int meta_index = 0;
				
		if(gStuff->hostSig == '8BIM')
		{
			// tEXt : copyright info
			PIGetProp(kPhotoshopSignature, 'cpyr', 0, &simpleProperty[meta_index], &complexProperty[meta_index]);
			
			if( simpleProperty[meta_index] )
			{
				meta_text[meta_index].key			=	"Copyright";
				meta_text[meta_index].compression	=	PNG_TEXT_COMPRESSION_NONE;
				meta_text[meta_index].text			=	"This image copyrighted.";
				
				meta_index++;
			}

			// tEXt : URL
			PIGetProp(kPhotoshopSignature, 'URL ', 0, &simpleProperty[meta_index], &complexProperty[meta_index]);
			
			if( (handleSize[meta_index] = myGetHandleSize(globals, complexProperty[meta_index])) )
			{	
				mySetHandleSize(globals, complexProperty[meta_index], handleSize[meta_index] + 1);
				png_charp the_text = (png_charp)myLockHandle(globals, complexProperty[meta_index]);
				the_text[handleSize[meta_index]] = '\0';
			
				meta_text[meta_index].key			=	"URL";
				meta_text[meta_index].compression	=	PNG_TEXT_COMPRESSION_NONE;
				meta_text[meta_index].text			=	the_text;
				
				meta_index++;
			}

			// tEXt : File Name
			PIGetProp(kPhotoshopSignature, 'titl', 0, &simpleProperty[meta_index], &complexProperty[meta_index]);
			
			if( (handleSize[meta_index] = myGetHandleSize(globals, complexProperty[meta_index])) )
			{	
				PISetHandleSize(complexProperty[meta_index], handleSize[meta_index] + 1);
				png_charp the_text = (png_charp)myLockHandle(globals, complexProperty[meta_index]);
				the_text[handleSize[meta_index]] = '\0';
			
				meta_text[meta_index].key			=	"File Name";
				meta_text[meta_index].compression	=	PNG_TEXT_COMPRESSION_NONE;
				meta_text[meta_index].text			=	the_text;
				
				meta_index++;
			}

			// iTXt : XMP
			PIGetProp(kPhotoshopSignature, 'xmpd', 0, &simpleProperty[meta_index], &complexProperty[meta_index]);
			
			if( (handleSize[meta_index] = myGetHandleSize(globals, complexProperty[meta_index])) )
			{	
				PISetHandleSize(complexProperty[meta_index], handleSize[meta_index] + 1);
				png_charp the_text = (png_charp)myLockHandle(globals, complexProperty[meta_index]);
				the_text[handleSize[meta_index]] = '\0';
			
				meta_text[meta_index].key			=	"XML:com.adobe.xmp";
				meta_text[meta_index].compression	=	PNG_ITXT_COMPRESSION_NONE;
				meta_text[meta_index].text			=	the_text;
				meta_text[meta_index].lang			=	NULL;
				meta_text[meta_index].lang_key		=	NULL;
				
				meta_index++;
			}
		
		}
		
		// tEXt : Software
		if(gStuff->hostSig == '8BIM' || gStuff->hostSig == 'FXTC')
		{
				meta_text[meta_index].key			=	"Software";
				meta_text[meta_index].compression	=	PNG_TEXT_COMPRESSION_NONE;
				if(gStuff->hostSig == '8BIM')
					meta_text[meta_index].text		=	"Adobe Photoshop";
				else if(gStuff->hostSig == 'FXTC')
					meta_text[meta_index].text		=	"Adobe After Effects";
					
				meta_index++;
		}
		
		// tEXt : Writer
		meta_text[meta_index].key			=	"Writer";
		meta_text[meta_index].compression	=	PNG_TEXT_COMPRESSION_NONE;
		meta_text[meta_index].text			=	"SuperPNG";
		
		meta_index++;


		if(meta_index)
		{
			int i=0;
			
			png_set_text(png_ptr, info_ptr, meta_text, meta_index);
			
			for(i=0;i<meta_index;i++)
			{
				if(complexProperty[i] != NULL)
				{
					myDisposeHandle(globals, complexProperty[i]); // p57 says I should dispose
					
					complexProperty[i] = NULL;
				}
			}
		}
	}
}

#pragma mark-

static void myHandle2PstrCpy(GPtr globals, Handle textHndl, Str255 p_string)
{
	unsigned long len = myGetHandleSize(globals, textHndl);
	Ptr textPtr = myLockHandle(globals, textHndl);
	
	int i;
	
	p_string[0] = len;
	
	for(i=0;i<len, i<255;i++)
		p_string[i+1] = textPtr[i];
	
	myUnlockHandle(globals, textHndl);
}

static void png_init_write(GPtr globals, png_structp *png_ptr, png_infop *info_ptr)
{
	*png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		
	*info_ptr = png_create_info_struct(*png_ptr);
	
	
	// Change write function from fwrite() to PS-friendly wrapper function.
	png_set_write_fn(*png_ptr, (void *)gStuff, png_replace_write_data, png_replace_flush);
}


template <typename T>
struct RGBApixel {
	T r;
	T g;
	T b;
	T a;
};

typedef RGBApixel<unsigned8> RGBApixel8;
typedef RGBApixel<unsigned16> RGBApixel16;


template <typename T>
struct YApixel {
	T y;
	T a;
};

typedef YApixel<unsigned8> YApixel8;
typedef YApixel<unsigned16> YApixel16;


template <typename T>
static void CleanRGBA(T *pix, int64 len)
{
	while(len--)
	{
		if(pix->a == 0)
		{
			pix->r = pix->g = pix->b = 0;
		}
		
		pix++;
	}
}


template <typename T>
static void CleanYA(T *pix, int64 len)
{
	while(len--)
	{
		if(pix->a == 0)
		{
			pix->y = 0;
		}
		
		pix++;
	}
}


static void CleanTransparent(GPtr globals, int num_channels, int64 len)
{
	if(num_channels == 4)
	{
		if(gStuff->depth == 16)
			CleanRGBA((RGBApixel16 *)gPixelData, len);
		else if(gStuff->depth == 8)
			CleanRGBA((RGBApixel8 *)gPixelData, len);
	}
	else if(num_channels == 2)
	{
		if(gStuff->depth == 16)
			CleanYA((YApixel16 *)gPixelData, len);
		else if(gStuff->depth == 8)
			CleanYA((YApixel8 *)gPixelData, len);
	}
}


typedef struct {
	unsigned8 *buf;
	size_t rowbytes;
} RGBbuf;

static void rgb2rgba_callback(liq_color row_out[], int row, int width, void* user_info)
{
	RGBbuf *rgb = (RGBbuf *)user_info;
	
	unsigned8 *in_pix = rgb->buf + (rgb->rowbytes * row);
	
	liq_color *out_pix = row_out;
	
	while(width--)
	{
		out_pix->r = *in_pix++;
		out_pix->g = *in_pix++;
		out_pix->b = *in_pix++;
		out_pix->a = 255;
		
		out_pix++;
	}
}


void SuperPNG_WriteFile(GPtr globals)
{
	if(gStuff->HostSupports32BitCoordinates && gStuff->imageSize32.h && gStuff->imageSize32.v)
		gStuff->PluginUsing32BitCoordinates = TRUE;
		
	const int width = (gStuff->PluginUsing32BitCoordinates ? gStuff->imageSize32.h : gStuff->imageSize.h);
	const int height = (gStuff->PluginUsing32BitCoordinates ? gStuff->imageSize32.v : gStuff->imageSize.v);
	
	
	// Set up image parameters

	int8 hi_plane, num_channels, color_type, bit_depth, plane_bytes;
	
	if(gStuff->imageMode == plugInModeIndexedColor)
	{
		hi_plane = 0;
		num_channels = 1;
		color_type = PNG_COLOR_TYPE_PALETTE;
	}
	else if(gStuff->imageMode == plugInModeBitmap)
	{
		hi_plane = 0;
		num_channels = 1;
		color_type = PNG_COLOR_TYPE_GRAY;
		//png_set_packing(png_ptr);
	}
	else if(gStuff->imageMode == plugInModeGrayScale ||
			gStuff->imageMode == plugInModeGray16)
	{
		assert(gStuff->planes >= 1);
	
		const bool have_transparency = (gStuff->planes >= 2);
		const bool have_alpha_channel = (gStuff->channelPortProcs && gStuff->documentInfo && gStuff->documentInfo->alphaChannels);

		const bool use_transparency = (have_transparency && gOptions.alpha == PNG_ALPHA_TRANSPARENCY);
		const bool use_alpha_channel = (have_alpha_channel && gOptions.alpha == PNG_ALPHA_CHANNEL);
		
		const bool use_alpha = (use_transparency || use_alpha_channel);
		
		hi_plane = (use_transparency ? 1 : 0);
		num_channels = (use_alpha ? 2 : 1);
		color_type = (use_alpha ? PNG_COLOR_TYPE_GRAY_ALPHA : PNG_COLOR_TYPE_GRAY);
	}
	else // Assuming RGB if not greyscale
	{
		assert(gStuff->planes >= 3);
	
		const bool have_transparency = (gStuff->planes >= 4);
		const bool have_alpha_channel = (gStuff->channelPortProcs && gStuff->documentInfo && gStuff->documentInfo->alphaChannels);

		const bool use_transparency = (have_transparency && gOptions.alpha == PNG_ALPHA_TRANSPARENCY);
		const bool use_alpha_channel = (have_alpha_channel && gOptions.alpha == PNG_ALPHA_CHANNEL);
		
		const bool use_alpha = (use_transparency || use_alpha_channel);
		
		hi_plane = (use_transparency ? 3 : 2);
		num_channels = (use_alpha ? 4 : 3);
		color_type = (use_alpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB);
	}
	
	
	int col_bytes = num_channels;
	
	if(gStuff->depth == 16)
	{
		bit_depth = 16;
		plane_bytes = 2;
		
		col_bytes *= 2;
	}
	else if(gStuff->depth == 1)
	{
		bit_depth = 1;
		plane_bytes = 1;
	}
	else
	{
		bit_depth = 8;
		plane_bytes = 1;
	}
	
	
	if(gOptions.pngquant)
	{
		if(gStuff->imageMode != plugInModeRGBColor || num_channels < 3 || gStuff->depth != 8)
		{
			gOptions.pngquant = FALSE;
		}
		else
		{
			color_type = PNG_COLOR_TYPE_PALETTE;
			
			if(gOptions.quant_quality < 0)
				gOptions.quant_quality = 0;
			else if(gOptions.quant_quality > 100)
				gOptions.quant_quality = 100;
		}
	}


	// Set up the PNG pointers
	png_structp png_ptr;
	png_infop info_ptr;
	
	png_init_write(globals, &png_ptr, &info_ptr);


	// This is how libpng handles errors
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


	const int interlace_type = (gOptions.interlace == PNG_INTERLACE_ADAM7 ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE);	
	
	// Send the image parameters to info_ptr 
	
	png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type,
		interlace_type, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		
		
	if( (gStuff->imageMode == plugInModeIndexedColor) ||
		(gStuff->imageMode == plugInModeBitmap) )
	{
		png_set_filter(png_ptr, PNG_FILTER_TYPE_DEFAULT, PNG_NO_FILTERS);
	}
	else if(gOptions.filter <= PNG_ALL_FILTERS)
	{
		png_set_filter(png_ptr, PNG_FILTER_TYPE_DEFAULT, gOptions.filter);
	}

	if(gOptions.compression != Z_DEFAULT_COMPRESSION)
		png_set_compression_level(png_ptr, gOptions.compression);
	
	if(gOptions.strategy != Z_DEFAULT_STRATEGY)
		png_set_compression_strategy(png_ptr, gOptions.strategy);

	
	if(gStuff->imageMode == plugInModeBitmap)
	{
		png_set_packing(png_ptr);
		//png_set_packswap(png_ptr);
	}
	else if(gStuff->imageMode == plugInModeIndexedColor)
	{
		// Set palette for indexed colored PNGs
		png_color palette[PNG_MAX_PALETTE_LENGTH];
		
		const int num_palette = (gStuff->lutCount == 0 ? 256 : gStuff->lutCount);
		
		for(int i=0; i < num_palette; i++)
		{
			palette[i].red   = gStuff->redLUT[i];
			palette[i].green = gStuff->greenLUT[i];
			palette[i].blue  = gStuff->blueLUT[i];
		}
		
		if(gStuff->lutCount <= 128)
		{
			png_set_packing(png_ptr);
			png_set_packswap(png_ptr);
		}
		
		png_set_PLTE(png_ptr, info_ptr, palette, num_palette);
		
		if(gStuff->transparentIndex < gStuff->lutCount)
		{
			png_byte trans[PNG_MAX_PALETTE_LENGTH];

			for(int i=0 ; i <= gStuff->transparentIndex ; i++)
			{
				trans[i] = (i == gStuff->transparentIndex ? 0 : 255);
			}
			
			png_set_tRNS(png_ptr, info_ptr, trans, (gStuff->transparentIndex + 1), NULL);
		}
	}
	
#ifndef __PIMacPPC__
	// this doesn't seem to be doing anything, so I'm swapping myself
	//if(gStuff->depth == 16)
	//	png_set_swap(png_ptr);
#endif
	
	if(gOptions.metadata)
		WriteMetadata(globals, png_ptr, info_ptr);

				
	gStuff->loPlane = 0;
	gStuff->hiPlane = hi_plane;
	gStuff->colBytes = col_bytes;
	gStuff->rowBytes = gRowBytes = col_bytes * width;
	gStuff->planeBytes = plane_bytes;
	
	gStuff->theRect.left = gStuff->theRect32.left = 0;
	gStuff->theRect.right = gStuff->theRect32.right = width;


	ReadPixelsProc ReadProc = NULL;
	ReadChannelDesc *alpha_channel = NULL;
	
	// ReadProc being non-null means we're going to get the channel from the channels palette
	if(gOptions.alpha == PNG_ALPHA_CHANNEL && (color_type & PNG_COLOR_MASK_ALPHA) &&
		gStuff->channelPortProcs && gStuff->documentInfo && gStuff->documentInfo->alphaChannels)
	{
		ReadProc = gStuff->channelPortProcs->readPixelsProc;
		
		alpha_channel = gStuff->documentInfo->alphaChannels;
	}
		
	
	if(interlace_type == PNG_INTERLACE_ADAM7 || gOptions.pngquant) 
	{
		// Load the whole image into RAM and write it all at once. 
		
		BufferID bufferID = 0;
		
		gResult = myAllocateBuffer(globals, gRowBytes * height, &bufferID);
		
		if(gResult == noErr && bufferID != 0)
		{
			gStuff->data = gPixelData = myLockBuffer(globals, bufferID, TRUE);
		
			gStuff->theRect.top = gStuff->theRect32.top = 0;
			gStuff->theRect.bottom = gStuff->theRect32.bottom = height;
	
			gResult = AdvanceState();
			
			if(ReadProc && gResult == noErr)
			{
				VRect wroteRect;
				VRect writeRect = { 0, 0, height, width };
				PSScaling scaling; scaling.sourceRect = scaling.destinationRect = writeRect;
				PixelMemoryDesc memDesc = { (char *)gPixelData + ((num_channels - 1) * plane_bytes), gRowBytes * 8, col_bytes * 8, 0, bit_depth };					
			
				gResult = ReadProc(alpha_channel->port, &scaling, &writeRect, &memDesc, &wroteRect);
			}
			
			
			if(gResult == noErr)
			{
				if(gOptions.clean_transparent)
				{
					const int64 pixels = (int64)width * (int64)height;
					
					CleanTransparent(globals, num_channels, pixels);
				}
				
				
				if(gOptions.pngquant)
				{
					liq_attr *attr = liq_attr_create();
					
					liq_set_quality(attr, 0, gOptions.quant_quality);
					
					// convoluted way of getting the user's speed selection from the dialog
					const int liq_speed = (gOptions.compression == Z_BEST_COMPRESSION ? 3 :
											gOptions.compression == Z_NO_COMPRESSION ? 9 :
											gOptions.filter == PNG_FILTER_SUB ? 7 :
											5);
					
					liq_set_speed(attr, liq_speed);
				
					liq_image *image = NULL;
					
					if(num_channels == 4)
					{
						image = liq_image_create_rgba(attr, gPixelData, width, height, 0);
					}
					else if(num_channels == 3)
					{
						RGBbuf buf = { (unsigned8 *)gPixelData, gRowBytes };
						
						image = liq_image_create_custom(attr, rgb2rgba_callback, &buf, width, height, 0);
					}
					
					if(image != NULL)
					{
						liq_result *result = liq_quantize_image(attr, image);
						
						PIUpdateProgress(1, 3);
						
						if(result != NULL)
						{
							liq_set_dithering_level(result, 1.0);
						
							const size_t pngbuf_rowbytes = sizeof(unsigned8) * width;
							const size_t pngbuf_size = pngbuf_rowbytes * height;
							
							gResult = TestAbort();
							
							BufferID pngID = 0;
							
							if(gResult == noErr)
								gResult = myAllocateBuffer(globals, pngbuf_size, &pngID);
							
							if(pngID != 0 && gResult == noErr)
							{
								Ptr pngbuf = myLockBuffer(globals, pngID, TRUE);
								
								liq_error liq_err = liq_write_remapped_image(result, image, pngbuf, pngbuf_size);
								
								PIUpdateProgress(2, 3);
								
								gResult = TestAbort();
								
								if(liq_err == LIQ_OK && gResult == noErr)
								{
									const liq_palette *pal = liq_get_palette(result);
									
									png_color palette[PNG_MAX_PALETTE_LENGTH];
									png_byte trans[PNG_MAX_PALETTE_LENGTH];

									for(int i=0; i < pal->count; i++)
									{
										palette[i].red   = pal->entries[i].r;
										palette[i].green = pal->entries[i].g;
										palette[i].blue  = pal->entries[i].b;
										
										trans[i] = pal->entries[i].a;
									}
									
									if(pal->count <= 128)
									{
										png_set_packing(png_ptr);
										png_set_packswap(png_ptr);
									}
									
									png_set_PLTE(png_ptr, info_ptr, palette, pal->count);
									
									if(num_channels == 4)
										png_set_tRNS(png_ptr, info_ptr, trans, pal->count, NULL);
									
									
									png_write_info(png_ptr, info_ptr);
									
									png_bytepp row_pointers = (png_bytepp)png_malloc(png_ptr, height * sizeof(png_bytep));
									
									for(int row=0; row < height; row++)
										row_pointers[row] = (png_bytep)( (char *)pngbuf + (row * pngbuf_rowbytes) );
									

									png_write_image(png_ptr, row_pointers);
									
									PIUpdateProgress(3, 3);
									
									png_free(png_ptr, (void *)row_pointers);
								}
								
								myFreeBuffer(globals, pngID);
							}
							
							liq_result_destroy(result);
						}
						else
							gResult = formatBadParameters;
						
						liq_image_destroy(image);
					}
					else
						gResult = formatBadParameters;
					
					liq_attr_destroy(attr);
				}
				else
				{
					if(gStuff->depth == 16)
					{
						// Convert Adobe 16-bit to full 16-bit and swap endian
						int64 samples = (int64)width * (int64)height * (int64)num_channels;
						
						unsigned16 *pix = (unsigned16 *)gPixelData;
						
						while(samples--)
						{
							*pix = ByteSwap( Promote(*pix) );
							pix++;
						}
					}

					png_write_info(png_ptr, info_ptr);
					
					png_bytepp row_pointers = (png_bytepp)png_malloc(png_ptr, height * sizeof(png_bytep));
					
					for(int row=0; row < height; row++)
						row_pointers[row] = (png_bytep)( (char *)gPixelData + (row * gRowBytes) );
					

					png_write_image(png_ptr, row_pointers);
					
					
					png_free(png_ptr, (void *)row_pointers);
				}
			}
			
			myFreeBuffer(globals, bufferID);
			
			gStuff->data = gPixelData = NULL;
		}
	}
	else
	{
		// write some reasonable number of scanlines at a time
		const int num_scanlines = (gStuff->tileHeight == 0 ? 256 : gStuff->tileHeight);
		
		BufferID bufferID = 0;
		
		gResult = myAllocateBuffer(globals, gRowBytes * num_scanlines, &bufferID);
		
		if(gResult == noErr && bufferID != 0)
		{
			png_write_info(png_ptr, info_ptr);
			
		
			gStuff->data = gPixelData = myLockBuffer(globals, bufferID, TRUE);
		
			png_bytepp row_pointers = (png_bytepp)png_malloc(png_ptr, num_scanlines * sizeof(png_bytep));
			
			for(int row=0; row < num_scanlines; row++)
				row_pointers[row] = (png_bytep)( (char *)gPixelData + (row * gRowBytes) );
		
		
			int y = 0;
			
			while(y < height && gResult == noErr)
			{
				const int high_scanline = minimum<int>(y + num_scanlines - 1, height - 1);
				const int block_height = 1 + high_scanline - y;
				
				gStuff->theRect.top = gStuff->theRect32.top = y;
				gStuff->theRect.bottom = gStuff->theRect32.bottom = high_scanline + 1;

				gResult = AdvanceState();
				
				if(ReadProc && gResult == noErr)
				{
					VRect wroteRect;
					VRect writeRect = { y, 0, high_scanline + 1, width };
					PSScaling scaling; scaling.sourceRect = scaling.destinationRect = writeRect;
					PixelMemoryDesc memDesc = { (char *)gPixelData + ((num_channels - 1) * plane_bytes), gRowBytes * 8, col_bytes * 8, 0, bit_depth };					
				
					gResult = ReadProc(alpha_channel->port, &scaling, &writeRect, &memDesc, &wroteRect);
				}
				
				
				if(gOptions.clean_transparent)
				{
					const int64 pixels = (int64)width * (int64)block_height;
					
					CleanTransparent(globals, num_channels, pixels);
				}
				
				
				if(gStuff->depth == 16)
				{
					int64 samples = (int64)width * (int64)block_height * (int64)num_channels;
					
					unsigned16 *pix = (unsigned16 *)gPixelData;
					
					while(samples--)
					{
						*pix = ByteSwap( Promote(*pix) );
						pix++;
					}
				}

				png_write_rows(png_ptr, row_pointers, block_height);
								
				PIUpdateProgress(y, height);
				
				y = high_scanline + 1;
			}
			
			
			png_free(png_ptr, (void *)row_pointers);	
			
			myFreeBuffer(globals, bufferID);
			
			gStuff->data = gPixelData = NULL;
		}
	}
	
	
	if(gResult == noErr)
	{
		png_write_end(png_ptr, info_ptr);
	}

	png_destroy_write_struct(&png_ptr, &info_ptr);
	

	// muy importante
	gStuff->data = NULL;
}
