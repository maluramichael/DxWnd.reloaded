#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include "dxwnd.h"
#include "dxwcore.hpp"
#include "syslibs.h"
#include "dxhelper.h"

/****************************************************************************
 *	Function Name	: gShowHideTaskBar()									*
 *	Parameters		: BOOL bHide (flag to toggle Show/Hide of Taskbar)		*
 *	Return type		: void													*
 *	Purpose			: Function is used to Show/Hide the TaskBar				*
 *	Author			: Ashutosh R. Bhatikar (ARB)							*
 *	Date written	: 20th December 2000									*
 *	Modification History :													*
 *	Date of modification					Reason							*
 *  25th December 2000				Added methods to Show/Hide menu			*
 ****************************************************************************/

void gShowHideTaskBar(BOOL bHide /*=FALSE*/)
{
	RECT rectWorkArea;
	RECT rectTaskBar;
	static HWND pWnd = NULL;
	static HWND pStart = NULL;

	if(!pWnd) {
		pWnd = FindWindow("Shell_TrayWnd", "");
		// WinXP find
		pStart = FindWindowEx(pWnd, NULL, "Button", NULL);
		// if unsuccessful, do a Win7/8 find
		if(!pStart) pStart = FindWindowEx((*pGetDesktopWindow)(), NULL, "Button", "Start");
	}

	(*pSystemParametersInfoA)(SPI_GETWORKAREA,0, (LPVOID)&rectWorkArea, 0);
	(*pGetWindowRect)(pWnd, &rectTaskBar);
	if( bHide ){
		// Code to Hide the System Task Bar
		rectWorkArea.bottom += (rectTaskBar.bottom - rectTaskBar.top);
		(*pSystemParametersInfoA)(SPI_SETWORKAREA, 0, (LPVOID)&rectWorkArea, 0);
		ShowWindow(pWnd, SW_HIDE);
		ShowWindow(pStart, SW_HIDE);
	}
	else{
		// Code to Show the System Task Bar
		rectWorkArea.bottom -= (rectTaskBar.bottom - rectTaskBar.top);
		(*pSystemParametersInfoA)(SPI_SETWORKAREA, 0, (LPVOID)&rectWorkArea, 0);
		ShowWindow(pWnd, SW_SHOW);
		ShowWindow(pStart, SW_SHOW);
	}
}

static bool quit = false;

static LRESULT CALLBACK Hider_Message_Handler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch(umsg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
        break;
        case WM_CLOSE:
            quit = true;
        break;
    }

    return DefWindowProc(hwnd, umsg, wparam, lparam);
}

