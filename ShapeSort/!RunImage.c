/*
	ShapeSort
	©Alex Waugh 1998
	See !Help file for conditions of use
*/

#include "DeskLib:Window.h"
#include "DeskLib:Error.h"
#include "DeskLib:Event.h"
#include "DeskLib:EventMsg.h"
#include "DeskLib:Msgs.h"
#include "DeskLib:Handler.h"
#include "DeskLib:Icon.h"
#include "DeskLib:Menu.h"
#include "DeskLib:Menu2.h"
#include "DeskLib:Resource.h"
#include "DeskLib:Screen.h"
#include "DeskLib:Template.h"
#include "DeskLib:File.h"
#include "DeskLib:Filing.h"
#include "DeskLib:Sprite.h"
#include "DeskLib:GFX.h"
#include "DeskLib:Str.h"
#include "DeskLib:KeyCodes.h"
#include "DeskLib:Dialog.h"
#include "DeskLib:Pointer.h"
#include "AJWLib:Window.h"
#include "AJWLib:Menu.h"
#include "AJWLib:Misc.h"
#include "AJWLib:Msgs.h"
#include "AJWLib:Handler.h"
#include "AJWLib:DrawFile.h"
#include "AJWLib:Flex.h"
#include "Questions.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include "kernel.h"

#define REPORT 0

#define VERSION "1.00 (9-Jun-98)"
#define RESOURCEDIR "Shape2D"
#define PICTURESDIR "<Shape2D$Dir>.Pictures" /*Dont change without changing LENGTHOFPICTURESDIR*/
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


window_handle info,mainwin,choosewin;
dialog report;
menu_ptr iconbarmenu;

BOOL mainwinopen=FALSE;
BOOL error=FALSE;
BOOL restart=FALSE;
char goback[256];
char goagain[256];

dfile drawfiles[MAX_DRAWFILES];
int numberofdrawfiles=0;
int currentdrawfile=0;
transformation_matrix matrix;
origin origins[MAX_DRAWFILES];

void SignalHandler(int sig)
{
	_kernel_oserror *err;
	if (error) abort();
	error=TRUE;
	err=_kernel_last_oserror();
	if (err==NULL)
		Msgs_Report(1,"Error.Fatal:","Unknown Error");
	else Msgs_Report(1,"Error.Fatal:",err->errmess);
	exit(EXIT_FAILURE);
}

void LoadDrawFile(char *filename)
{
	int height,width;
	int size;
	File_GetLength(filename,&size);
	flex_alloc((flex_ptr)&(drawfiles[numberofdrawfiles].drawfile),size);
	if (drawfiles[numberofdrawfiles].drawfile==NULL) Msgs_ReportFatal(1,"Error.NoMem:Not enough free memory");
	File_LoadTo(filename,drawfiles[numberofdrawfiles].drawfile,NULL);
	drawfiles[numberofdrawfiles].size=size;
	width=drawfiles[numberofdrawfiles].drawfile->bbox.max.x-drawfiles[numberofdrawfiles].drawfile->bbox.min.x;
	height=drawfiles[numberofdrawfiles].drawfile->bbox.max.y-drawfiles[numberofdrawfiles].drawfile->bbox.min.y;
    drawfiles[numberofdrawfiles].xoffset=(PICTURESIZE-width)/2-drawfiles[numberofdrawfiles].drawfile->bbox.min.x;
    drawfiles[numberofdrawfiles].yoffset=(PICTURESIZE-height)/2-drawfiles[numberofdrawfiles].drawfile->bbox.min.y;
	strcpy(drawfiles[numberofdrawfiles].name,Filing_FindLeafname(filename));
	numberofdrawfiles++;
}

BOOL RedrawMainWin(event_pollblock *block, void *r)
{
	BOOL more;
	int ox,oy;
	window_redrawblock *blk=(window_redrawblock*)block->data.bytes;
	Wimp_RedrawWindow(blk,&more);
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
		Wimp_GetRectangle(blk,&more);
	}
	return TRUE;
}

BOOL RedrawChooseWin(event_pollblock *block, void *r)
{
	BOOL more;
	int ox,oy;
	window_redrawblock *blk=(window_redrawblock*)block->data.bytes;
	Wimp_RedrawWindow(blk,&more);
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
		Wimp_GetRectangle(blk,&more);
	}
	return TRUE;
}

void RedrawQuestion(void)
{
	Icon_SetText(mainwin,icon_QUESTION,GetQuestion());
	Icon_SetText(mainwin,icon_YES,GetLeftText());
	Icon_SetText(mainwin,icon_NO,GetRightText());
	if (RootNode()) Icon_Shade(mainwin,icon_BACK); else Icon_Unshade(mainwin,icon_BACK);
}

