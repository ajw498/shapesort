/*
	Questions
	©Alex Waugh 1998
	See !Help file for conditions of use
*/

#include "Desk.Error.h"
#include "Desk.File.h"
#include "Desk.Filing.h"
#include "Desk.Msgs.h"
#include "Desk.Str.h"
#include "Desk.Sound.h"
#include "Desk.DeskMem.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LEAFNODE (-1)
#define NOPARENT (-1)


typedef struct {
	int  leftptr;
	int  rightptr;
	int  parentptr;
	char question[256];
	char lefttext[20];
	char righttext[20];
	char filename[12];
} node;

static node *nodes=NULL;

static int currentnode,numberofnodes;

static int Add(int parent,char *taggroup);

void LoadFile(char *filename)
{
	if (nodes!=NULL) Desk_Error_Report(1,"You cannot load two description files"),exit(EXIT_FAILURE);
	Desk_Msgs_LoadFile(filename);
	numberofnodes=0;
	Add(NOPARENT,"Node0");
	currentnode=0;
}

static void StripColon(char *text)
{
	char tempbuffer[256];
	if (text[0]==':') {
		strcpy(tempbuffer,text);
		strcpy(text,tempbuffer+1);
	}
}

static int Add(int parent,char *taggroup)
{
	char buffer[256];
	char buffer2[256];
	int currentnode;
	if (parent==NOPARENT) {
		nodes=Desk_DeskMem_Malloc(sizeof(node));
		numberofnodes=1;
	} else {
		nodes=Desk_DeskMem_Realloc(nodes,sizeof(node)*(++numberofnodes));
	}
	currentnode=numberofnodes-1;
	strcpy(buffer,taggroup);
	strcat(buffer,".Question:?");
	Desk_Msgs_Lookup(buffer,buffer2,256);
	StripColon(buffer2);
	strcpy(nodes[currentnode].question,buffer2);
	strcpy(buffer,taggroup);
	strcat(buffer,".LeftText:Yes");
	Desk_Msgs_Lookup(buffer,buffer2,20);
	StripColon(buffer2);
	if (strcmp(buffer2,"X")==0) strcpy(buffer2,"");
	strcpy(nodes[currentnode].lefttext,buffer2);
	strcpy(buffer,taggroup);
	strcat(buffer,".RightText:No");
	Desk_Msgs_Lookup(buffer,buffer2,20);
	StripColon(buffer2);
	if (strcmp(buffer2,"X")==0) strcpy(buffer2,"");
	strcpy(nodes[currentnode].righttext,buffer2);
	nodes[currentnode].parentptr=parent;
	strcpy(buffer,taggroup);
	strcat(buffer,".LeftNode:None");
	Desk_Msgs_Lookup(buffer,buffer2,20);
	StripColon(buffer2);
	if (Desk_stricmp(buffer2,"None")==0) nodes[currentnode].leftptr=LEAFNODE; else nodes[currentnode].leftptr=Add(currentnode,buffer2);
	strcpy(buffer,taggroup);
	strcat(buffer,".RightNode:None");
	Desk_Msgs_Lookup(buffer,buffer2,20);
	StripColon(buffer2);
	if (Desk_stricmp(buffer2,"None")==0) nodes[currentnode].rightptr=LEAFNODE; else nodes[currentnode].rightptr=Add(currentnode,buffer2);
	strcpy(buffer,taggroup);
	strcat(buffer,".Filename:None");
	Desk_Msgs_Lookup(buffer,nodes[currentnode].filename,11);
	StripColon(buffer2);
	return currentnode;
}

char *GetLeftText(void)
{
	return nodes[currentnode].lefttext;
}

char *GetRightText(void)
{
	return nodes[currentnode].righttext;
}

char *GetQuestion(void)
{
	return nodes[currentnode].question;
}

char *GetFilename(void)
{
	return nodes[currentnode].filename;
}

Desk_bool LeafNode(void)
{
	if (nodes[currentnode].leftptr==LEAFNODE || nodes[currentnode].rightptr==LEAFNODE) return Desk_TRUE;
	return Desk_FALSE;
}

Desk_bool RootNode(void)
{
	if (nodes[currentnode].parentptr==NOPARENT) return Desk_TRUE;
	return Desk_FALSE;
}

void MoveLeft(void)
{
	if (nodes[currentnode].leftptr!=LEAFNODE)
		currentnode=nodes[currentnode].leftptr;
	else Desk_Sound_SysBeep();
}

void MoveRight(void)
{
	if (nodes[currentnode].rightptr!=LEAFNODE)
		currentnode=nodes[currentnode].rightptr;
	else Desk_Sound_SysBeep();
}

void MoveBack(void)
{
	if (nodes[currentnode].parentptr!=NOPARENT)
		currentnode=nodes[currentnode].parentptr;
	else Desk_Sound_SysBeep();
}

void MoveToRoot(void)
{
	currentnode=0;
}
