/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file sdl2_v.cpp Implementation of the SDL2 video driver. */

#include "../stdafx.h"
#include "../openttd.h"
#include "../gfx_func.h"
#include "../rev.h"
#include "../blitter/factory.hpp"
#include "../thread.h"
#include "../progress.h"
#include "../core/random_func.hpp"
#include "../core/math_func.hpp"
#include "../core/mem_func.hpp"
#include "../core/geometry_func.hpp"
#include "../fileio_func.h"
#include "../framerate_type.h"
#include "../window_func.h"
#include "sdl2_v.h"
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#	include <emscripten.h>
#	include <emscripten/html5.h>
#endif

#include "../safeguards.h"

static bool old_ctrl_pressed = false;
static Point _multitouch_second_point = {-1, -1};
static int _multitouch_finger_distance = 0;
enum { PINCH_ZOOM_SENSITIVITY = 20 };
#ifdef __EMSCRIPTEN__
static bool fullscreen_first_click = false;
#endif

void VideoDriver_SDL_Base::MakeDirty(int left, int top, int width, int height)
{
	Rect r = {left, top, left + width, top + height};
	this->dirty_rect = BoundingRect(this->dirty_rect, r);
}

void VideoDriver_SDL_Base::CheckPaletteAnim()
{
	if (!CopyPalette(this->local_palette)) return;
	this->MakeDirty(0, 0, _screen.width, _screen.height);
}

static const Dimension default_resolutions[] = {
	{  640,  480 },
	{  800,  600 },
	{ 1024,  768 },
	{ 1152,  864 },
	{ 1280,  800 },
	{ 1280,  960 },
	{ 1280, 1024 },
	{ 1400, 1050 },
	{ 1600, 1200 },
	{ 1680, 1050 },
	{ 1920, 1200 }
};

static void FindResolutions()
{
	_resolutions.clear();

	for (int i = 0; i < SDL_GetNumDisplayModes(0); i++) {
		SDL_DisplayMode mode;
		SDL_GetDisplayMode(0, i, &mode);

		if (mode.w < 640 || mode.h < 480) continue;
		if (std::find(_resolutions.begin(), _resolutions.end(), Dimension(mode.w, mode.h)) != _resolutions.end()) continue;
		_resolutions.emplace_back(mode.w, mode.h);
	}

	/* We have found no resolutions, show the default list */
	if (_resolutions.empty()) {
		_resolutions.assign(std::begin(default_resolutions), std::end(default_resolutions));
	}

	SortResolutions();

#ifdef __EMSCRIPTEN__
	// Only the browser window size is available on Emscripten, clean all other modes
	_resolutions.clear();
	SDL_DisplayMode mode;
	SDL_GetDesktopDisplayMode(0, &mode);
	// Stupid mobile web browser reports 2x smaller window size
	mode.w *= emscripten_get_device_pixel_ratio();
	mode.h *= emscripten_get_device_pixel_ratio();
	_resolutions.emplace_back(mode.w, mode.h); // 1920x1080
	_resolutions.emplace_back(mode.w * 8 / 9, mode.h * 8 / 9); // 8 / 9 : 1920x1080 -> 1706x960
	_resolutions.emplace_back(mode.w * 7 / 8, mode.h * 7 / 8); // 7 / 8 : 1920x1080 -> 1680x945
	_resolutions.emplace_back(mode.w * 5 / 6, mode.h * 5 / 6); // 5 / 6 : 1920x1080 -> 1600x900
	_resolutions.emplace_back(mode.w * 7 / 9, mode.h * 7 / 9); // 7 / 9 : 1920x1080 -> 1493x840
	_resolutions.emplace_back(mode.w * 6 / 8, mode.h * 6 / 8); // 6 / 8 : 1920x1080 -> 1440x810
	_resolutions.emplace_back(mode.w * 4 / 6, mode.h * 4 / 6); // 4 / 6 : 1920x1080 -> 1280x720
	_resolutions.emplace_back(mode.w * 5 / 8, mode.h * 5 / 8); // 5 / 8 : 1920x1080 -> 1200x675
	_resolutions.emplace_back(mode.w * 5 / 9, mode.h * 5 / 9); // 5 / 9 : 1920x1080 -> 1066x600
	_resolutions.emplace_back(mode.w * 3 / 6, mode.h * 3 / 6); // 3 / 6 : 1920x1080 -> 960x540
	_resolutions.emplace_back(mode.w * 4 / 9, mode.h * 4 / 9); // 4 / 9 : 1920x1080 -> 853x480
	_resolutions.emplace_back(mode.w * 3 / 8, mode.h * 3 / 8); // 3 / 8 : 1920x1080 -> 720x405
	_resolutions.emplace_back(mode.w * 2 / 6, mode.h * 2 / 6); // 2 / 6 : 1920x1080 -> 640x360
	_resolutions.emplace_back(mode.w * 5 / 18, mode.h * 5 / 18); // 5 / 18: 1920x1080 -> 533x300
	_resolutions.emplace_back(mode.w * 2 / 8, mode.h * 2 / 8); // 2 / 8 : 1920x1080 -> 480x270
	_resolutions.emplace_back(mode.w * 2 / 9, mode.h * 2 / 9); // 2 / 9 : 1920x1080 -> 426x240
#endif
}