BOOL CloseDialog(event_pollblock *block,void *r)
{
	if (block->data.mouse.button.data.select) {
		Dialog_Hide(report);
		Pointer_Unrestrict();
		if (mainwinopen) Icon_SetCaret(mainwin,-1); else Icon_SetCaret(choosewin,-1);
		return TRUE;
	}
	return FALSE;
}

BOOL ChooseClick(event_pollblock *block,void *r)
{
	int i;
	if (block->data.mouse.button.data.menu==TRUE) return FALSE;
	for (i=0;i<numberofdrawfiles;i++) {
		if (origins[i].icon==block->data.mouse.icon) break;
	}
	if (i>=numberofdrawfiles) return FALSE;
	currentdrawfile=i;
	Window_Show(mainwin,open_CENTERED);
	mainwinopen=TRUE;
	Icon_SetText(mainwin,icon_BACK,goback);
	restart=FALSE;
	Window_Hide(choosewin);
	MoveToRoot();
	RedrawQuestion();
	if (LeafNode()) {
		Icon_SetShade(mainwin,icon_YES,TRUE);
		Icon_SetShade(mainwin,icon_NO,TRUE);
	} else {
		Icon_SetShade(mainwin,icon_YES,FALSE);
		Icon_SetShade(mainwin,icon_NO,FALSE);
	}
	Icon_SetCaret(mainwin,-1);
	return TRUE;
}

BOOL IconBarClick(event_pollblock *block, void *r)
{
	if (block->data.mouse.button.data.select==1) {
		if (mainwinopen) {
			Window_BringToFront(mainwin);
			Icon_SetCaret(mainwin,-1);
		} else {
			Window_Show(choosewin,open_WHEREVER); /*So the window is centered properly (as it might have just been resized)*/
			Window_Show(choosewin,open_CENTERED);
#if REPORT
			Report(Msgs_TempLookup("Report.Choose"));
			Pointer_RestrictToWindow(Dialog_WindowHandle(report));
#else
			Icon_SetCaret(choosewin,-1);
#endif
		}
		return TRUE;
 	}
 	return FALSE;
}

BOOL CloseMainWin(event_pollblock *block, void *r)
{
	Window_Hide(mainwin);
	mainwinopen=FALSE;
	return TRUE;
}

BOOL ReportKeyHandler(event_pollblock *block, void *r)
{
	if (block->data.key.code==keycode_RETURN) {
		Icon_Select(block->data.key.caret.window,icon_REPORT_OK);
		Dialog_Hide(report);
		Pointer_Unrestrict();
		if (mainwinopen) Icon_SetCaret(mainwin,-1); else Icon_SetCaret(choosewin,-1);
		Icon_Deselect(block->data.key.caret.window,icon_REPORT_OK);
		return TRUE;
	}
	return FALSE;
}

void Report(char *text)
{
	Icon_SetText(Dialog_WindowHandle(report),icon_REPORT_TEXT,text);
	Dialog_Show(report);
	Icon_SetCaret(Dialog_WindowHandle(report),-1);
	Beep();
}

void CheckLeaf(void)
{
	if (LeafNode()) {
		Icon_SetShade(mainwin,icon_YES,TRUE);
		Icon_SetShade(mainwin,icon_NO,TRUE);
		if (stricmp(GetFilename(),drawfiles[currentdrawfile].name)==0) {
			Report(GetQuestion());
			restart=TRUE;
			Icon_SetText(mainwin,icon_BACK,goagain);
		} else {
			char buffer[256];
			Msgs_Lookup("Wrong.Shape:You have made a mistake",buffer,256);
			Report(buffer);
			Icon_SetText(mainwin,icon_QUESTION,buffer);
		}
	} else {
		Icon_SetShade(mainwin,icon_YES,FALSE);
		Icon_SetShade(mainwin,icon_NO,FALSE);
	}
}

BOOL YesClick(event_pollblock *block,void *r)
{
	MoveLeft();
	RedrawQuestion();
	CheckLeaf();
	return TRUE;
}

BOOL NoClick(event_pollblock *block,void *r)
{
	MoveRight();
	RedrawQuestion();
	CheckLeaf();
	return TRUE;
}

BOOL BackClick(event_pollblock *block,void *r)
{
	if (restart) {
		CloseMainWin(NULL,NULL);
		Window_Show(choosewin,open_CENTERED);
#if REPORT
		Report(Msgs_TempLookup("Report.Choose"));
		Pointer_RestrictToWindow(Dialog_WindowHandle(report));
#else
		Icon_SetCaret(choosewin,-1);
#endif
	} else {
		MoveBack();
		RedrawQuestion();
		Icon_SetShade(mainwin,icon_YES,FALSE);
		Icon_SetShade(mainwin,icon_NO,FALSE);
	}
	return TRUE;
}