void dxwCore::HideDesktop(HWND hwnd)
{
	static BOOL DoOnce=TRUE;
	static ATOM aClass;
	static HWND wleft=0, wright=0, wtop=0, wbottom=0;
	RECT wRect, wDesktop;
	static HINSTANCE hinst=NULL;
	if(DoOnce){
		WNDCLASSEX WndClsEx;
		DoOnce=FALSE;

#ifdef DISABLEAERODESKTOP
		typedef HRESULT (WINAPI *DwmEnableComposition_Type)(UINT);
		DwmEnableComposition_Type pDwmEnableComposition;
		HMODULE hlib;

		// try to disable AERO desktop interface, if possible ...
		pDwmEnableComposition = NULL;
		hlib=(*pLoadLibraryA)("dwmapi.dll");
		if(hlib){
			pDwmEnableComposition = (DwmEnableComposition_Type)(*pGetProcAddress)(hlib, "DwmEnableComposition");
		}
		if(pDwmEnableComposition) {
			(*pDwmEnableComposition)(FALSE);
		}
#endif

		hinst=GetModuleHandle(NULL);
		if(!hinst)
			OutTrace("GetModuleHandle ERROR err=%d\n", GetLastError());
		else
			OutTrace("GetModuleHandle hinst=%x\n", hinst);

		WndClsEx.cbSize        = sizeof(WNDCLASSEX);
		WndClsEx.style         = 0;
		WndClsEx.lpfnWndProc   = Hider_Message_Handler; //DefWindowProc;
		WndClsEx.cbClsExtra    = 0;
		WndClsEx.cbWndExtra    = 0;
		WndClsEx.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
		//WndClsEx.hCursor       = LoadCursor(NULL, IDC_CROSS);
		WndClsEx.hCursor       = NULL;
		WndClsEx.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		//WndClsEx.hbrBackground = CreateSolidBrush(RGB(200,0,0));
		WndClsEx.lpszMenuName  = NULL;
		WndClsEx.lpszClassName = "dxwnd:hider";
		WndClsEx.hInstance     = hinst;
		WndClsEx.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

		// Register the application
		aClass=RegisterClassEx(&WndClsEx);
		if(!aClass) OutTraceE("RegisterClassEx: ERROR err=%d\n", GetLastError());
	}

	if(!(*pGetWindowRect)(hwnd, &wRect)) {
		OutTrace("GetWindowRect ERROR hwnd=%x err=%d\n", hWnd, GetLastError());
		return;
	}
	if(!(*pGetWindowRect)((*pGetDesktopWindow)(), &wDesktop)) {
		OutTrace("GetWindowRect ERROR hwnd=%x err=%d\n", NULL, GetLastError());
		return;
	}

	if(dxw.dwFlags6 & HIDETASKBAR){
		wDesktop.left = 0;
		wDesktop.top = 0;
		wDesktop.right = (*pGetSystemMetrics)(SM_CXSCREEN);
		wDesktop.bottom = (*pGetSystemMetrics)(SM_CYSCREEN);
		gShowHideTaskBar(TRUE);
	}

	// this is tricky: if you create a window with zero style, the manager seems to apply a default
	// overlapped window style. The only way I got to be sure that the style is exactly nothing
	// at all is to repeat the command by a SetWindowLong(hwnd, GWL_STYLE, 0) that sets again zero
	// as the correct window style.

	HWND hParent = (*pGetDesktopWindow)();
	if(!wleft) {
		wleft=(*pCreateWindowExA)(0, "dxwnd:hider", "hider", 0, 0, 0, 0, 0, hParent, NULL, hinst, NULL);
		(*pSetWindowLongA)(wleft, GWL_STYLE, 0);
	}
	if(!wright) {
		wright=(*pCreateWindowExA)(0, "dxwnd:hider", "hider", 0, 0, 0, 0, 0, hParent, NULL, hinst, NULL);
		(*pSetWindowLongA)(wright, GWL_STYLE, 0);
	}
	if(!wtop) {
		wtop=(*pCreateWindowExA)(0, "dxwnd:hider", "hider", 0, 0, 0, 0, 0, hParent, NULL, hinst, NULL);
		(*pSetWindowLongA)(wtop, GWL_STYLE, 0);
	}
	if(!wbottom) {
		wbottom=(*pCreateWindowExA)(0, "dxwnd:hider", "hider", 0, 0, 0, 0, 0, hParent, NULL, hinst, NULL);
		(*pSetWindowLongA)(wbottom, GWL_STYLE, 0);
	}

	(*pMoveWindow)(wleft, wDesktop.left, wDesktop.top, wRect.left, wDesktop.bottom, TRUE);
	(*pMoveWindow)(wright, wRect.right, wDesktop.top, wDesktop.right-wRect.right, wDesktop.bottom, TRUE);
	(*pMoveWindow)(wtop, wDesktop.left, wDesktop.top, wDesktop.right, wRect.top-wDesktop.top, TRUE);
	(*pMoveWindow)(wbottom, wDesktop.left, wRect.bottom, wDesktop.right, wDesktop.bottom-wRect.bottom, TRUE);

	(*pSetWindowLongA)(wleft, GWL_EXSTYLE, WS_EX_TOPMOST);
	(*pSetWindowLongA)(wright, GWL_EXSTYLE, WS_EX_TOPMOST);
	(*pSetWindowLongA)(wtop, GWL_EXSTYLE, WS_EX_TOPMOST);
	(*pSetWindowLongA)(wbottom, GWL_EXSTYLE, WS_EX_TOPMOST);

	(*pShowWindow)(wleft, SW_SHOW);
	(*pShowWindow)(wright, SW_SHOW);
	(*pShowWindow)(wtop, SW_SHOW);
	(*pShowWindow)(wbottom, SW_SHOW);
}
