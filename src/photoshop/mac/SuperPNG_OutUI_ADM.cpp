
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

#include "SuperPNG_UI.h"

#include "PIUSuites.h"

#include "SuperPNG_version.h"

#include "math.h"

// ==========
// Only building this on 32-bit (non-Cocoa) architectures
// ==========
#if !__LP64__

#include "ADMVersion.h"
#include "ADMDialog.h"
#include "ADMItem.h"
#include "ADMNotifier.h"
#include "ADMDrawer.h"
#include "ADMImage.h"


static AutoSuite<ADMDialogSuite3> sADMDialog(kADMDialogSuite, kADMDialogSuiteVersion3);
static AutoSuite<ADMItemSuite4> sADMItem(kADMItemSuite, kADMItemSuiteVersion4);
static AutoSuite<ADMNotifierSuite1> sADMNotify(kADMNotifierSuite, kADMNotifierSuiteVersion1);
static AutoSuite<ADMDrawerSuite3> sADMDrawer(kADMDrawerSuite, kADMDrawerSuiteVersion3);
static AutoSuite<ADMImageSuite2> sADMImage(kADMImageSuite, kADMImageSuiteVersion2);

static void UnloadSuites()
{
	sADMDialog.Unload();
	sADMItem.Unload();
	sADMNotify.Unload();
	sADMDrawer.Unload();
	sADMImage.Unload();
}


// dialog comtrols
enum {
	OUT_noUI = -1,
	OUT_OK = 1,
	OUT_Cancel,
	OUT_Picture,
	OUT_Radio_1,
	OUT_Radio_2,
	OUT_Radio_3,
	OUT_Radio_4,
	OUT_Faster_Label,
	OUT_Smaller_Label,
	OUT_Alpha_None,
	OUT_Alpha_Transparency,
	OUT_Alpha_Channel,
	OUT_Alpha_Border,
	OUT_Alpha_Border_Text,
	OUT_Interlace_Check,
	OUT_Metadata_Check
};



#define DIALOG_TITLE	"SuperPNG Options"

#define DIALOG_WIDTH	400
#define DIALOG_HEIGHT	310


#define BUTTON_WIDTH	80
#define BUTTON_HEIGHT	20

#define MENU_WIDTH		100
#define MENU_HEIGHT		25
#define MENU_LABEL_DOWN	2

#define LABEL_WIDTH		100
#define LABEL_HEIGHT	20
#define LABEL_SPACE		5

#define RADIO_WIDTH		20
#define RADIO_HEIGHT	20

#define CHECKBOX_WIDTH	150
#define CHECKBOX_HEIGHT	15
#define CHECKBOX_SPACE	8

#define DIALOG_MARGIN	20

// vertical space between items
#define ITEM_SPACE		10


#define OK_TOP		(DIALOG_HEIGHT - (DIALOG_MARGIN + BUTTON_HEIGHT) )
#define OK_BOTTOM	(OK_TOP + BUTTON_HEIGHT)
#define OK_LEFT		(DIALOG_WIDTH - (DIALOG_MARGIN + BUTTON_WIDTH) )
#define OK_RIGHT	(OK_LEFT + BUTTON_WIDTH)
#define OK_TEXT		"OK"

#define CANCEL_SEPERATION	20

#define CANCEL_TOP		OK_TOP
#define CANCEL_BOTTOM	OK_BOTTOM
#define CANCEL_LEFT		(OK_LEFT - (BUTTON_WIDTH + CANCEL_SEPERATION) )
#define CANCEL_RIGHT	(CANCEL_LEFT + BUTTON_WIDTH)
#define CANCEL_TEXT		"Cancel"

#define PICTURE_WIDTH		325
#define PICTURE_HEIGHT		40

#define PICTURE_NUDGE_UP	10

#define PICTURE_TOP		(DIALOG_MARGIN - PICTURE_NUDGE_UP)
#define PICTURE_BOTTOM	(PICTURE_TOP + PICTURE_HEIGHT)
#define PICTURE_LEFT	( (DIALOG_WIDTH - PICTURE_WIDTH) / 2 )
#define PICTURE_RIGHT	(PICTURE_LEFT + PICTURE_WIDTH)

#define RADIO_SEPERATE		70

