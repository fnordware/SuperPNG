
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


#include "SuperPNG_UI.h"

#import "SuperPNG_InUI_Controller.h"
#import "SuperPNG_OutUI_Controller.h"
#import "SuperPNG_About_Controller.h"

#include "SuperPNG_version.h"

// ==========
// Only building this on 64-bit (Cocoa) architectures
// ==========
#if __LP64__


bool
SuperPNG_InUI(
	SuperPNG_InUI_Data	*params,
	const void			*plugHndl,
	const void			*mwnd)
{
	bool result = true;
	
	params->alpha = DIALOG_ALPHA_TRANSPARENCY;
	params->mult = false;
	
	// get the prefs
	BOOL always_dialog = FALSE;
	
	CFPropertyListRef alphaMode_val = CFPreferencesCopyAppValue(CFSTR(SUPERPNG_PREFS_ALPHA), CFSTR(SUPERPNG_PREFS_ID));
	CFPropertyListRef mult_val = CFPreferencesCopyAppValue(CFSTR(SUPERPNG_PREFS_MULT), CFSTR(SUPERPNG_PREFS_ID));
	CFPropertyListRef always_val = CFPreferencesCopyAppValue(CFSTR(SUPERPNG_PREFS_ALWAYS), CFSTR(SUPERPNG_PREFS_ID));

	if(alphaMode_val)
	{
	   char alphaMode_char;
		
		if( CFNumberGetValue((CFNumberRef)alphaMode_val, kCFNumberCharType, &alphaMode_char) )
		{
			params->alpha = (DialogAlpha)alphaMode_char;
		}
		
		CFRelease(alphaMode_val);
	}

	if(mult_val)
	{
		params->mult = CFBooleanGetValue((CFBooleanRef)mult_val);
		
		CFRelease(mult_val);
	}

	if(always_val)
	{
		always_dialog = CFBooleanGetValue((CFBooleanRef)always_val);
		
		CFRelease(always_val);
	}
	
	// user can force dialog open by holding shift or option
	const NSUInteger flags = [[NSApp currentEvent] modifierFlags];
	const bool shift_key = ( (flags & NSShiftKeyMask) || (flags & NSAlternateKeyMask) );

	if(always_dialog || shift_key)
	{
		NSString *bundle_id = [NSString stringWithUTF8String:(const char *)plugHndl];

		Class ui_controller_class = [[NSBundle bundleWithIdentifier:bundle_id]
										classNamed:@"SuperPNG_InUI_Controller"];

		if(ui_controller_class)
		{
			SuperPNG_InUI_Controller *ui_controller = [[ui_controller_class alloc] init:params->alpha
														multiply:params->mult
														alwaysDialog:always_dialog];
			
			if(ui_controller)
			{
				NSWindow *my_window = [ui_controller getWindow];
				
				if(my_window)
				{
					NSInteger modal_result = [NSApp runModalForWindow:my_window];
					
					if(modal_result == NSRunStoppedResponse)
					{
						params->alpha	= [ui_controller getAlpha];
						params->mult	= [ui_controller getMult];
						always_dialog	= [ui_controller getAlways];
						
						// record the always pref every time
						CFBooleanRef always =  (always_dialog ? kCFBooleanTrue : kCFBooleanFalse);
						CFPreferencesSetAppValue(CFSTR(SUPERPNG_PREFS_ALWAYS), always, CFSTR(SUPERPNG_PREFS_ID));
						CFPreferencesAppSynchronize(CFSTR(SUPERPNG_PREFS_ID));
						CFRelease(always);
						
						result = true;
					}
					else
						result = false;
						
					[my_window close];
				}

				[ui_controller release];
			}
		}
	}


	return result;
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
	bool result = false;

	NSString *bundle_id = [NSString stringWithUTF8String:(const char *)plugHndl];

	Class ui_controller_class = [[NSBundle bundleWithIdentifier:bundle_id]
									classNamed:@"SuperPNG_OutUI_Controller"];

	if(ui_controller_class)
	{
		SuperPNG_OutUI_Controller *ui_controller = [[ui_controller_class alloc] init:params->compression
														quantize:params->quantize
														quantQuality:params->quantize_quality
														alpha:params->alpha
														clean_transparent:params->clean_transparent
														interlace:params->interlace
														metadata:params->metadata
														isRGB8:isRGB8
														have_transparency:have_transparency
														alpha_name:alpha_name];
		if(ui_controller)
		{
			NSWindow *my_window = [ui_controller getWindow];
			
			if(my_window)
			{
				NSInteger modal_result = [NSApp runModalForWindow:my_window];
				
				if(modal_result == NSRunStoppedResponse)
				{
					params->compression			= [ui_controller getCompression];
					params->quantize			= [ui_controller getQuantize];
					params->quantize_quality	= [ui_controller getQuantizeQuality];
					params->alpha				= [ui_controller getAlpha];
					params->clean_transparent	= [ui_controller getCleanTransparent];
					params->interlace			= [ui_controller getInterlace];
					params->metadata			= [ui_controller getMetadata];
					
					result = true;
				}
				else
					result = false;
					
				[my_window close];
			}
			
			[ui_controller release];
		}
	}
	
	// don't release when created with stringWithUTF8String
	//[bundle_id release];
	
	return result;
}


void
SuperPNG_About(
	const void		*plugHndl,
	const void		*mwnd)
{
	NSString *bundle_id = [NSString stringWithUTF8String:(const char *)plugHndl];

	Class about_controller_class = [[NSBundle bundleWithIdentifier:bundle_id]
									classNamed:@"SuperPNG_About_Controller"];
	
	if(about_controller_class)
	{
		SuperPNG_About_Controller *about_controller = [[about_controller_class alloc]
														init:"v" SuperPNG_Version_String " - " SuperPNG_Build_Date
														dummy:"you dumb"];
		
		if(about_controller)
		{
			NSWindow *the_window = [about_controller getWindow];
			
			if(the_window)
			{
				[NSApp runModalForWindow:the_window];
				
				[the_window close];
			}
			
			[about_controller release];
		}
	}
}

#endif // __LP64__
