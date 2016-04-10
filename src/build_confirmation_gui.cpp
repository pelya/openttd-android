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
#include "viewport_func.h"
#include "zoom_func.h"
#include "settings_type.h"

#include "widgets/build_confirmation_widget.h"
#include "build_confirmation_func.h"

#include "table/strings.h"

#include "safeguards.h"


/** GUI for confirming building actions. */
struct BuildConfirmationWindow : Window {

	// TODO: show estimated price
	static bool shown;   ///< Just to speed up window hiding, HideBuildConfirmationWindow() is called very often.
	Point selstart;      ///< The selection start on the viewport.
	Point selend;        ///< The selection end on the viewport.

	BuildConfirmationWindow(WindowDesc *desc) : Window(desc)
	{
		// Save tile selection points, they will be reset by subsequent code, and we must keep them
		selstart = _thd.selstart;
		selend = _thd.selend;

		this->InitNested(0);

		Point pt;
		const Window *w = FindWindowById(WC_MAIN_WINDOW, 0);
		NWidgetViewport *nvp = this->GetWidget<NWidgetViewport>(WID_BC_OK);

		pt.x = w->viewport->scrollpos_x + ScaleByZoom(_cursor.pos.x - nvp->current_x / 2, w->viewport->zoom);
		pt.y = w->viewport->scrollpos_y + ScaleByZoom(_cursor.pos.y - nvp->current_y / 4, w->viewport->zoom);

		nvp->InitializeViewport(this, 0, w->viewport->zoom);
		nvp->disp_flags |= ND_SHADE_DIMMED;

		this->viewport->scrollpos_x = pt.x;
		this->viewport->scrollpos_y = pt.y;
		this->viewport->dest_scrollpos_x = this->viewport->scrollpos_x;
		this->viewport->dest_scrollpos_y = this->viewport->scrollpos_y;

		BuildConfirmationWindow::shown = true;
	}

	~BuildConfirmationWindow()
	{
		BuildConfirmationWindow::shown = false;
	}

	void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_BC_OK:
				if (pt.y <= (int)GetWidget<NWidgetViewport>(WID_BC_OK)->current_y / 2) {
					_thd.selstart = selstart;
					_thd.selend = selend;
					ConfirmPlacingObject();
					ToolbarSelectLastTool();
				} else {
					ResetObjectToPlace();
				}
				break;
		}
		HideBuildConfirmationWindow(); // this == NULL after this call
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case WID_BC_OK:
				size->width = GetMinSizing(NWST_BUTTON) * 2;
				size->height = GetMinSizing(NWST_BUTTON) * 3;
				break;
		}
	}

	virtual void OnPaint()
	{
		this->DrawWidgets();

		DrawButtonFrame(0, 0, this->width - 1, this->height / 2 - 2, STR_BUTTON_OK);
		DrawButtonFrame(0, this->height / 2, this->width - 1, this->height / 2 - 1, STR_BUTTON_CANCEL);
	}

	void DrawButtonFrame(int x, int y, int w, int h, int str)
	{
		DrawFrameRect(x, y, x + w, y + h, COLOUR_GREY, FR_BORDERONLY);
		Dimension d = GetStringBoundingBox(str);
		DrawFrameRect(x + w / 2 - d.width / 2 - 1,
						Center(y, h) - 2,
						x + w / 2 + d.width / 2 + 1,
						Center(y, h) + d.height,
						COLOUR_GREY, FR_NONE);
		DrawString(x, x + w, Center(y, h), str, TC_FROMSTRING, SA_HOR_CENTER);
	}
};

bool BuildConfirmationWindow::shown = false;

static const NWidgetPart _nested_build_confirmation_widgets[] = {
	NWidget(WWT_PANEL, COLOUR_GREY, WID_BC_PANEL),
		NWidget(NWID_VIEWPORT, INVALID_COLOUR, WID_BC_OK), SetSizingType(NWST_VIEWPORT), SetResize(1, 1), SetFill(1, 1), //SetPadding(2, 2, 2, 2),
	EndContainer(),
};

static WindowDesc _build_confirmation_desc(
	WDP_MANUAL, "build_confirmation", 0, 0,
	WC_BUILD_CONFIRMATION, WC_NONE,
	0,
	_nested_build_confirmation_widgets, lengthof(_nested_build_confirmation_widgets)
);

/**
 * Show build confirmation window under the mouse cursor
*/
void ShowBuildConfirmationWindow()
{
	HideBuildConfirmationWindow();

	if (!_settings_client.gui.build_confirmation || _shift_pressed) {
		ConfirmPlacingObject();
		ToolbarSelectLastTool();
		return;
	}

	BuildConfirmationWindow *w = new BuildConfirmationWindow(&_build_confirmation_desc);

	int old_left = w->left;
	int old_top = w->top;
	w->left = _cursor.pos.x - w->width / 2;
	w->top = _cursor.pos.y - w->height / 4;
	w->viewport->left += w->left - old_left;
	w->viewport->top += w->top - old_top;
	w->SetDirty();
	SetDirtyBlocks(0, 0, _screen.width, _screen.height); // I don't know what does this do, but it looks important
}

/**
 * Destory build confirmation window, this does not cancel current action
*/
void HideBuildConfirmationWindow()
{
	if (!BuildConfirmationWindow::shown) return;

	DeleteWindowById(WC_BUILD_CONFIRMATION, 0);
}

bool ConfirmationWindowShown()
{
	return BuildConfirmationWindow::shown;
}

bool BuildConfirmationWindowProcessViewportClick()
{
	if (!BuildConfirmationWindow::shown) return false;

	Window *w = FindWindowById(WC_BUILD_CONFIRMATION, 0);
	if (w != NULL && IsInsideBS(_cursor.pos.x, w->left, w->width) && IsInsideBS(_cursor.pos.y, w->top, w->height)) {
		Point pt;
		pt.x = _cursor.pos.x - w->left;
		pt.y = _cursor.pos.y - w->top;
		w->OnClick(pt, WID_BC_OK, 1);
		return true;
	}

	HideBuildConfirmationWindow();

	_thd.new_outersize = _thd.outersize; // Revert station catchment area highlight, which is getting set to zero inside drawing funcs

	return false;
}
