/*
	ShapeSort
	© Alex Waugh 1998

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "Desk/Window.h"
#include "Desk/Error.h"
#include "Desk/Event.h"
#include "Desk/EventMsg.h"
#include "Desk/Msgs.h"
#include "Desk/Handler.h"
#include "Desk/Icon.h"
#include "Desk/Menu.h"
#include "Desk/DeskMem.h"
#include "Desk/Menu2.h"
#include "Desk/Resource.h"
#include "Desk/Screen.h"
#include "Desk/Template.h"
#include "Desk/File.h"
#include "Desk/Filing.h"
#include "Desk/Sprite.h"
#include "Desk/GFX.h"
#include "Desk/Str.h"
#include "Desk/Sound.h"
#include "Desk/KeyCodes.h"
#include "Desk/Pointer.h"

#include "AJWLib/Window.h"
#include "AJWLib/Error2.h"
#include "AJWLib/Menu.h"
#include "AJWLib/Msgs.h"
#include "AJWLib/DrawFile.h"

#include "Questions.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include "kernel.h"


#ifdef BUILD2D
 #ifdef BUILD3D
  #error You cannot specify two build versions at once
 #else
  #define TYPE "2D"
  #define REPORT 0
 #endif
#else
 #ifdef BUILD3D
  #define TYPE "3D"
  #define REPORT 1
 #else
  #error You must specify the build version
 #endif
#endif


#define VERSION "1.02 (7-Dec-02)"
#define RESOURCEDIR "Shape"TYPE
#define PICTURESDIR "<Shape"TYPE"$Dir>.Pictures" /*Dont change without changing LENGTHOFPICTURESDIR*/
#define LENGTHOFPICTURESDIR 22
#define TREEDESCRIPTIONFILE "TreeData"
#define MAX_DRAWFILES 15
#define PICTURESIZE (400<<8)
#define PICTUREOFFSET_X (564<<8)
#define PICTUREOFFSET_Y (-428<<8)

#define icon_PICTURE 0
#define icon_QUESTION 1
#define icon_YES 2
#define icon_NO 3
#define icon_BACK 4
#define icon_REPORT_TEXT 1
#define icon_REPORT_OK 0

#define menuitem_INFO 0
#define menuitem_QUIT 1

void Report(char *text);

typedef struct {
	int xscale;
	int zero1;
	int zero2;
	int yscale;
	int xoffset;
	int yoffset;
} transformation_matrix;

typedef struct {
	char name[12];
	drawfile_diagram *drawfile;
	int size;
	int xoffset;
	int yoffset;
} dfile;

typedef struct {
	int x;
	int y;
	int icon;
} origin;


Desk_window_handle info,mainwin,choosewin,report;
Desk_menu_ptr iconbarmenu;

Desk_bool mainwinopen=Desk_FALSE;
Desk_bool restart=Desk_FALSE;
char goback[256];
char goagain[256];

dfile drawfiles[MAX_DRAWFILES];
int numberofdrawfiles=0;
int currentdrawfile=0;
transformation_matrix matrix;
origin origins[MAX_DRAWFILES];

void LoadDrawFile(char *filename)
{
	int height,width;
	int size;
	size=Desk_File_GetLength(filename);
	drawfiles[numberofdrawfiles].drawfile=Desk_DeskMem_Malloc(size);
	Desk_File_LoadTo(filename,drawfiles[numberofdrawfiles].drawfile,NULL);
	drawfiles[numberofdrawfiles].size=size;
	width=drawfiles[numberofdrawfiles].drawfile->bbox.max.x-drawfiles[numberofdrawfiles].drawfile->bbox.min.x;
	height=drawfiles[numberofdrawfiles].drawfile->bbox.max.y-drawfiles[numberofdrawfiles].drawfile->bbox.min.y;
    drawfiles[numberofdrawfiles].xoffset=(PICTURESIZE-width)/2-drawfiles[numberofdrawfiles].drawfile->bbox.min.x;
    drawfiles[numberofdrawfiles].yoffset=(PICTURESIZE-height)/2-drawfiles[numberofdrawfiles].drawfile->bbox.min.y;
	strcpy(drawfiles[numberofdrawfiles].name,Desk_Filing_FindLeafname(filename));
	numberofdrawfiles++;
}

