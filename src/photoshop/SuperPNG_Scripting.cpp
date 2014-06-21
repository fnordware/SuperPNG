
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

#include "SuperPNG_Terminology.h"
		

static PNG_Alpha KeyToAlpha(OSType key)
{
	return	(key == alphaChannelNone)			? PNG_ALPHA_NONE :
			(key == alphaChannelTransparency)	? PNG_ALPHA_TRANSPARENCY :
			(key == alphaChannelChannel)		? PNG_ALPHA_CHANNEL :
			PNG_ALPHA_TRANSPARENCY;
}

Boolean ReadScriptParamsOnWrite(GPtr globals)
{
	PIReadDescriptor			token = NULL;
	DescriptorKeyID				key = 0;
	DescriptorTypeID			type = 0;
	OSType						shape = 0, create = 0;
	DescriptorKeyIDArray		array = { NULLID };
	int32						flags = 0;
	OSErr						gotErr = noErr, stickyError = noErr;
	Boolean						returnValue = true;
	int32						storeValue;
	DescriptorEnumID			ostypeStoreValue;
	Boolean						boolStoreValue;
	
	if (DescriptorAvailable(NULL))
	{
		token = OpenReader(array);
		if (token)
		{
			while (PIGetKey(token, &key, &type, &flags))
			{
				switch (key)
				{
					case keyPNGcompression:
							PIGetInt(token, &storeValue);
							gOptions.compression = storeValue;
							break;
					
					case keyPNGfilter:
							PIGetInt(token, &storeValue);
							gOptions.filter = storeValue;
							break;

					case keyPNGstrategy:
							PIGetInt(token, &storeValue);
							gOptions.strategy = storeValue;
							break;

					case keyPNGinterlace:
							PIGetBool(token, &boolStoreValue);
							gOptions.interlace = boolStoreValue;
							break;
					
					case keyPNGmeta:
							PIGetBool(token, &boolStoreValue);
							gOptions.metadata = boolStoreValue;
							break;
					
					case keyPNGalpha:
							PIGetEnum(token, &ostypeStoreValue);
							gOptions.alpha = KeyToAlpha(ostypeStoreValue);
							break;
							
					case keyPNGcleanTransparent:
							PIGetBool(token, &boolStoreValue);
							gOptions.clean_transparent = boolStoreValue;
							break;
				}
			}

			stickyError = CloseReader(&token); // closes & disposes.
				
			if (stickyError)
			{
				if (stickyError == errMissingParameter) // missedParamErr == -1715
					;
					/* (descriptorKeyIDArray != NULL)
					   missing parameter somewhere.  Walk IDarray to find which one. */
				else
					gResult = stickyError;
			}
		}
		
		returnValue = PlayDialog();
		// return TRUE if want to show our Dialog
	}
	
	return returnValue;
}

		

static OSType AlphaToKey(PNG_Alpha alpha)
{
	return	(alpha == PNG_ALPHA_NONE)			? alphaChannelNone :
			(alpha == PNG_ALPHA_TRANSPARENCY)	? alphaChannelTransparency :
			(alpha == PNG_ALPHA_CHANNEL)		? alphaChannelChannel :
			alphaChannelTransparency;
}

OSErr WriteScriptParamsOnWrite(GPtr globals)
{
	PIWriteDescriptor			token = nil;
	OSErr						gotErr = noErr;
			
	if (DescriptorAvailable(NULL))
	{
		token = OpenWriter();
		if (token)
		{
			// write keys here
			PIPutInt(token, keyPNGcompression, gOptions.compression);
			PIPutInt(token, keyPNGfilter, gOptions.filter);
			PIPutInt(token, keyPNGstrategy, gOptions.strategy);
			PIPutBool(token, keyPNGinterlace, gOptions.interlace);
			PIPutBool(token, keyPNGmeta, gOptions.metadata);
			PIPutEnum(token, keyPNGalpha, typeAlphaChannel, AlphaToKey(gOptions.alpha));
			
			if(gOptions.alpha != PNG_ALPHA_NONE)
				PIPutBool(token, keyPNGcleanTransparent, gOptions.clean_transparent);
			
			gotErr = CloseWriter(&token); /* closes and sets dialog optional */
			/* done.  Now pass handle on to Photoshop */
		}
	}
	return gotErr;
}