static void GetAvailableVideoMode(uint *w, uint *h)
{
#ifndef __EMSCRIPTEN__ // Ignore fullscreen flag in the mobile web browser, we want to match the resolution anyway
	/* All modes available? */
	if (!_fullscreen || _resolutions.empty()) return;
#endif
	/* Is the wanted mode among the available modes? */
	if (std::find(_resolutions.begin(), _resolutions.end(), Dimension(*w, *h)) != _resolutions.end()) return;

	/* Use the closest possible resolution */
	uint best = 0;
	uint delta = Delta(_resolutions[0].width, *w) * Delta(_resolutions[0].height, *h);
	if (*w <= 1) {
		delta = Delta(_resolutions[0].height, *h);
	}
	for (uint i = 1; i != _resolutions.size(); ++i) {
		uint newdelta = Delta(_resolutions[i].width, *w) * Delta(_resolutions[i].height, *h);
		if (*w <= 1) {
			newdelta = Delta(_resolutions[i].height, *h);
		}
		if (newdelta < delta) {
			best = i;
			delta = newdelta;
		}
	}
	*w = _resolutions[best].width;
	*h = _resolutions[best].height;
}

static uint FindStartupDisplay(uint startup_display)
{
	int num_displays = SDL_GetNumVideoDisplays();

	/* If the user indicated a valid monitor, use that. */
	if (IsInsideBS(startup_display, 0, num_displays)) return startup_display;

	/* Mouse position decides which display to use. */
	int mx, my;
	SDL_GetGlobalMouseState(&mx, &my);
	for (int display = 0; display < num_displays; ++display) {
		SDL_Rect r;
		if (SDL_GetDisplayBounds(display, &r) == 0 && IsInsideBS(mx, r.x, r.w) && IsInsideBS(my, r.y, r.h)) {
			Debug(driver, 1, "SDL2: Mouse is at ({}, {}), use display {} ({}, {}, {}, {})", mx, my, display, r.x, r.y, r.w, r.h);
			return display;
		}
	}

	return 0;
}

void VideoDriver_SDL_Base::ClientSizeChanged(int w, int h, bool force)
{
	/* Allocate backing store of the new size. */
	if (this->AllocateBackingStore(w, h, force)) {
		CopyPalette(this->local_palette, true);

		BlitterFactory::GetCurrentBlitter()->PostResize();

		GameSizeChanged();
	}
}

bool VideoDriver_SDL_Base::CreateMainWindow(uint w, uint h, uint flags)
{
	if (this->sdl_window != nullptr) return true;

	flags |= SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

	if (_fullscreen) {
		flags |= SDL_WINDOW_FULLSCREEN;
#ifdef __EMSCRIPTEN__
		fullscreen_first_click = true;
#endif
	}

	int x = SDL_WINDOWPOS_UNDEFINED, y = SDL_WINDOWPOS_UNDEFINED;
	SDL_Rect r;
	if (SDL_GetDisplayBounds(this->startup_display, &r) == 0) {
		x = r.x + std::max(0, r.w - static_cast<int>(w)) / 2;
		y = r.y + std::max(0, r.h - static_cast<int>(h)) / 4; // decent desktops have taskbars at the bottom
	}

	SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, "#canvas");
	char caption[50];
	seprintf(caption, lastof(caption), "OpenTTD %s", _openttd_revision);
	this->sdl_window = SDL_CreateWindow(
		caption,
		x, y,
		w, h,
		flags);

	if (this->sdl_window == nullptr) {
		Debug(driver, 0, "SDL2: Couldn't allocate a window to draw on: {}", SDL_GetError());
		return false;
	}

	std::string icon_path = FioFindFullPath(BASESET_DIR, "openttd.32.bmp");
	if (!icon_path.empty()) {
		/* Give the application an icon */
		SDL_Surface *icon = SDL_LoadBMP(icon_path.c_str());
		if (icon != nullptr) {
			/* Get the colourkey, which will be magenta */
			uint32 rgbmap = SDL_MapRGB(icon->format, 255, 0, 255);

			SDL_SetColorKey(icon, SDL_TRUE, rgbmap);
			SDL_SetWindowIcon(this->sdl_window, icon);
			SDL_FreeSurface(icon);
		}
	}

	return true;
}

