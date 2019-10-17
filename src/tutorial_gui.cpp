/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file tutorial_gui.cpp
 * Links to video tutorials on Youtube.
 */

#include "stdafx.h"

#ifdef __ANDROID__
#include <SDL_android.h>
#endif

#include "tutorial_gui.h"
#include "debug.h"
#include "strings_func.h"
#include "window_func.h"
#include "fios.h"
#include "string_func.h"
#include "language.h"
#include "widget_type.h"
#include "window_type.h"
#include "window_func.h"
#include "window_gui.h"
#include "widgets/station_widget.h"
#include "table/strings.h"
#include "table/sprites.h"


static bool showTutorialMainMenu = false;

static const char * ANY_LANG = "ANY_LANG";

struct VideoLink_t {
	const char *lang;
	const char *video;
};

static VideoLink_t busTutorial[] = {
	{ "en",     "https://www.youtube.com/watch?v=EULXRMR4PyE" },
	{ ANY_LANG, "https://www.youtube.com/watch?v=EULXRMR4PyE" },
	{ NULL, NULL }
};

static VideoLink_t trainTutorial[] = {
	{ "en",     "https://www.youtube.com/watch?v=VdMdL2qyZ6s" },
	{ ANY_LANG, "https://www.youtube.com/watch?v=VdMdL2qyZ6s" },
	{ NULL, NULL }
};

static VideoLink_t truckTutorial[] = {
	{ "en",     "https://www.youtube.com/watch?v=B-CL-XFGNtw" },
	{ ANY_LANG, "https://www.youtube.com/watch?v=B-CL-XFGNtw" },
	{ NULL, NULL }
};

static VideoLink_t shipTutorial[] = {
	{ "en",     "https://www.youtube.com/watch?v=a5JHlWtIg3A" },
	{ ANY_LANG, "https://www.youtube.com/watch?v=a5JHlWtIg3A" },
	{ NULL, NULL }
};

static VideoLink_t cargoTutorial[] = {
	{ "en",     "https://www.youtube.com/watch?v=GwjiQYsu3xg" },
	{ ANY_LANG, "https://www.youtube.com/watch?v=GwjiQYsu3xg" },
	{ NULL, NULL }
};

void OpenExternTutorialVideo(VideoLink_t *tutorial)
{
	const char *link = NULL;
	for (; tutorial->lang != NULL; tutorial++) {
		if (strcmp(tutorial->lang, _current_language->isocode) == 0) {
			link = tutorial->video;
			break;
		}
		if (strcmp(tutorial->lang, ANY_LANG) == 0) {
			link = tutorial->video;
			break;
		}
	}
	if (!link) {
		return;
	}
#ifdef __ANDROID__
	SDL_ANDROID_OpenExternalWebBrowser(link);
#else
	char cmd[PATH_MAX] =
#ifdef WIN32
		"start ";
#else
		"xdg-open ";
#endif
	strcat(cmd, link);
	system(cmd);
#endif
}

static const NWidgetPart _nested_tutorial_widgets[] = {
	NWidget(NWID_HORIZONTAL),
		NWidget(WWT_CLOSEBOX, COLOUR_GREY),
		NWidget(WWT_CAPTION, COLOUR_GREY), SetDataTip(STR_TUTORIAL_WINDOW_TITLE, STR_TUTORIAL_WINDOW_TOOLTIP),
	EndContainer(),
	NWidget(WWT_PANEL, COLOUR_GREY),
		NWidget(NWID_HORIZONTAL),
			NWidget(NWID_SPACER), SetMinimalSize(6, 0), SetFill(1, 0),
			NWidget(NWID_VERTICAL), SetPIP(16, 2, 6),
				// TODO: make different button IDs
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_STL_BUS), SetMinimalSize(120, 20), SetDataTip(STR_TUTORIAL_ROADS_AND_STATIONS, STR_TUTORIAL_ROADS_AND_STATIONS), SetFill(1, 1),
				NWidget(NWID_SPACER), SetMinimalSize(0, 10), SetFill(1, 0),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_STL_TRAIN), SetMinimalSize(120, 20), SetDataTip(STR_TUTORIAL_RAILWAYS, STR_TUTORIAL_RAILWAYS), SetFill(1, 1),
				NWidget(NWID_SPACER), SetMinimalSize(0, 10), SetFill(1, 0),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_STL_TRUCK), SetMinimalSize(120, 20), SetDataTip(STR_TUTORIAL_ROAD_VEHICLES, STR_TUTORIAL_ROAD_VEHICLES), SetFill(1, 1),
				NWidget(NWID_SPACER), SetMinimalSize(0, 10), SetFill(1, 0),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_STL_SHIP), SetMinimalSize(120, 20), SetDataTip(STR_TUTORIAL_SHIPS, STR_TUTORIAL_SHIPS), SetFill(1, 1),
				NWidget(NWID_SPACER), SetMinimalSize(0, 10), SetFill(1, 0),
				NWidget(WWT_TEXTBTN, COLOUR_GREY, WID_STL_FACILALL), SetMinimalSize(120, 20), SetDataTip(STR_TUTORIAL_CARGO, STR_TUTORIAL_CARGO), SetFill(1, 1),
			EndContainer(),
			NWidget(NWID_SPACER), SetMinimalSize(6, 0), SetFill(1, 0),
		EndContainer(),
	EndContainer(),
};

static WindowDesc _tutorial_desc(
	WDP_CENTER, NULL, 0, 0,
	WC_GAME_OPTIONS, WC_NONE,
	0,
	_nested_tutorial_widgets, lengthof(_nested_tutorial_widgets)
);


struct TutorialWindow : public Window {
	VideoLink_t *video;

	TutorialWindow() : Window(&_tutorial_desc)
	{
		this->InitNested(WN_GAME_OPTIONS_ABOUT);
		this->video = NULL;
	}

	virtual void OnClick(Point pt, int widget, int click_count)
	{
		showTutorialMainMenu = false;
		this->video = NULL;
		this->SetDirty();
		switch (widget) {
			case WID_STL_BUS:
				this->video = busTutorial;
				break;
			case WID_STL_TRUCK:
				this->video = truckTutorial;
				break;
			case WID_STL_TRAIN:
				this->video = trainTutorial;
				break;
			case WID_STL_SHIP:
				this->video = shipTutorial;
				break;
			//case WID_STL_AIRPLANE:
			//	this->video = planeTutorial;
			//	break;
			case WID_STL_FACILALL:
				this->video = cargoTutorial;
				break;
		}
		if (this->video) {
			OpenExternTutorialVideo(this->video);
			this->video = NULL;
		}
	}
};

void ShowTutorialWindow()
{
	DeleteWindowByClass(WC_GAME_OPTIONS);
	new TutorialWindow();
}

void ShowTutorialWindowOnceAfterInstall()
{
	// Close button on tutorial window is gone, so don't show that windows on first run, it's confusing
#if 0
	static const char * TUTORIAL_SHOWN_FLAG = ".tutorial-shown-3.flag";

	FILE *ff = fopen(TUTORIAL_SHOWN_FLAG, "r");
	if (ff) {
		fclose(ff);
		if (!showTutorialMainMenu)
			return;
	}
	showTutorialMainMenu = true;
	ff = fopen(TUTORIAL_SHOWN_FLAG, "w");
	fprintf(ff, "Tutorial shown");
	fclose(ff);
	ShowTutorialWindow();
#endif
}
