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
#include "blitter/factory.hpp"
#include "video/video_driver.hpp"

#include "widgets/build_confirmation_widget.h"
#include "build_confirmation_func.h"

#include "table/strings.h"

#include "safeguards.h"


/** GUI for confirming building actions. */
struct BuildConfirmationWindow : Window {

	// TODO: show estimated price
	static bool shown; // Just to speed up window hiding, HideBuildConfirmationWindow() is called very often
	uint8 * background;

	BuildConfirmationWindow(WindowDesc *desc) : Window(desc)
	{
		this->background = NULL;
		this->InitNested(0);
		BuildConfirmationWindow::shown = true;
	}

	~BuildConfirmationWindow()
	{
		BuildConfirmationWindow::shown = false;
		free(this->background);
	}

	void OnClick(Point pt, int widget, int click_count)
	{
		switch (widget) {
			case WID_BC_OK:
				ConfirmPlacingObject();
				ToolbarSelectLastTool();
				break;
			case WID_BC_CANCEL:
				ResetObjectToPlace();
				break;
		}
		HideBuildConfirmationWindow(); // this == NULL after this call
	}

	virtual void UpdateWidgetSize(int widget, Dimension *size, const Dimension &padding, Dimension *fill, Dimension *resize)
	{
		switch (widget) {
			case WID_BC_OK:
			case WID_BC_CANCEL:
				size->width = GetMinSizing(NWST_BUTTON) * 2;
				size->height = GetMinSizing(NWST_BUTTON) * 1.5;
				break;
		}
	}

	virtual void DrawWidget(const Rect &r, int widget) const
	{
		const NWidgetCore *w = GetWidget<NWidgetCore>(widget);
		Blitter *blitter = BlitterFactory::GetCurrentBlitter();
		// Draw our background
		
		blitter->CopyFromBuffer(blitter->MoveTo(_screen.dst_ptr, this->left, this->top + w->pos_y),
									this->background + w->pos_y * this->width * blitter->GetBytesPerPixel(),
									this->width, w->current_y);
		VideoDriver::GetInstance()->MakeDirty(this->left, this->top + w->pos_y, this->width, w->current_y);

		DrawFrameRect(w->pos_x, w->pos_y, w->pos_x + w->current_x, w->pos_y + w->current_y, w->colour, FR_TRANSPARENT); //FR_BORDERONLY
		DrawFrameRect(w->pos_x, w->pos_y, w->pos_x + w->current_x, w->pos_y + w->current_y, w->colour, FR_BORDERONLY);
		Dimension d = GetStringBoundingBox(w->widget_data);
		DrawFrameRect((w->pos_x + w->current_x - d.width - 2) / 2,
						Center(w->pos_y, w->current_y) - 2,
						(w->pos_x + w->current_x + d.width + 2) / 2,
						Center(w->pos_y, w->current_y) + d.height,
						w->colour, FR_NONE);
		DrawString(w->pos_x, w->pos_x + w->current_x, Center(w->pos_y, w->current_y), w->widget_data, TC_FROMSTRING, SA_HOR_CENTER);
	}

	void OnPaint()
	{
		Blitter *blitter = BlitterFactory::GetCurrentBlitter();
		if (this->background == NULL) {
			// Make a copy of the screen as it is before painting
			this->background = ReallocT(this->background, this->width * this->height * blitter->GetBytesPerPixel());
			printf("blitter->CopyToBuffer %d %d %p %d %d size %d\n", this->left, this->top, this->background, this->width, this->height, this->width * this->height * blitter->GetBytesPerPixel());
			blitter->CopyToBuffer(blitter->MoveTo(_screen.dst_ptr, this->left, this->top), this->background, this->width, this->height);
		}

		this->DrawWidgets();
	}
};

bool BuildConfirmationWindow::shown = false;

static const NWidgetPart _nested_build_confirmation_widgets[] = {
	NWidget(NWID_VERTICAL),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_BC_OK), SetDataTip(STR_BUTTON_OK, STR_NULL),
		NWidget(NWID_SPACER), SetMinimalSize(1, 1), SetFill(1, 1),
		NWidget(WWT_PUSHTXTBTN, COLOUR_GREY, WID_BC_CANCEL), SetDataTip(STR_BUTTON_CANCEL, STR_NULL),
	EndContainer(),
};

static WindowDesc _build_confirmation_desc(
	WDP_AUTO, "build_confirmation", 100, 80,
	WC_BUILD_CONFIRMATION, WC_NONE,
	WDF_CONSTRUCTION,
	_nested_build_confirmation_widgets, lengthof(_nested_build_confirmation_widgets)
);

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
