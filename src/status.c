/*************************************************************************
 * Functions for displaying window move/resize status.
 * Copyright (C) 2004 Joe Wingbermuehle
 *************************************************************************/

#include "jwm.h"
#include "status.h"
#include "font.h"
#include "screen.h"
#include "color.h"
#include "main.h"
#include "client.h"
#include "error.h"

typedef enum {
	SW_INVALID,
	SW_OFF,
	SW_SCREEN,
	SW_WINDOW,
	SW_CORNER
} StatusWindowType;

static Window statusWindow;
static GC statusGC;
static unsigned int statusWindowHeight;
static unsigned int statusWindowWidth;
static int statusWindowX, statusWindowY;
static StatusWindowType moveStatusType;
static StatusWindowType resizeStatusType;

static void CreateMoveResizeWindow(const ClientNode *np,
	StatusWindowType type);
static void DrawMoveResizeWindow(const ClientNode *np, StatusWindowType type);
static void DestroyMoveResizeWindow();
static void GetMoveResizeCoordinates(const ClientNode *np,
	StatusWindowType type, int *x, int *y);
static StatusWindowType ParseType(const char *str);

/*************************************************************************
 *************************************************************************/
void GetMoveResizeCoordinates(const ClientNode *np, StatusWindowType type,
	int *x, int *y) {

	int screen;
	int screenx, screeny;
	int screenWidth, screenHeight;

	if(type == SW_WINDOW) {
		*x = np->x + np->width / 2 - statusWindowWidth / 2;
		*y = np->y + np->height / 2 - statusWindowHeight / 2;
		return;
	}

	screen = GetCurrentScreen(np->x, np->y);
	screenx = GetScreenX(screen);
	screeny = GetScreenY(screen);

	if(type == SW_CORNER) {
		*x = screenx;
		*y = screeny;
		return;
	}

	/* SW_SCREEN */

	screenWidth = GetScreenWidth(screen);
	screenHeight = GetScreenHeight(screen);

	*x = screenx + screenWidth / 2 - statusWindowWidth / 2;
	*y = screeny + screenHeight / 2 - statusWindowHeight / 2;

}

/*************************************************************************
 *************************************************************************/
void CreateMoveResizeWindow(const ClientNode *np, StatusWindowType type) {

	XSetWindowAttributes attrs;

	if(type == SW_OFF) {
		return;
	}

	statusWindowHeight = GetStringHeight(FONT_MENU) + 8;
	statusWindowWidth = GetStringWidth(FONT_MENU, " 00000 x 00000 ");

	GetMoveResizeCoordinates(np, type, &statusWindowX, &statusWindowY);

	attrs.background_pixel = colors[COLOR_MENU_BG];
	attrs.save_under = True;
	attrs.override_redirect = True;

	statusWindow = JXCreateWindow(display, rootWindow,
		statusWindowX, statusWindowY,
		statusWindowWidth, statusWindowHeight, 0,
		CopyFromParent, InputOutput, CopyFromParent,
		CWBackPixel | CWOverrideRedirect | CWSaveUnder,
		&attrs);

	JXMapRaised(display, statusWindow);
	statusGC = JXCreateGC(display, statusWindow, 0, NULL);
	JXSetBackground(display, statusGC, colors[COLOR_MENU_BG]);

}

/*************************************************************************
 *************************************************************************/
