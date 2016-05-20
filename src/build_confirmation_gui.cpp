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
#include "station_gui.h"
#include "build_confirmation_func.h"
#include "widgets/build_confirmation_widget.h"
#include "widgets/misc_widget.h"

#include "table/strings.h"

#include "safeguards.h"

static const NWidgetPart _nested_build_info_widgets[] = {
	NWidget(WWT_PANEL, COLOUR_GREY, WID_TT_BACKGROUND), SetMinimalSize(200, 32), EndContainer(),
};

static WindowDesc _build_info_desc(
	WDP_MANUAL, NULL, 0, 0, // Coordinates and sizes are not used,
	WC_TOOLTIPS, WC_NONE,
	WDF_NO_FOCUS,
	_nested_build_info_widgets, lengthof(_nested_build_info_widgets)
);

/** Window for displaying accepted goods for a station. */
struct BuildInfoWindow : public Window
{
	StationCoverageType sct;
	bool station;
	static Money cost;

	static void show()
	{
		bool station = _settings_client.gui.station_show_coverage; // Station info is inaccurate when station coverage area option is disabled
		StationCoverageType sct = SCT_ALL;
		if (FindWindowByClass(WC_BUILD_STATION) != NULL) sct = SCT_ALL;
		else if (FindWindowByClass(WC_BUS_STATION) != NULL) sct = SCT_PASSENGERS_ONLY;
		else if (FindWindowByClass(WC_TRUCK_STATION) != NULL) sct = SCT_NON_PASSENGERS_ONLY;
		else station = false;
		new BuildInfoWindow(station, sct);
	}

	BuildInfoWindow(bool station, StationCoverageType sct) : Window(&_build_info_desc)
	{
		this->station = station;
		this->sct = sct;
		this->InitNested();

		CLRBITS(this->flags, WF_WHITE_BORDER);
	}

	virtual Point OnInitialPosition(int16 sm_width, int16 sm_height, int window_number)
	{
		Point pt;
		pt.y = GetMainViewTop();
		pt.x = _screen.width - sm_width - FindWindowById(WC_MAIN_TOOLBAR, 0)->width;
		return pt;
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		size->width  = GetStringBoundingBox(STR_STATION_BUILD_COVERAGE_AREA_TITLE).width * 2.5;
		size->height = GetStringHeight(STR_STATION_BUILD_COVERAGE_AREA_TITLE, size->width) * (this->station ? 3 : 1);

		/* Increase slightly to have some space around the box. */
		size->width  += 2 + WD_FRAMERECT_LEFT + WD_FRAMERECT_RIGHT;
		size->height += 2 + WD_FRAMERECT_TOP + WD_FRAMERECT_BOTTOM;
	}

	virtual void DrawWidget(const Rect &r, int widget) const
	{
		/* There is only one widget. */
		GfxFillRect(r.left, r.top, r.right, r.bottom, PC_BLACK);
		GfxFillRect(r.left + 1, r.top + 1, r.right - 1, r.bottom - 1, PC_LIGHT_YELLOW);

		int top = r.top + WD_FRAMERECT_TOP;
		Money cost = BuildInfoWindow::cost;
		StringID msg = STR_MESSAGE_ESTIMATED_COST;
		SetDParam(0, cost);
		if (cost < 0) {
			msg = STR_MESSAGE_ESTIMATED_INCOME;
			SetDParam(0, -cost);
		}
		top = DrawStringMultiLine(r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, top, INT32_MAX, msg);

		if (!this->station) return;

		top = DrawStationCoverageAreaText(r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, top, sct, _thd.outersize.x / TILE_SIZE / 2, false);
		if (top - r.top <= GetStringHeight(STR_STATION_BUILD_COVERAGE_AREA_TITLE, r.right - r.left) * 1.5) {
			DrawStationCoverageAreaText(r.left + WD_FRAMERECT_LEFT, r.right - WD_FRAMERECT_RIGHT, top, sct, _thd.outersize.x / TILE_SIZE / 2, true);
		}
	}
};

Money BuildInfoWindow::cost = 0;

/** GUI for confirming building actions. */
struct BuildConfirmationWindow : Window {

	// TODO: show estimated price
	static bool shown;   ///< Just to speed up window hiding, HideBuildConfirmationWindow() is called very often.
	static bool estimating_cost; ///< Calculate action cost instead of executing action.
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
		BuildConfirmationWindow::estimating_cost = true;
		ConfirmationWindowSetEstimatedCost(0); // Clear old value, just in case
		// This is a hack - we invoke the build command with estimating_cost flag, which is equal to _shift_pressed,
		// then we select last build tool, restore viewport selection, and hide all windows, which pop up when command is invoked,
		// and all that just to get cost estimate value.
		ConfirmPlacingObject();
		ToolbarSelectLastTool();
		_thd.selstart = selstart;
		_thd.selend = selend;
		BuildConfirmationWindow::estimating_cost = false;
		MoveAllWindowsOffScreen();
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
bool BuildConfirmationWindow::estimating_cost = false;

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
	if (ConfirmationWindowEstimatingCost()) return; // Special case, ignore recursive call

	HideBuildConfirmationWindow();

	if (!_settings_client.gui.build_confirmation || _shift_pressed) {
		ConfirmPlacingObject();
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

	BuildInfoWindow::show();
}

/**
 * Destroy build confirmation window, this does not cancel current action
*/
void HideBuildConfirmationWindow()
{
	if (ConfirmationWindowEstimatingCost()) return; // Special case, ignore recursive call

	if (!BuildConfirmationWindow::shown) return;

	DeleteWindowById(WC_BUILD_CONFIRMATION, 0);
	DeleteWindowById(WC_TOOLTIPS, 0);
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

bool ConfirmationWindowEstimatingCost()
{
	return BuildConfirmationWindow::estimating_cost;
}

void ConfirmationWindowSetEstimatedCost(Money cost)
{
	BuildInfoWindow::cost = cost;
}
