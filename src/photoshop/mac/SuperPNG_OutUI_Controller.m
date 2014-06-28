
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


#import "SuperPNG_OutUI_Controller.h"

@implementation SuperPNG_OutUI_Controller

- (id)init:(DialogCompression)compression
	quantize:(BOOL)quantize
	quantQuality:(NSInteger)quantQuality
	alpha:(DialogAlpha)the_alpha
	clean_transparent:(BOOL)clean_transparent
	interlace:(BOOL)do_interlace
	metadata:(BOOL)do_metadata
	isRGB8:(BOOL)isRGB8
	have_transparency:(BOOL)has_transparency
	alpha_name:(const char *)alphaName;
{
	self = [super init];
	
	if(!([NSBundle loadNibNamed:@"SuperPNG_OutUI" owner:self]))
		return nil;

	[compressionMatrix selectCellAtRow:0 column:(NSInteger)compression];
	[quantizeCheckbox setState:(quantize ? NSOnState : NSOffState)];
	[quantizeSlider setIntValue:quantQuality];
	[cleanTransparentCheckbox setState:(clean_transparent ? NSOnState : NSOffState)];
	[interlaceCheckbox setState:(do_interlace ? NSOnState : NSOffState)];
	[metadataCheckbox setState:(do_metadata ? NSOnState : NSOffState)];
	
	if(!isRGB8)
	{
		[quantizeCheckbox setState:NSOffState];
		[quantizeCheckbox setEnabled:FALSE];
	}
	
	[self trackQuantize:self];
	[self trackQuantQuality:self];
	
	if(!has_transparency)
	{
		[[alphaMatrix cellAtRow:1 column:0] setEnabled:FALSE];
		
		if(the_alpha == DIALOG_ALPHA_TRANSPARENCY)
		{
			the_alpha = (alphaName ? DIALOG_ALPHA_CHANNEL : DIALOG_ALPHA_NONE);
		}
	}
	
	if(alphaName)
	{
		[[alphaMatrix cellAtRow:2 column:0] setTitle:[NSString stringWithUTF8String:alphaName]];
	}
	else
	{
		[[alphaMatrix cellAtRow:2 column:0] setEnabled:FALSE];
		
		if(the_alpha == DIALOG_ALPHA_CHANNEL)
		{
			the_alpha = (has_transparency ? DIALOG_ALPHA_TRANSPARENCY : DIALOG_ALPHA_NONE);
		}
	}

	[alphaMatrix selectCellAtRow:(NSInteger)the_alpha column:0];

	[self trackAlpha:self];
	

	[theWindow center];
	
	theResult = DIALOG_RESULT_CONTINUE;
	
	return self;
}

- (IBAction)clickedOK:(id)sender {
	theResult = DIALOG_RESULT_OK;
}

- (IBAction)clickedCancel:(id)sender {
    theResult = DIALOG_RESULT_CANCEL;
}

- (DialogResult)getResult {
	return theResult;
}

- (IBAction)trackAlpha:(id)sender {
	const BOOL enable_clean = ([self getAlpha] != DIALOG_ALPHA_NONE);
	
	[cleanTransparentCheckbox setEnabled:enable_clean];
}

- (IBAction)trackQuantize:(id)sender {
	const BOOL enabled = [self getQuantize];

	[quantizeSlider setEnabled:enabled];
	
	NSColor *label_color = (enabled ? [NSColor textColor] : [NSColor disabledControlTextColor]);
	
	[sliderLabel setTextColor:label_color];
	
	[label_color release];
}

- (IBAction)trackQuantQuality:(id)sender {
	const NSInteger quality = [self getQuantizeQuality];

	NSString *quality_string = (quality == 100 ? @"Highest Quality" :
								quality > 75 ? @"High Quality" :
                                quality > 50 ? @"Medium Quality" :
								quality > 25 ? @"Low Quality" :
								@"Lowest Quality"
								);
										
	[sliderLabel setStringValue:quality_string];
}

- (NSWindow *)getWindow {
	return theWindow;
}

- (DialogCompression)getCompression {
	switch([compressionMatrix selectedColumn])
	{
		case 0:		return DIALOG_COMPRESSION_NONE;
		case 1:		return DIALOG_COMPRESSION_LOW;
		case 2:		return DIALOG_COMPRESSION_NORMAL;
		case 3:		return DIALOG_COMPRESSION_HIGH;
		default:	return DIALOG_COMPRESSION_HIGH;
	}
}

- (BOOL)getQuantize {
	return ([quantizeCheckbox state] == NSOnState);
}

- (NSInteger)getQuantizeQuality {
	return [quantizeSlider intValue];
}

- (DialogAlpha)getAlpha {
	switch([alphaMatrix selectedRow])
	{
		case 0:		return DIALOG_ALPHA_NONE;
		case 1:		return DIALOG_ALPHA_TRANSPARENCY;
		case 2:		return DIALOG_ALPHA_CHANNEL;
		default:	return DIALOG_ALPHA_NONE;
	}
}

- (BOOL)getCleanTransparent {
	return ([cleanTransparentCheckbox state] == NSOnState);
}

- (BOOL)getInterlace {
	return ([interlaceCheckbox state] == NSOnState);
}

- (BOOL)getMetadata {
	return ([metadataCheckbox state] == NSOnState);
}

@end