Desk_bool RedrawMainWin(Desk_event_pollblock *block, void *r)
{
	Desk_bool more;
	int ox,oy;
	Desk_window_redrawblock *blk=(Desk_window_redrawblock*)block->data.bytes;
	Desk_Wimp_RedrawWindow(blk,&more);
	ox=blk->rect.min.x-blk->scroll.x;
	oy=blk->rect.max.y-blk->scroll.y;
	matrix.zero1=0;
	matrix.zero2=0;
	matrix.xscale=1<<16;
	matrix.yscale=1<<16;
	matrix.xoffset=(ox<<8)+PICTUREOFFSET_X+drawfiles[currentdrawfile].xoffset;
	matrix.yoffset=(oy<<8)+PICTUREOFFSET_Y+drawfiles[currentdrawfile].yoffset;
	while (more) {
		DrawFile_Render(0,drawfiles[currentdrawfile].drawfile,drawfiles[currentdrawfile].size,(os_trfm*)&matrix,&blk->cliprect,0);
		Desk_Wimp_GetRectangle(blk,&more);
	}
	return Desk_TRUE;
}

Desk_bool RedrawChooseWin(Desk_event_pollblock *block, void *r)
{
	Desk_bool more;
	int ox,oy;
	Desk_window_redrawblock *blk=(Desk_window_redrawblock*)block->data.bytes;
	Desk_Wimp_RedrawWindow(blk,&more);
	ox=blk->rect.min.x-blk->scroll.x;
	oy=blk->rect.max.y-blk->scroll.y;
	matrix.zero1=0;
	matrix.zero2=0;
	matrix.xscale=1<<15; /* 50% */
	matrix.yscale=1<<15; /*     */
	while (more) {
		int i;
		for (i=0;i<numberofdrawfiles;i++) {
			matrix.xoffset=(ox<<8)+origins[i].x+drawfiles[i].xoffset/2;
			matrix.yoffset=(oy<<8)+origins[i].y+drawfiles[i].yoffset/2;
			DrawFile_Render(0,drawfiles[i].drawfile,drawfiles[i].size,(os_trfm*)&matrix,&blk->cliprect,0);
		}
		Desk_Wimp_GetRectangle(blk,&more);
	}
	return Desk_TRUE;
}

void RedrawQuestion(void)
{
	Desk_Icon_SetText(mainwin,icon_QUESTION,GetQuestion());
	Desk_Icon_SetText(mainwin,icon_YES,GetLeftText());
	Desk_Icon_SetText(mainwin,icon_NO,GetRightText());
	if (RootNode()) Desk_Icon_Shade(mainwin,icon_BACK); else Desk_Icon_Unshade(mainwin,icon_BACK);
}

Desk_bool CloseDialog(Desk_event_pollblock *block,void *r)
{
	if (block->data.mouse.button.data.select) {
		Desk_Pointer_Unrestrict();
		if (mainwinopen) Desk_Icon_SetCaret(mainwin,-1); /*else Desk_Icon_SetCaret(choosewin,-1);*/
		Desk_Window_Hide(report);
		return Desk_TRUE;
	}
	return Desk_FALSE;
}

Desk_bool ChooseClick(Desk_event_pollblock *block,void *r)
{
	int i;
	if (block->data.mouse.button.data.menu==Desk_TRUE) return Desk_FALSE;
	for (i=0;i<numberofdrawfiles;i++) {
		if (origins[i].icon==block->data.mouse.icon) break;
	}
	if (i>=numberofdrawfiles) return Desk_FALSE;
	currentdrawfile=i;
	Desk_Window_Show(mainwin,Desk_open_CENTERED);
	mainwinopen=Desk_TRUE;
	Desk_Icon_SetText(mainwin,icon_BACK,goback);
	restart=Desk_FALSE;
	Desk_Window_Hide(choosewin);
	MoveToRoot();
	RedrawQuestion();
	if (LeafNode()) {
		Desk_Icon_SetShade(mainwin,icon_YES,Desk_TRUE);
		Desk_Icon_SetShade(mainwin,icon_NO,Desk_TRUE);
	} else {
		Desk_Icon_SetShade(mainwin,icon_YES,Desk_FALSE);
		Desk_Icon_SetShade(mainwin,icon_NO,Desk_FALSE);
	}
	Desk_Icon_SetCaret(mainwin,-1);
	return Desk_TRUE;
}

Desk_bool IconBarClick(Desk_event_pollblock *block, void *r)
{
	if (block->data.mouse.button.data.select==1) {
		if (mainwinopen) {
			Desk_Window_BringToFront(mainwin);
			Desk_Icon_SetCaret(mainwin,-1);
		} else {
			Desk_Window_Show(choosewin,Desk_open_WHEREVER); /*So the window is centered properly (as it might have just been resized)*/
			Desk_Window_Show(choosewin,Desk_open_CENTERED);
#if REPORT
			Report(AJWLib_Msgs_TempLookup("Report.Choose"));
			Desk_Pointer_RestrictToWindow(report);
#else
			Desk_Icon_SetCaret(choosewin,-1);
#endif
		}
		return Desk_TRUE;
 	}
 	return Desk_FALSE;
}

