/*
** optionmenuitems.cpp
** Control items for option menus
**
**---------------------------------------------------------------------------
** Copyright 2010 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/
#include <float.h>

#include "menu.h"
#include "c_dispatch.h"
#include "v_video.h"
#include "v_palette.h"
#include "d_event.h"
#include "c_bind.h"
#include "gi.h"
#include "v_text.h"
#include "gstrings.h"
#include "optionmenuitems.h"

void M_DrawConText (int color, int x, int y, const char *str);
void M_SetVideoMode();

//=============================================================================
//
// FOptionMenuItemSubmenu
//
// opens a submenu, action is a submenu name
//
//=============================================================================

FOptionMenuItemSubmenu::FOptionMenuItemSubmenu(const char *label, const char *menu, int param)
	: FOptionMenuItem(label, menu)
{
	mParam = param;
}

int FOptionMenuItemSubmenu::Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected)
{
	drawLabel(indent, y, selected? OptionSettings.mFontColorSelection : OptionSettings.mFontColorMore);
	return indent;
}

bool FOptionMenuItemSubmenu::Activate()
{
	S_Sound (CHAN_VOICE | CHAN_UI, "menu/choose", snd_menuvolume, ATTN_NONE);
	M_SetMenu(mAction, mParam);
	return true;
}


//=============================================================================
//
// FOptionMenuItemCommand
//
// Executes a CCMD, action is a CCMD name
//
//=============================================================================

FOptionMenuItemCommand::FOptionMenuItemCommand(const char *label, const char *menu)
	: FOptionMenuItemSubmenu(label, menu)
{
}

bool FOptionMenuItemCommand::Activate()
{
	S_Sound (CHAN_VOICE | CHAN_UI, "menu/choose", snd_menuvolume, ATTN_NONE);
	C_DoCommand(mAction);
	return true;
}

//=============================================================================
//
// FOptionMenuItemSafeCommand
//
// Executes a CCMD after confirmation, action is a CCMD name
//
//=============================================================================

FOptionMenuItemSafeCommand::FOptionMenuItemSafeCommand(const char *label, const char *menu, const char *prompt)
	: FOptionMenuItemCommand(label, menu)
	, mPrompt(prompt)
{
}

bool FOptionMenuItemSafeCommand::MenuEvent (int mkey, bool fromcontroller)
{
	if (mkey == MKEY_MBYes)
	{
		C_DoCommand(mAction);
		return true;
	}
	return FOptionMenuItemCommand::MenuEvent(mkey, fromcontroller);
}

bool FOptionMenuItemSafeCommand::Activate()
{
	const char *msg = mPrompt.IsNotEmpty() ? mPrompt.GetChars() : "$SAFEMESSAGE";
	if (*msg == '$')
	{
		msg = GStrings(msg + 1);
	}

	const char *actionLabel = mLabel.GetChars();
	if (actionLabel != NULL)
	{
		if (*actionLabel == '$')
		{
			actionLabel = GStrings(actionLabel + 1);
		}
	}

	FString FullString;
	FullString.Format(TEXTCOLOR_WHITE "%s" TEXTCOLOR_NORMAL "\n\n" "%s", actionLabel != NULL ? actionLabel : "", msg);

	if (msg && FullString)
		M_StartMessage(FullString, 0);

	return true;
}

//=============================================================================
//
// FOptionMenuItemOptionBase
//
// Base class for option lists
//
//=============================================================================

FOptionMenuItemOptionBase::FOptionMenuItemOptionBase(const char *label, const char *menu, const char *values, const char *graycheck, int center)
	: FOptionMenuItem(label, menu)
{
	mValues = values;
	mGrayCheck = (FBoolCVar*)FindCVar(graycheck, NULL);
	mCenter = center;
}

bool FOptionMenuItemOptionBase::SetString(int i, const char *newtext)
{
	if (i == OP_VALUES) 
	{
		FOptionValues **opt = OptionValues.CheckKey(newtext);
		mValues = newtext;
		if (opt != NULL && *opt != NULL) 
		{
			int s = GetSelection();
			if (s >= (int)(*opt)->mValues.Size()) s = 0;
			SetSelection(s);	// readjust the CVAR if its value is outside the range now
			return true;
		}
	}
	return false;
}

int FOptionMenuItemOptionBase::Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected)
{
	bool grayed = mGrayCheck != NULL && !(mGrayCheck->GetGenericRep(CVAR_Bool).Bool);

	if (mCenter)
	{
		indent = (screen->GetWidth() / 2);
	}
	drawLabel(indent, y, selected? OptionSettings.mFontColorSelection : OptionSettings.mFontColor, grayed);

	int overlay = grayed? MAKEARGB(96,48,0,0) : 0;
	const char *text;
	int Selection = GetSelection();
	FOptionValues **opt = OptionValues.CheckKey(mValues);
	if (Selection < 0 || opt == NULL || *opt == NULL)
	{
		text = "Unknown";
	}
	else
	{
		text = (*opt)->mValues[Selection].Text;
	}
	if (*text == '$') text = GStrings(text + 1);
	screen->DrawText (SmallFont, OptionSettings.mFontColorValue, indent + CURSORSPACE, y, 
		text, DTA_CleanNoMove_1, true, DTA_ColorOverlay, overlay, TAG_DONE);
	return indent;
}

bool FOptionMenuItemOptionBase::MenuEvent (int mkey, bool fromcontroller)
{
	FOptionValues **opt = OptionValues.CheckKey(mValues);
	if (opt != NULL && *opt != NULL && (*opt)->mValues.Size() > 0)
	{
		int Selection = GetSelection();
		if (mkey == MKEY_Left)
		{
			if (Selection == -1) Selection = 0;
			else if (--Selection < 0) Selection = (*opt)->mValues.Size()-1;
		}
		else if (mkey == MKEY_Right || mkey == MKEY_Enter)
		{
			if (++Selection >= (int)(*opt)->mValues.Size()) Selection = 0;
		}
		else
		{
			return FOptionMenuItem::MenuEvent(mkey, fromcontroller);
		}
		SetSelection(Selection);
		S_Sound (CHAN_VOICE | CHAN_UI, "menu/change", snd_menuvolume, ATTN_NONE);
	}
	else
	{
		return FOptionMenuItem::MenuEvent(mkey, fromcontroller);
	}
	return true;
}

bool FOptionMenuItemOptionBase::Selectable()
{
	return !(mGrayCheck != NULL && !(mGrayCheck->GetGenericRep(CVAR_Bool).Bool));
}

//=============================================================================
//
// FOptionMenuItemOption
//
// Change a CVAR, action is the CVAR name
//
//=============================================================================

FOptionMenuItemOption::FOptionMenuItemOption(const char *label, const char *menu, const char *values, const char *graycheck, int center)
	: FOptionMenuItemOptionBase(label, menu, values, graycheck, center)
{
	mCVar = FindCVar(mAction, NULL);
}

int FOptionMenuItemOption::GetSelection()
{
	int Selection = -1;
	FOptionValues **opt = OptionValues.CheckKey(mValues);
	if (opt != NULL && *opt != NULL && mCVar != NULL && (*opt)->mValues.Size() > 0)
	{
		if ((*opt)->mValues[0].TextValue.IsEmpty())
		{
			UCVarValue cv = mCVar->GetGenericRep(CVAR_Float);
			for(unsigned i = 0; i < (*opt)->mValues.Size(); i++)
			{ 
				if (fabs(cv.Float - (*opt)->mValues[i].Value) < FLT_EPSILON)
				{
					Selection = i;
					break;
				}
			}
		}
		else
		{
			const char *cv = mCVar->GetHumanString();
			for(unsigned i = 0; i < (*opt)->mValues.Size(); i++)
			{
				if ((*opt)->mValues[i].TextValue.CompareNoCase(cv) == 0)
				{
					Selection = i;
					break;
				}
			}
		}
	}
	return Selection;
}

void FOptionMenuItemOption::SetSelection(int Selection)
{
	UCVarValue value;
	FOptionValues **opt = OptionValues.CheckKey(mValues);
	if (opt != NULL && *opt != NULL && mCVar != NULL && (*opt)->mValues.Size() > 0)
	{
		if ((*opt)->mValues[0].TextValue.IsEmpty())
		{
			value.Float = (float)(*opt)->mValues[Selection].Value;
			mCVar->SetGenericRep (value, CVAR_Float);
		}
		else
		{
			value.String = (*opt)->mValues[Selection].TextValue.LockBuffer();
			mCVar->SetGenericRep (value, CVAR_String);
			(*opt)->mValues[Selection].TextValue.UnlockBuffer();
		}
	}
}

//=============================================================================
//
// DEnterKey
//
// This class is used to capture the key to be used as the new key binding
// for a control item
//
//=============================================================================

DEnterKey::DEnterKey(DMenu *parent, int *keyptr)
	: DMenu(parent)
{
	pKey = keyptr;
	SetMenuMessage(1);
	menuactive = MENU_WaitKey;	// There should be a better way to disable GUI capture...
}

bool DEnterKey::TranslateKeyboardEvents()
{
	return false; 
}

void DEnterKey::SetMenuMessage(int which)
{
	if (mParentMenu->IsKindOf(RUNTIME_CLASS(DOptionMenu)))
	{
		DOptionMenu *m = barrier_cast<DOptionMenu*>(mParentMenu);
		FListMenuItem *it = m->GetItem(NAME_Controlmessage);
		if (it != NULL)
		{
			it->SetValue(0, which);
		}
	}
}

bool DEnterKey::Responder(event_t *ev)
{
	if (ev->type == EV_KeyDown)
	{
		*pKey = ev->data1;
		menuactive = MENU_On;
		SetMenuMessage(0);
		Close();
		mParentMenu->MenuEvent((ev->data1 == KEY_ESCAPE)? MKEY_Abort : MKEY_Input, 0);
		return true;
	}
	return false;
}

void DEnterKey::Drawer()
{
	mParentMenu->Drawer();
}

IMPLEMENT_ABSTRACT_CLASS(DEnterKey)

//=============================================================================
//
// FOptionMenuItemControl
//
// Edit a key binding, Action is the CCMD to bind
//
//=============================================================================

FOptionMenuItemControl::FOptionMenuItemControl(const char *label, const char *menu, FKeyBindings *bindings)
	: FOptionMenuItem(label, menu)
{
	mBindings = bindings;
	mWaiting = false;
}

int FOptionMenuItemControl::Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected)
{
	drawLabel(indent, y, mWaiting? OptionSettings.mFontColorHighlight: 
		(selected? OptionSettings.mFontColorSelection : OptionSettings.mFontColor));

	char description[64];
	int Key1, Key2;

	mBindings->GetKeysForCommand(mAction, &Key1, &Key2);
	C_NameKeys (description, Key1, Key2);
	if (description[0])
	{
		M_DrawConText(CR_WHITE, indent + CURSORSPACE, y + (OptionSettings.mLinespacing-8)*CleanYfac_1, description);
	}
	else
	{
		screen->DrawText(SmallFont, CR_BLACK, indent + CURSORSPACE, y + (OptionSettings.mLinespacing-8)*CleanYfac_1, "---",
			DTA_CleanNoMove_1, true, TAG_DONE);
	}
	return indent;
}

bool FOptionMenuItemControl::MenuEvent(int mkey, bool fromcontroller)
{
	if (mkey == MKEY_Input)
	{
		mWaiting = false;
		mBindings->SetBind(mInput, mAction);
		return true;
	}
	else if (mkey == MKEY_Clear)
	{
		mBindings->UnbindACommand(mAction);
		return true;
	}
	else if (mkey == MKEY_Abort)
	{
		mWaiting = false;
		return true;
	}
	return false;
}

bool FOptionMenuItemControl::Activate()
{
	S_Sound (CHAN_VOICE | CHAN_UI, "menu/choose", snd_menuvolume, ATTN_NONE);
	mWaiting = true;
	DMenu *input = new DEnterKey(DMenu::CurrentMenu, &mInput);
	M_ActivateMenu(input);
	return true;
}

//=============================================================================
//
// FOptionMenuItemStaticText
//
//=============================================================================

FOptionMenuItemStaticText::FOptionMenuItemStaticText(const char *label, bool header)
	: FOptionMenuItem(label, NAME_None, true)
{
	mColor = header ? OptionSettings.mFontColorHeader : OptionSettings.mFontColor;
}

FOptionMenuItemStaticText::FOptionMenuItemStaticText(const char *label, EColorRange cr)
	: FOptionMenuItem(label, NAME_None, true)
{
	mColor = cr;
}

int FOptionMenuItemStaticText::Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected)
{
	drawLabel(indent, y, mColor);
	return -1;
}

bool FOptionMenuItemStaticText::Selectable()
{
	return false;
}

//=============================================================================
//
// FOptionMenuItemStaticTextSwitchable
//
//=============================================================================

FOptionMenuItemStaticTextSwitchable::FOptionMenuItemStaticTextSwitchable(const char *label, const char *label2, FName action, EColorRange cr)
	: FOptionMenuItem(label, action, true)
{
	mColor = cr;
	mAltText = label2;
	mCurrent = 0;
}

int FOptionMenuItemStaticTextSwitchable::Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected)
{
	const char *txt = mCurrent? mAltText.GetChars() : mLabel.GetChars();
	if (*txt == '$') txt = GStrings(txt + 1);
	int w = SmallFont->StringWidth(txt) * CleanXfac_1;
	int x = (screen->GetWidth() - w) / 2;
	screen->DrawText (SmallFont, mColor, x, y, txt, DTA_CleanNoMove_1, true, TAG_DONE);
	return -1;
}

bool FOptionMenuItemStaticTextSwitchable::SetValue(int i, int val)
{
	if (i == 0) 
	{
		mCurrent = val;
		return true;
	}
	return false;
}

bool FOptionMenuItemStaticTextSwitchable::SetString(int i, const char *newtext)
{
	if (i == 0) 
	{
		mAltText = newtext;
		return true;
	}
	return false;
}

bool FOptionMenuItemStaticTextSwitchable::Selectable()
{
	return false;
}

//=============================================================================
//
// FOptionMenuSliderBase
//
// Base class for sliders.
//
//=============================================================================

FOptionMenuSliderBase::FOptionMenuSliderBase(const char *label, double min, double max, double step, int showval)
	: FOptionMenuItem(label, NAME_None)
{
	mMin = min;
	mMax = max;
	mStep = step;
	mShowValue = showval;
	mDrawX = 0;
	mSliderShort = 0;
}

//=============================================================================
//
// FOptionMenuSliderBase :: DrawSlider
//
// Draw a slider. Set fracdigits negative to not display the current value numerically.
//
//=============================================================================

void FOptionMenuSliderBase::DrawSlider (int x, int y, double min, double max, double cur, int fracdigits, int indent)
{
	char textbuf[16];
	double range;
	int maxlen = 0;
	int right = x + (12*8 + 4) * CleanXfac_1;
	int cy = y + (OptionSettings.mLinespacing-8)*CleanYfac_1;

	range = max - min;
	double ccur = clamp(cur, min, max) - min;

	if (fracdigits >= 0)
	{
		mysnprintf(textbuf, countof(textbuf), "%.*f", fracdigits, max);
		maxlen = SmallFont->StringWidth(textbuf) * CleanXfac_1;
	}

	mSliderShort = right + maxlen > screen->GetWidth();

	if (!mSliderShort)
	{
		M_DrawConText(CR_WHITE, x, cy, "\x10\x11\x11\x11\x11\x11\x11\x11\x11\x11\x11\x12");
		M_DrawConText(CR_ORANGE, x + int((5 + ((ccur * 78) / range)) * CleanXfac_1), cy, "\x13");
	}
	else
	{
		// On 320x200 we need a shorter slider
		M_DrawConText(CR_WHITE, x, cy, "\x10\x11\x11\x11\x11\x11\x12");
		M_DrawConText(CR_ORANGE, x + int((5 + ((ccur * 38) / range)) * CleanXfac_1), cy, "\x13");
		right -= 5*8*CleanXfac_1;
	}

	if (fracdigits >= 0 && right + maxlen <= screen->GetWidth())
	{
		mysnprintf(textbuf, countof(textbuf), "%.*f", fracdigits, cur);
		screen->DrawText(SmallFont, CR_DARKGRAY, right, y, textbuf, DTA_CleanNoMove_1, true, TAG_DONE);
	}
}


int FOptionMenuSliderBase::Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected)
{
	drawLabel(indent, y, selected? OptionSettings.mFontColorSelection : OptionSettings.mFontColor);
	mDrawX = indent + CURSORSPACE;
	DrawSlider (mDrawX, y, mMin, mMax, GetSliderValue(), mShowValue, indent);
	return indent;
}

bool FOptionMenuSliderBase::MenuEvent (int mkey, bool fromcontroller)
{
	double value = GetSliderValue();

	if (mkey == MKEY_Left)
	{
		value -= mStep;
	}
	else if (mkey == MKEY_Right)
	{
		value += mStep;
	}
	else
	{
		return FOptionMenuItem::MenuEvent(mkey, fromcontroller);
	}
	if (fabs(value) < FLT_EPSILON) value = 0;
	SetSliderValue(clamp(value, mMin, mMax));
	S_Sound (CHAN_VOICE | CHAN_UI, "menu/change", snd_menuvolume, ATTN_NONE);
	return true;
}

bool FOptionMenuSliderBase::MouseEvent(int type, int x, int y)
{
	DOptionMenu *lm = static_cast<DOptionMenu*>(DMenu::CurrentMenu);
	if (type != DMenu::MOUSE_Click)
	{
		if (!lm->CheckFocus(this))
			return false;
	}
	if (type == DMenu::MOUSE_Release)
	{
		lm->ReleaseFocus();
	}

	int slide_left = mDrawX + 8 * CleanXfac_1;
	int slide_right = slide_left + (10 * 8 * CleanXfac_1 >> mSliderShort);	// 12 char cells with 8 pixels each.

	if (type == DMenu::MOUSE_Click)
	{
		if (x < slide_left || x >= slide_right)
			return true;
	}

	x = clamp(x, slide_left, slide_right);
	double v = mMin + ((x - slide_left) * (mMax - mMin)) / (slide_right - slide_left);
	if (v != GetSliderValue())
	{
		SetSliderValue(v);
		//S_Sound (CHAN_VOICE | CHAN_UI, "menu/change", snd_menuvolume, ATTN_NONE);
	}
	if (type == DMenu::MOUSE_Click)
	{
		lm->SetFocus(this);
	}
	return true;
}

//=============================================================================
//
// FOptionMenuSliderCVar
//
// A slider that changes the value of a CVar.
//
//=============================================================================

FOptionMenuSliderCVar::FOptionMenuSliderCVar(const char *label, const char *menu, double min, double max, double step, int showval)
	: FOptionMenuSliderBase(label, min, max, step, showval)
{
	mCVar = FindCVar(menu, NULL);
}

double FOptionMenuSliderCVar::GetSliderValue()
{
	if (mCVar != NULL)
		return mCVar->GetGenericRep(CVAR_Float).Float;
	else
		return 0;
}

void FOptionMenuSliderCVar::SetSliderValue(double val)
{
	if (mCVar != NULL)
	{
		UCVarValue value;
		value.Float = (float)val;
		mCVar->SetGenericRep(value, CVAR_Float);
	}
}

//=============================================================================
//
// FOptionMenuSliderVar
//
// A slider that changes a variable.
//
//=============================================================================

FOptionMenuSliderVar::FOptionMenuSliderVar(const char *label, float *pVal, double min, double max, double step, int showval)
	: FOptionMenuSliderBase(label, min, max, step, showval)
{
	mPVal = pVal;
}

double FOptionMenuSliderVar::GetSliderValue()
{
	return *mPVal;
}

void FOptionMenuSliderVar::SetSliderValue(double val)
{
	*mPVal = (float)val;
}

//=============================================================================
//
// FOptionMenuItemColorPicker
//
// Edit a key binding, Action is the CCMD to bind
//
//=============================================================================

FOptionMenuItemColorPicker::FOptionMenuItemColorPicker(const char *label, const char *menu)
	: FOptionMenuItem(label, menu)
{
	FBaseCVar *cv = FindCVar(menu, NULL);
	
	if (cv != NULL && cv->GetRealType() == CVAR_Color)
		mCVar = (FColorCVar*)cv;
	else
		mCVar = NULL;
}

int FOptionMenuItemColorPicker::Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected)
{
	drawLabel(indent, y, selected? OptionSettings.mFontColorSelection : OptionSettings.mFontColor);

	if (mCVar != NULL)
	{
		int box_x = indent + CURSORSPACE;
		int box_y = y + CleanYfac_1;
		screen->Clear (box_x, box_y, box_x + 32*CleanXfac_1, box_y + OptionSettings.mLinespacing*CleanYfac_1,
			-1, (uint32)*mCVar | 0xff000000);
	}
	return indent;
}

bool FOptionMenuItemColorPicker::SetValue(int i, int v)
{
	if (i == CPF_RESET && mCVar != NULL)
	{
		mCVar->ResetToDefault();
		return true;
	}
	return false;
}

bool FOptionMenuItemColorPicker::Activate()
{
	if (mCVar != NULL)
	{
		S_Sound (CHAN_VOICE | CHAN_UI, "menu/choose", snd_menuvolume, ATTN_NONE);
		DMenu *picker = StartPickerMenu(DMenu::CurrentMenu, mLabel, mCVar);
		if (picker != NULL)
		{
			M_ActivateMenu(picker);
			return true;
		}
	}
	return false;
}

//=============================================================================
//
// FOptionMenuScreenResolutionLine
//
// A line of screen resolutions for the video menu.
//
//=============================================================================

FOptionMenuScreenResolutionLine::FOptionMenuScreenResolutionLine(const char *action)
	: FOptionMenuItem("", action)
{
	mSelection = 0;
	mHighlight = -1;
}

bool FOptionMenuScreenResolutionLine::SetValue(int i, int v)
{
	if (i == SRL_SELECTION)
	{
		mSelection = v;
		return true;
	}
	else if (i == SRL_HIGHLIGHT)
	{
		mHighlight = v;
		return true;
	}
	return false;
}

bool FOptionMenuScreenResolutionLine::GetValue(int i, int *v)
{
	if (i == SRL_SELECTION)
	{
		*v = mSelection;
		return true;
	}
	return false;
}

bool FOptionMenuScreenResolutionLine::SetString(int i, const char *newtext)
{
	if (i >= SRL_INDEX && i <= SRL_INDEX+2) 
	{
		mResTexts[i-SRL_INDEX] = newtext;
		if (mResTexts[0].IsEmpty())
			mMaxValid = -1;
		else if (mResTexts[1].IsEmpty())
			mMaxValid = 0;
		else if (mResTexts[2].IsEmpty())
			mMaxValid = 1;
		else
			mMaxValid = 2;
		return true;
	}
	return false;
}

bool FOptionMenuScreenResolutionLine::GetString(int i, char *s, int len)
{
	if (i >= SRL_INDEX && i <= SRL_INDEX+2) 
	{
		strncpy(s, mResTexts[i-SRL_INDEX], len-1);
		s[len-1] = 0;
		return true;
	}
	return false;
}

bool FOptionMenuScreenResolutionLine::MenuEvent (int mkey, bool fromcontroller)
{
	if (mkey == MKEY_Left)
	{
		if (--mSelection < 0) mSelection = mMaxValid;
		S_Sound (CHAN_VOICE | CHAN_UI, "menu/cursor", snd_menuvolume, ATTN_NONE);
		return true;
	}
	else if (mkey == MKEY_Right)
	{
		if (++mSelection > mMaxValid) mSelection = 0;
		S_Sound (CHAN_VOICE | CHAN_UI, "menu/cursor", snd_menuvolume, ATTN_NONE);
		return true;
	}
	else 
	{
		return FOptionMenuItem::MenuEvent(mkey, fromcontroller);
	}
	return false;
}

bool FOptionMenuScreenResolutionLine::MouseEvent(int type, int x, int y)
{
	int colwidth = screen->GetWidth() / 3;
	mSelection = x / colwidth;
	return FOptionMenuItem::MouseEvent(type, x, y);
}

bool FOptionMenuScreenResolutionLine::Activate()
{
	S_Sound (CHAN_VOICE | CHAN_UI, "menu/choose", snd_menuvolume, ATTN_NONE);
	M_SetVideoMode();
	return true;
}

int FOptionMenuScreenResolutionLine::Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected)
{
	int colwidth = screen->GetWidth() / 3;
	EColorRange color;

	for (int x = 0; x < 3; x++)
	{
		if (selected && mSelection == x)
			color = OptionSettings.mFontColorSelection;
		else if (x == mHighlight)
			color = OptionSettings.mFontColorHighlight;
		else
			color = OptionSettings.mFontColorValue;

		screen->DrawText (SmallFont, color, colwidth * x + 20 * CleanXfac_1, y, mResTexts[x], DTA_CleanNoMove_1, true, TAG_DONE);
	}
	return colwidth * mSelection + 20 * CleanXfac_1 - CURSORSPACE;
}

bool FOptionMenuScreenResolutionLine::Selectable()
{
	return mMaxValid >= 0;
}

void FOptionMenuScreenResolutionLine::Ticker()
{
	if (Selectable() && mSelection > mMaxValid)
	{
		mSelection = mMaxValid;
	}
}

//=============================================================================
//
// [TP] FOptionMenuFieldBase
//
// Base class for input fields
//
//=============================================================================

FOptionMenuFieldBase::FOptionMenuFieldBase (const char *label, const char *menu, const char *graycheck) :
	FOptionMenuItem (label, menu),
	mCVar (FindCVar(mAction, NULL)),
	mGrayCheck ((graycheck && strlen(graycheck)) ? FindCVar(graycheck, NULL) : NULL) {}

const char* FOptionMenuFieldBase::GetCVarString()
{
	if (mCVar == NULL)
		return "";
	else
		return mCVar->GetHumanString();
}

FString FOptionMenuFieldBase::Represent()
{
	return GetCVarString();
}

int FOptionMenuFieldBase::Draw (FOptionMenuDescriptor*, int y, int indent, bool selected)
{
	bool grayed = mGrayCheck != NULL && !(mGrayCheck->GetGenericRep(CVAR_Bool).Bool);
	drawLabel( indent, y, selected ? OptionSettings.mFontColorSelection : OptionSettings.mFontColor, grayed );
	int overlay = grayed? MAKEARGB( 96, 48, 0, 0 ) : 0;
	screen->DrawText(SmallFont, OptionSettings.mFontColorValue, indent + CURSORSPACE, y,
		Represent().GetChars(), DTA_CleanNoMove_1, true, DTA_ColorOverlay, overlay, TAG_DONE);
	return indent;
}

bool FOptionMenuFieldBase::GetString (int i, char* s, int len)
{
	if (i == 0)
	{
		strncpy(s, GetCVarString(), len);
		s[len - 1] = '\0';
		return true;
	}
	else
	{
		return false;
	}
}

bool FOptionMenuFieldBase::SetString (int i, const char* s)
{
	if (i == 0)
	{
		if (mCVar)
		{
			UCVarValue vval;
			vval.String = s;
			mCVar->SetGenericRep(vval, CVAR_String);
		}

		return true;
	}

	return false;
}

//=============================================================================
//
// [TP] FOptionMenuTextField
//
// A text input field widget, for use with string CVars.
//
//=============================================================================

FOptionMenuTextField::FOptionMenuTextField ( const char *label, const char* menu, const char* graycheck ) :
	FOptionMenuFieldBase ( label, menu, graycheck ),
	mEntering ( false ) {}

FString FOptionMenuTextField::Represent()
{
	FString text = mEntering ? mEditName : GetCVarString();

	if (mEntering)
		text += (gameinfo.gametype & GAME_DoomStrifeChex) ? '_' : '[';

	return text;
}

int FOptionMenuTextField::Draw (FOptionMenuDescriptor* desc, int y, int indent, bool selected)
{
	if (mEntering)
	{
		// reposition the text so that the cursor is visible when in entering mode.
		FString text = Represent();
		int tlen = SmallFont->StringWidth(text) * CleanXfac_1;
		int newindent = screen->GetWidth() - tlen - CURSORSPACE;
		if (newindent < indent)
			indent = newindent;
	}
	return FOptionMenuFieldBase::Draw(desc, y, indent, selected);
}

bool FOptionMenuTextField::MenuEvent (int mkey, bool fromcontroller)
{
	if ( mkey == MKEY_Enter )
	{
		S_Sound(CHAN_VOICE | CHAN_UI, "menu/choose", snd_menuvolume, ATTN_NONE);
		strcpy(mEditName, GetCVarString());
		mEntering = true;
		DMenu* input = new DTextEnterMenu (DMenu::CurrentMenu, mEditName, sizeof mEditName, 2, fromcontroller);
		M_ActivateMenu( input );
		return true;
	}
	else if (mkey == MKEY_Input)
	{
		if (mCVar)
		{
			UCVarValue vval;
			vval.String = mEditName;
			mCVar->SetGenericRep(vval, CVAR_String);
		}

		mEntering = false;
		return true;
	}
	else if (mkey == MKEY_Abort)
	{
		mEntering = false;
		return true;
	}

	return FOptionMenuItem::MenuEvent(mkey, fromcontroller);
}

//=============================================================================
//
// [TP] FOptionMenuNumberField
//
// A numeric input field widget, for use with number CVars where sliders are inappropriate (i.e.
// where the user is interested in the exact value specifically)
//
//=============================================================================

FOptionMenuNumberField::FOptionMenuNumberField (const char *label, const char* menu, float minimum, float maximum, float step, const char* graycheck)
	: FOptionMenuFieldBase (label, menu, graycheck),
	mMinimum (minimum),
	mMaximum (maximum),
	mStep (step)
{
	if (mMaximum <= mMinimum)
		swapvalues(mMinimum, mMaximum);

	if (mStep < 1)
		mStep = 1;
}

bool FOptionMenuNumberField::FOptionMenuNumberField::MenuEvent ( int mkey, bool fromcontroller )
{
	if (mCVar)
	{
		float value = mCVar->GetGenericRep( CVAR_Float ).Float;

		if (mkey == MKEY_Left)
		{
			value -= mStep;

			if (value < mMinimum)
				value = mMaximum;
		}
		else if (mkey == MKEY_Right || mkey == MKEY_Enter)
		{
			value += mStep;

			if (value > mMaximum)
				value = mMinimum;
		}
		else
			return FOptionMenuItem::MenuEvent(mkey, fromcontroller);

		UCVarValue vval;
		vval.Float = value;
		mCVar->SetGenericRep(vval, CVAR_Float);
		S_Sound(CHAN_VOICE | CHAN_UI, "menu/change", snd_menuvolume, ATTN_NONE);
	}

	return true;
}
