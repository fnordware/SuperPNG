//
//  SuperPNG_InUI_Controller.h
//
//  Created by Brendan Bolles on 10/9/11.
//  Copyright 2011 fnord. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "SuperPNG_UI.h"


@interface SuperPNG_InUI_Controller : NSObject {
	IBOutlet NSWindow *theWindow;
	IBOutlet NSButton *transparentRadio;
	IBOutlet NSButton *channelRadio;
	IBOutlet NSButton *multCheckbox;
	IBOutlet NSButton *alwaysCheckbox;
}
- (id)init:(DialogAlpha)the_alpha
	multiply:(BOOL)mult
	alwaysDialog:(BOOL)always;

- (IBAction)clickedOK:(id)sender;
- (IBAction)clickedCancel:(id)sender;
- (IBAction)clickedSetDefaults:(id)sender;

- (IBAction)trackAlpha:(id)sender;

- (NSWindow *)getWindow;

- (DialogAlpha)getAlpha;
- (BOOL)getMult;
- (BOOL)getAlways;
@end