#define RADIO_1_TOP		(PICTURE_BOTTOM + ITEM_SPACE + ITEM_SPACE)
#define RADIO_1_BOTTOM	(RADIO_1_TOP + RADIO_HEIGHT)
#define RADIO_TOTAL_WIDTH		( (RADIO_WIDTH * 4) + (RADIO_SEPERATE * 3) )
#define RADIO_1_LEFT	( (DIALOG_WIDTH - RADIO_TOTAL_WIDTH) / 2 )
#define RADIO_1_RIGHT	(RADIO_1_LEFT + RADIO_WIDTH)

#define RADIO_2_TOP		RADIO_1_TOP
#define RADIO_2_BOTTOM	RADIO_1_BOTTOM
#define RADIO_2_LEFT	(RADIO_1_RIGHT + RADIO_SEPERATE)
#define RADIO_2_RIGHT	(RADIO_2_LEFT + RADIO_WIDTH)

#define RADIO_3_TOP		RADIO_1_TOP
#define RADIO_3_BOTTOM	RADIO_1_BOTTOM
#define RADIO_3_LEFT	(RADIO_2_RIGHT + RADIO_SEPERATE)
#define RADIO_3_RIGHT	(RADIO_3_LEFT + RADIO_WIDTH)

#define RADIO_4_TOP		RADIO_1_TOP
#define RADIO_4_BOTTOM	RADIO_1_BOTTOM
#define RADIO_4_LEFT	(RADIO_3_RIGHT + RADIO_SEPERATE)
#define RADIO_4_RIGHT	(RADIO_4_LEFT + RADIO_WIDTH)

#define FASTER_TOP		(RADIO_1_BOTTOM + LABEL_SPACE)
#define FASTER_BOTTOM	(FASTER_TOP + LABEL_HEIGHT)
#define FASTER_LEFT		( (RADIO_1_LEFT + (RADIO_WIDTH / 2) ) - (LABEL_WIDTH / 2) )
#define FASTER_RIGHT	(FASTER_LEFT + LABEL_WIDTH)
#define FASTER_TEXT		"Faster Saves"

#define SMALLER_TOP		FASTER_TOP
#define SMALLER_BOTTOM	FASTER_BOTTOM
#define SMALLER_LEFT	( (RADIO_4_LEFT + (RADIO_WIDTH / 2) ) - (LABEL_WIDTH / 2) )
#define SMALLER_RIGHT	(SMALLER_LEFT + LABEL_WIDTH)
#define SMALLER_TEXT	"Smaller Files"

#define ALPHA_NONE_TOP		(FASTER_BOTTOM + ITEM_SPACE + ITEM_SPACE + ITEM_SPACE + ITEM_SPACE)
#define ALPHA_NONE_BOTTOM	(ALPHA_NONE_TOP + RADIO_HEIGHT)
#define ALPHA_NONE_LEFT		RADIO_1_LEFT
#define ALPHA_NONE_RIGHT	(ALPHA_NONE_LEFT + CHECKBOX_WIDTH)
#define ALPHA_NONE_TEXT		"None"

#define ALPHA_TRANS_TOP		(ALPHA_NONE_BOTTOM + ITEM_SPACE)
#define ALPHA_TRANS_BOTTOM	(ALPHA_TRANS_TOP + RADIO_HEIGHT)
#define ALPHA_TRANS_LEFT	ALPHA_NONE_LEFT
#define ALPHA_TRANS_RIGHT	ALPHA_NONE_RIGHT
#define ALPHA_TRANS_TEXT	"Transparency"

#define ALPHA_CHANNEL_TOP	(ALPHA_TRANS_BOTTOM + ITEM_SPACE)
#define ALPHA_CHANNEL_BOTTOM (ALPHA_CHANNEL_TOP + RADIO_HEIGHT)
#define ALPHA_CHANNEL_LEFT	ALPHA_NONE_LEFT
#define ALPHA_CHANNEL_RIGHT	ALPHA_NONE_RIGHT
#define ALPHA_CHANNEL_TEXT	"Channel Blah"