bool VideoDriver_SDL_Base::CreateMainSurface(uint w, uint h, bool resize)
{
	GetAvailableVideoMode(&w, &h);
	Debug(driver, 1, "SDL2: using mode {}x{}", w, h);

	if (!this->CreateMainWindow(w, h)) return false;
#ifdef __EMSCRIPTEN__
	int cur_w = w, cur_h = h;
	SDL_GetWindowSize(this->sdl_window, &cur_w, &cur_h);
	if (cur_w != (int)w || cur_h != (int)h) resize = true;
#endif
	if (resize) SDL_SetWindowSize(this->sdl_window, w, h);
	this->ClientSizeChanged(w, h, true);

	/* When in full screen, we will always have the mouse cursor
	 * within the window, even though SDL does not give us the
	 * appropriate event to know this. */
	if (_fullscreen) _cursor.in_window = true;

	return true;
}

bool VideoDriver_SDL_Base::ClaimMousePointer()
{
	/* Emscripten never claims the pointer, so we do not need to change the cursor visibility. */
#ifndef __EMSCRIPTEN__
	SDL_ShowCursor(0);
#endif
	return true;
}

/**
 * This is called to indicate that an edit box has gained focus, text input mode should be enabled.
 */
void VideoDriver_SDL_Base::EditBoxGainedFocus()
{
	if (!this->edit_box_focused) {
		SDL_StartTextInput();
		this->edit_box_focused = true;
	}
}

/**
 * This is called to indicate that an edit box has lost focus, text input mode should be disabled.
 */
void VideoDriver_SDL_Base::EditBoxLostFocus()
{
	if (this->edit_box_focused) {
		SDL_StopTextInput();
		this->edit_box_focused = false;
	}
}

std::vector<int> VideoDriver_SDL_Base::GetListOfMonitorRefreshRates()
{
	std::vector<int> rates = {};
	for (int i = 0; i < SDL_GetNumVideoDisplays(); i++) {
		SDL_DisplayMode mode = {};
		if (SDL_GetDisplayMode(i, 0, &mode) != 0) continue;
		if (mode.refresh_rate != 0) rates.push_back(mode.refresh_rate);
	}
	return rates;
}


struct SDLVkMapping {
	SDL_Keycode vk_from;
	byte vk_count;
	byte map_to;
	bool unprintable;
};

#define AS(x, z) {x, 0, z, false}
#define AM(x, y, z, w) {x, (byte)(y - x), z, false}
#define AS_UP(x, z) {x, 0, z, true}
#define AM_UP(x, y, z, w) {x, (byte)(y - x), z, true}