void DrawMoveResizeWindow(const ClientNode *np, StatusWindowType type) {

	int x, y;

	GetMoveResizeCoordinates(np, type, &x, &y);
	if(x != statusWindowX || y != statusWindowX) {
		statusWindowX = x;
		statusWindowY = y;
		JXMoveResizeWindow(display, statusWindow, x, y,
			statusWindowWidth, statusWindowHeight);
	}

	JXSetForeground(display, statusGC, colors[COLOR_MENU_BG]);
	JXFillRectangle(display, statusWindow, statusGC, 2, 2,
		statusWindowWidth - 3, statusWindowHeight - 3);

	JXSetForeground(display, statusGC, colors[COLOR_MENU_UP]);
	JXDrawLine(display, statusWindow, statusGC,
		0, 0, statusWindowWidth - 1, 0);
	JXDrawLine(display, statusWindow, statusGC,
		0, 1, statusWindowWidth - 2, 1);
	JXDrawLine(display, statusWindow, statusGC,
		0, 2, 0, statusWindowHeight - 1);
	JXDrawLine(display, statusWindow, statusGC,
		1, 2, 1, statusWindowHeight - 2);

	JXSetForeground(display, statusGC, colors[COLOR_MENU_DOWN]);
	JXDrawLine(display, statusWindow, statusGC,
		1, statusWindowHeight - 1, statusWindowWidth - 1,
		statusWindowHeight - 1);
	JXDrawLine(display, statusWindow, statusGC,
		2, statusWindowHeight - 2, statusWindowWidth - 1,
		statusWindowHeight - 2);
	JXDrawLine(display, statusWindow, statusGC,
		statusWindowWidth - 1, 1, statusWindowWidth - 1,
		statusWindowHeight - 3);
	JXDrawLine(display, statusWindow, statusGC,
		statusWindowWidth - 2, 2, statusWindowWidth - 2,
		statusWindowHeight - 3);

}

/*************************************************************************
 *************************************************************************/
void DestroyMoveResizeWindow() {

	if(statusWindow != None) {
		JXFreeGC(display, statusGC);
		JXDestroyWindow(display, statusWindow);
		statusWindow = None;
	}

}

/*************************************************************************
 *************************************************************************/
void CreateMoveWindow(ClientNode *np) {

	CreateMoveResizeWindow(np, moveStatusType);

}

/*************************************************************************
 *************************************************************************/
void UpdateMoveWindow(ClientNode *np) {

	char str[80];
	unsigned int width;

	if(moveStatusType == SW_OFF) {
		return;
	}

	DrawMoveResizeWindow(np, moveStatusType);

	snprintf(str, sizeof(str), "(%d, %d)", np->x, np->y);
	width = GetStringWidth(FONT_MENU, str);
	RenderString(statusWindow, statusGC, FONT_MENU, COLOR_MENU_FG,
		statusWindowWidth / 2 - width / 2, 4, rootWidth, str);

}

/*************************************************************************
 *************************************************************************/
void DestroyMoveWindow() {

	DestroyMoveResizeWindow();

}

/*************************************************************************
 *************************************************************************/
void CreateResizeWindow(ClientNode *np) {

	CreateMoveResizeWindow(np, resizeStatusType);

}

/*************************************************************************
 *************************************************************************/
void UpdateResizeWindow(ClientNode *np, int gwidth, int gheight) {

	char str[80];
	unsigned int fontWidth;

	if(resizeStatusType == SW_OFF) {
		return;
	}

	DrawMoveResizeWindow(np, resizeStatusType);

	snprintf(str, sizeof(str), "%d x %d", gwidth, gheight);
	fontWidth = GetStringWidth(FONT_MENU, str);
	RenderString(statusWindow, statusGC, FONT_MENU, COLOR_MENU_FG,
		statusWindowWidth / 2 - fontWidth / 2, 4, rootWidth, str);

}

/*************************************************************************
 *************************************************************************/
void DestroyResizeWindow() {

	DestroyMoveResizeWindow();

}

/*************************************************************************
 *************************************************************************/
StatusWindowType ParseType(const char *str) {

	if(!str) {
		return SW_SCREEN;
	} else if(!strcmp(str, "off")) {
		return SW_OFF;
	} else if(!strcmp(str, "screen")) {
		return SW_SCREEN;
	} else if(!strcmp(str, "window")) {
		return SW_WINDOW;
	} else if(!strcmp(str, "corner")) {
		return SW_CORNER;
	} else {
		return SW_INVALID;
	}

}

/*************************************************************************
 *************************************************************************/
void SetMoveStatusType(const char *str) {

	StatusWindowType type;

	type = ParseType(str);
	if(type == SW_INVALID) {
		moveStatusType = SW_SCREEN;
		Warning("invalid MoveMode coordinates: \"%s\"", str);
	} else {
		moveStatusType = type;
	}

}

/*************************************************************************
 *************************************************************************/
void SetResizeStatusType(const char *str) {

	StatusWindowType type;

	type = ParseType(str);
	if(type == SW_INVALID) {
		resizeStatusType = SW_SCREEN;
		Warning("invalid ResizeMode coordinates: \"%s\"", str);
	} else {
		resizeStatusType = type;
	}

}