Desk_bool CloseMainWin(Desk_event_pollblock *block, void *r)
{
	Desk_Window_Hide(mainwin);
	mainwinopen=Desk_FALSE;
	return Desk_TRUE;
}

Desk_bool ReportKeyHandler(Desk_event_pollblock *block, void *r)
{
	if (block->data.key.code==Desk_keycode_RETURN) {
		Desk_Icon_Select(block->data.key.caret.window,icon_REPORT_OK);
		Desk_Window_Hide(report);
		Desk_Pointer_Unrestrict();
		if (mainwinopen) Desk_Icon_SetCaret(mainwin,-1); else Desk_Icon_SetCaret(choosewin,-1);
		Desk_Icon_Deselect(block->data.key.caret.window,icon_REPORT_OK);
		return Desk_TRUE;
	}
	return Desk_FALSE;
}

void Report(char *text)
{
	Desk_Icon_SetText(report,icon_REPORT_TEXT,text);
	AJWLib_Window_OpenTransient(report);
	Desk_Icon_SetCaret(report,-1);
	Desk_Sound_SysBeep();
}

void CheckLeaf(void)
{
	if (LeafNode()) {
		Desk_Icon_SetShade(mainwin,icon_YES,Desk_TRUE);
		Desk_Icon_SetShade(mainwin,icon_NO,Desk_TRUE);
		if (Desk_stricmp(GetFilename(),drawfiles[currentdrawfile].name)==0) {
			Report(GetQuestion());
			restart=Desk_TRUE;
			Desk_Icon_SetText(mainwin,icon_BACK,goagain);
		} else {
			char buffer[256];
			Desk_Msgs_Lookup("Wrong.Shape:You have made a mistake",buffer,256);
			Report(buffer);
			Desk_Icon_SetText(mainwin,icon_QUESTION,buffer);
		}
	} else {
		Desk_Icon_SetShade(mainwin,icon_YES,Desk_FALSE);
		Desk_Icon_SetShade(mainwin,icon_NO,Desk_FALSE);
	}
}

Desk_bool YesClick(Desk_event_pollblock *block,void *r)
{
	MoveLeft();
	RedrawQuestion();
	CheckLeaf();
	return Desk_TRUE;
}

Desk_bool NoClick(Desk_event_pollblock *block,void *r)
{
	MoveRight();
	RedrawQuestion();
	CheckLeaf();
	return Desk_TRUE;
}

Desk_bool BackClick(Desk_event_pollblock *block,void *r)
{
	if (restart) {
		CloseMainWin(NULL,NULL);
		Desk_Window_Show(choosewin,Desk_open_CENTERED);
#if REPORT
		Report(AJWLib_Msgs_TempLookup("Report.Choose"));
		Desk_Pointer_RestrictToWindow(report);
#else
		Desk_Icon_SetCaret(choosewin,-1);
#endif
	} else {
		MoveBack();
		RedrawQuestion();
		Desk_Icon_SetShade(mainwin,icon_YES,Desk_FALSE);
		Desk_Icon_SetShade(mainwin,icon_NO,Desk_FALSE);
	}
	return Desk_TRUE;
}

void IconBarMenuClick(int item, void *r)
{
	if (item==menuitem_QUIT) Desk_Event_CloseDown();
}

void LoadDrawFiles(void)
{
	int offset=0,number=1,x=1,y=1,i;
	int currentx,currenty;
	char buffer[256+LENGTHOFPICTURESDIR+1]=PICTURESDIR".";
	Desk_Filing_ReadDirNames(PICTURESDIR,buffer+LENGTHOFPICTURESDIR+1,&number,&offset,256,NULL);
	while (offset!=-1) {
		LoadDrawFile(buffer);
		number=1;
		Desk_Filing_ReadDirNames(PICTURESDIR,buffer+LENGTHOFPICTURESDIR+1,&number,&offset,256,NULL);
	}
	switch (numberofdrawfiles) {
		case 1:
			x=1;
			break;
		case 2:
		case 4:
			x=2;
			break;
		case 3:
		case 6:
		case 9:
			x=3;
			break;
		case 7:
		case 8:
		case 11:
		case 12:
			x=4;
			break;
		case 5:
		case 13:
		case 14:
		case 15:
			x=5;
			break;
		default:
			Desk_Msgs_ReportFatal(1,"Error.TooMany:Too many pictures",MAX_DRAWFILES);
	}
	switch (numberofdrawfiles) {
		case 1:
		case 2:
		case 3:
		case 5:
			y=1;
			break;
		case 4:
		case 6:
		case 8:
		case 10:
			y=2;
			break;
		case 7:
		case 9:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
			y=3;
			break;
		default:
			Desk_Msgs_ReportFatal(1,"Error.ToManyPics:Too many pictures",MAX_DRAWFILES);
	}
	Desk_Window_SetExtent(choosewin,0,-216*y-16,216*x+16,0);
	currentx=0;
	currenty=1;
	for (i=0;i<numberofdrawfiles;i++) {
		if (currentx>=x) {
			currentx=0;
			currenty++;
		}
		origins[i].x=(16+(currentx)*216)<<8;
		origins[i].y=(-(currenty)*216)<<8;
		origins[i].icon=(currentx++)+5*(currenty-1);
	}
}