#define ALPHA_BORDER_TOP	(ALPHA_NONE_TOP - ITEM_SPACE)
#define ALPHA_BORDER_BOTTOM	(ALPHA_CHANNEL_BOTTOM + ITEM_SPACE)
#define ALPHA_BORDER_LEFT	(ALPHA_NONE_LEFT - DIALOG_MARGIN)
#define ALPHA_BORDER_RIGHT	(ALPHA_NONE_RIGHT + DIALOG_MARGIN)

#define BORDER_TEXT_TOP		(ALPHA_BORDER_TOP - ITEM_SPACE)
#define BORDER_TEXT_BOTTOM	(BORDER_TEXT_TOP + ITEM_SPACE)
#define BORDER_TEXT_LEFT	(ALPHA_BORDER_LEFT + ITEM_SPACE)
#define BORDER_TEXT_RIGHT	(BORDER_TEXT_LEFT + CHECKBOX_WIDTH)
#define BORDER_TEXT_TEXT	"Alpha Channel"

#define INTERLACING_TOP		ALPHA_NONE_TOP
#define INTERLACING_BOTTOM	(INTERLACING_TOP + CHECKBOX_HEIGHT)
#define INTERLACING_LEFT	RADIO_3_LEFT
#define INTERLACING_RIGHT	(INTERLACING_LEFT + CHECKBOX_WIDTH)
#define INTERLACING_TEXT	"PNG Interlacing"

#define METADATA_TOP		(INTERLACING_BOTTOM + CHECKBOX_SPACE)
#define METADATA_BOTTOM		(METADATA_TOP + CHECKBOX_HEIGHT)
#define METADATA_LEFT		INTERLACING_LEFT
#define METADATA_RIGHT		INTERLACING_RIGHT
#define METADATA_TEXT		"Save Metadata"


#if ADMVMajor <= 0x02
typedef ASRect ADMRect;
#endif


#ifdef MAC_ENV
#define INIT_RECT(TOP, LEFT, BOTTOM, RIGHT)		{ (TOP), (LEFT), (BOTTOM), (RIGHT) }
#else
#define INIT_RECT(TOP, LEFT, BOTTOM, RIGHT)		{ (LEFT), (TOP), (RIGHT), (BOTTOM) }
#endif

extern SPBasicSuite		*sSPBasic;
extern SPPluginRef		gPlugInRef;



static DialogCompression	g_compression	= DIALOG_COMPRESSION_NORMAL;
static bool					g_interlace = FALSE;
static bool					g_metadata = TRUE;
static DialogAlpha			g_alpha = DIALOG_ALPHA_NONE;

static bool					g_have_transparency = false;
static const char			*g_alpha_name = NULL;

ADMImageRef					g_image = NULL;



static void ASAPI
RadioNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	//AEGP_SuiteHandler suites(sP);

	sADMItem->DefaultNotify(item, notifier);
		
	if (sADMNotify->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		if( sADMItem->GetBooleanValue(item) ) // this may be unnecessary
		{
			ASInt32 itemID = sADMItem->GetID(item);
			
			switch(itemID)
			{
				case OUT_Radio_1:
					g_compression = DIALOG_COMPRESSION_NONE;
					break;
				
				case OUT_Radio_2:
					g_compression = DIALOG_COMPRESSION_LOW;
					break;

				case OUT_Radio_3:
					g_compression = DIALOG_COMPRESSION_NORMAL;
					break;
					
				case OUT_Radio_4:
					g_compression = DIALOG_COMPRESSION_HIGH;
					break;
			}
		}
	}
}

static void ASAPI
AlphaNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	//AEGP_SuiteHandler suites(sP);

	sADMItem->DefaultNotify(item, notifier);
		
	if (sADMNotify->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		if( sADMItem->GetBooleanValue(item) ) // this may be unnecessary
		{
			ASInt32 itemID = sADMItem->GetID(item);
			
			switch(itemID)
			{
				case OUT_Alpha_None:
					g_alpha = DIALOG_ALPHA_NONE;
					break;
				
				case OUT_Alpha_Transparency:
					g_alpha = DIALOG_ALPHA_TRANSPARENCY;
					break;

				case OUT_Alpha_Channel:
					g_alpha = DIALOG_ALPHA_CHANNEL;
					break;
			}
		}
	}
}

