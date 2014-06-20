
//
// SuperPNG
//
// by Brendan Bolles
//

//-------------------------------------------------------------------------------
//	Definitions -- Required by include files.
//-------------------------------------------------------------------------------

#include "SuperPNG_Version.h"

#define plugInName			"SuperPNG"
#define plugInCopyrightYear	SuperPNG_Copyright_Year
#define plugInDescription SuperPNG_Description
#define VersionString 	SuperPNG_Version_String
#define ReleaseString	SuperPNG_Build_Date_Manual
#define CurrentYear		SuperPNG_Build_Year

//-------------------------------------------------------------------------------
//	Definitions -- Required by other resources in this rez file.
//-------------------------------------------------------------------------------

// Dictionary (aete) resources:

#define vendorName			"fnord"
#define plugInAETEComment 	SuperPNG_Description

#define plugInSuiteID		'sdK4'
#define plugInClassID		'sPNG' //'simP'
#define plugInEventID		typeNull // must be this

//-------------------------------------------------------------------------------
//	Set up included files for Macintosh and Windows.
//-------------------------------------------------------------------------------

#include "PIDefines.h"

#ifdef __PIMac__
	#include "Types.r"
	#include "SysTypes.r"
	#include "PIGeneral.r"
	//#include "PIUtilities.r"
	//#include "DialogUtilities.r"
#elif defined(__PIWin__)
	#include "PIGeneral.h"
	//#include "PIUtilities.r"
	//#include "WinDialogUtils.r"
#endif

#ifndef ResourceID
	#define ResourceID		16000
#endif

#include "PITerminology.h"
#include "PIActions.h"

#include "SuperPNG_Terminology.h"

//-------------------------------------------------------------------------------
//	PiPL resource
//-------------------------------------------------------------------------------

resource 'PiPL' (ResourceID, plugInName " PiPL", purgeable)
{
    {
		Kind { ImageFormat },
		Name { plugInName },

		Category { "PNG" },
		Priority { 1 }, // Causes SuperPNG to be used over Adobe's PNG plug-in

		//ZStringName { "$$$/AdobePlugin/PIPLInfo/PluginName/PNG=PNG" },
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
	
		// ClassID, eventID, aete ID, uniqueString:
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
		FormatFlags { fmtSavesImageResources, //(by saying we do, PS won't store them, thereby avoiding problems)
		              fmtCanRead, 
					  fmtCanWrite, 
					  fmtCanWriteIfRead, 
					  fmtCanWriteTransparency, // yes, Transparency!
					  fmtCannotCreateThumbnail },
	#ifdef PHOTOSHOP_SDK_VERSION_8
		PlugInMaxSize { 2147483647, 2147483647 }, // Photoshop 8
	#endif
		FormatMaxSize { { 32767, 32767 } },
		FormatMaxChannels { {   1, 2, 1, 4, 0, 0, 
							   0, 0, 0, 0, 2, 4 } },
		FormatICCFlags { 	iccCanEmbedGray,
							iccCanEmbedIndexed,
							iccCanEmbedRGB,
							iccCannotEmbedCMYK },
							
	#ifdef PHOTOSHOP_SDK_VERSION_8
		XMPWrite { },
		XMPRead { }
	#endif	
		},
	};


//-------------------------------------------------------------------------------
//	PiMI resource (kept for backward compatibility)
//-------------------------------------------------------------------------------

resource 'PiMI' (ResourceID, plugInName " PiMI", purgeable)
{
	latestFormatVersion, 	/* Version, subVersion, and priority of the interface */
	latestFormatSubVersion,
	0,

	supportsGrayScale +
	supportsRGBColor,			/* Supported Image Modes */
	'    ',						/* Required host */
	
	{
		canRead,
		cannotReadAll,
		canWrite,
		canWriteIfRead,
		savesResources,
		{  0, 2, 0, 4,		/* Maximum # of channels for each plug-in mode */
		  0, 0, 0, 0,
		  0, 0,  2,  4,
		   0,  0,  0,  0 },
		32767,				/* Maximum rows allowed in document */
		32767,				/* Maximum columns allowed in document */
		'PNGf',				/* The file type if we create a file. */
		'8BIM',				/* The creator type if we create a file. */
		{					/* The type-creator pairs supported. */
			'8B1F', '    '
		},
		{					/* The extensions supported. */
		}
	},
	
};

//-------------------------------------------------------------------------------
//	Dictionary (scripting) resource
//-------------------------------------------------------------------------------

resource 'aete' (ResourceID, plugInName " dictionary", purgeable)
{
	1, 0, english, roman,									/* aete version and language specifiers */
	{
		vendorName,											/* vendor suite name */
		"Super-duper PNG format",							/* optional description */
		plugInSuiteID,										/* suite ID */
		1,													/* suite code, must be 1 */
		1,													/* suite level, must be 1 */
		{},													/* structure for filters */
		{													/* non-filter plug-in class here */
			//vendorName " simpleFormat",
			"SuperPNG",										/* unique class name */
			plugInClassID,									/* class ID, must be unique or Suite ID */
			plugInAETEComment,								/* optional description */
			{												/* define inheritance */
				"$$$/private/AETE/Inheritance=<Inheritance>",							/* must be exactly this */
				keyInherits,								/* must be keyInherits */
				classFormat,								/* parent: Format, Import, Export */
				"parent class format",						/* optional description */
				flagsSingleProperty,						/* if properties, list below */
							
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
				flagsSingleProperty

				/* no properties */
			},
			{}, /* elements (not supported) */
			/* class descriptions */
		},
		{}, /* comparison ops (not supported) */
		{	/* any enumerations */
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

//-------------------------------------------------------------------------------
//	Version 'vers' resources.
//-------------------------------------------------------------------------------

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