int main(void)
{
	Desk_Error2_HandleAllSignals();
	Desk_Error2_SetHandler(AJWLib_Error2_ReportFatal);
	Desk_Resource_Initialise(RESOURCEDIR);
	Desk_Msgs_LoadFile("Messages");
	Desk_Event_Initialise(AJWLib_Msgs_TempLookup("Task.Name"));
	Desk_EventMsg_Initialise();
	Desk_Screen_CacheModeInfo();
	Desk_EventMsg_Claim(Desk_message_MODECHANGE,Desk_event_ANY,Desk_Handler_ModeChange,NULL);
	Desk_Event_Claim(Desk_event_CLOSE,Desk_event_ANY,Desk_event_ANY,Desk_Handler_CloseWindow,NULL);
	Desk_Event_Claim(Desk_event_OPEN,Desk_event_ANY,Desk_event_ANY,Desk_Handler_OpenWindow,NULL);
	Desk_Event_Claim(Desk_event_REDRAW,Desk_event_ANY,Desk_event_ANY,Desk_Handler_HatchRedraw,NULL);
	Desk_Event_Claim(Desk_event_KEY,Desk_event_ANY,Desk_event_ANY,Desk_Handler_Key,NULL);
	Desk_Icon_BarIcon(AJWLib_Msgs_TempLookup("Task.Icon:"),Desk_iconbar_RIGHT);
	Desk_Template_Initialise();
	Desk_Template_LoadFile("Templates");
	info=AJWLib_Window_CreateInfoWindowFromMsgs("Task.Name:","Task.Purpose:","© Alex Waugh 1998",VERSION);
	mainwin=Desk_Window_Create("Main",Desk_template_TITLEMIN);
	Desk_Msgs_Lookup("MainWin.Back:",goback,256);
	Desk_Msgs_Lookup("MainWin.GoAgain:",goagain,256);
	AJWLib_Msgs_SetTitle(mainwin,"MainWin.Title:");
	choosewin=Desk_Window_Create("Choose",Desk_template_TITLEMIN);
	AJWLib_Msgs_SetTitle(choosewin,"Choose.Title:?");
	report=Desk_Window_Create("Report",Desk_template_TITLEMIN);
	AJWLib_Msgs_SetText(report,icon_REPORT_OK,"Report.Ok:?");
	AJWLib_Msgs_SetTitle(report,"Report.Title:?");
	Desk_Event_Claim(Desk_event_CLICK,report,icon_REPORT_OK,CloseDialog,NULL);
	Desk_Event_Claim(Desk_event_KEY,report,Desk_event_ANY,ReportKeyHandler,NULL);
	iconbarmenu=AJWLib_Menu_CreateFromMsgs("Task.Name:","Menu.IconBar:",IconBarMenuClick,NULL);
	Desk_Menu_AddSubMenu(iconbarmenu,menuitem_INFO,(Desk_menu_ptr)info);
	AJWLib_Menu_Attach(Desk_window_ICONBAR,Desk_event_ANY,iconbarmenu,Desk_button_MENU);
	Desk_Event_Claim(Desk_event_CLICK,Desk_window_ICONBAR,Desk_event_ANY,IconBarClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,mainwin,icon_YES,YesClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,mainwin,icon_NO,NoClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,mainwin,icon_BACK,BackClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,choosewin,Desk_event_ANY,ChooseClick,NULL);
	Desk_Event_Claim(Desk_event_CLOSE,mainwin,Desk_event_ANY,CloseMainWin,NULL);
	Desk_Event_Claim(Desk_event_REDRAW,mainwin,Desk_event_ANY,RedrawMainWin,NULL);
	Desk_Event_Claim(Desk_event_REDRAW,choosewin,Desk_event_ANY,RedrawChooseWin,NULL);
	LoadDrawFiles();
	LoadFile(TREEDESCRIPTIONFILE);
#if REPORT
	Report(AJWLib_Msgs_TempLookup("Report.Init"));
	Desk_Pointer_RestrictToWindow(report);
#endif
	while (Desk_TRUE) Desk_Event_Poll();
	return 0;
}

