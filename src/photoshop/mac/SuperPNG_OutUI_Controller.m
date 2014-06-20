//
//  SuperPNG_OutUI_Controller.m
//
//  Created by Brendan Bolles on 10/9/11.
//  Copyright 2011 fnord. All rights reserved.
//

#import "SuperPNG_OutUI_Controller.h"

@implementation SuperPNG_OutUI_Controller

- (id)init:(DialogCompression)compression
	alpha:(DialogAlpha)the_alpha
	interlace:(BOOL)do_interlace
	metadata:(BOOL)do_metadata
	have_transparency:(BOOL)has_transparency
	alpha_name:(const char *)alphaName;
{
	self = [super init];
	
	if(!([NSBundle loadNibNamed:@"SuperPNG_OutUI" owner:self]))
		return nil;

	[compressionMatrix selectCellAtRow:0 column:(NSInteger)compression];
	[interlaceCheckbox setState:(do_interlace ? NSOnState : NSOffState)];
	[metadataCheckbox setState:(do_metadata ? NSOnState : NSOffState)];
	
	
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


	[theWindow center];
	
	return self;
}

- (IBAction)clickedOK:(id)sender {
	[NSApp stopModal];
}

- (IBAction)clickedCancel:(id)sender {
    [NSApp abortModal];
}

- (NSWindow *)getWindow {
	return theWindow;
}

- (DialogCompression)getCompression {
	if([[compressionMatrix cellAtRow:0 column:0] state] == NSOnState)
		return DIALOG_COMPRESSION_NONE;
	else if([[compressionMatrix cellAtRow:0 column:1] state] == NSOnState)
		return DIALOG_COMPRESSION_LOW;
	else if([[compressionMatrix cellAtRow:0 column:2] state] == NSOnState)
		return DIALOG_COMPRESSION_NORMAL;
	else if([[compressionMatrix cellAtRow:0 column:3] state] == NSOnState)
		return DIALOG_COMPRESSION_HIGH;
	else
		return DIALOG_COMPRESSION_NORMAL;
}

- (DialogAlpha)getAlpha {
	// got to be a better way to do this, right?
	if([[alphaMatrix cellAtRow:0 column:0] state] == NSOnState)
		return DIALOG_ALPHA_NONE;
	else if([[alphaMatrix cellAtRow:1 column:0] state] == NSOnState)
		return DIALOG_ALPHA_TRANSPARENCY;
	else if([[alphaMatrix cellAtRow:2 column:0] state] == NSOnState)
		return DIALOG_ALPHA_CHANNEL;
	else
		return DIALOG_ALPHA_NONE;
}

- (BOOL)getInterlace {
	return ([interlaceCheckbox state] == NSOnState);
}

- (BOOL)getMetadata {
	return ([metadataCheckbox state] == NSOnState);
}

@end