static const SDLVkMapping _vk_mapping[] = {
	/* Pageup stuff + up/down */
	AS_UP(SDLK_PAGEUP,   WKC_PAGEUP),
	AS_UP(SDLK_PAGEDOWN, WKC_PAGEDOWN),
	AS_UP(SDLK_UP,     WKC_UP),
	AS_UP(SDLK_DOWN,   WKC_DOWN),
	AS_UP(SDLK_LEFT,   WKC_LEFT),
	AS_UP(SDLK_RIGHT,  WKC_RIGHT),

	AS_UP(SDLK_HOME,   WKC_HOME),
	AS_UP(SDLK_END,    WKC_END),

	AS_UP(SDLK_INSERT, WKC_INSERT),
	AS_UP(SDLK_DELETE, WKC_DELETE),

	/* Map letters & digits */
	AM(SDLK_a, SDLK_z, 'A', 'Z'),
	AM(SDLK_0, SDLK_9, '0', '9'),

	AS_UP(SDLK_ESCAPE,    WKC_ESC),
	AS_UP(SDLK_PAUSE,     WKC_PAUSE),
	AS_UP(SDLK_BACKSPACE, WKC_BACKSPACE),

	AS(SDLK_SPACE,     WKC_SPACE),
	AS(SDLK_RETURN,    WKC_RETURN),
	AS(SDLK_TAB,       WKC_TAB),

	/* Function keys */
	AM_UP(SDLK_F1, SDLK_F12, WKC_F1, WKC_F12),

	/* Numeric part. */
	AM(SDLK_KP_0, SDLK_KP_9, '0', '9'),
	AS(SDLK_KP_DIVIDE,   WKC_NUM_DIV),
	AS(SDLK_KP_MULTIPLY, WKC_NUM_MUL),
	AS(SDLK_KP_MINUS,    WKC_NUM_MINUS),
	AS(SDLK_KP_PLUS,     WKC_NUM_PLUS),
	AS(SDLK_KP_ENTER,    WKC_NUM_ENTER),
	AS(SDLK_KP_PERIOD,   WKC_NUM_DECIMAL),

	/* Other non-letter keys */
	AS(SDLK_SLASH,        WKC_SLASH),
	AS(SDLK_SEMICOLON,    WKC_SEMICOLON),
	AS(SDLK_EQUALS,       WKC_EQUALS),
	AS(SDLK_LEFTBRACKET,  WKC_L_BRACKET),
	AS(SDLK_BACKSLASH,    WKC_BACKSLASH),
	AS(SDLK_RIGHTBRACKET, WKC_R_BRACKET),

	AS(SDLK_QUOTE,   WKC_SINGLEQUOTE),
	AS(SDLK_COMMA,   WKC_COMMA),
	AS(SDLK_MINUS,   WKC_MINUS),
	AS(SDLK_PERIOD,  WKC_PERIOD)
};

static uint ConvertSdlKeyIntoMy(SDL_Keysym *sym, WChar *character)
{
	const SDLVkMapping *map;
	uint key = 0;
	bool unprintable = false;

	for (map = _vk_mapping; map != endof(_vk_mapping); ++map) {
		if ((uint)(sym->sym - map->vk_from) <= map->vk_count) {
			key = sym->sym - map->vk_from + map->map_to;
			unprintable = map->unprintable;
			break;
		}
	}

	/* check scancode for BACKQUOTE key, because we want the key left of "1", not anything else (on non-US keyboards) */
	if (sym->scancode == SDL_SCANCODE_GRAVE) key = WKC_BACKQUOTE;

	/* META are the command keys on mac */
	if (sym->mod & KMOD_GUI)   key |= WKC_META;
	if (sym->mod & KMOD_SHIFT) key |= WKC_SHIFT;
	if (sym->mod & KMOD_CTRL)  key |= WKC_CTRL;
	if (sym->mod & KMOD_ALT)   key |= WKC_ALT;

	/* The mod keys have no character. Prevent '?' */
	if (sym->mod & KMOD_GUI ||
		sym->mod & KMOD_CTRL ||
		sym->mod & KMOD_ALT ||
		unprintable) {
		*character = WKC_NONE;
	} else {
		*character = sym->sym;
	}

	return key;
}

/**
 * Like ConvertSdlKeyIntoMy(), but takes an SDL_Keycode as input
 * instead of an SDL_Keysym.
 */
static uint ConvertSdlKeycodeIntoMy(SDL_Keycode kc)
{
	const SDLVkMapping *map;
	uint key = 0;

	for (map = _vk_mapping; map != endof(_vk_mapping); ++map) {
		if ((uint)(kc - map->vk_from) <= map->vk_count) {
			key = kc - map->vk_from + map->map_to;
			break;
		}
	}

	/* check scancode for BACKQUOTE key, because we want the key left
	 * of "1", not anything else (on non-US keyboards) */
	SDL_Scancode sc = SDL_GetScancodeFromKey(kc);
	if (sc == SDL_SCANCODE_GRAVE) key = WKC_BACKQUOTE;

	return key;
}

