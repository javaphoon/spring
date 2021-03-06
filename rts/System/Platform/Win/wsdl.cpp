/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * emulate SDL on Windows.
 */

#include "wsdl.h"

#include <stdio.h>
#include <math.h>
#include <queue>
#include <algorithm>


namespace wsdl
{

static HWND g_hWnd = (HWND)INVALID_HANDLE_VALUE;

void SetHandle(HWND hWnd)
{
	g_hWnd = hWnd;
}

static void queue_event(SDL_Event& ev)
{
	::SDL_PushEvent(&ev);
}

//----------------------------------------------------------------------------
// app activation

enum SdlActivationType { LOSE = 0, GAIN = 1 };

static inline void queue_active_event(SdlActivationType type, size_t changed_app_state)
{
	// SDL says this event is not generated when the window is created,
	// but skipping the first event may confuse things.

	SDL_Event ev;
	ev.type = SDL_ACTIVEEVENT;
	ev.active.state = (uint8_t)changed_app_state;
	ev.active.gain  = (uint8_t)((type == GAIN)? 1 : 0);
	queue_event(ev);
}


// SDL_APP* bitflags indicating whether we are active.
// note: responsibility for yielding lies with SDL apps -
// they control the main loop.
static Uint8 app_state;

void active_change_state(SdlActivationType type, Uint8 changed_app_state)
{
	Uint8 old_app_state = app_state;

	if(type == GAIN)
		app_state = Uint8(app_state | changed_app_state);
	else
		app_state = Uint8(app_state & ~changed_app_state);

	// generate an event - but only if the given state flags actually changed.
	if((old_app_state & changed_app_state) != (app_state & changed_app_state))
		queue_active_event(type, changed_app_state);
}

LRESULT OnActivate(HWND hWnd, UINT state, HWND hWndActDeact, BOOL fMinimized)
{
	SdlActivationType type;
	Uint8 changed_app_state;

	// went active and not minimized
	if(state != WA_INACTIVE && !fMinimized)
	{
		type = GAIN;
		changed_app_state = SDL_APPINPUTFOCUS|SDL_APPACTIVE;

	}
	// deactivated (Alt+Tab out) or minimized
	else
	{
		type = LOSE;
		changed_app_state = SDL_APPINPUTFOCUS;
		if(fMinimized)
			changed_app_state |= SDL_APPACTIVE;
	}

	active_change_state(type, changed_app_state);
	return 0;
}


Uint8 SDL_GetAppState()
{
	return app_state;
}

static void queue_quit_event()
{
	SDL_Event ev;
	ev.type = SDL_QUIT;
	queue_event(ev);
}


//----------------------------------------------------------------------------
// mouse

// background: there are several types of coordinates.
// - screen coords are relative to the primary desktop and may therefore be
//   negative on multi-monitor systems (e.g. if secondary monitor is left of
//   primary). they are prefixed with screen_*.
// - "client" coords are simply relative to the parent window's origin and
//   can also be negative (e.g. in the window's NC area).
//   these are prefixed with client_*.
// - "idealized" coords are what the app sees. these range from 0 to
//   windowDimensions-1. they are returned by GetCoords and have no prefix.

void queue_mouse_event(int x, int y, int relx, int rely)
{
	SDL_Event ev;
	ev.type = SDL_MOUSEMOTION;
	ev.motion.x = (Uint16)x;
	ev.motion.y = (Uint16)y;
	ev.motion.xrel = (Sint16)relx;
	ev.motion.yrel = (Sint16)rely;
	queue_event(ev);
}

void queue_button_event(int button, int state, int x, int y)
{
	SDL_Event ev;
	ev.type = Uint8((state == SDL_PRESSED)? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP);
	ev.button.button = (uint8_t)button;
	ev.button.state  = (uint8_t)state;
	ev.button.x = (Uint16)x;
	ev.button.y = (Uint16)y;
	queue_event(ev);
}


static int mouse_x, mouse_y;

// generate a mouse move message and update our notion of the mouse position.
// x, y are client pixel coordinates.
// notes:
// - does not actually move the OS cursor;
// - called from mouse_update and SDL_WarpMouse.
static void mouse_moved(int x, int y)
{
	// nothing to do if it hasn't changed since last time
	if(mouse_x == x && mouse_y == y)
		return;

	queue_mouse_event(x, y, x-mouse_x, y-mouse_y);
	mouse_x = x;
	mouse_y = y;
}

static POINT ScreenFromClient(int client_x, int client_y)
{
	POINT screen_pt;
	screen_pt.x = (LONG)client_x;
	screen_pt.y = (LONG)client_y;
	ClientToScreen(g_hWnd, &screen_pt);
	return screen_pt;
}

// get idealized client coordinates or return false if outside our window.
static bool GetCoords(int screen_x, int screen_y, int& x, int& y)
{
	POINT screen_pt;
	screen_pt.x = (LONG)screen_x;
	screen_pt.y = (LONG)screen_y;

	POINT client_pt;
	{
		SetLastError(0);
		client_pt = screen_pt;	// translated below
		const int ret = MapWindowPoints(HWND_DESKTOP, g_hWnd, &client_pt, 1);
	}

	{
		RECT client_rect;
		GetClientRect(g_hWnd, &client_rect);
		if(!PtInRect(&client_rect, client_pt))
			return false;
	}

	if(WindowFromPoint(screen_pt) != g_hWnd)
		return false;

	x = client_pt.x;
	y = client_pt.y;
	return true;
}

void mouse_update()
{
	// window not created yet or already shut down. no sense reporting
	// mouse position, and bail now to avoid ScreenToClient failing.
	if (g_hWnd == 0 || g_hWnd == INVALID_HANDLE_VALUE)
		return;

	// don't use DirectInput, because we want to respect the user's mouse
	// sensitivity settings. Windows messages are laggy, so query current
	// position directly.
	// note: GetCursorPos fails if the desktop is switched (e.g. after
	// pressing Ctrl+Alt+Del), which can be ignored.
	POINT screen_pt;
	if(!GetCursorPos(&screen_pt))
		return;
	int x, y;
	if(GetCoords(screen_pt.x, screen_pt.y, x, y))
	{
		active_change_state(GAIN, SDL_APPMOUSEFOCUS);
		mouse_moved(x, y);
	}
	// moved outside of window
	else
		active_change_state(LOSE, SDL_APPMOUSEFOCUS);
}

static size_t mouse_buttons;

// (we define a new function signature since the windowsx.h message crackers
// don't provide for passing uMsg)
LRESULT OnMouseButton(HWND hWnd, UINT uMsg, int client_x, int client_y, UINT flags)
{
	int button;
	int state;
	switch(uMsg)
	{
	case WM_LBUTTONDOWN:
		button = SDL_BUTTON_LEFT;
		state = SDL_PRESSED;
		break;
	case WM_LBUTTONUP:
		button = SDL_BUTTON_LEFT;
		state = SDL_RELEASED;
		break;
	case WM_RBUTTONDOWN:
		button = SDL_BUTTON_RIGHT;
		state = SDL_PRESSED;
		break;
	case WM_RBUTTONUP:
		button = SDL_BUTTON_RIGHT;
		state = SDL_RELEASED;
		break;
	case WM_MBUTTONDOWN:
		button = SDL_BUTTON_MIDDLE;
		state = SDL_PRESSED;
		break;
	case WM_MBUTTONUP:
		button = SDL_BUTTON_MIDDLE;
		state = SDL_RELEASED;
		break;
	}

	const POINT screen_pt = ScreenFromClient(client_x, client_y);

	// mouse capture
	static int outstanding_press_events = 0;
	static SDL_GrabMode oldMode;
	static bool saveMode = false;
	if(state == SDL_PRESSED)
	{
		// grab mouse to ensure we get up events
		if(++outstanding_press_events > 0)
		{
			if (!saveMode)
			{
				oldMode = SDL_WM_GrabInput(SDL_GRAB_QUERY);
				saveMode = true;
			}
#ifndef DEBUG
			SDL_WM_GrabInput(SDL_GRAB_ON);
#endif
			SetCursorPos(screen_pt.x, screen_pt.y);
		}
	}
	else
	{
		// release after all up events received
		if(--outstanding_press_events <= 0)
		{
			if (saveMode)
			{
				SDL_WM_GrabInput(oldMode);
				SetCursorPos(screen_pt.x, screen_pt.y);
				saveMode = false;
			}
			outstanding_press_events = 0;
		}
	}

	// update button bitfield
	if(state == SDL_PRESSED)
		mouse_buttons |= SDL_BUTTON(button);
	else
		mouse_buttons &= ~SDL_BUTTON(button);

	int x, y;
	if(GetCoords(screen_pt.x, screen_pt.y, x, y))
		queue_button_event(button, state, x, y);
	return 0;
}

// (note: this message is sent even if the cursor is outside our window)
LRESULT OnMouseWheel(HWND hWnd, int screen_x, int screen_y, int zDelta, UINT fwKeys)
{
	int x, y;
	if(GetCoords(screen_x, screen_y, x, y))
	{
		int button = (zDelta < 0)? SDL_BUTTON_WHEELDOWN : SDL_BUTTON_WHEELUP;
		// SDL says this sends a down message followed by up.
		queue_button_event(button, SDL_PRESSED,  x, y);
		queue_button_event(button, SDL_RELEASED, x, y);
	}

	return 0;	// handled
}

void SDL_WarpMouse(int x, int y)
{
	//mouse_update();
	// SDL interface provides for int, but the values should be
	// idealized client coords (>= 0)
	//mouse_moved(x, y);

	const int client_x = x, client_y = y;
	const POINT screen_pt = ScreenFromClient(client_x, client_y);
	SetCursorPos(screen_pt.x, screen_pt.y);
	mouse_x = screen_pt.x;
	mouse_y = screen_pt.y;
}
}
