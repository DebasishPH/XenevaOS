/**
* BSD 2-Clause License
*
* Copyright (c) 2022-2023, Manas Kamal Choudhury
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
**/

#ifndef __POPUP_H__
#define __POPUP_H__

#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#define POPUP_TYPE_MENU (1<<0)
#define POPUP_TYPE_TOOLTIP (1<<1)
#define POPUP_TYPE_NOTIFICATION (1<<2)

#pragma pack(push,1)
typedef struct _popup_sh_win_ {
	int x;
	int y;
	int w;
	int h;
	bool dirty;
	bool close;
	bool hide;
	bool popuped;
	bool alpha;
}PopupSharedWin;
#pragma pack(pop)

typedef struct _PopupWin_ {
	PopupSharedWin* shwin;
	uint32_t* buffer;
	uint8_t owner_id;
	uint16_t shwinKey;
	uint16_t buffWinKey;
	uint32_t* shadowBuffers;
	bool shadowUpdate;
}PopupWindow;

/*
* CreatePopupWindow -- create a new popup window
* @param x -- X location
* @param y -- Y location
* @param w -- Width of the window
* @param h -- Height of the window
* @param owner_id -- Owner of the window
*/
extern PopupWindow* CreatePopupWindow(int x, int y, int w, int h, uint16_t owner_id);

/*
* PopupWindowDestroy -- destroy popup window
* @param win -- Pointer to popup window
*/
extern void PopupWindowDestroy(PopupWindow* win);
#endif