bool VideoDriver_SDL_Base::PollEvent()
{
	SDL_Event ev;

	if (!SDL_PollEvent(&ev)) return false;

	switch (ev.type) {
		case SDL_MOUSEMOTION: {
			int32_t x = ev.motion.x;
			int32_t y = ev.motion.y;

			if (_cursor.fix_at) {
				/* Get all queued mouse events now in case we have to warp the cursor. In the
				 * end, we only care about the current mouse position and not bygone events. */
				while (SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_MOUSEMOTION, SDL_MOUSEMOTION)) {
					x = ev.motion.x;
					y = ev.motion.y;
				}
			}

			if (_cursor.UpdateCursorPosition(x, y)) {
				SDL_WarpMouseInWindow(this->sdl_window, _cursor.pos.x, _cursor.pos.y);
			}
			HandleMouseEvents();
			break;
		}

		case SDL_MOUSEWHEEL:
			if (ev.wheel.y > 0) {
				_cursor.wheel--;
			} else if (ev.wheel.y < 0) {
				_cursor.wheel++;
			}
			break;

		case SDL_MOUSEBUTTONDOWN:
			if (_rightclick_emulate && SDL_GetModState() & KMOD_CTRL) {
				ev.button.button = SDL_BUTTON_RIGHT;
			}

			switch (ev.button.button) {
				case SDL_BUTTON_LEFT:
					_left_button_down = true;
					break;

				case SDL_BUTTON_RIGHT:
					_right_button_down = true;
					_right_button_clicked = true;
					_right_button_down_pos.x = _cursor.pos.x;
					_right_button_down_pos.y = _cursor.pos.y;
					break;

				default: break;
			}
			HandleMouseEvents();
			break;

		case SDL_MOUSEBUTTONUP:
			if (_rightclick_emulate) {
				_right_button_down = false;
				_left_button_down = false;
				_left_button_clicked = false;
			} else if (ev.button.button == SDL_BUTTON_LEFT) {
				_left_button_down = false;
				_left_button_clicked = false;
			} else if (ev.button.button == SDL_BUTTON_RIGHT) {
				_right_button_down = false;
			}
			HandleMouseEvents();
#ifdef __EMSCRIPTEN__
			if (fullscreen_first_click) {
				fullscreen_first_click = false;
				EM_ASM(
					if (document.documentElement.requestFullscreen) {
						console.log("Attempting to set fullscreen mode");
						document.documentElement.requestFullscreen().then(() => {
							console.log("Seting fullscreen mode success");
						}).catch(err => {
							console.error("Error setting fullscreen mode: " + err);
						});
					}
				);
			}
#endif
			break;

		case SDL_FINGERDOWN:
			//Debug(misc, 0, "Finger {} down at {}:{}", ev.tfinger.fingerId, ev.tfinger.x, ev.tfinger.y);
			if (ev.tfinger.fingerId == 1) {
				_left_button_down = false;
				_left_button_clicked = false;
				HandleMouseEvents(); // Release left mouse button
				_multitouch_second_point.x = Clamp(ev.tfinger.x, 0.0f, 1.0f) * _screen.width;
				_multitouch_second_point.y = Clamp(ev.tfinger.y, 0.0f, 1.0f) * _screen.height;
				Point mouse;
				SDL_GetMouseState(&mouse.x, &mouse.y);
				_cursor.UpdateCursorPosition((mouse.x + _multitouch_second_point.x) / 2, (mouse.y + _multitouch_second_point.y) / 2, false);
				_right_button_down_pos.x = _cursor.pos.x;
				_right_button_down_pos.y = _cursor.pos.y;
				_multitouch_finger_distance = sqrtf(powf(mouse.x - _multitouch_second_point.x, 2) + powf(mouse.y - _multitouch_second_point.y, 2));
				HandleMouseEvents(); // Move mouse with no buttons pressed to the middle position between fingers
				_right_button_down = true;
				_right_button_clicked = true;
				HandleMouseEvents(); // Simulate right button click with two finger touch
			}
			break;

		case SDL_FINGERUP:
			//Debug(misc, 0, "Finger {} up at {}:{}", ev.tfinger.fingerId, ev.tfinger.x, ev.tfinger.y);
			if (ev.tfinger.fingerId == 1) {
				_right_button_down = false;
				_multitouch_second_point.x = -1;
				_multitouch_second_point.y = -1;
				HandleMouseEvents();
			}
			break;

		case SDL_FINGERMOTION:
			//Debug(misc, 0, "Finger {} move at {}:{}", ev.tfinger.fingerId, ev.tfinger.x, ev.tfinger.y);
			if (ev.tfinger.fingerId == 1) {
				_multitouch_second_point.x = Clamp(ev.tfinger.x, 0.0f, 1.0f) * _screen.width;
				_multitouch_second_point.y = Clamp(ev.tfinger.y, 0.0f, 1.0f) * _screen.height;
				Point mouse;
				SDL_GetMouseState(&mouse.x, &mouse.y);
				_cursor.UpdateCursorPosition((mouse.x + _multitouch_second_point.x) / 2, (mouse.y + _multitouch_second_point.y) / 2, false);
				int pinch_zoom_threshold = std::min(_screen.width, _screen.height) / PINCH_ZOOM_SENSITIVITY;
				int new_distance = sqrtf(powf(mouse.x - _multitouch_second_point.x, 2) + powf(mouse.y - _multitouch_second_point.y, 2));
				if (new_distance - _multitouch_finger_distance >= pinch_zoom_threshold) {
					_multitouch_finger_distance += pinch_zoom_threshold;
					_cursor.wheel--;
					_right_button_down = false;
				}
				if (_multitouch_finger_distance - new_distance >= pinch_zoom_threshold) {
					_multitouch_finger_distance -= pinch_zoom_threshold;
					_cursor.wheel++;
					_right_button_down = false;
				}
				HandleMouseEvents();
			}
			break;

		case SDL_QUIT:
			HandleExitGameRequest();
			break;

		case SDL_KEYDOWN: // Toggle full-screen on ALT + ENTER/F
			if ((ev.key.keysym.mod & (KMOD_ALT | KMOD_GUI)) &&
					(ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_f)) {
				if (ev.key.repeat == 0) ToggleFullScreen(!_fullscreen);
			} else {
				WChar character;

				uint keycode = ConvertSdlKeyIntoMy(&ev.key.keysym, &character);
				// Only handle non-text keys here. Text is handled in
				// SDL_TEXTINPUT below.
				if (!this->edit_box_focused ||
					keycode == WKC_DELETE ||
					keycode == WKC_NUM_ENTER ||
					keycode == WKC_LEFT ||
					keycode == WKC_RIGHT ||
					keycode == WKC_UP ||
					keycode == WKC_DOWN ||
					keycode == WKC_HOME ||
					keycode == WKC_END ||
					keycode & WKC_META ||
					keycode & WKC_CTRL ||
					keycode & WKC_ALT ||
					(keycode >= WKC_F1 && keycode <= WKC_F12) ||
					!IsValidChar(character, CS_ALPHANUMERAL)) {
					HandleKeypress(keycode, character);
				}
				if (ev.key.keysym.sym == SDLK_LCTRL || ev.key.keysym.sym == SDLK_RCTRL) {
					_ctrl_pressed = true;
				}
				if (ev.key.keysym.sym == SDLK_LSHIFT || ev.key.keysym.sym == SDLK_RSHIFT) {
					_shift_pressed = true;
				}
			}
			break;

		case SDL_KEYUP:
			if (ev.key.keysym.sym == SDLK_LCTRL || ev.key.keysym.sym == SDLK_RCTRL) {
				_ctrl_pressed = false;
			}
			if (ev.key.keysym.sym == SDLK_LSHIFT || ev.key.keysym.sym == SDLK_RSHIFT) {
				_shift_pressed = false;
			}
			break;

		case SDL_TEXTINPUT: {
			if (!this->edit_box_focused) break;
			SDL_Keycode kc = SDL_GetKeyFromName(ev.text.text);
			uint keycode = ConvertSdlKeycodeIntoMy(kc);

			if (keycode == WKC_BACKQUOTE && FocusedWindowIsConsole()) {
				WChar character;
				Utf8Decode(&character, ev.text.text);
				HandleKeypress(keycode, character);
			} else {
				HandleTextInput(ev.text.text);
			}
			break;
		}
		case SDL_WINDOWEVENT: {
			if (ev.window.event == SDL_WINDOWEVENT_EXPOSED) {
				// Force a redraw of the entire screen.
				this->MakeDirty(0, 0, _screen.width, _screen.height);
			} else if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
#ifdef __EMSCRIPTEN__ // Just re-create the video surface
				CreateMainSurface(_cur_resolution.width, _cur_resolution.height, false);
#else
				int w = std::max(ev.window.data1, 64);
				int h = std::max(ev.window.data2, 64);
				CreateMainSurface(w, h, w != ev.window.data1 || h != ev.window.data2);
#endif
			} else if (ev.window.event == SDL_WINDOWEVENT_ENTER) {
				// mouse entered the window, enable cursor
				_cursor.in_window = true;
				/* Ensure pointer lock will not occur. */
				SDL_SetRelativeMouseMode(SDL_FALSE);
			} else if (ev.window.event == SDL_WINDOWEVENT_LEAVE) {
				// mouse left the window, undraw cursor
				UndrawMouseCursor();
				_cursor.in_window = false;
			}
			break;
		}
	}

	return true;
}

