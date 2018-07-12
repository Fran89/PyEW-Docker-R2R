/*$Id: cfgutil.c 5818 2013-08-14 20:47:04Z paulf $*/
/*   Configuration file utility module.
     Copyright 1994-1997 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 15 Mar 94 WHO Translated from cfgutil.pas
    1 20 Mar 94 WHO split procedure added.
    2 30 May 94 WHO Extra semicolons in read_cfg removed. Incorrect
                    parameter to fclose in close_cfg fixed (DSN).
    3  9 Jun 94 WHO Cleanup to avoid warnings.
    4 27 Feb 95 WHO Start of conversion to run on OS9.
    5  9 Jul 96 WHO Fix read_cfg so that it notices end of file when
                    it really happens, rather than repeating the last
                    line read.
    6 16 Jul 96 WHO Put back in the initialization of "tries" in skipto,
                    it disapeared somehow.
    7  3 Nov 97 WHO Larger configuration sizes. Add VER_CFGUTIL.
    8  9 Nov 99 IGD Porting to SuSE 6.1 LINUX begins
                        IGD  In version 7 which I have read_cfg () expression "if     (feof(cs->cfgfile))"
                                 was "if feof(cs->cfgfile)" causing a compiler parsing error. Fixed.
*/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifndef _OSK

#include <fcntl.h>
#include <sys/types.h>
#include <ctype.h>
#else
#include "os9stuff.h"
#endif
#include "stuff.h"

/* type definitions needed to use this module */
#ifdef _OSK
#define CFGWIDTH 200
#define SECWIDTH 120
#else
#define CFGWIDTH 512
#define SECWIDTH 256
#endif

short VER_CFGUTIL = 8 ;

  typedef struct
    {
      char lastread[CFGWIDTH] ;
      FILE *cfgfile ;
    } config_struc ;
   
/* skip to the specified section */
  short skipto (config_struc *cs, pchar section)
    {
      char s[SECWIDTH] ;
      short tries ;
      pchar tmp ;
   
      /* repeat up to twice if looking for a specific section, or once
         if looking for any section "*" */
      tries = 0 ;
      do
        {
          /* build "[section]" string */
          strcpy(s, "[") ;
          strcat(s, section) ;
          strcat(s, "]") ;
          /* If the last line read is the section desired or if the section
             is "*" and the line is any section start then return with FALSE
             status */
          if ((strcasecmp((pchar)&cs->lastread, (pchar)&s) == 0) ||
             ((strcmp(section, "*") == 0) && (cs->lastread[0] == '[')))
              return FALSE ;
          /* read lines (removing trailing spaces) until either the desired
             section is found, or if the section is "*" and the new line is
             any section start. If found, return FALSE */
          do
            {
              tmp = fgets(cs->lastread, CFGWIDTH-1, cs->cfgfile) ;
              untrail (cs->lastread) ;
            }
          while ((tmp != NULL) && (strcasecmp((pchar)&cs->lastread, (pchar)&s) != 0) &&
            ((strcmp(section, "*") != 0) || (cs->lastread[0] != '['))) ;
            
          /* if eof(cfgfile) */
          if (tmp == NULL)
              {
                tries++ ;
                rewind(cs->cfgfile) ;
                cs->lastread[0] = '\0' ;
              }
            else
              return FALSE ;
          }
        while ((tries <= 1) && (strcmp(section, "*") != 0)) ;
      return TRUE ; /* no good */    
    }

/* open the configuration file, return TRUE if the file cannot be opened
   or the section is not found */
  short open_cfg (config_struc *cs, pchar fname, pchar section)
    {
      cs->cfgfile = fopen(fname, "r") ;
      if (cs->cfgfile == NULL)
          return TRUE ;
      cs->lastread[0] = '\0' ; /* there is no next section */
      return skipto(cs, section) ; /* skip to section */
    }

/* Returns the part to the left of the "=" in s1, upshifted. Returns the
   part to the right of the "=" in s2, not upshifted. Returns with s1
   a null string if no more strings in the section */
  void read_cfg (config_struc *cs, pchar s1, pchar s2)
    {
      pchar tmp;

/* start with two null strings, in case things don't go well */
      *s1 = '\0' ;
      *s2 = '\0' ;
      do
        {
          if  (feof(cs->cfgfile)) /* IGD was originally, generating compilation error-> if feof(cs->cfgfile)  */
              return ;
/* read a line and remove trailing spaces */
          tmp = fgets(cs->lastread, CFGWIDTH-1, cs->cfgfile) ;
          untrail (cs->lastread) ;
          if ((tmp != NULL) && (cs->lastread[0] != '\0'))
              {
/* if starts with a '[', it is start of new section, return */
                if (cs->lastread[0] == '[')
                    return ;
/* if first character in a line is A-Z or 0-9, then process line */
                if (isalnum(cs->lastread[0]))
                    break ;
              }
        }
      while (1) ;
      strcpy(s1, cs->lastread) ;
      tmp = strchr(s1, '=') ;
      if (tmp)
          {
            str_right (s2, tmp) ; /* copy from tmp+1 to terminator */
            *tmp = '\0' ; /* replace "=" with terminator */
          }
      upshift(s1) ;
    }

  void close_cfg (config_struc *cs)
    {
      fclose(cs->cfgfile) ;
    }

/* Find the separator character in the source string, the portion to the
   right is moved into "right". The portion to the left remains in "src".
   The separator itself is removed. If the separator is not found then
   "src" is unchanged and "right" is a null string.
*/
  void comserv_split (pchar src, pchar right, char sep)
    {
      pchar tmp ;
      
      tmp = strchr (src, sep) ;
      if (tmp)
          {
            str_right (right, tmp) ;
            *tmp = '\0' ;
          }
        else
          right[0] = '\0' ;
    }
