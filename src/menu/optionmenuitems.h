/*
** optionmenuitems.h
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
#include "menu.h"

class FOptionMenuItemSubmenu : public FOptionMenuItem
{
	int mParam;
public:
	FOptionMenuItemSubmenu(const char *label, const char *menu, int param = 0);
	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected);
	bool Activate();
};

class FOptionMenuItemCommand : public FOptionMenuItemSubmenu
{
public:
	FOptionMenuItemCommand(const char *label, const char *menu);
	bool Activate();
};

class FOptionMenuItemSafeCommand : public FOptionMenuItemCommand
{
	// action is a CCMD
protected:
	FString mPrompt;

public:
	FOptionMenuItemSafeCommand(const char *label, const char *menu, const char *prompt);
	bool MenuEvent (int mkey, bool fromcontroller);
	bool Activate();
};

class FOptionMenuItemOptionBase : public FOptionMenuItem
{
protected:
	// action is a CVAR
	FName mValues;	// Entry in OptionValues table
	FBaseCVar *mGrayCheck;
	int mCenter;
public:

	enum
	{
		OP_VALUES = 0x11001
	};

	FOptionMenuItemOptionBase(const char *label, const char *menu, const char *values, const char *graycheck, int center);
	bool SetString(int i, const char *newtext);
	virtual int GetSelection() = 0;
	virtual void SetSelection(int Selection) = 0;
	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected);
	bool MenuEvent (int mkey, bool fromcontroller);
	bool Selectable();
};

class FOptionMenuItemOption : public FOptionMenuItemOptionBase
{
	// action is a CVAR
	FBaseCVar *mCVar;
public:

	FOptionMenuItemOption(const char *label, const char *menu, const char *values, const char *graycheck, int center);
	int GetSelection();
	void SetSelection(int Selection);
};

class DEnterKey : public DMenu
{
	DECLARE_CLASS(DEnterKey, DMenu)

	int *pKey;

public:
	DEnterKey(DMenu *parent, int *keyptr);
	bool TranslateKeyboardEvents();
	void SetMenuMessage(int which);
	bool Responder(event_t *ev);
	void Drawer();
};

class FOptionMenuItemControl : public FOptionMenuItem
{
	FKeyBindings *mBindings;
	int mInput;
	bool mWaiting;
public:

	FOptionMenuItemControl(const char *label, const char *menu, FKeyBindings *bindings);
	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected);
	bool MenuEvent(int mkey, bool fromcontroller);
	bool Activate();
};

class FOptionMenuItemStaticText : public FOptionMenuItem
{
	EColorRange mColor;
public:
	FOptionMenuItemStaticText(const char *label, bool header);
	FOptionMenuItemStaticText(const char *label, EColorRange cr);
	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected);
	bool Selectable();
};

class FOptionMenuItemStaticTextSwitchable : public FOptionMenuItem
{
	EColorRange mColor;
	FString mAltText;
	int mCurrent;

public:
	FOptionMenuItemStaticTextSwitchable(const char *label, const char *label2, FName action, EColorRange cr);
	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected);
	bool SetValue(int i, int val);
	bool SetString(int i, const char *newtext);
	bool Selectable();
};

class FOptionMenuSliderBase : public FOptionMenuItem
{
	// action is a CVAR
	double mMin, mMax, mStep;
	int mShowValue;
	int mDrawX;
	int mSliderShort;

public:
	FOptionMenuSliderBase(const char *label, double min, double max, double step, int showval);
	virtual double GetSliderValue() = 0;
	virtual void SetSliderValue(double val) = 0;
	void DrawSlider (int x, int y, double min, double max, double cur, int fracdigits, int indent);
	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected);
	bool MenuEvent (int mkey, bool fromcontroller);
	bool MouseEvent(int type, int x, int y);
};

class FOptionMenuSliderCVar : public FOptionMenuSliderBase
{
public:
	FOptionMenuSliderCVar(const char *label, const char *menu, double min, double max, double step, int showval);
	double GetSliderValue();
	void SetSliderValue(double val);

private:
	FBaseCVar *mCVar;
};

class FOptionMenuSliderVar : public FOptionMenuSliderBase
{
public:
	FOptionMenuSliderVar(const char *label, float *pVal, double min, double max, double step, int showval);
	double GetSliderValue();
	void SetSliderValue(double val);

private:
	float *mPVal;
};

class FOptionMenuItemColorPicker : public FOptionMenuItem
{
public:
	enum
	{
		CPF_RESET = 0x20001,
	};

	FOptionMenuItemColorPicker(const char *label, const char *menu);
	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected);
	bool SetValue(int i, int v);
	bool Activate();

private:
	FColorCVar *mCVar;
};

class FOptionMenuScreenResolutionLine : public FOptionMenuItem
{
public:
	enum
	{
		SRL_INDEX = 0x30000,
		SRL_SELECTION = 0x30003,
		SRL_HIGHLIGHT = 0x30004,
	};

	FOptionMenuScreenResolutionLine(const char *action);
	bool SetValue(int i, int v);
	bool GetValue(int i, int *v);
	bool SetString(int i, const char *newtext);
	bool GetString(int i, char *s, int len);
	bool MenuEvent (int mkey, bool fromcontroller);
	bool MouseEvent(int type, int x, int y);
	bool Activate();
	int Draw(FOptionMenuDescriptor *desc, int y, int indent, bool selected);
	bool Selectable();
	void Ticker();
	
private:
	FString mResTexts[3];
	int mSelection;
	int mHighlight;
	int mMaxValid;
};

class FOptionMenuFieldBase : public FOptionMenuItem
{
public:
	FOptionMenuFieldBase (const char* label, const char* menu, const char* graycheck);
	const char* GetCVarString();
	virtual FString Represent();
	int Draw (FOptionMenuDescriptor*, int y, int indent, bool selected);
	bool GetString (int i, char* s, int len);
	bool SetString (int i, const char* s);

protected:
	// Action is a CVar in this class and derivatives.
	FBaseCVar* mCVar;
	FBaseCVar* mGrayCheck;
};

class FOptionMenuTextField : public FOptionMenuFieldBase
{
public:
	FOptionMenuTextField (const char *label, const char* menu, const char* graycheck);

	FString Represent();
	int Draw (FOptionMenuDescriptor*desc, int y, int indent, bool selected);
	bool MenuEvent (int mkey, bool fromcontroller);

private:
	bool mEntering;
	char mEditName[128];
};

class FOptionMenuNumberField : public FOptionMenuFieldBase
{
public:
	FOptionMenuNumberField (const char *label, const char* menu, float minimum, float maximum, float step, const char* graycheck);
	bool MenuEvent (int mkey, bool fromcontroller);

private:
	float mMinimum;
	float mMaximum;
	float mStep;
};
