/*
 *   THIS FILE IS UNDER CVS - DO NOT MODIFY UNLESS...HELLO,
 *   ARE YOU READING THIS?  BUELLER?
 *
 *    $Id: stalist_parse.c,v 1.3 2009/02/17 19:07:59 mark Exp $
 *
 *    Revision history:
 *     $Log: stalist_parse.c,v $
 *     Revision 1.3  2009/02/17 19:07:59  mark
 *     Added ParseGlassList
 *
 *     Revision 1.2  2009/02/13 00:23:11  mark
 *     Fixed compile errors
 *
 *     Revision 1.1  2009/02/04 18:28:07  mark
 *     Initial checkin
 *
 */

#include <watchdog_client.h>
#include "scnl_list_utils.h"
#include <stdio.h>
#include <string.h>
#include <earthworm.h>
#ifndef _WINNT
#include <errno.h>
#endif

// Hypoinverse field positions.  These are taken from stalist_convert_hinv.c
#define STA_POS 1
#define NET_POS 7
#define CHAN_POS 11
#define LOC_POS 81

#define STA_LEN 5
#define NET_LEN 2
#define CHAN_LEN 3
#define LOC_LEN 2


int ParsePickerList(char *szFile, SCNL *pSCNL, int SCNLSize, int *pNumSCNL)
{
    FILE *pFile;
    char szLine[128];
    char *szReturn;
    int err, funcret = EW_SUCCESS;

    if (pSCNL == NULL || pNumSCNL == NULL || szFile == NULL)
    {
        reportError(WD_FATAL_ERROR, GENFATERR, "Bad params to ParsePickerList\n");
        return EW_FAILURE;
    }

    // Open the file
 #ifdef _WINNT
    err = fopen_s(&pFile, szFile, "r");
 #else
    pFile = fopen(szFile, "r");
    err = errno;
 #endif
    if (err != 0)
    {
        reportError(WD_FATAL_ERROR, SYSERR, "Unable to open picker list <%s>\n", szFile);
        return EW_FAILURE;
    }

    *pNumSCNL = 0;

    while (TRUE)
    {
        // Read the next line from the input file.
        szReturn = fgets(szLine, sizeof(szLine), pFile);
        if (szReturn == NULL)
        {
            if (feof(pFile))
            {
                // End of the file - nothing more to read.
                break;
            }

            // Something more serious has happened.
            err = ferror(pFile);
            reportError(WD_FATAL_ERROR, GENFATERR,"ParsePickerList(): Error %d reading from input file\n", err);
            funcret = EW_FAILURE;
            break;
        }

        //
        // Parse this entry into our input struct.
        //
        if (strncmp(szLine, "SCNL", 4) != 0)
        {
            reportError(WD_FATAL_ERROR, GENFATERR, "ParsePickerList(): Invalid entry <%s>\n", szLine);
            funcret = EW_FAILURE;
            break;
        }
#ifdef _WINNT
        if (sscanf_s(&(szLine[4]), " %s %s %s %s", pSCNL[*pNumSCNL].sta, (unsigned)sizeof(pSCNL[*pNumSCNL].sta),
                    pSCNL[*pNumSCNL].comp, (unsigned)sizeof(pSCNL[*pNumSCNL].comp),
                    pSCNL[*pNumSCNL].net, (unsigned)sizeof(pSCNL[*pNumSCNL].net),
                    pSCNL[*pNumSCNL].loc, (unsigned)sizeof(pSCNL[*pNumSCNL].loc)) < 4)
#else
        if (sscanf(&(szLine[4]), " %s %s %s %s", pSCNL[*pNumSCNL].sta,
                  pSCNL[*pNumSCNL].comp,
                  pSCNL[*pNumSCNL].net,
                  pSCNL[*pNumSCNL].loc) < 4)
#endif
        {
            reportError(WD_FATAL_ERROR, GENFATERR, "ParsePickerList(): Unable to get 4 fields from entry <%s>\n", szLine);
            funcret = EW_FAILURE;
            break;
        }

        // Check for any blank fields.
        if (strlen(pSCNL[*pNumSCNL].sta) == 0 || strlen(pSCNL[*pNumSCNL].comp) == 0 ||
                strlen(pSCNL[*pNumSCNL].net) == 0 || strlen(pSCNL[*pNumSCNL].loc) == 0)
        {
            reportError(WD_FATAL_ERROR, GENFATERR, "ParsePickerList(): Blank fields from entry <%s>\n", szLine);
            funcret = EW_FAILURE;
            break;
        }

        // This SCNL is OK.  Increment our counter.
        (*pNumSCNL)++;

        if (*pNumSCNL >= SCNLSize)
        {
            // Our given input isn't big enough.  Warn the calling function and exit.
            reportError(WD_INFO, 0, "ParsePickerList(): Input buffer too small\n");
            funcret = EW_WARNING;
            break;
        }
    }   // while (TRUE)

    fclose(pFile);
    return funcret;
}