static const char *InitializeSDL()
{
	/* Explicitly disable hardware acceleration. Enabling this causes
	 * UpdateWindowSurface() to update the window's texture instead of
	 * its surface. */
	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "0");

	/* Check if the video-driver is already initialized. */
	if (SDL_WasInit(SDL_INIT_VIDEO) != 0) return nullptr;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) return SDL_GetError();
	return nullptr;
}

const char *VideoDriver_SDL_Base::Initialize()
{
	this->UpdateAutoResolution();

	const char *error = InitializeSDL();
	if (error != nullptr) return error;

	FindResolutions();
	Debug(driver, 2, "Resolution for display: {}x{}", _cur_resolution.width, _cur_resolution.height);

	return nullptr;
}

const char *VideoDriver_SDL_Base::Start(const StringList &param)
{
	if (BlitterFactory::GetCurrentBlitter()->GetScreenDepth() == 0) return "Only real blitters supported";

	const char *error = this->Initialize();
	if (error != nullptr) return error;

	this->startup_display = FindStartupDisplay(GetDriverParamInt(param, "display", -1));

	if (!CreateMainSurface(_cur_resolution.width, _cur_resolution.height, false)) {
		return SDL_GetError();
	}

	const char *dname = SDL_GetCurrentVideoDriver();
	Debug(driver, 1, "SDL2: using driver '{}'", dname);

	this->driver_info = this->GetName();
	this->driver_info += " (";
	this->driver_info += dname;
	this->driver_info += ")";

	MarkWholeScreenDirty();

	SDL_StopTextInput();
	this->edit_box_focused = false;

#ifdef __EMSCRIPTEN__
	this->is_game_threaded = false;
#else
	this->is_game_threaded = !GetDriverParamBool(param, "no_threads") && !GetDriverParamBool(param, "no_thread");
#endif

	return nullptr;
}

