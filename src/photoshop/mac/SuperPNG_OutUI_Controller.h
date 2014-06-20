//
//  SuperPNG_OutUI_Controller.h
//
//  Created by Brendan Bolles on 10/9/11.
//  Copyright 2011 fnord. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include "SuperPNG_UI.h"

@interface SuperPNG_OutUI_Controller : NSObject {
	IBOutlet NSWindow *theWindow;
	IBOutlet NSMatrix *compressionMatrix;
	IBOutlet NSMatrix *alphaMatrix;
	IBOutlet NSButton *interlaceCheckbox;
	IBOutlet NSButton *metadataCheckbox;
}
- (id)init:(DialogCompression)compression
	alpha:(DialogAlpha)the_alpha
	interlace:(BOOL)do_interlace
	metadata:(BOOL)do_metadata
	have_transparency:(BOOL)has_transparency
	alpha_name:(const char *)alphaName;

- (IBAction)clickedOK:(id)sender;
- (IBAction)clickedCancel:(id)sender;

- (NSWindow *)getWindow;

- (DialogCompression)getCompression;
- (DialogAlpha)getAlpha;
- (BOOL)getInterlace;
- (BOOL)getMetadata;

@end
