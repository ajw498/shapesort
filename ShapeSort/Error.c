/*
    ####             #    #     # #
    #   #            #    #       #          The FreeWare C library for
    #   #  ##   ###  #  # #     # ###             RISC OS machines
    #   # #  # #     # #  #     # #  #   ___________________________________
    #   # ####  ###  ##   #     # #  #
    #   # #        # # #  #     # #  #    Please refer to the accompanying
    ####   ### ####  #  # ##### # ###    documentation for conditions of use
    ________________________________________________________________________

    File:    Error.c
    Author:  Copyright © 1992 Jason Williams
             Improved by Philip Colmer
    Version: 0.19 (13 Jul 1993)
    Purpose: Centralised error handling functions
    Mods:    7 Apr 1992 - JCW - Added Error_OutOfMemory
            30 Apr 1993 - JCW - Fixed (Wimp_ReportError prototype changed)
            14 Jun 1993 - PJC - Allowed Error_Report(Fatal) to take
                                variable arguments
            13 Jul 1993 - PJC - Added varargs to "Internal" versions of above
*/


#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "DeskLib:Error.h"
#include "DeskLib:WimpSWIs.h"
#include "DeskLib:Event.h"

extern void Error_Report(int errornum, char *report, ...)
{
  va_list va;
  os_error    error;
  error_flags eflags;

  va_start(va, report);
  vsprintf(error.errmess, report, va);
  va_end(va);
  error.errnum = errornum;

  eflags.value = 1;
  Wimp_ReportError(&error, eflags.value, event_taskname);
}



extern void Error_ReportFatal(int errornum, char *report, ...)
{
  va_list va;
  os_error    error;
  error_flags eflags;

  va_start(va, report);
  vsprintf(error.errmess, report, va);
  va_end(va);
  error.errnum = errornum;

  eflags.value = 1;
  Wimp_ReportError(&error, eflags.value, event_taskname);
  exit(1);
}



extern void Error_ReportInternal(int errornum, char *report, ...)
{
  va_list va;
  char errmess[256];

  va_start(va, report);
  vsprintf(errmess, report, va);
  va_end(va);
  Error_Report(errornum, errmess);
}



extern void Error_ReportFatalInternal(int errornum, char *report, ...)
{
  va_list va;
  char errmess[256];

  va_start(va, report);
  vsprintf(errmess, report, va);
  va_end(va);
  Error_ReportFatal(errornum, errmess);
}



extern BOOL Error_Check(os_error *error)
{
  if (error != NULL)
  {
    Error_Report(error->errnum, error->errmess);
    return(TRUE);
  }
  return(FALSE);
}



extern void Error_CheckFatal(os_error *error)
{
  if (error != NULL)
    Error_ReportFatal(error->errnum, error->errmess);
}



extern BOOL Error_OutOfMemory(BOOL fatal, char *place)
{
  if (fatal)
    Error_ReportFatal(0, "Unable to get enough memory for the %s", place);
  else
    Error_Report(0, "Unable to get enough memory for the %s", place);

  return(FALSE);  /*  Always returns FALSE so can return FALSE from your
                   *  own function at the same time as reporting the error
                   */
}
