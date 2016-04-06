/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file build_confirmation_gui.cpp Transparent confirmation dialog for building anything on the map. */

#include "stdafx.h"
#include "string_func.h"
#include "strings_func.h"
#include "window_func.h"
#include "widget_type.h"
#include "window_gui.h"
#include "gfx_func.h"
#include "tilehighlight_func.h"

#include "widgets/build_confirmation_widget.h"
#include "build_confirmation_func.h"

#include "table/strings.h"

#include "safeguards.h"


static const NWidgetPart _nested_build_confirmation_widgets[] = {
	NWidget(NWID_VERTICAL),
		NWidget(NWID_SPACER), SetFill(1, 1),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_BC_OK), SetDataTip(STR_BUTTON_OK, STR_NULL), SetSizingType(NWST_BUTTON),
		NWidget(NWID_SPACER), SetFill(1, 1),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_BC_CANCEL), SetDataTip(STR_BUTTON_CANCEL, STR_NULL), SetSizingType(NWST_BUTTON),
		NWidget(NWID_SPACER), SetFill(1, 1),
	EndContainer(),
};

/** GUI for confirming building actions. */
struct BuildConfirmationWindow : Window {

	BuildConfirmationWindow(WindowDesc *desc) : Window(desc)
	{
		this->InitNested(0);
	}

	void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_BC_OK:
				ConfirmPlacingObject();
				break;
			case WID_BC_CANCEL:
				ResetObjectToPlace();
				break;
		}
		HideBuildConfirmationWindow(); // this == NULL after this call
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
	}

	/*
	virtual void DrawWidget(const Rect &r, int widget) const
	{
		switch (widget) {
			case WID_BV_LIST:
				break;
		}
	}
	*/

	/*
	virtual void OnPaint()
	{
		this->DrawWidgets();
	}
	*/
};

static WindowDesc _build_confirmation_desc(
	WDP_AUTO, "build_confirmation", 100, 80,
	WC_BUILD_CONFIRMATION, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_build_confirmation_widgets, lengthof(_nested_build_confirmation_widgets)
);

static bool _confirmationWindowShown = false; // Just to speed up window hiding, HideBuildConfirmationWindow() is called very often

/**
 * Show build confirmation window under the mouse cursor
*/
void ShowBuildConfirmationWindow()
{
	HideBuildConfirmationWindow();
	BuildConfirmationWindow *wnd = new BuildConfirmationWindow(&_build_confirmation_desc);
	wnd->left = _cursor.pos.x - wnd->width / 2;
	wnd->top = _cursor.pos.y - wnd->height / 4;
	wnd->SetDirty();
	_confirmationWindowShown = true;
}

/**
 * Destory build confirmation window, this does not cancel current action
*/
void HideBuildConfirmationWindow()
{
	if (!_confirmationWindowShown) {
		//return;
	}
	DeleteWindowById(WC_BUILD_CONFIRMATION, 0);
	_confirmationWindowShown = false;
}

bool ConfirmationWindowShown()
{
	return _confirmationWindowShown;
}