static void ASAPI
InterlaceNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	//AEGP_SuiteHandler suites(sP);

	sADMItem->DefaultNotify(item, notifier);
		
	if (sADMNotify->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		g_interlace = sADMItem->GetBooleanValue(item);
	}
}

static void ASAPI
MetadataNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	//AEGP_SuiteHandler suites(sP);

	sADMItem->DefaultNotify(item, notifier);
		
	if (sADMNotify->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		g_metadata = sADMItem->GetBooleanValue(item);
	}
}


static void ASAPI
DrawPictureProc(ADMItemRef item, ADMDrawerRef drawer)
{
	//AEGP_SuiteHandler suites(sP);
	
	extern ADMImageRef g_image;
	ADMRect pos;
	
	if( g_image ) //sADMItem->IsVisible(item)
	{
		sADMDrawer->GetBoundsRect(drawer, &pos);

		sADMImage->BeginADMDrawer(g_image);
		sADMDrawer->DrawADMImageCentered(drawer, g_image, &pos);
		sADMImage->EndADMDrawer(g_image);
	}
	else
		sADMDrawer->Clear(drawer);
}


static ASErr ASAPI DialogInit(ADMDialogRef dialog)
{
	ASErr err = kSPNoError;

	//AEGP_SuiteHandler	suites(sP);

	// set the dialog's size and name
	sADMDialog->Size(dialog, DIALOG_WIDTH, DIALOG_HEIGHT);
	sADMDialog->SetText(dialog, DIALOG_TITLE);
	
	
	// create the OK button
	ADMRect OK_rect = INIT_RECT(OK_TOP, OK_LEFT, OK_BOTTOM, OK_RIGHT);
	
	ADMItemRef OK_button = sADMItem->Create(dialog, OUT_OK,
								kADMTextPushButtonType, &OK_rect, NULL, NULL);
	
	sADMItem->SetText(OK_button, OK_TEXT);
	

	// create the Cancel button
	ADMRect Cancel_rect = INIT_RECT(CANCEL_TOP, CANCEL_LEFT, CANCEL_BOTTOM, CANCEL_RIGHT);
	
	ADMItemRef Cancel_button = sADMItem->Create(dialog, OUT_Cancel,
								kADMTextPushButtonType, &Cancel_rect, NULL, NULL);
	
	sADMItem->SetText(Cancel_button, CANCEL_TEXT);
	

	// picture
	ADMRect Picure_rect = INIT_RECT(PICTURE_TOP, PICTURE_LEFT, PICTURE_BOTTOM, PICTURE_RIGHT);
	
	ADMItemRef Picture_item = sADMItem->Create(dialog, OUT_Picture,
								kADMPictureStaticType, &Picure_rect, NULL, NULL);
	
	sADMItem->SetDrawProc(Picture_item, DrawPictureProc);


	// radio buttons
	ADMRect Radio_1_rect = INIT_RECT(RADIO_1_TOP, RADIO_1_LEFT, RADIO_1_BOTTOM, RADIO_1_RIGHT);
	ADMRect Radio_2_rect = INIT_RECT(RADIO_2_TOP, RADIO_2_LEFT, RADIO_2_BOTTOM, RADIO_2_RIGHT);
	ADMRect Radio_3_rect = INIT_RECT(RADIO_3_TOP, RADIO_3_LEFT, RADIO_3_BOTTOM, RADIO_3_RIGHT);
	ADMRect Radio_4_rect = INIT_RECT(RADIO_4_TOP, RADIO_4_LEFT, RADIO_4_BOTTOM, RADIO_4_RIGHT);
	
	ADMItemRef Radio_1 = sADMItem->Create(dialog, OUT_Radio_1,
								kADMTextRadioButtonType, &Radio_1_rect, NULL, NULL);

	ADMItemRef Radio_2 = sADMItem->Create(dialog, OUT_Radio_2,
								kADMTextRadioButtonType, &Radio_2_rect, NULL, NULL);

	ADMItemRef Radio_3 = sADMItem->Create(dialog, OUT_Radio_3,
								kADMTextRadioButtonType, &Radio_3_rect, NULL, NULL);

	ADMItemRef Radio_4 = sADMItem->Create(dialog, OUT_Radio_4,
								kADMTextRadioButtonType, &Radio_4_rect, NULL, NULL);

	
	sADMItem->SetNotifyProc(Radio_1, RadioNotifyProc);
	sADMItem->SetNotifyProc(Radio_2, RadioNotifyProc);
	sADMItem->SetNotifyProc(Radio_3, RadioNotifyProc);
	sADMItem->SetNotifyProc(Radio_4, RadioNotifyProc);

	// radio setting
	ADMItemRef active_radio = (	(g_compression == DIALOG_COMPRESSION_NONE)	? Radio_1 :
								(g_compression == DIALOG_COMPRESSION_LOW)	? Radio_2 :
								(g_compression == DIALOG_COMPRESSION_HIGH)	? Radio_4 :
								Radio_3 );
	
	sADMItem->SetBooleanValue(active_radio, TRUE);
	
	
	// labels
	ADMRect Faster_rect = INIT_RECT(FASTER_TOP, FASTER_LEFT, FASTER_BOTTOM, FASTER_RIGHT);
	ADMRect Smaller_rect = INIT_RECT(SMALLER_TOP, SMALLER_LEFT, SMALLER_BOTTOM, SMALLER_RIGHT);
	
	ADMItemRef Faster = sADMItem->Create(dialog, OUT_Faster_Label,
								kADMTextStaticType, &Faster_rect, NULL, NULL);

	ADMItemRef Smaller = sADMItem->Create(dialog, OUT_Smaller_Label,
								kADMTextStaticType, &Smaller_rect, NULL, NULL);
								
	sADMItem->SetText(Faster, FASTER_TEXT);
	sADMItem->SetText(Smaller, SMALLER_TEXT);
	
	sADMItem->SetJustify(Faster, kADMCenterJustify);
	sADMItem->SetJustify(Smaller, kADMCenterJustify);
	
	
	// alpha border
	ADMRect Alpha_border_rect = INIT_RECT(ALPHA_BORDER_TOP, ALPHA_BORDER_LEFT, ALPHA_BORDER_BOTTOM, ALPHA_BORDER_RIGHT);
	
	ADMItemRef Alpha_border = sADMItem->Create(dialog, OUT_Alpha_Border,
								kADMFrameType, &Alpha_border_rect, NULL, NULL);
	
	sADMItem->SetItemStyle(Alpha_border, kADMEtchedFrameStyle);
	
	
	// alpha border label
	ADMRect Border_text_rect = INIT_RECT(BORDER_TEXT_TOP, BORDER_TEXT_LEFT, BORDER_TEXT_BOTTOM, BORDER_TEXT_RIGHT);

	ADMItemRef Border_text = sADMItem->Create(dialog, OUT_Alpha_Border_Text,
								kADMTextStaticType, &Border_text_rect, NULL, NULL);

	sADMItem->SetText(Border_text, BORDER_TEXT_TEXT);
	sADMItem->SetJustify(Border_text, kADMLeftJustify);
	sADMItem->SetFont(Border_text, kADMPaletteFont);

	
	// alpha radio buttons
	ADMRect Alpha_none_rect = INIT_RECT(ALPHA_NONE_TOP, ALPHA_NONE_LEFT, ALPHA_NONE_BOTTOM, ALPHA_NONE_RIGHT);
	ADMRect Alpha_trans_rect = INIT_RECT(ALPHA_TRANS_TOP, ALPHA_TRANS_LEFT, ALPHA_TRANS_BOTTOM, ALPHA_TRANS_RIGHT);
	ADMRect Alpha_channel_rect = INIT_RECT(ALPHA_CHANNEL_TOP, ALPHA_CHANNEL_LEFT, ALPHA_CHANNEL_BOTTOM, ALPHA_CHANNEL_RIGHT);
	
	ADMItemRef Alpha_none = sADMItem->Create(dialog, OUT_Alpha_None, kADMTextRadioButtonType, &Alpha_none_rect, NULL, NULL);
	ADMItemRef Alpha_trans = sADMItem->Create(dialog, OUT_Alpha_Transparency, kADMTextRadioButtonType, &Alpha_trans_rect, NULL, NULL);
	ADMItemRef Alpha_channel = sADMItem->Create(dialog, OUT_Alpha_Channel, kADMTextRadioButtonType, &Alpha_channel_rect, NULL, NULL);
	
	sADMItem->SetText(Alpha_none, ALPHA_NONE_TEXT);
	sADMItem->SetText(Alpha_trans, ALPHA_TRANS_TEXT);
	
	const char *no_channel_name = "Channels palette";
	sADMItem->SetText(Alpha_channel, (g_alpha_name ? g_alpha_name : no_channel_name));
	
	
	sADMItem->SetFont(Alpha_none, kADMPaletteFont);
	sADMItem->SetFont(Alpha_trans, kADMPaletteFont);
	sADMItem->SetFont(Alpha_channel, kADMPaletteFont);
	
	sADMItem->SetNotifyProc(Alpha_none, AlphaNotifyProc);
	sADMItem->SetNotifyProc(Alpha_trans, AlphaNotifyProc);
	sADMItem->SetNotifyProc(Alpha_channel, AlphaNotifyProc);
	
	
	ADMItemRef active_alpha = (	(g_alpha == DIALOG_ALPHA_TRANSPARENCY)	? Alpha_trans :
								(g_alpha == DIALOG_ALPHA_CHANNEL)	? Alpha_channel :
								Alpha_none );
								
	if(!g_have_transparency)
	{
		sADMItem->Enable(Alpha_trans, FALSE);
		
		if(g_alpha == DIALOG_ALPHA_TRANSPARENCY)
		{
			if(g_alpha_name != NULL)
			{
				active_alpha = Alpha_channel;
				g_alpha = DIALOG_ALPHA_CHANNEL;
			}
			else
			{
				active_alpha = Alpha_none;
				g_alpha = DIALOG_ALPHA_NONE;
			}
		}
	}
	
	if(g_alpha_name == NULL)
	{
		sADMItem->Enable(Alpha_channel, FALSE);
		
		if(g_alpha == DIALOG_ALPHA_CHANNEL)
		{
			if(g_have_transparency)
			{
				active_alpha = Alpha_trans;
				g_alpha = DIALOG_ALPHA_TRANSPARENCY;
			}
			else
			{
				active_alpha = Alpha_none;
				g_alpha = DIALOG_ALPHA_NONE;
			}
		}
	}
	
	sADMItem->SetBooleanValue(active_alpha, TRUE);
	
	
	// checkboxes
	ADMRect Interlace_rect = INIT_RECT(INTERLACING_TOP, INTERLACING_LEFT, INTERLACING_BOTTOM, INTERLACING_RIGHT);
	ADMRect Metadata_rect = INIT_RECT(METADATA_TOP, METADATA_LEFT, METADATA_BOTTOM, METADATA_RIGHT);
	
	ADMItemRef Interlace = sADMItem->Create(dialog, OUT_Interlace_Check,
								kADMTextCheckBoxType, &Interlace_rect, NULL, NULL);

	ADMItemRef Metadata = sADMItem->Create(dialog, OUT_Metadata_Check,
								kADMTextCheckBoxType, &Metadata_rect, NULL, NULL);
	
	sADMItem->SetText(Interlace, INTERLACING_TEXT);
	sADMItem->SetText(Metadata, METADATA_TEXT);
	
	sADMItem->SetNotifyProc(Interlace, InterlaceNotifyProc);
	sADMItem->SetNotifyProc(Metadata, MetadataNotifyProc);
	
	sADMItem->SetBooleanValue(Interlace, g_interlace);
	sADMItem->SetBooleanValue(Metadata, g_metadata);
	
	
	return err;
}