void VideoDriver_SDL_Base::Stop()
{
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	if (SDL_WasInit(SDL_INIT_EVERYTHING) == 0) {
		SDL_Quit(); // If there's nothing left, quit SDL
	}
}

void VideoDriver_SDL_Base::InputLoop()
{
	uint32 mod = SDL_GetModState();
	const Uint8 *keys = SDL_GetKeyboardState(nullptr);

#ifndef __EMSCRIPTEN__
#if defined(_DEBUG)
	this->fast_forward_key_pressed = _shift_pressed;
#else
	/* Speedup when pressing tab, except when using ALT+TAB
	 * to switch to another application. */
	this->fast_forward_key_pressed = keys[SDL_SCANCODE_TAB] && (mod & KMOD_ALT) == 0;
#endif /* defined(_DEBUG) */
#endif // __EMSCRIPTEN__

	/* Determine which directional keys are down. */
	_dirkeys =
		(keys[SDL_SCANCODE_LEFT]  ? 1 : 0) |
		(keys[SDL_SCANCODE_UP]    ? 2 : 0) |
		(keys[SDL_SCANCODE_RIGHT] ? 4 : 0) |
		(keys[SDL_SCANCODE_DOWN]  ? 8 : 0);

	if (old_ctrl_pressed != _ctrl_pressed) HandleCtrlChanged();
	old_ctrl_pressed = _ctrl_pressed;

	if (!this->set_clipboard_text.empty()) {
		SDL_SetClipboardText(this->set_clipboard_text.c_str());
		this->set_clipboard_text = "";
	}
}

