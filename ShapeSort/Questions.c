/*
	Questions
	©Alex Waugh 1998
	See !Help file for conditions of use
*/

#include "DeskLib:Error.h"
#include "DeskLib:File.h"
#include "DeskLib:Filing.h"
#include "DeskLib:Msgs.h"
#include "DeskLib:Str.h"
#include "AJWLib:Flex.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define LEAFNODE (-1)
#define NOPARENT (-1)

extern void Beep(void);

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
	if (nodes!=NULL) Error_Report(1,"You cannot load two description files"),exit(EXIT_FAILURE);
	Msgs_LoadFile(filename);
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
	char buffer[30];
	char buffer2[20];
	int currentnode;
	if (parent==NOPARENT) {
		flex_alloc((flex_ptr)&nodes,sizeof(node));
		if (nodes==NULL) Msgs_ReportFatal(1,"Error.NoMem:Not enough free memory");
		numberofnodes=1;
	} else {
		if (!flex_extend((flex_ptr)&nodes,sizeof(node)*++numberofnodes)) Msgs_ReportFatal(1,"Error.NoMem:Not enough free memory");
	}
	currentnode=numberofnodes-1;
	strcpy(buffer,taggroup);
	strcat(buffer,".Question:?");
	Msgs_Lookup(buffer,buffer2,256);
	StripColon(buffer2);
	strcpy(nodes[currentnode].question,buffer2);
	strcpy(buffer,taggroup);
	strcat(buffer,".LeftText:Yes");
	Msgs_Lookup(buffer,buffer2,20);
	StripColon(buffer2);
	if (strcmp(buffer2,"X")==0) strcpy(buffer2,"");
	strcpy(nodes[currentnode].lefttext,buffer2);
	strcpy(buffer,taggroup);
	strcat(buffer,".RightText:No");
	Msgs_Lookup(buffer,buffer2,20);
	StripColon(buffer2);
	if (strcmp(buffer2,"X")==0) strcpy(buffer2,"");
	strcpy(nodes[currentnode].righttext,buffer2);
	nodes[currentnode].parentptr=parent;
	strcpy(buffer,taggroup);
	strcat(buffer,".LeftNode:None");
	Msgs_Lookup(buffer,buffer2,20);
	StripColon(buffer2);
	if (stricmp(buffer2,"None")==0) nodes[currentnode].leftptr=LEAFNODE; else nodes[currentnode].leftptr=Add(currentnode,buffer2);
	strcpy(buffer,taggroup);
	strcat(buffer,".RightNode:None");
	Msgs_Lookup(buffer,buffer2,20);
	StripColon(buffer2);
	if (stricmp(buffer2,"None")==0) nodes[currentnode].rightptr=LEAFNODE; else nodes[currentnode].rightptr=Add(currentnode,buffer2);
	strcpy(buffer,taggroup);
	strcat(buffer,".Filename:None");
	Msgs_Lookup(buffer,nodes[currentnode].filename,11);
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

BOOL LeafNode(void)
{
	if (nodes[currentnode].leftptr==LEAFNODE || nodes[currentnode].rightptr==LEAFNODE) return TRUE;
	return FALSE;
}

BOOL RootNode(void)
{
	if (nodes[currentnode].parentptr==NOPARENT) return TRUE;
	return FALSE;
}

void MoveLeft(void)
{
	if (nodes[currentnode].leftptr!=LEAFNODE)
		currentnode=nodes[currentnode].leftptr;
	else Beep();
}

void MoveRight(void)
{
	if (nodes[currentnode].rightptr!=LEAFNODE)
		currentnode=nodes[currentnode].rightptr;
	else Beep();
}

void MoveBack(void)
{
	if (nodes[currentnode].parentptr!=NOPARENT)
		currentnode=nodes[currentnode].parentptr;
	else Beep();
}

void MoveToRoot(void)
{
	currentnode=0;
}
