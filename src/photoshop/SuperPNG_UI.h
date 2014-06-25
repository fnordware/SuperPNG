
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


#ifndef SUPERPNG_UI_H
#define SUPERPNG_UI_H


typedef enum {
	DIALOG_COMPRESSION_NONE = 0,
	DIALOG_COMPRESSION_LOW,
	DIALOG_COMPRESSION_NORMAL,
	DIALOG_COMPRESSION_HIGH
} DialogCompression;

typedef enum {
	DIALOG_ALPHA_NONE = 0,
	DIALOG_ALPHA_TRANSPARENCY,
	DIALOG_ALPHA_CHANNEL
} DialogAlpha;


typedef struct {
	DialogAlpha		alpha;
	bool			mult;
} SuperPNG_InUI_Data;

typedef struct {
	DialogCompression	compression;
	bool				quantize;
	unsigned int		quantize_quality;
	bool				interlace;
	bool				metadata;
	DialogAlpha			alpha;
	bool				clean_transparent;
} SuperPNG_OutUI_Data;

// SuperPNG UI
//
// return true if user hit OK
// if user hit OK, params block will have been modified
//
// send in block of parameters, names for profile menu, and weather to show subsample menu
// plugHndl is bundle identifier string on Mac, hInstance on win
// mwnd is the main windowfor Windows, ADM pointers on Mac

bool
SuperPNG_InUI(
	SuperPNG_InUI_Data	*params,
	const void			*plugHndl,
	const void			*mwnd);

bool
SuperPNG_OutUI(
	SuperPNG_OutUI_Data	*params,
	bool				isRGB8,
	bool				have_transparency,
	const char			*alpha_name,
	const void			*plugHndl,
	const void			*mwnd);

void
SuperPNG_About(
	const void		*plugHndl,
	const void		*mwnd);


// Mac prefs keys
#define SUPERPNG_PREFS_ID		"com.fnordware.SuperPNG"
#define SUPERPNG_PREFS_ALPHA	"Alpha Mode"
#define SUPERPNG_PREFS_MULT		"Mult"
#define SUPERPNG_PREFS_ALWAYS	"Do Dialog"


// Windows registry keys
#define SUPERPNG_PREFIX		 "Software\\fnord\\SuperPNG"
#define SUPERPNG_ALPHA_KEY "Alpha"
#define SUPERPNG_MULT_KEY "Mult"
#define SUPERPNG_ALWAYS_KEY "Always"


#endif // SUPERPNG_UI_H
