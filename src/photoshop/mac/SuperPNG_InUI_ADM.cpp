
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


#if TARGET_API_MAC_CARBON
#include "CFPreferences.h"
#include "CFNumber.h"
#endif

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
	IN_noUI = -1,
	IN_OK = 1,
	IN_Cancel,
	IN_Set_Defaults,
	IN_Picture,
	IN_Transparency_Radio,
	IN_Channel_Radio,
	IN_Multiply_Check,
	IN_Always_Appear_Check
};



#define DIALOG_TITLE	"SuperPNG Input Options"

#define DIALOG_WIDTH	375
#define DIALOG_HEIGHT	250


#define BUTTON_WIDTH	80
#define BUTTON_HEIGHT	20

#define RADIO_WIDTH		250
#define RADIO_HEIGHT	20

#define CHECKBOX_WIDTH	250
#define CHECKBOX_HEIGHT	20
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

#define SET_DEFAULTS_TOP	OK_TOP
#define SET_DEFAULTS_BOTTOM	OK_BOTTOM
#define SET_DEFAULTS_LEFT	DIALOG_MARGIN
#define SET_DEFAULTS_RIGHT	(SET_DEFAULTS_LEFT + BUTTON_WIDTH + ITEM_SPACE + ITEM_SPACE)
#define SET_DEFAULTS_TEXT	"Set Defaults"

#define PICTURE_WIDTH		325
#define PICTURE_HEIGHT		40

#define PICTURE_NUDGE_UP	10

#define PICTURE_TOP		(DIALOG_MARGIN - PICTURE_NUDGE_UP)
#define PICTURE_BOTTOM	(PICTURE_TOP + PICTURE_HEIGHT)
#define PICTURE_LEFT	( (DIALOG_WIDTH - PICTURE_WIDTH) / 2 )
#define PICTURE_RIGHT	(PICTURE_LEFT + PICTURE_WIDTH)

#define TRANS_RADIO_TOP		(PICTURE_BOTTOM + ITEM_SPACE)
#define TRANS_RADIO_BOTTOM	(TRANS_RADIO_TOP + RADIO_HEIGHT)
#define TRANS_RADIO_LEFT	(PICTURE_LEFT + ITEM_SPACE + ITEM_SPACE + ITEM_SPACE + ITEM_SPACE)
#define TRANS_RADIO_RIGHT	(TRANS_RADIO_LEFT + RADIO_WIDTH)
#define TRANS_RADIO_TEXT	"Alpha makes layer transparent"

#define CHANNEL_RADIO_TOP	(TRANS_RADIO_BOTTOM + ITEM_SPACE - 5)
#define CHANNEL_RADIO_BOTTOM (CHANNEL_RADIO_TOP + RADIO_HEIGHT)
#define CHANNEL_RADIO_LEFT	TRANS_RADIO_LEFT
#define CHANNEL_RADIO_RIGHT	TRANS_RADIO_RIGHT
#define CHANNEL_RADIO_TEXT	"Alpha appears as separate channel"

#define MULT_CHECK_TOP		(CHANNEL_RADIO_BOTTOM + ITEM_SPACE - 5)
#define MULT_CHECK_BOTTOM	(MULT_CHECK_TOP + CHECKBOX_HEIGHT)
#define MULT_CHECK_LEFT		(CHANNEL_RADIO_LEFT + ITEM_SPACE + ITEM_SPACE)
#define MULT_CHECK_RIGHT	(MULT_CHECK_LEFT + CHECKBOX_WIDTH)
#define MULT_CHECK_TEXT		"Multiply RGB by Alpha"

#define ALWAYS_CHECK_TOP	(MULT_CHECK_BOTTOM + ITEM_SPACE + ITEM_SPACE + ITEM_SPACE + ITEM_SPACE)
#define ALWAYS_CHECK_BOTTOM	(ALWAYS_CHECK_TOP + CHECKBOX_HEIGHT)
#define ALWAYS_CHECK_LEFT	MULT_CHECK_LEFT
#define ALWAYS_CHECK_RIGHT	(ALWAYS_CHECK_LEFT + CHECKBOX_WIDTH)
#define ALWAYS_CHECK_TEXT	"Always bring up this dialog"


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