void VideoDriver_SDL_Base::LoopOnce()
{
	if (_exit_game) {
#ifdef __EMSCRIPTEN__
		/* Emscripten is event-driven, and as such the main loop is inside
		 * the browser. So if _exit_game goes true, the main loop ends (the
		 * cancel call), but we still have to call the cleanup that is
		 * normally done at the end of the main loop for non-Emscripten.
		 * After that, Emscripten just halts, and the HTML shows a nice
		 * "bye, see you next time" message. */
		emscripten_cancel_main_loop();
		emscripten_exit_pointerlock();
		/* In effect, the game ends here. As emscripten_set_main_loop() caused
		 * the stack to be unwound, the code after MainLoop() in
		 * openttd_main() is never executed. */
		EM_ASM(if (window["openttd_syncfs"]) openttd_syncfs());
		EM_ASM(if (window["openttd_exit"]) openttd_exit());
#endif
		return;
	}

	this->Tick();

/* Emscripten is running an event-based mainloop; there is already some
 * downtime between each iteration, so no need to sleep. */
#ifndef __EMSCRIPTEN__
	this->SleepTillNextTick();
#endif
}

void VideoDriver_SDL_Base::MainLoop()
{
#ifdef __EMSCRIPTEN__
	/* Run the main loop event-driven, based on RequestAnimationFrame. */
	emscripten_set_main_loop_arg(&this->EmscriptenLoop, this, 0, 1);
#else
	this->StartGameThread();

	while (!_exit_game) {
		LoopOnce();
	}

	this->StopGameThread();
#endif
}

bool VideoDriver_SDL_Base::ChangeResolution(int w, int h)
{
	return CreateMainSurface(w, h, true);
}

bool VideoDriver_SDL_Base::ToggleFullscreen(bool fullscreen)
{
	/* Remember current window size */
	int w, h;
	SDL_GetWindowSize(this->sdl_window, &w, &h);

	if (fullscreen) {
#ifdef __EMSCRIPTEN__
		EM_ASM( if (document.documentElement.requestFullscreen) { document.documentElement.requestFullscreen().then(() => {}).catch(err => {}); } );
#endif

		/* Find fullscreen window size */
		SDL_DisplayMode dm;
		if (SDL_GetCurrentDisplayMode(0, &dm) < 0) {
			Debug(driver, 0, "SDL_GetCurrentDisplayMode() failed: {}", SDL_GetError());
		} else {
			SDL_SetWindowSize(this->sdl_window, dm.w, dm.h);
		}
	} else {
#ifdef __EMSCRIPTEN__
		EM_ASM( if (document.exitFullscreen) { document.exitFullscreen().then(() => {}).catch(err => {}); } );
#endif
	}

	Debug(driver, 1, "SDL2: Setting {}", fullscreen ? "fullscreen" : "windowed");
	int ret = SDL_SetWindowFullscreen(this->sdl_window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
	if (ret == 0) {
		/* Switching resolution succeeded, set fullscreen value of window. */
		_fullscreen = fullscreen;
		if (!fullscreen) SDL_SetWindowSize(this->sdl_window, w, h);
	} else {
		Debug(driver, 0, "SDL_SetWindowFullscreen() failed: {}", SDL_GetError());
	}

	InvalidateWindowClassesData(WC_GAME_OPTIONS, 3);
	return ret == 0;
}

bool VideoDriver_SDL_Base::AfterBlitterChange()
{
	assert(BlitterFactory::GetCurrentBlitter()->GetScreenDepth() != 0);
	int w, h;
	SDL_GetWindowSize(this->sdl_window, &w, &h);
	return CreateMainSurface(w, h, false);
}

Dimension VideoDriver_SDL_Base::GetScreenSize() const
{
#ifdef __EMSCRIPTEN__
	return VideoDriver::GetScreenSize();
#endif
	SDL_DisplayMode mode;
	if (SDL_GetCurrentDisplayMode(this->startup_display, &mode) != 0) return VideoDriver::GetScreenSize();

	return { static_cast<uint>(mode.w), static_cast<uint>(mode.h) };
}

bool VideoDriver_SDL_Base::LockVideoBuffer()
{
	if (this->buffer_locked) return false;
	this->buffer_locked = true;

	_screen.dst_ptr = this->GetVideoPointer();
	assert(_screen.dst_ptr != nullptr);

	return true;
}

void VideoDriver_SDL_Base::UnlockVideoBuffer()
{
	if (_screen.dst_ptr != nullptr) {
		/* Hand video buffer back to the drawing backend. */
		this->ReleaseVideoPointer();
		_screen.dst_ptr = nullptr;
	}

	this->buffer_locked = false;
}
