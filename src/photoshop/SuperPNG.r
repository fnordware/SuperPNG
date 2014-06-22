
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


#include "SuperPNG_Version.h"

#define plugInName			"SuperPNG"
#define plugInCopyrightYear	SuperPNG_Copyright_Year
#define plugInDescription SuperPNG_Description
#define VersionString 	SuperPNG_Version_String
#define ReleaseString	SuperPNG_Build_Date_Manual
#define CurrentYear		SuperPNG_Build_Year

#define vendorName			"fnord"
#define plugInAETEComment 	SuperPNG_Description

#define plugInSuiteID		'sdK4'
#define plugInClassID		'sPNG'
#define plugInEventID		typeNull // must be this



#include "PIDefines.h"

#ifdef __PIMac__
	#include "Types.r"
	#include "SysTypes.r"
	#include "PIGeneral.r"
#elif defined(__PIWin__)
	#include "PIGeneral.h"
#endif

#ifndef ResourceID
	#define ResourceID		16000
#endif

#include "PITerminology.h"
#include "PIActions.h"


#include "SuperPNG_Terminology.h"


resource 'PiPL' (ResourceID, plugInName " PiPL", purgeable)
{
    {
		Kind { ImageFormat },
		Name { plugInName },

		Category { "PNG" },
		Priority { 1 }, // Causes SuperPNG to be used over Adobe's PNG plug-in

		Version { (latestFormatVersion << 16) | latestFormatSubVersion },

		#ifdef __PIMac__
			#ifdef BUILDING_FOR_MACH
				#if (defined(__x86_64__))
					CodeMacIntel64 { "PluginMain" },
				#endif
				#if (defined(__i386__))
					CodeMacIntel32 { "PluginMain" },
				#endif
				#if (defined(__ppc__))
					CodeMachOPowerPC { 0, 0, "PluginMain" },
				#endif
			#else
				#if TARGET_CARBON
			        CodeCarbonPowerPC { 0, 0, "" },
			    #else
					CodePowerPC { 0, 0, "" },		
				#endif
			#endif
		#else
			#if defined(_WIN64)
				CodeWin64X86 { "PluginMain" },
			#else
				CodeWin32X86 { "PluginMain" },
			#endif
		#endif
	
		HasTerminology { plugInClassID, plugInEventID, ResourceID, vendorName " " plugInName },
		
		SupportedModes
		{
			doesSupportBitmap, doesSupportGrayScale,
			doesSupportIndexedColor, doesSupportRGBColor,
			noCMYKColor, noHSLColor,
			noHSBColor, noMultichannel,
			noDuotone, noLABColor
		},
			
		EnableInfo { "in (PSHOP_ImageMode, BitmapMode, GrayScaleMode, IndexedMode, RGBMode, Gray16Mode, RGB48Mode)" },
	
		FmtFileType { 'PNGf', '8BIM' },
		ReadTypes { { 'PNGf', '    ' } },
		ReadExtensions { { 'PNG ' } },
		WriteExtensions { { 'PNG ' } },
		FilteredExtensions { { 'PNG ' } },
		FormatFlags { fmtSavesImageResources, // by saying we do, PS won't store them, thereby avoiding problems
		              fmtCanRead, 
					  fmtCanWrite, 
					  fmtCanWriteIfRead, 
					  fmtCanWriteTransparency,
					  fmtCannotCreateThumbnail },
		PlugInMaxSize { 2147483647, 2147483647 },
		FormatMaxSize { { 32767, 32767 } },
		FormatMaxChannels { {   1, 2, 1, 4, 0, 0, 
							   0, 0, 0, 0, 2, 4 } },
		FormatICCFlags { 	iccCanEmbedGray,
							iccCanEmbedIndexed,
							iccCanEmbedRGB,
							iccCannotEmbedCMYK },
		XMPWrite { },
		XMPRead { }
		},
	};


resource 'PiMI' (ResourceID, plugInName " PiMI", purgeable)
{
	latestFormatVersion,
	latestFormatSubVersion,
	0,

	supportsGrayScale +
	supportsRGBColor +
	supportsIndexedColor +
	supportsBitmap,
	'    ',
	
	{
		canRead,
		cannotReadAll,
		canWrite,
		canWriteIfRead,
		savesResources,
		{  0, 2, 0, 4,		// FormatMaxChannels
		  0, 0, 0, 0,
		  0, 0,  2,  4,
		   0,  0,  0,  0 },
		32767,
		32767,
		'PNGf',
		'8BIM',
		{
			'8B1F', '    '
		},
		{
		}
	},
	
};


resource 'aete' (ResourceID, plugInName " dictionary", purgeable)
{
	1, 0, english, roman,
	{
		vendorName,
		"Super-duper PNG format",
		plugInSuiteID,
		1,
		1,
		{},
		{
			"SuperPNG",
			plugInClassID,
			plugInAETEComment,
			{
				"$$$/private/AETE/Inheritance=<Inheritance>",
				keyInherits,
				classFormat,
				"parent class format",
				flagsSingleProperty,
							
				"Compression",
				keyPNGcompression,
				typeInteger,
				"PNG compression level used",
				flagsSingleProperty,
				
				"Filtering",
				keyPNGfilter,
				typeInteger,
				"PNG filtering property",
				flagsSingleProperty,

				"Strategy",
				keyPNGstrategy,
				typeInteger,
				"zlib compression strategy",
				flagsSingleProperty,

				"Interlacing",
				keyPNGinterlace,
				typeBoolean,
				"PNG interlacing used",
				flagsSingleProperty,

				"Meta Data",
				keyPNGmeta,
				typeBoolean,
				"Save PNG Meta Data",
				flagsSingleProperty,

				"Alpha Channel",
				keyPNGalpha,
				typeEnumerated,
				"Source of the alpha channel",
				flagsSingleProperty,
				
				"Clean Transparent",
				keyPNGcleanTransparent,
				typeBoolean,
				"Clear areas where alpha is 0",
				flagsSingleProperty,
				
				"Smart Quantize",
				keyPNGpngquant,
				typeBoolean,
				"Quantize and save and PNG8",
				flagsSingleProperty,
				
				"Quantize Quality",
				keyPNGquantQuality,
				typeInteger,
				"Quality of the Quantization",
				flagsSingleProperty,

			},
			{},
		},
		{},
		{
			typeAlphaChannel,
			{
                "None",
                alphaChannelNone,
                "No alpha channel",

                "Transparency",
                alphaChannelTransparency,
                "Get alpha from Transparency",

                "Channel",
                alphaChannelChannel,
                "Get alpha from channels palette"
			}
		}
	}
};

#ifdef __PIMac__

resource 'vers' (1, plugInName " Version", purgeable)
{
	5, 0x50, final, 0, verUs,
	VersionString,
	VersionString " ©" plugInCopyrightYear " fnord"
};

resource 'vers' (2, plugInName " Version", purgeable)
{
	5, 0x50, final, 0, verUs,
	VersionString,
	"by Brendan Bolles"
};

#endif // __PIMac__