int ParseGlassList(char *szFile, SCNL *pSCNL, int SCNLSize, int *pNumSCNL)
{
    FILE *pFile;
    char szLine[128];
    char *szReturn;
    int err, funcret = EW_SUCCESS;

    if (pSCNL == NULL || pNumSCNL == NULL || szFile == NULL)
    {
        reportError(WD_FATAL_ERROR, GENFATERR, "Bad params to ParseGlassList\n");
        return EW_FAILURE;
    }

    // Open the file
 #ifdef _WINNT
    err = fopen_s(&pFile, szFile, "r");
 #else
    pFile = fopen(szFile, "r");
    err = errno;
 #endif
    if (err != 0)
    {
        reportError(WD_FATAL_ERROR, SYSERR, "Unable to open glass list <%s>\n", szFile);
        return EW_FAILURE;
    }

    *pNumSCNL = 0;

    while (TRUE)
    {
        // Read the next line from the input file.
        szReturn = fgets(szLine, sizeof(szLine), pFile);
        if (szReturn == NULL)
        {
            if (feof(pFile))
            {
                // End of the file - nothing more to read.
                break;
            }

            // Something more serious has happened.
            err = ferror(pFile);
            reportError(WD_FATAL_ERROR, GENFATERR,"ParseGlassList(): Error %d reading from input file\n", err);
            funcret = EW_FAILURE;
            break;
        }

        //
        // Parse this entry into our input struct.
        //

        // Ignore comments and blank lines.
        if (szLine[0] == '#')
            continue;
        if (szLine[0] == '\n')
            continue;

        // Make sure the entry is big enough...
        if(strlen(szLine) < 81 /* Magic Number */)
        {
            // Treat these as fatal.  The automatics are supposed to throw these entries away,
            // so something fell apart there.
            reportError(WD_FATAL_ERROR, GENFATERR, "ParseGlassList: Entry too short: <%s>", szLine);
            funcret = EW_FAILURE;
            break;
        }

        strncpy(pSCNL[*pNumSCNL].sta, &szLine[STA_POS-1], STA_LEN);
        strncpy(pSCNL[*pNumSCNL].comp, &szLine[CHAN_POS-1], CHAN_LEN);
        strncpy(pSCNL[*pNumSCNL].net, &szLine[NET_POS-1], NET_LEN);
        strncpy(pSCNL[*pNumSCNL].loc, &szLine[LOC_POS-1], LOC_LEN);

        // Strip any trailing blanks
        strib(pSCNL[*pNumSCNL].sta);
        strib(pSCNL[*pNumSCNL].comp);
        strib(pSCNL[*pNumSCNL].net);
        strib(pSCNL[*pNumSCNL].loc);

        // Check for any blank fields.
        if (strlen(pSCNL[*pNumSCNL].sta) == 0 || strlen(pSCNL[*pNumSCNL].comp) == 0 ||
                strlen(pSCNL[*pNumSCNL].net) == 0 || strlen(pSCNL[*pNumSCNL].loc) == 0)
        {
            reportError(WD_FATAL_ERROR, GENFATERR, "ParseGlassList(): Blank fields from entry <%s>\n", szLine);
            funcret = EW_FAILURE;
            break;
        }

        // This SCNL is OK.  Increment our counter.
        (*pNumSCNL)++;

        if (*pNumSCNL >= SCNLSize)
        {
            // Our given input isn't big enough.  Warn the calling function and exit.
            reportError(WD_INFO, 0, "ParseGlassList(): Input buffer too small\n");
            funcret = EW_WARNING;
            break;
        }
    }   // while (TRUE)

    fclose(pFile);
    return funcret;
}