static DialogAlpha			g_alpha = DIALOG_ALPHA_TRANSPARENCY;
static bool					g_mult = true;
static bool					g_always = false;


// found in the other ADM file
extern ADMImageRef			g_image;
void CreateImage();


static void SetImportPrefs()
{
#if TARGET_API_MAC_CARBON
	CFStringRef prefs_id = CFStringCreateWithCString(NULL, SUPERPNG_PREFS_ID, kCFStringEncodingASCII);
	
	CFStringRef alpha_id = CFStringCreateWithCString(NULL, SUPERPNG_PREFS_ALPHA, kCFStringEncodingASCII);
	CFStringRef mult_id = CFStringCreateWithCString(NULL, SUPERPNG_PREFS_MULT, kCFStringEncodingASCII);
	
	char alphaMode_char = g_alpha;
	
	CFNumberRef alphaMode = CFNumberCreate(kCFAllocatorDefault, kCFNumberCharType, &alphaMode_char);
	CFBooleanRef mult =  g_mult ? kCFBooleanTrue : kCFBooleanFalse;
	
	CFPreferencesSetAppValue(alpha_id, alphaMode, prefs_id);
	CFPreferencesSetAppValue(mult_id, mult, prefs_id);
	
	CFPreferencesAppSynchronize(prefs_id);
	
	CFRelease(alphaMode);
	CFRelease(mult);
	
	CFRelease(alpha_id);
	CFRelease(mult_id);
	
	CFRelease(prefs_id);
#endif
}

static void GetImportPrefs()
{
#if TARGET_API_MAC_CARBON
	CFStringRef prefs_id = CFStringCreateWithCString(NULL, SUPERPNG_PREFS_ID, kCFStringEncodingASCII);
	
	CFStringRef alpha_id = CFStringCreateWithCString(NULL, SUPERPNG_PREFS_ALPHA, kCFStringEncodingASCII);
	CFStringRef mult_id = CFStringCreateWithCString(NULL, SUPERPNG_PREFS_MULT, kCFStringEncodingASCII);

	CFPropertyListRef alphaMode_val = CFPreferencesCopyAppValue(alpha_id, prefs_id);
	CFPropertyListRef mult_val = CFPreferencesCopyAppValue(mult_id, prefs_id);
	
	if(alphaMode_val)
	{
	   char alphaMode_char;
		
		if( CFNumberGetValue((CFNumberRef)alphaMode_val, kCFNumberCharType, &alphaMode_char) )
			g_alpha = (DialogAlpha)alphaMode_char;
		
		CFRelease(alphaMode_val);
	}
	
	if(mult_val)
	{
		g_mult = CFBooleanGetValue((CFBooleanRef)mult_val);
		
		CFRelease(mult_val);
	}

	CFRelease(alpha_id);
	CFRelease(mult_id);
	
	CFRelease(prefs_id);
#endif
}

static void SetAlways()
{
#if TARGET_API_MAC_CARBON
	CFStringRef prefs_id = CFStringCreateWithCString(NULL, SUPERPNG_PREFS_ID, kCFStringEncodingASCII);
	
	CFStringRef always_id = CFStringCreateWithCString(NULL, SUPERPNG_PREFS_ALWAYS, kCFStringEncodingASCII);


	CFBooleanRef always =  g_always ? kCFBooleanTrue : kCFBooleanFalse;
	
	CFPreferencesSetAppValue(always_id, always, prefs_id);
	
	CFPreferencesAppSynchronize(prefs_id);
	
	CFRelease(always);

	CFRelease(always_id);
	
	CFRelease(prefs_id);
#endif
}

