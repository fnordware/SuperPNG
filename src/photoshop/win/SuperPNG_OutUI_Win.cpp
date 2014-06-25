
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


#include "SuperPNG.h"

#include "SuperPNG_UI.h"
#include "SuperPNG_Version.h"

#include <Windows.h>
#include <commctrl.h>

extern HINSTANCE hDllInstance;

enum {
	OUT_noUI = -1,
	OUT_OK = IDOK,
	OUT_Cancel = IDCANCEL,
	OUT_Picture,
	OUT_Compression_Radio_None,
	OUT_Compression_Radio_Low,
	OUT_Compression_Radio_Normal,
	OUT_Compression_Radio_High,
	OUT_Quantize_Check,
	OUT_Quantize_Quality,
	OUT_Quantize_Label,
	OUT_Interlacing_Check,
	OUT_Metadata_Check,
	OUT_Alpha_Radio_None,
	OUT_Alpha_Radio_Transparency,
	OUT_Alpha_Radio_Channel,
	OUT_Clean_Transparent
};

// sensible Win macros
#define GET_ITEM(ITEM)	GetDlgItem(hwndDlg, (ITEM))

#define SET_CHECK(ITEM, VAL)	SendMessage(GET_ITEM(ITEM), BM_SETCHECK, (WPARAM)(VAL), (LPARAM)0)
#define GET_CHECK(ITEM)			SendMessage(GET_ITEM(ITEM), BM_GETCHECK, (WPARAM)0, (LPARAM)0)

#define SET_SLIDER(ITEM, VAL)	SendMessage(GET_ITEM(ITEM),(UINT)TBM_SETPOS, (WPARAM)(BOOL)TRUE, (LPARAM)(VAL));
#define GET_SLIDER(ITEM)		SendMessage(GET_ITEM(ITEM), TBM_GETPOS, (WPARAM)0, (LPARAM)0 )

#define ENABLE_ITEM(ITEM, ENABLE)	EnableWindow(GetDlgItem(hwndDlg, (ITEM)), (ENABLE));



static DialogCompression	g_compression = DIALOG_COMPRESSION_NORMAL;
static bool					g_quantize = false;
static int					g_quant_qual = 80;
static bool					g_interlace = false;
static bool					g_metadata = true;
static DialogAlpha			g_alpha = DIALOG_ALPHA_NONE;
static bool					g_clean_trans = false;

static bool					g_isRGB8 = true;
static bool					g_have_transparency = false;
static const char			*g_alpha_name = NULL;

static WORD	g_item_clicked = 0;


static void TrackQuantize(HWND hwndDlg)
{
	const BOOL enable_slider = GET_CHECK(OUT_Quantize_Check);

	ENABLE_ITEM(OUT_Quantize_Quality, enable_slider);
	ENABLE_ITEM(OUT_Quantize_Label, enable_slider);
}

static void TrackSlider(HWND hwndDlg)
{
	const int quality = GET_SLIDER(OUT_Quantize_Quality);

	const char *quality_string = (quality > 95 ? "Highest Quality" :
									quality > 65 ? "High Quality" :
									quality < 5 ? "Lowest Quality" :
									quality < 35 ? "Low Quality" :
									"Medium Quality");

	SetDlgItemText(hwndDlg, OUT_Quantize_Label, quality_string);
}

static void TrackAlpha(HWND hwndDlg)
{
	const BOOL none_checked = GET_CHECK(OUT_Alpha_Radio_None);

	ENABLE_ITEM(OUT_Clean_Transparent, !none_checked);
}

