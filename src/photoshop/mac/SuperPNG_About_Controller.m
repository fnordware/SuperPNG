//
//  SuperPNG_About_Controller.m
//
//  Created by Brendan Bolles on 10/9/11.
//  Copyright 2011 fnord. All rights reserved.
//

#import "SuperPNG_About_Controller.h"

@implementation SuperPNG_About_Controller

- (id)init:(const char *)version_string
	dummy:(const char *)dummy_string;
{
	self = [super init];
	
	if(!([NSBundle loadNibNamed:@"SuperPNG_About" owner:self]))
		return nil;

	[versionString setStringValue:[NSString stringWithUTF8String:version_string]];
	
	[theWindow center];
	
	return self;
}

- (IBAction)clickedOK:(id)sender {
    [NSApp stopModal];
}

- (NSWindow *)getWindow {
	return theWindow;
}

@end
