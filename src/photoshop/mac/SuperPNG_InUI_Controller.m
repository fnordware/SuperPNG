
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


#import "SuperPNG_InUI_Controller.h"

@implementation SuperPNG_InUI_Controller

- (id)init:(DialogAlpha)the_alpha
	multiply:(BOOL)mult
	alwaysDialog:(BOOL)always
{
	self = [super init];
	
	if(!([NSBundle loadNibNamed:@"SuperPNG_InUI" owner:self]))
		return nil;
	
	if(the_alpha == DIALOG_ALPHA_TRANSPARENCY)
	{
		[transparentRadio setState:NSOnState];
		[channelRadio setState:NSOffState];
		[multCheckbox setEnabled:FALSE];
	}
	else
	{
		[transparentRadio setState:NSOffState];
		[channelRadio setState:NSOnState];
		[multCheckbox setEnabled:TRUE];
	}
	
	
	[multCheckbox setState:(mult ? NSOnState : NSOffState)];
	[alwaysCheckbox setState:(always ? NSOnState : NSOffState)];
	
	[theWindow center];
	
	return self;
}

- (IBAction)clickedOK:(id)sender {
	[NSApp stopModal];
}

- (IBAction)clickedCancel:(id)sender {
    [NSApp abortModal];
}

- (IBAction)clickedSetDefaults:(id)sender {
	char alphaMode_char = (([transparentRadio state] == NSOnState) ? DIALOG_ALPHA_TRANSPARENCY : DIALOG_ALPHA_CHANNEL);
	CFNumberRef alphaMode = CFNumberCreate(kCFAllocatorDefault, kCFNumberCharType, &alphaMode_char);
	CFBooleanRef mult =  (([multCheckbox state] == NSOnState) ? kCFBooleanTrue : kCFBooleanFalse);
	CFBooleanRef always =  (([alwaysCheckbox state] == NSOnState) ? kCFBooleanTrue : kCFBooleanFalse);
	
	CFPreferencesSetAppValue(CFSTR(SUPERPNG_PREFS_ALPHA), alphaMode, CFSTR(SUPERPNG_PREFS_ID));
	CFPreferencesSetAppValue(CFSTR(SUPERPNG_PREFS_MULT), mult, CFSTR(SUPERPNG_PREFS_ID));
	CFPreferencesSetAppValue(CFSTR(SUPERPNG_PREFS_ALWAYS), always, CFSTR(SUPERPNG_PREFS_ID));
	
	CFPreferencesAppSynchronize(CFSTR(SUPERPNG_PREFS_ID));
	
	CFRelease(alphaMode);
	CFRelease(mult);
	CFRelease(always);
}

- (IBAction)trackAlpha:(id)sender {
	if(sender == transparentRadio)
	{
		[channelRadio setState:NSOffState];
		[multCheckbox setEnabled:FALSE];
	}
		
	if(sender == channelRadio)
	{
		[transparentRadio setState:NSOffState];
		[multCheckbox setEnabled:TRUE];
	}
}

- (NSWindow *)getWindow {
	return theWindow;
}

- (DialogAlpha)getAlpha {
	if([transparentRadio state] == NSOnState)
		return DIALOG_ALPHA_TRANSPARENCY;
	else if([channelRadio state] == NSOnState)
		return DIALOG_ALPHA_CHANNEL;
	else
		return DIALOG_ALPHA_TRANSPARENCY;
}

- (BOOL)getMult {
	return ([multCheckbox state] == NSOnState);
}

- (BOOL)getAlways {
	return ([alwaysCheckbox state] == NSOnState);
}

@end