static inline A_u_char GammaCorrect(A_u_char input)
{
#ifdef __PIMac__

	return input; // banner authored on a Mac, we must corrrect for Win

#else


#define MIN(A,B)			( (A) < (B) ? (A) : (B))
#define MAX(A,B)			( (A) > (B) ? (A) : (B))
#define AE8_RANGE(NUM)		(A_u_short)MIN( MAX( (NUM), 0 ), 255 )

#define AE8_TO_FLOAT(NUM)		( (float)(NUM) / 255.0f )
#define FLOAT_TO_AE8(NUM)		AE8_RANGE( ( (NUM) * 255.0f ) + 0.5f)

#define CORRECT_GAMMA (1.8f / 2.2f)
#define GAMMA_CORRECT(NUM)	FLOAT_TO_AE8( pow( AE8_TO_FLOAT( NUM ), CORRECT_GAMMA ) )

	return GAMMA_CORRECT(input);
	
#endif
}


typedef struct {
	unsigned char	alpha, red, green, blue;
} PF_Pixel;

void
CreateImage(void)
{
	char pixel_buf[] =
#include "SuperPNG_banner.h"
	
	g_image = sADMImage->Create(PICTURE_WIDTH, PICTURE_HEIGHT, 0);
	ASBytePtr image_data =  sADMImage->BeginBaseAddressAccess(g_image);
	ASInt32 rowbytes = sADMImage->GetByteWidth(g_image);
	
	A_u_char *row = image_data;
	A_u_char *buf = (A_u_char *)pixel_buf;
	
	PF_Pixel temp;
	
	for(int y=0; y<PICTURE_HEIGHT; y++)
	{
		A_u_char *pix = row;
		
		for(int x=0; x<PICTURE_WIDTH; x++)
		{
			temp.alpha = *buf++; // alpha (ignored)
			temp.red   = GammaCorrect( *buf++ ); // red
			temp.green = GammaCorrect( *buf++ ); // green
			temp.blue  = GammaCorrect( *buf++ ); // blue

		#ifdef __PIMac__
			*pix++ = temp.alpha;
			*pix++ = temp.red;
			*pix++ = temp.green;
			*pix++ = temp.blue;
		#else
			*pix++ = temp.blue;
			*pix++ = temp.green;
			*pix++ = temp.red;
			*pix++ = temp.alpha;
		#endif
		}
		
		row += rowbytes;
	}
	
	sADMImage->EndBaseAddressAccess(g_image);
}

	
bool
SuperPNG_OutUI(
	SuperPNG_OutUI_Data	*params,
	bool				isRGB8,
	bool				have_transparency,
	const char			*alpha_name,
	const void			*plugHndl,
	const void			*mwnd)
{
	// global globals
	GPtr globals = (GPtr)mwnd;
	
	g_compression	= params->compression;
	g_interlace		= params->interlace;
	g_metadata		= params->metadata;
	g_alpha			= params->alpha;
	
	g_have_transparency = have_transparency;
	g_alpha_name = alpha_name;
		

	// create banner image
	CreateImage();

	
	ASInt32 item = sADMDialog->Modal((SPPluginRef)gStuff->plugInRef, DIALOG_TITLE, NULL,
							kADMModalDialogStyle, DialogInit, NULL);
	

	// kill image
	if(g_image)
	{
		sADMImage->Destroy(g_image);
		g_image = NULL;
	}
	
	// release suites
	UnloadSuites();

	if(item == OUT_OK)
	{
		params->compression		= g_compression;
		params->interlace		= g_interlace;
		params->metadata		= g_metadata; 
		params->alpha			= g_alpha;
		
		return true;
	}		
	else
		return false;
}


