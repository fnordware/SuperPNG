
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
	OUT_Interlacing_Check,
	OUT_Metadata_Check,
	OUT_Alpha_Radio_None,
	OUT_Alpha_Radio_Transparency,
	OUT_Alpha_Radio_Channel
};

// sensible Win macros
#define GET_ITEM(ITEM)	GetDlgItem(hwndDlg, (ITEM))

#define SET_CHECK(ITEM, VAL)	SendMessage(GET_ITEM(ITEM), BM_SETCHECK, (WPARAM)(VAL), (LPARAM)0)
#define GET_CHECK(ITEM)			SendMessage(GET_ITEM(ITEM), BM_GETCHECK, (WPARAM)0, (LPARAM)0)

#define ENABLE_ITEM(ITEM, ENABLE)	EnableWindow(GetDlgItem(hwndDlg, (ITEM)), (ENABLE));



static DialogCompression	g_compression = DIALOG_COMPRESSION_NORMAL;
static bool					g_interlace = FALSE;
static bool					g_metadata = TRUE;
static DialogAlpha			g_alpha = DIALOG_ALPHA_NONE;

static bool					g_have_transparency = false;
static const char			*g_alpha_name = NULL;

static WORD	g_item_clicked = 0;



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

			return TRUE;
 
		case WM_NOTIFY:
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

					g_interlace = GET_CHECK(OUT_Interlacing_Check);
					g_metadata = GET_CHECK(OUT_Metadata_Check);

					g_alpha =	GET_CHECK(OUT_Alpha_Radio_None) ? DIALOG_ALPHA_NONE :
								GET_CHECK(OUT_Alpha_Radio_Transparency) ? DIALOG_ALPHA_TRANSPARENCY :
								GET_CHECK(OUT_Alpha_Radio_Channel) ? DIALOG_ALPHA_CHANNEL :
								DIALOG_ALPHA_TRANSPARENCY;


					EndDialog(hwndDlg, 0);
					return TRUE;
            } 
    } 
    return FALSE; 
} 

bool
SuperPNG_OutUI(
	SuperPNG_OutUI_Data	*params,
	bool				have_transparency,
	const char			*alpha_name,
	const void			*plugHndl,
	const void			*mwnd)
{
	g_compression	= params->compression;
	g_interlace		= params->interlace;
	g_metadata		= params->metadata;
	g_alpha			= params->alpha;
	
	g_have_transparency = have_transparency;
	g_alpha_name = alpha_name;


	int status = DialogBox(hDllInstance, (LPSTR)"OUT_DIALOG", (HWND)mwnd, (DLGPROC)DialogProc);


	if(g_item_clicked == OUT_OK)
	{
		params->compression		= g_compression;
		params->interlace		= g_interlace;
		params->metadata		= g_metadata; 
		params->alpha			= g_alpha;

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

