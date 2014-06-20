//
//  SuperPNG_About_Controller.h
//
//  Created by Brendan Bolles on 10/9/11.
//  Copyright 2011 fnord. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface SuperPNG_About_Controller : NSObject {
	IBOutlet NSWindow *theWindow;
	IBOutlet NSTextField *versionString;
}

- (id)init:(const char *)version_string
	dummy:(const char *)dummy_string;

- (IBAction)clickedOK:(id)sender;

- (NSWindow *)getWindow;

@end