#pragma mark-


// About dialog functions

#define ABOUT_TITLE	"About SuperPNG"

#define ABOUT_WIDTH		350
#define ABOUT_HEIGHT	210

#define ABOUT_OK_TOP		(ABOUT_HEIGHT - (DIALOG_MARGIN + BUTTON_HEIGHT) )
#define ABOUT_OK_BOTTOM		(ABOUT_OK_TOP + BUTTON_HEIGHT)
#define ABOUT_OK_LEFT		( (ABOUT_WIDTH - BUTTON_WIDTH) / 2 )
#define ABOUT_OK_RIGHT		(ABOUT_OK_LEFT + BUTTON_WIDTH)

#define ABOUT_PICTURE_TOP		(DIALOG_MARGIN - PICTURE_NUDGE_UP)
#define ABOUT_PICTURE_BOTTOM	(ABOUT_PICTURE_TOP + PICTURE_HEIGHT)
#define ABOUT_PICTURE_LEFT		( (ABOUT_WIDTH - PICTURE_WIDTH) / 2 )
#define ABOUT_PICTURE_RIGHT		(ABOUT_PICTURE_LEFT + PICTURE_WIDTH)

#define ABOUT_TEXT_TOP		(PICTURE_BOTTOM + DIALOG_MARGIN)
#define ABOUT_TEXT_BOTTOM	(ABOUT_TEXT_TOP + 40)
#define ABOUT_TEXT_LEFT		(DIALOG_MARGIN + DIALOG_MARGIN)
#define ABOUT_TEXT_RIGHT	(ABOUT_WIDTH - ABOUT_TEXT_LEFT)
#define ABOUT_TEXT_TEXT		"SuperPNG\n" \
							"by Brendan Bolles"

