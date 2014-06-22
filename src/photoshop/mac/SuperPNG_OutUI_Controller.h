
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


#import <Cocoa/Cocoa.h>

#include "SuperPNG_UI.h"

@interface SuperPNG_OutUI_Controller : NSObject {
	IBOutlet NSWindow *theWindow;
	IBOutlet NSMatrix *compressionMatrix;
	IBOutlet NSButton *quantizeCheckbox;
	IBOutlet NSSlider *quantizeSlider;
	IBOutlet NSTextField *sliderLabel;
	IBOutlet NSMatrix *alphaMatrix;
	IBOutlet NSButton *cleanTransparentCheckbox;
	IBOutlet NSButton *interlaceCheckbox;
	IBOutlet NSButton *metadataCheckbox;
}
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

- (IBAction)clickedOK:(id)sender;
- (IBAction)clickedCancel:(id)sender;

- (IBAction)trackAlpha:(id)sender;
- (IBAction)trackQuantize:(id)sender;

- (NSWindow *)getWindow;

- (DialogCompression)getCompression;
- (BOOL)getQuantize;
- (NSInteger)getQuantizeQuality;
- (DialogAlpha)getAlpha;
- (BOOL)getCleanTransparent;
- (BOOL)getInterlace;
- (BOOL)getMetadata;

@end