void IconBarMenuClick(int item, void *r)
{
	if (item==menuitem_QUIT) Event_CloseDown();
}

void LoadDrawFiles(void)
{
	int offset=0,number=1,x,y,i;
	int currentx,currenty;
	char buffer[256+LENGTHOFPICTURESDIR+1]=PICTURESDIR".";
	Filing_ReadDirNames(PICTURESDIR,buffer+LENGTHOFPICTURESDIR+1,&number,&offset,256,NULL);
	while (offset!=-1) {
		LoadDrawFile(buffer);
		number=1;
		Filing_ReadDirNames(PICTURESDIR,buffer+LENGTHOFPICTURESDIR+1,&number,&offset,256,NULL);
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
			Msgs_ReportFatal(1,"Error.TooMany:Too many pictures",MAX_DRAWFILES);
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
			Msgs_ReportFatal(1,"Error.ToManyPics:Too many pictures",MAX_DRAWFILES);
	}
	Window_SetExtent(choosewin,0,-216*y-16,216*x+16,0);
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
	signal(SIGINT,SignalHandler);
	signal(SIGFPE,SignalHandler);
	signal(SIGILL,SignalHandler);
	signal(SIGSEGV,SignalHandler);
	signal(SIGSTAK,SignalHandler);
	signal(SIGOSERROR,SignalHandler);
	Resource_Initialise(RESOURCEDIR);
	Msgs_LoadFile("Messages");
	Event_Initialise(Msgs_TempLookup("Task.Name"));
	EventMsg_Initialise();
	Screen_CacheModeInfo();
	EventMsg_Claim(message_MODECHANGE,event_ANY,Handler_ModeChange,NULL);
	Event_Claim(event_CLOSE,event_ANY,event_ANY,Handler_CloseWindow,NULL);
	Event_Claim(event_OPEN,event_ANY,event_ANY,Handler_OpenWindow,NULL);
	Event_Claim(event_REDRAW,event_ANY,event_ANY,Handler_HatchRedraw,NULL);
	Event_Claim(event_KEY,event_ANY,event_ANY,Handler_KeyPress,NULL);
	Icon_BarIcon(Msgs_TempLookup("Task.Icon:"),iconbar_RIGHT);
	Template_Initialise();
	Template_LoadFile("Templates");
	info=Window_CreateInfoWindowFromMsgs("Task.Name:","Task.Purpose:","©Alex Waugh 1998",VERSION);
	mainwin=Window_Create("Main",template_TITLEMIN);
	Msgs_Lookup("MainWin.Back:",goback,256);
	Msgs_Lookup("MainWin.GoAgain:",goagain,256);
	Msgs_SetTitle(mainwin,"MainWin.Title:");
	choosewin=Window_Create("Choose",template_TITLEMIN);
	Msgs_SetTitle(choosewin,"Choose.Title:?");
	report=Dialog_Create("Report",template_TITLEMIN);
	Msgs_SetText(Dialog_WindowHandle(report),icon_REPORT_OK,"Report.Ok:?");
	Msgs_SetTitle(Dialog_WindowHandle(report),"Report.Title:?");
	Event_Claim(event_CLICK,Dialog_WindowHandle(report),icon_REPORT_OK,CloseDialog,NULL);
	Event_Claim(event_KEY,Dialog_WindowHandle(report),event_ANY,ReportKeyHandler,NULL);
	iconbarmenu=Menu_CreateFromMsgs("Task.Name:","Menu.IconBar:",IconBarMenuClick,NULL);
	Menu_AddSubMenu(iconbarmenu,menuitem_INFO,(menu_ptr)info);
	Menu_Attach(window_ICONBAR,event_ANY,iconbarmenu,button_MENU);
	Event_Claim(event_CLICK,window_ICONBAR,event_ANY,IconBarClick,NULL);
	Event_Claim(event_CLICK,mainwin,icon_YES,YesClick,NULL);
	Event_Claim(event_CLICK,mainwin,icon_NO,NoClick,NULL);
	Event_Claim(event_CLICK,mainwin,icon_BACK,BackClick,NULL);
	Event_Claim(event_CLICK,choosewin,event_ANY,ChooseClick,NULL);
	Event_Claim(event_CLOSE,mainwin,event_ANY,CloseMainWin,NULL);
	Event_Claim(event_REDRAW,mainwin,event_ANY,RedrawMainWin,NULL);
	Event_Claim(event_REDRAW,choosewin,event_ANY,RedrawChooseWin,NULL);
	flex_init("ShapeSort",NULL);
	LoadDrawFiles();
	LoadFile(TREEDESCRIPTIONFILE);
#if REPORT
	Report(Msgs_TempLookup("Report.Init"));
	Pointer_RestrictToWindow(Dialog_WindowHandle(report));
#endif
	while (TRUE) Event_Poll();
	return 0;
}