#define SMALL_TEXT_TOP		ABOUT_TEXT_BOTTOM
#define SMALL_TEXT_BOTTOM	(SMALL_TEXT_TOP + 60)
#define SMALL_TEXT_LEFT		ABOUT_TEXT_LEFT
#define SMALL_TEXT_RIGHT	ABOUT_TEXT_RIGHT
#define SMALL_TEXT_TEXT		"v" SuperPNG_Version_String "\n " \
							__DATE__

// dialog controls
enum {
	ABOUT_noUI = -1,
	ABOUT_OK = 1,
	ABOUT_Cancel, // not used
	ABOUT_Picture,
	ABOUT_Text,
	ABOUT_Small_Text
};

static ASErr ASAPI AboutInit(ADMDialogRef dialog)
{
	ASErr err = kSPNoError;

	// set the dialog's size and name
	sADMDialog->Size(dialog, ABOUT_WIDTH, ABOUT_HEIGHT);
	sADMDialog->SetText(dialog, ABOUT_TITLE);
	
	
	// create the OK button
	ADMRect OK_rect = INIT_RECT(ABOUT_OK_TOP, ABOUT_OK_LEFT, ABOUT_OK_BOTTOM, ABOUT_OK_RIGHT );
	
	ADMItemRef OK_button = sADMItem->Create(dialog, ABOUT_OK,
								kADMTextPushButtonType, &OK_rect, NULL, NULL);
	
	sADMItem->SetText(OK_button, OK_TEXT);
	

	// picture
	ADMRect Picure_rect = INIT_RECT(ABOUT_PICTURE_TOP, ABOUT_PICTURE_LEFT, ABOUT_PICTURE_BOTTOM, ABOUT_PICTURE_RIGHT);
	
	ADMItemRef Picture_item = sADMItem->Create(dialog, ABOUT_Picture,
								kADMPictureStaticType, &Picure_rect, NULL, NULL);
	
	sADMItem->SetDrawProc(Picture_item, DrawPictureProc);
	

	// create border text
	ADMRect About_text_rect = INIT_RECT(ABOUT_TEXT_TOP, ABOUT_TEXT_LEFT, ABOUT_TEXT_BOTTOM, ABOUT_TEXT_RIGHT);

	ADMItemRef About_text = sADMItem->Create(dialog, ABOUT_Text,
								kADMTextStaticType, &About_text_rect, NULL, NULL);

	sADMItem->SetText(About_text, ABOUT_TEXT_TEXT);
	sADMItem->SetJustify(About_text, kADMCenterJustify);
	//sADMItem->SetFont(About_text, kADMPaletteFont);
	
	// create small text
	ADMRect Small_text_rect = INIT_RECT(SMALL_TEXT_TOP, SMALL_TEXT_LEFT, SMALL_TEXT_BOTTOM, SMALL_TEXT_RIGHT);

	ADMItemRef Small_text = sADMItem->Create(dialog, ABOUT_Small_Text,
								kADMTextStaticType, &Small_text_rect, NULL, NULL);

	sADMItem->SetText(Small_text, SMALL_TEXT_TEXT);
	sADMItem->SetJustify(Small_text, kADMCenterJustify);
	sADMItem->SetFont(Small_text, kADMPaletteFont);
	
	return err;
}


void
SuperPNG_About(
	const void		*plugHndl,
	const void		*mwnd)
{
	AboutRecordPtr about = (AboutRecordPtr)mwnd;

	// create banner image
	CreateImage();
	
	ASInt32 item = sADMDialog->Modal((SPPluginRef)about->plugInRef, ABOUT_TITLE, NULL,
							kADMModalDialogStyle, AboutInit, NULL);
	
	// kill image
	if(g_image)
	{
		sADMImage->Destroy(g_image);
		g_image = NULL;
	}

	// release suites
	UnloadSuites();
}


#endif // !__LP64__
