
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

#include <Windows.h>
#include <commctrl.h>

extern HINSTANCE hDllInstance;

enum {
	IN_noUI = -1,
	IN_OK = IDOK,
	IN_Cancel = IDCANCEL,
	IN_Set_Defaults_Button,
	IN_Alpha_Radio_Transparent,
	IN_Alpha_Radio_Channel,
	IN_Multiply_Checkbox,
	IN_Always_Checkbox
};

// sensible Win macros
#define GET_ITEM(ITEM)	GetDlgItem(hwndDlg, (ITEM))

#define SET_CHECK(ITEM, VAL)	SendMessage(GET_ITEM(ITEM), BM_SETCHECK, (WPARAM)(VAL), (LPARAM)0)
#define GET_CHECK(ITEM)			SendMessage(GET_ITEM(ITEM), BM_GETCHECK, (WPARAM)0, (LPARAM)0)

#define ENABLE_ITEM(ITEM, ENABLE)	EnableWindow(GetDlgItem(hwndDlg, (ITEM)), (ENABLE));


static DialogAlpha			g_alpha = DIALOG_ALPHA_TRANSPARENCY;
static bool					g_mult = true;
static bool					g_always = false;


static void ReadPrefs()
{
	// read prefs from registry
	HKEY superpng_hkey;
	LONG reg_error = RegOpenKeyEx(HKEY_CURRENT_USER, SUPERPNG_PREFIX, 0, KEY_READ, &superpng_hkey);

	if(reg_error == ERROR_SUCCESS)
	{
		DWORD type;
		DWORD size = sizeof(DWORD);

		DWORD alpha = g_alpha,
				mult = g_mult,
				always = g_always;

		reg_error = RegQueryValueEx(superpng_hkey, SUPERPNG_ALPHA_KEY, NULL, &type, (LPBYTE)&alpha, &size);

		if(reg_error == ERROR_SUCCESS && type == REG_DWORD)
			g_alpha = (DialogAlpha)alpha;

		reg_error = RegQueryValueEx(superpng_hkey, SUPERPNG_MULT_KEY, NULL, &type, (LPBYTE)&mult, &size);

		if(reg_error == ERROR_SUCCESS && type == REG_DWORD)
			g_mult = mult;

		reg_error = RegQueryValueEx(superpng_hkey, SUPERPNG_ALWAYS_KEY, NULL, &type, (LPBYTE)&always, &size);

		if(reg_error == ERROR_SUCCESS && type == REG_DWORD)
			g_always = always;

		reg_error = RegCloseKey(superpng_hkey);
	}
}

static void WriteAlphaPrefs()
{
	HKEY superpng_hkey;

	LONG reg_error = RegCreateKeyEx(HKEY_CURRENT_USER, SUPERPNG_PREFIX, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &superpng_hkey, NULL);

	if(reg_error == ERROR_SUCCESS)
	{
		DWORD alpha = g_alpha,
				mult = g_mult;

		reg_error = RegSetValueEx(superpng_hkey, SUPERPNG_ALPHA_KEY, NULL, REG_DWORD, (BYTE *)&alpha, sizeof(DWORD));
		reg_error = RegSetValueEx(superpng_hkey, SUPERPNG_MULT_KEY, NULL, REG_DWORD, (BYTE *)&mult, sizeof(DWORD));

		reg_error = RegCloseKey(superpng_hkey);
	}
}

static void WriteAlwaysPrefs()
{
	HKEY superpng_hkey;

	LONG reg_error = RegCreateKeyEx(HKEY_CURRENT_USER, SUPERPNG_PREFIX, NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &superpng_hkey, NULL);

	if(reg_error == ERROR_SUCCESS)
	{
		DWORD always = g_always;

		reg_error = RegSetValueEx(superpng_hkey, SUPERPNG_ALWAYS_KEY, NULL, REG_DWORD, (BYTE *)&always, sizeof(DWORD));

		reg_error = RegCloseKey(superpng_hkey);
	}
}


static WORD	g_item_clicked = 0;

static BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    BOOL fError; 
 
    switch(message) 
    { 
		case WM_INITDIALOG:
			SET_CHECK( (g_alpha == DIALOG_ALPHA_TRANSPARENCY ? IN_Alpha_Radio_Transparent :
						g_alpha == DIALOG_ALPHA_CHANNEL ? IN_Alpha_Radio_Channel :
						IN_Alpha_Radio_Transparent), TRUE);

			SET_CHECK(IN_Multiply_Checkbox, g_mult);
			SET_CHECK(IN_Always_Checkbox, g_always);

			ENABLE_ITEM(IN_Multiply_Checkbox, (g_alpha == DIALOG_ALPHA_CHANNEL));

			return TRUE;
 
		case WM_NOTIFY:
			return FALSE;

        case WM_COMMAND: 
			g_alpha = GET_CHECK(IN_Alpha_Radio_Transparent) ? DIALOG_ALPHA_TRANSPARENCY :
							GET_CHECK(IN_Alpha_Radio_Channel) ? DIALOG_ALPHA_CHANNEL :
							DIALOG_ALPHA_TRANSPARENCY;

			g_mult = GET_CHECK(IN_Multiply_Checkbox);
			g_always = GET_CHECK(IN_Always_Checkbox);

			g_item_clicked = LOWORD(wParam);

            switch(g_item_clicked)
            { 
                case IN_OK: 
				case IN_Cancel:
					EndDialog(hwndDlg, 0);
					return TRUE;

				case IN_Set_Defaults_Button:
					WriteAlphaPrefs();
					WriteAlwaysPrefs();
					return TRUE;

				case IN_Alpha_Radio_Transparent:
				case IN_Alpha_Radio_Channel:
					ENABLE_ITEM(IN_Multiply_Checkbox, (g_alpha == DIALOG_ALPHA_CHANNEL));
					return TRUE;
            } 
    } 
    return FALSE; 
} 


static inline bool KeyIsDown(int vKey)
{
	return (GetAsyncKeyState(vKey) & 0x8000);
}


bool
SuperPNG_InUI(
	SuperPNG_InUI_Data	*params,
	const void			*plugHndl,
	const void			*mwnd)
{
	bool continue_reading = true;

	g_alpha = DIALOG_ALPHA_TRANSPARENCY;
	g_mult = false;
	g_always = false;

	ReadPrefs();

	// check for that shift key
	bool shift_key = ( KeyIsDown(VK_LSHIFT) || KeyIsDown(VK_RSHIFT) || KeyIsDown(VK_LMENU) || KeyIsDown(VK_RMENU) );

	if(g_always || shift_key)
	{
		int status = DialogBox(hDllInstance, (LPSTR)"IN_DIALOG", (HWND)mwnd, (DLGPROC)DialogProc);

		if(g_item_clicked == IN_OK)
		{
			WriteAlwaysPrefs();

			continue_reading = true;
		}
		else
			continue_reading = false;
	}

	params->alpha	= g_alpha;
	params->mult	= g_mult;

	return continue_reading;
}