static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    BOOL fError; 
 
    switch(message) 
    { 
		case WM_INITDIALOG:
			SET_CHECK( (g_compression == DIALOG_COMPRESSION_NONE ? OUT_Compression_Radio_None :
						g_compression == DIALOG_COMPRESSION_LOW ? OUT_Compression_Radio_Low :
						g_compression == DIALOG_COMPRESSION_NORMAL ? OUT_Compression_Radio_Normal :
						g_compression == DIALOG_COMPRESSION_HIGH ? OUT_Compression_Radio_High :
						OUT_Compression_Radio_Normal), TRUE);

			SET_CHECK(OUT_Quantize_Check, g_quantize);

			do{
				HWND slider = GetDlgItem(hwndDlg, OUT_Quantize_Quality);

				if (slider)
				{
					SendMessage(slider, (UINT)TBM_SETRANGEMIN, (WPARAM)(BOOL)FALSE, (LPARAM)0);
					SendMessage(slider, (UINT)TBM_SETRANGEMAX, (WPARAM)(BOOL)FALSE, (LPARAM)100);
					SendMessage(slider, (UINT)TBM_SETPOS, (WPARAM)(BOOL)TRUE, (LPARAM)g_quant_qual);
				}
			} while (0);

			if(!g_isRGB8)
			{
				g_quantize = false;

				ENABLE_ITEM(OUT_Quantize_Check, FALSE);
			}

			TrackQuantize(hwndDlg);
			TrackSlider(hwndDlg);

			SET_CHECK(OUT_Interlacing_Check, g_interlace);
			SET_CHECK(OUT_Metadata_Check, g_metadata);

			if(!g_have_transparency)
			{
				ENABLE_ITEM(OUT_Alpha_Radio_Transparency, FALSE);

				if(g_alpha == DIALOG_ALPHA_TRANSPARENCY)
				{
					g_alpha = (g_alpha_name != NULL ? DIALOG_ALPHA_CHANNEL : DIALOG_ALPHA_NONE);
				}
			}

			if(g_alpha_name == NULL)
			{
				ENABLE_ITEM(OUT_Alpha_Radio_Channel, FALSE);

				if(g_alpha == DIALOG_ALPHA_CHANNEL)
				{
					g_alpha = (g_have_transparency ? DIALOG_ALPHA_TRANSPARENCY : DIALOG_ALPHA_NONE);
				}
			}
			else
			{
				SetDlgItemText(hwndDlg, OUT_Alpha_Radio_Channel, g_alpha_name);
			}

			SET_CHECK( (g_alpha == DIALOG_ALPHA_NONE ? OUT_Alpha_Radio_None :
						g_alpha == DIALOG_ALPHA_TRANSPARENCY ? OUT_Alpha_Radio_Transparency :
						g_alpha == DIALOG_ALPHA_CHANNEL ? OUT_Alpha_Radio_Channel :
						OUT_Alpha_Radio_None), TRUE);

			TrackAlpha(hwndDlg);

			SET_CHECK(OUT_Clean_Transparent, g_clean_trans);

			return TRUE;
 
		case WM_NOTIFY:
			switch (LOWORD(wParam))
			{
				case OUT_Quantize_Check:
					TrackQuantize(hwndDlg);
					return TRUE;

				case OUT_Quantize_Quality:
					TrackSlider(hwndDlg);
					return TRUE;

				case OUT_Alpha_Radio_None:
				case OUT_Alpha_Radio_Transparency:
				case OUT_Alpha_Radio_Channel:
					TrackAlpha(hwndDlg);
					return TRUE;
			}
			return FALSE;

        case WM_COMMAND: 
			g_item_clicked = LOWORD(wParam);

            switch(g_item_clicked)
            { 
                case OUT_OK: 
				case OUT_Cancel:  // do the same thing, but g_item_clicked will be different
					g_compression = GET_CHECK(OUT_Compression_Radio_None) ? DIALOG_COMPRESSION_NONE :
									GET_CHECK(OUT_Compression_Radio_Low) ? DIALOG_COMPRESSION_LOW :
									GET_CHECK(OUT_Compression_Radio_Normal) ? DIALOG_COMPRESSION_NORMAL :
									GET_CHECK(OUT_Compression_Radio_High) ? DIALOG_COMPRESSION_HIGH :
									DIALOG_COMPRESSION_NORMAL;

					g_quantize = GET_CHECK(OUT_Quantize_Check);
					g_quant_qual = GET_SLIDER(OUT_Quantize_Quality);

					g_interlace = GET_CHECK(OUT_Interlacing_Check);
					g_metadata = GET_CHECK(OUT_Metadata_Check);

					g_alpha =	GET_CHECK(OUT_Alpha_Radio_None) ? DIALOG_ALPHA_NONE :
								GET_CHECK(OUT_Alpha_Radio_Transparency) ? DIALOG_ALPHA_TRANSPARENCY :
								GET_CHECK(OUT_Alpha_Radio_Channel) ? DIALOG_ALPHA_CHANNEL :
								DIALOG_ALPHA_TRANSPARENCY;

					g_clean_trans = GET_CHECK(OUT_Clean_Transparent);


					EndDialog(hwndDlg, 0);
					return TRUE;
            } 
    } 
    return FALSE; 
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
	g_compression	= params->compression;
	g_quantize		= params->quantize;
	g_quant_qual	= params->quantize_quality;
	g_interlace		= params->interlace;
	g_metadata		= params->metadata;
	g_alpha			= params->alpha;
	g_clean_trans	= params->clean_transparent;
	
	g_isRGB8 = isRGB8;
	g_have_transparency = have_transparency;
	g_alpha_name = alpha_name;


	int status = DialogBox(hDllInstance, (LPSTR)"OUT_DIALOG", (HWND)mwnd, (DLGPROC)DialogProc);


	if(g_item_clicked == OUT_OK)
	{
		params->compression		= g_compression;
		params->quantize		= g_quantize;
		params->quantize_quality = g_quant_qual;
		params->interlace		= g_interlace;
		params->metadata		= g_metadata; 
		params->alpha			= g_alpha;
		params->clean_transparent = g_clean_trans;

		return true;
	}
	else
		return false;
}


enum {
	ABOUT_noUI = -1,
	ABOUT_OK = IDOK,
	ABOUT_Cancel = IDCANCEL,
	ABOUT_Picture,
	ABOUT_Version_String
};

static BOOL CALLBACK AboutProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    BOOL fError; 
 
    switch(message) 
    { 
		case WM_INITDIALOG:
				SetDlgItemText(hwndDlg, ABOUT_Version_String, "v" SuperPNG_Version_String " - " SuperPNG_Build_Date);

			return TRUE;
 
		case WM_NOTIFY:
			return FALSE;

        case WM_COMMAND: 
            switch(LOWORD(wParam))
            { 
                case OUT_OK: 
				case OUT_Cancel:
					EndDialog(hwndDlg, 0);
					return TRUE;
            } 
    } 
    return FALSE; 
} 

void
SuperPNG_About(
	const void		*plugHndl,
	const void		*mwnd)
{
	int status = DialogBox(hDllInstance, (LPSTR)"ABOUT_DIALOG", (HWND)mwnd, (DLGPROC)AboutProc);
}