static void GetAlways()
{
#if TARGET_API_MAC_CARBON
	CFStringRef prefs_id = CFStringCreateWithCString(NULL, SUPERPNG_PREFS_ID, kCFStringEncodingASCII);
	
	CFStringRef always_id = CFStringCreateWithCString(NULL, SUPERPNG_PREFS_ALWAYS, kCFStringEncodingASCII);

	CFPropertyListRef always_val = CFPreferencesCopyAppValue(always_id, prefs_id);
	
	if(always_val)
	{
		g_always = CFBooleanGetValue((CFBooleanRef)always_val);
		
		CFRelease(always_val);
	}
	
	CFRelease(always_id);
	
	CFRelease(prefs_id);
#endif
}

static bool ForceOpen()
{
#if TARGET_API_MAC_CARBON
    //UInt32 keys = GetCurrentEventKeyModifiers();
	UInt32 keys = GetCurrentKeyModifiers();
	
	return ( (keys & shiftKey) || (keys & rightShiftKey) || (keys & optionKey) || (keys & rightOptionKey) );
#else
	return false;
#endif
}

static void ASAPI
PrefsNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	sADMItem->DefaultNotify(item, notifier);
		
	if(sADMNotify->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		SetImportPrefs();
		SetAlways();
	}
}


static void ASAPI
RadioNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	sADMItem->DefaultNotify(item, notifier);
		
	if(sADMNotify->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		if( sADMItem->GetBooleanValue(item) ) // this may be unnecessary
		{
			ADMDialogRef dialog = sADMItem->GetDialog(item);
			ADMItemRef Mult_check = sADMDialog->GetItem(dialog, IN_Multiply_Check);
			
			ASInt32 itemID = sADMItem->GetID(item);
			
			switch(itemID)
			{
				case IN_Transparency_Radio:
					g_alpha = DIALOG_ALPHA_TRANSPARENCY;
					sADMItem->Enable(Mult_check, FALSE);
					break;
				
				case IN_Channel_Radio:
					g_alpha = DIALOG_ALPHA_CHANNEL;
					sADMItem->Enable(Mult_check, TRUE);
					break;
			}
		}
	}
}


static void ASAPI
MultiplyNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	sADMItem->DefaultNotify(item, notifier);
		
	if(sADMNotify->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		g_mult = sADMItem->GetBooleanValue(item);
	}
}

static void ASAPI
AlwaysNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	sADMItem->DefaultNotify(item, notifier);
		
	if(sADMNotify->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		g_always = sADMItem->GetBooleanValue(item);
	}
}



static void ASAPI
DrawPictureProc(ADMItemRef item, ADMDrawerRef drawer)
{
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

	// set the dialog's size and name
	sADMDialog->Size(dialog, DIALOG_WIDTH, DIALOG_HEIGHT);
	sADMDialog->SetText(dialog, DIALOG_TITLE);
	
	
	// create the OK button
	ADMRect OK_rect = INIT_RECT(OK_TOP, OK_LEFT, OK_BOTTOM, OK_RIGHT);
	
	ADMItemRef OK_button = sADMItem->Create(dialog, IN_OK,
								kADMTextPushButtonType, &OK_rect, NULL, NULL);
	
	sADMItem->SetText(OK_button, OK_TEXT);
	

	// create the Cancel button
	ADMRect Cancel_rect = INIT_RECT(CANCEL_TOP, CANCEL_LEFT, CANCEL_BOTTOM, CANCEL_RIGHT);
	
	ADMItemRef Cancel_button = sADMItem->Create(dialog, IN_Cancel,
								kADMTextPushButtonType, &Cancel_rect, NULL, NULL);
	
	sADMItem->SetText(Cancel_button, CANCEL_TEXT);
	

	// create the Set Defaults button
	ADMRect Defaults_rect = INIT_RECT(SET_DEFAULTS_TOP, SET_DEFAULTS_LEFT, SET_DEFAULTS_BOTTOM, SET_DEFAULTS_RIGHT);
	
	ADMItemRef Defaults_button = sADMItem->Create(dialog, IN_Set_Defaults,
								kADMTextPushButtonType, &Defaults_rect, NULL, NULL);
	
	sADMItem->SetText(Defaults_button, SET_DEFAULTS_TEXT);

	sADMItem->SetNotifyProc(Defaults_button, PrefsNotifyProc);
	
	// picture
	ADMRect Picure_rect = INIT_RECT(PICTURE_TOP, PICTURE_LEFT, PICTURE_BOTTOM, PICTURE_RIGHT);
	
	ADMItemRef Picture_item = sADMItem->Create(dialog, IN_Picture,
								kADMPictureStaticType, &Picure_rect, NULL, NULL);
	
	sADMItem->SetDrawProc(Picture_item, DrawPictureProc);


	// radio buttons
	ADMRect Transparency_radio_rect = INIT_RECT(TRANS_RADIO_TOP, TRANS_RADIO_LEFT, TRANS_RADIO_BOTTOM, TRANS_RADIO_RIGHT);
	ADMRect Channel_radio_rect = INIT_RECT(CHANNEL_RADIO_TOP, CHANNEL_RADIO_LEFT, CHANNEL_RADIO_BOTTOM, CHANNEL_RADIO_RIGHT);
	
	ADMItemRef Transparency_radio = sADMItem->Create(dialog, IN_Transparency_Radio,
								kADMTextRadioButtonType, &Transparency_radio_rect, NULL, NULL);

	ADMItemRef Channel_radio = sADMItem->Create(dialog, IN_Channel_Radio,
								kADMTextRadioButtonType, &Channel_radio_rect, NULL, NULL);

	sADMItem->SetText(Transparency_radio, TRANS_RADIO_TEXT);
	sADMItem->SetText(Channel_radio, CHANNEL_RADIO_TEXT);
	
	sADMItem->SetNotifyProc(Transparency_radio, RadioNotifyProc);
	sADMItem->SetNotifyProc(Channel_radio, RadioNotifyProc);

	sADMItem->SetBooleanValue((g_alpha == DIALOG_ALPHA_CHANNEL ? Channel_radio : Transparency_radio), TRUE);
	
	
	// checkboxes
	ADMRect Mult_check_rect = INIT_RECT(MULT_CHECK_TOP, MULT_CHECK_LEFT, MULT_CHECK_BOTTOM, MULT_CHECK_RIGHT);
	ADMRect Always_check_rect = INIT_RECT(ALWAYS_CHECK_TOP, ALWAYS_CHECK_LEFT, ALWAYS_CHECK_BOTTOM, ALWAYS_CHECK_RIGHT);
	
	ADMItemRef Mult_check = sADMItem->Create(dialog, IN_Multiply_Check,
								kADMTextCheckBoxType, &Mult_check_rect, NULL, NULL);

	ADMItemRef Always_check = sADMItem->Create(dialog, IN_Always_Appear_Check,
								kADMTextCheckBoxType, &Always_check_rect, NULL, NULL);
	
	sADMItem->SetText(Mult_check, MULT_CHECK_TEXT);
	sADMItem->SetText(Always_check, ALWAYS_CHECK_TEXT);
	
	sADMItem->SetNotifyProc(Mult_check, MultiplyNotifyProc);
	sADMItem->SetNotifyProc(Always_check, AlwaysNotifyProc);
	
	sADMItem->SetBooleanValue(Mult_check, g_mult);
	sADMItem->SetBooleanValue(Always_check, g_always);
	
	if(g_alpha == DIALOG_ALPHA_TRANSPARENCY)
		sADMItem->Enable(Mult_check, FALSE);
	
	
	return err;
}

	
bool
SuperPNG_InUI(
	SuperPNG_InUI_Data	*params,
	const void			*plugHndl,
	const void			*mwnd)
{
	bool continue_reading = true;
	
	// global globals
	GPtr globals = (GPtr)mwnd;
	
	GetImportPrefs();
	GetAlways();
	
	if(g_always || ForceOpen())
	{
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

		if(item == IN_OK)
		{
			SetAlways();

			continue_reading = true;
		}		
		else
			continue_reading = false;
	}
	
	
	params->alpha	= g_alpha;
	params->mult	= g_mult;

	
	return continue_reading;
}


#endif // !__LP64__
