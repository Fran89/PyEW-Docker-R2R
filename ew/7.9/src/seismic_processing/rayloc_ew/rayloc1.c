/***************************************************************************
 *  This code is a part of rayloc_ew / USGS EarthWorm module               *
 *                                                                         *
 *  It is written by ISTI (Instrumental Software Technologies, Inc.)       *
 *          as a part of a contract with CERI USGS.                        *
 * For support contact info@isti.com                                       *
 *   Ilya Dricker (i.dricker@isti.com)                                     *
 *                                                   Aug 2004              *
 ***************************************************************************/

/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: rayloc1.c 2055 2006-01-19 19:04:55Z friberg $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2006/01/19 19:04:55  friberg
 *     rayloc upgraded to use a library version of rayloc_message_rw.c
 *
 *     Revision 1.1  2004/08/05 04:15:11  friberg
 *     First commit of rayloc_ew in EW-CENTRAL CVS
 *
 *     Revision 1.7  2004/08/04 19:27:54  ilya
 *     Towards version 1.0
 *
 *     Revision 1.6  2004/08/03 18:26:05  ilya
 *     Now we use stock EW functions from v6.2
 *
 *     Revision 1.5  2004/08/03 17:51:47  ilya
 *     Finalizing the project: using EW globals
 *
 *     Revision 1.4  2004/07/29 21:32:03  ilya
 *     New logging; tests; fixes
 *
 *     Revision 1.3  2004/07/29 17:28:54  ilya
 *     Fixed makefile.sol; added makefile.sol_gcc; tested cc compilation
 *
 *     Revision 1.2  2004/06/25 14:22:15  ilya
 *     Working version: no output
 *
 *     Revision 1.1.1.1  2004/06/22 21:12:06  ilya
 *     initial import into CVS
 *
 */

/***************************************************************************
                          rayloc1.c  -  description
                             -------------------
    begin                : Tue Jun 1 2004
    copyright            : (C) 2004 by Ilya Dricker, ISTI
    email                : i.dricker@isti.com
 ***************************************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <earthworm.h>
#include <chron3.h>
#include "rayloc1.h"
#include <global_loc_rw.h>
#include <rayloc_message_rw.h>

#define MODEL_HEAD "ak135.hed"
#define MODEL_TABLE "ak135.tbl"
#define TAU_TABLE "tau.table"


int
	rayloc_print_single_phase(FILE *fd,
	               RAYLOC_PHASES *unused_list,
	               GLOBAL_PHSLINE_STRUCT *phase,
	               RAYLOC_STATIONS *stns);
int
	rayloc_print_phases(FILE *fd,
	              RAYLOC_PHASES *unused_list,
	              GLOBAL_LOC_STRUCT *p_loc,
	              RAYLOC_STATIONS *stns);

int
	rayloc_maintain_token_files(const char *token, const char *dirName);


int
	rayloc_msg_to_rayloc_input(GLOBAL_LOC_STRUCT *p_loc,
	                       RAYLOC_PHASES *unused_list,
	                       RAYLOC_PROC_FLAGS *flags,
	                       const char *rayloc_in_fname,
	                       RAYLOC_STATIONS *sta);

void
	rayloc_print_flags(FILE *fd, RAYLOC_PROC_FLAGS *flags);


int
	lib_rayloc_stub(const char *token, const char *dirname)
	{
			extern int libray_();
			char token1[1001];
			char dirname1[1001];
			char tmpFile[1001];
			int retVal;
			struct stat buf;


			/* Maintainance portion of the code */

			/* Check if provided directory is really a writable dir */
			retVal = stat(dirname, &buf);
			if (-1 == retVal)
				return RAYLOC_NOT_DIRECTORY;
			if (!(buf.st_mode & S_IFDIR)) /* Must be directory */
				return RAYLOC_NOT_DIRECTORY;
			if (!(buf.st_mode & S_IWUSR)) /* Must be writable by user */
				return RAYLOC_DIR_NOT_WRITABLE;

			/* Check if model files exist */
			sprintf(tmpFile, "%s/%s", dirname, MODEL_HEAD);
			retVal = stat(tmpFile, &buf);
			if (-1 == retVal)
				return RAYLOC_MODEL_HEAD_NOT_EXIST;
			if (!(buf.st_mode & S_IFREG)) /* Must be regular file */
				return RAYLOC_MODEL_HEAD_NOT_EXIST;

			sprintf(tmpFile, "%s/%s", dirname, MODEL_TABLE);
			retVal = stat(tmpFile, &buf);
			if (-1 == retVal)
				return RAYLOC_MODEL_TABLE_NOT_EXIST;
			if (!(buf.st_mode & S_IFREG)) /* Must be regular file */
				return RAYLOC_MODEL_TABLE_NOT_EXIST;

			sprintf(tmpFile, "%s/%s", dirname, TAU_TABLE);
			retVal = stat(tmpFile, &buf);
			if (-1 == retVal)
				return RAYLOC_TAU_TABLE_NOT_EXIST;
			if (!(buf.st_mode & S_IFREG)) /* Must be regular file */
				return RAYLOC_TAU_TABLE_NOT_EXIST;

			retVal = rayloc_maintain_token_files(token, dirname);

			memset(token1, ' ', sizeof(token1));
			memset(dirname1, ' ', sizeof(dirname1));
			strncpy(token1, token, strlen(token));
			strncpy(dirname1, dirname, strlen(dirname));

			libray_ (token1, dirname1, strlen(token1), strlen(dirname1));
			return RAYLOC_SUCCESS;
	}

int
	rayloc_maintain_token_files(const char *token, const char *dirName)
	{
		char tmp[1000];
		struct stat buf;
		int retVal;

		/* FORTRAN CODE we deal with assumes three files in "dirName" directory
		 * RayLocInput[token].txt -> must exist and be readable;
		 * RayLocOutput[token].txt -> must not exist;
		 * RayLocError[token].txt -> must not exist;
		 * Here we check if the input file exists; error if not
		 * We remove the other two files if they exist
		 */

		sprintf(tmp, "%s/RayLocInput%s.txt", dirName, token);
		retVal = stat(tmp, &buf);
		if (-1 == retVal)
			return RAYLOC_INPUT_NOT_EXIST;

		sprintf(tmp, "%s/RayLocOutput%s.txt", dirName, token);
		retVal = stat(tmp, &buf);
		if (0 == retVal)
		{
			retVal = unlink(tmp);
			if (-1 == retVal)
				return RAYLOC_FAILED_TO_UNLINK;
		}

		sprintf(tmp, "%s/RayLocError%s.txt", dirName, token);
		retVal = stat(tmp, &buf);
		retVal = stat(tmp, &buf);
		if (0 == retVal)
		{
			retVal = unlink(tmp);
			if (-1 == retVal)
				return RAYLOC_FAILED_TO_UNLINK;
		}

		return RAYLOC_SUCCESS;
	}

int
	lib_rayloc(const char *in_message,
	           int in_mesLen,
	           RAYLOC_PHASES *unused_list,
	           RAYLOC_PROC_FLAGS *flags,
	           const char *token,
	           const char *dirname,
	           unsigned char headerType,
	           unsigned char headerMod,
	           unsigned char headerInst,
	           unsigned char picksType,
	           unsigned char picksMod,
	           unsigned char picksInst,
	           char **out_message,
	           char **err_message,
	           RAYLOC_STATIONS *stns)
	{
		int retVal;
		char rayloc_in_fname[1000];
		char rayloc_out_fname[1000];
		char *rayLocMessage = NULL;
		unsigned int mes_length;
#ifdef USE_LOGIT
		int i;
#endif

		RAYLOC_MESSAGE_HEADER_STRUCT *p_struct = NULL;
		GLOBAL_LOC_STRUCT p_loc;

		retVal = StringToLoc(&p_loc, (char *)in_message);
		if(retVal != GLOBAL_MSG_SUCCESS)
		{
#ifdef USE_LOGIT
			logit("pt", "lib_rayloc: Failed to convert Input Message into structure\n");
#endif
			return RAYLOC_FAILED;
		}


		sprintf(rayloc_in_fname, "%s/RayLocInput%s.txt", dirname, token);
#ifdef USE_LOGIT
			logit("pt", "lib_rayloc: Processing event ID %d (origin ID %d) ; lat = %f; lon = %f; depth = %f\n",
			     p_loc.event_id, p_loc.origin_id, p_loc.lat, p_loc.lon, p_loc.depth);
#endif
		retVal = rayloc_msg_to_rayloc_input(&p_loc, unused_list, flags, rayloc_in_fname, stns);
		if(retVal != RAYLOC_SUCCESS)
		{
#ifdef USE_LOGIT
			logit("pt", "lib_rayloc: Failed to write Input Structure to the file %s\n", rayloc_in_fname);
#endif
			return retVal;
		}
#ifdef USE_LOGIT
			logit("pt", "lib_rayloc: Wrote Input Structure to the file %s\n", rayloc_in_fname);
#endif

		retVal = lib_rayloc_stub(token, dirname);
		if(retVal != RAYLOC_SUCCESS)
		{
#ifdef USE_LOGIT
			logit("pt", "lib_rayloc: Failed to run RAYLOC FORTRAN CODE: retVal = %d\n", retVal);
#endif
			return retVal;
		}

		sprintf(rayloc_out_fname, "%s/RayLocOutput%s.txt", dirname, token);
		retVal = rayloc_fileToRaylocHeader(&p_struct,
		                      rayloc_out_fname,
		                      p_loc.event_id,
		                      headerType,
		                      headerMod,
		                      headerInst,
		                      picksType,
		                      picksMod,
		                      picksInst);
		if (retVal != RAYLOC_MSG_SUCCESS)
		{
#ifdef USE_LOGIT
			logit("pt", "lib_rayloc: Failed to interpret RAYLOC output from file %s\n", rayloc_out_fname);
#endif
			return retVal;
		}

#ifdef USE_LOGIT
		logit("pt", "lib_rayloc: Processed RAYLOC output from file %s\n", rayloc_out_fname);
		logit("pt", "lib_rayloc: OUTPUT TYPE_RAYLOC message contains:\n");
		logit("pt", "lib_rayloc: event ID = %d Origin Time = %s Lat = %f; lon = %f; depth = %f\n",
		   p_struct->event_id , p_struct->origin_time_char , p_struct->elat , p_struct->elon , p_struct->edepth );
		logit ("pt", "lib_rayloc: Used %d Channels in processing\n", p_struct->numPicks);
		logit ("pt", "    lib_rayloc: NET STA CHAN LOC-ID EV-ID PHASE DIST AZ WEIGHT RESIDUAL\n");
		for (i = 0; i < p_struct->numPicks; i++)
		{
			logit ("pt", "    lib_rayloc: %s  %s  %s  %s  %ld %s %8.3f %8.3f %c %8.3f\n",
			     (p_struct->picks[0])->network ,
			     (p_struct->picks[0])->station ,
			     (p_struct->picks[0])->channel ,
			     (p_struct->picks[0])->location ,
			     (p_struct->picks[0])->pick_id ,
			     (p_struct->picks[0])->phase_name ,
			     (p_struct->picks[0])->dist ,
			     (p_struct->picks[0])->az ,
			     (p_struct->picks[0])->weight_flag ,
			     (p_struct->picks[0])->residual );
		}
#endif
		rayLocMessage = rayloc_WriteRaylocHeaderBuffer(p_struct, &mes_length, &retVal);
		if (*out_message)
			free(*out_message);
		rayloc_FreeRaylocHeader( &p_struct);

		*out_message = rayLocMessage;
		if (!rayLocMessage)
		{
			logit ("pt", "lib_rayloc: Failed to format TYPE_RAYLOC Output message\n");
			return RAYLOC_FAILED;
		}


		return mes_length;
	}

int
	file2string(const char *fname, char **str)
	{
		FILE *fd;
		char tmp[10000];
		int curLen = 0;
		int firstRun = RAYLOC_TRUE;

		fd = fopen(fname, "r");
		if (!fd)
			return RAYLOC_FAILED;
		if (*str)
		{
			free(*str);
			*str = NULL;
		}

		while (fgets (tmp, 10000, fd))
		{
			if (strlen(tmp) < 1)
			{
				continue;
			}
			*str = realloc(*str, curLen + 1 + strlen(tmp));
			if (!(*str))
			{
				fclose(fd);
				return RAYLOC_FAILED;
			}

			if (RAYLOC_TRUE == firstRun)
			{
				strncpy(*str, tmp, strlen(tmp));
				firstRun = RAYLOC_FALSE;
			}
			else
				strcat(*str, tmp);
			curLen += strlen(tmp);
			memset(tmp, '\0', 9999);
		}

		fclose(fd);
		return curLen;

	}


int
	rayloc_msg_to_rayloc_input(GLOBAL_LOC_STRUCT *p_loc,
	                          RAYLOC_PHASES *unused_list,
	                          RAYLOC_PROC_FLAGS *flags,
	                          const char *rayloc_in_fname,
	                          RAYLOC_STATIONS *stns)
	{
		FILE *fd;
		double epoch_time;
		int retVal;

		fd = fopen(rayloc_in_fname, "w");
		if (!fd)
			return RAYLOC_FAILED_TO_CREATE_INPUT;

                /* date string is now a double epoch seconds (withers 2005126)
		retVal = epochsec17( &epoch_time, p_loc->origin_time);
		if (0 != retVal)
		{
			fclose(fd);
			return RAYLOC_FAILED_TIME_CONVERTION;
		} */
                epoch_time = p_loc->tOrigin;

		fprintf(fd, "%14.3f ", epoch_time);
		fprintf(fd, "%+8.4f ", p_loc->lat);
		fprintf(fd, "%+9.4f ", p_loc->lon);
		fprintf(fd, "%6.2f ", p_loc->depth);
		if (flags)
			rayloc_print_flags(fd, flags);
		else
		{
			fclose(fd);
			return RAYLOC_FAILED_TO_GET_FLAGS;
		}

		retVal = rayloc_print_phases(fd, unused_list, p_loc, stns);
		fclose(fd);

		if (retVal != RAYLOC_SUCCESS)
		return retVal;

		return RAYLOC_SUCCESS;
	}

void
	rayloc_print_flags(FILE *fd, RAYLOC_PROC_FLAGS *flags)
	{
		if (RAYLOC_TRUE == flags->hold_params)
			fprintf(fd, "T ");
		else
			fprintf(fd, "F ");

		if (RAYLOC_TRUE == flags->fix_depth)
			fprintf(fd, "T ");
		else
			fprintf(fd, "F ");

		if (RAYLOC_TRUE == flags->use_PKP)
			fprintf(fd, "T ");
		else
			fprintf(fd, "F ");

		if (RAYLOC_TRUE == flags->use_depth_ph)
			fprintf(fd, "T ");
		else
			fprintf(fd, "F ");

		if (RAYLOC_TRUE == flags->use_S_ph)
			fprintf(fd, "T ");
		else
			fprintf(fd, "F ");
			
		if (RAYLOC_TRUE == flags->pick_weight_interval)
			fprintf(fd, "T ");
		else
			fprintf(fd, "F ");
		fprintf(fd, "%+4.2f %+4.2f\n", flags->Rmin, flags->Rmax);

		/* Second line */
		if (RAYLOC_TRUE == flags->D1)
			fprintf(fd, "T ");
		else
			fprintf(fd, "F ");
		fprintf(fd, "%5.1f %5.1f ", flags->Dmin1, flags->Dmax1);
		
		if (RAYLOC_TRUE == flags->D2)
			fprintf(fd, "T ");
		else
			fprintf(fd, "F ");
		fprintf(fd, "%5.1f %5.1f ", flags->Dmin2, flags->Dmax2);

			if (RAYLOC_TRUE == flags->D3)
			fprintf(fd, "T ");
		else
			fprintf(fd, "F ");
		fprintf(fd, "%5.1f %5.1f ", flags->Dmin3, flags->Dmax3);

		if (RAYLOC_TRUE == flags->D4)
			fprintf(fd, "T ");
		else
			fprintf(fd, "F ");
		fprintf(fd, "%5.1f %5.1f ", flags->Dmin4, flags->Dmax4);

		if (RAYLOC_TRUE == flags->D5)
			fprintf(fd, "T ");
		else
			fprintf(fd, "F ");
		fprintf(fd, "%5.1f %5.1f\n", flags->Dmin5, flags->Dmax5);

		return;
}

int
	rayloc_print_phases(FILE *fd,
	             RAYLOC_PHASES *unused_list,
	             GLOBAL_LOC_STRUCT *p_loc,
	             RAYLOC_STATIONS *stns)
	{
		int            i;
		int            retVal;
		int            num_pha = 0;

		for (i = 0; i < p_loc->nphs; i++)
		{
			retVal = rayloc_print_single_phase(fd, unused_list, &(p_loc->phases[i]), stns);
			if (RAYLOC_SUCCESS == retVal)
				num_pha++;
		}
		if (0 == num_pha)
			return RAYLOC_FAILED;
		return RAYLOC_SUCCESS;
	}

int
	rayloc_print_single_phase(FILE *fd,
	           RAYLOC_PHASES *unused_list,
	           GLOBAL_PHSLINE_STRUCT *phase,
	           RAYLOC_STATIONS *stns)
	{
		int            retVal;
		double         epoch_time;
		double         lat;
		double         lon;
		double         elev;

		retVal = rayloc_if_in_the_station_list(stns, phase->network, phase->station);
		if (retVal < 0)
			return REAYLOC_FAILED_TO_FIND_STATION;

		lat = rayloc_get_station_lat(stns, retVal);
		lon = rayloc_get_station_lon(stns, retVal);
		elev = rayloc_get_station_elev(stns, retVal);
		
                /* date string is now a double epoch seconds (withers 20051206)
		retVal = epochsec17( &epoch_time, phase->pick_time);
		if (0 != retVal)
		{
			fclose(fd);
			return RAYLOC_FAILED_TIME_CONVERTION;
		} */
                epoch_time = phase->tPhase;
		
		fprintf(fd, "%10d ", (int)phase->sequence);
		fprintf(fd, "%5s ", phase->station);
		fprintf(fd, "%3s ", phase->channel);
		fprintf(fd, "%2s ", phase->network);
		if ('?' != phase->location[0])
			fprintf(fd, "%2s ", phase->location);
		else
			fprintf(fd, "   ");
		fprintf(fd, "%+8.4f ", lat);
		fprintf(fd, "%+9.4f ", lon);
		fprintf(fd, "%+4.2f ", elev);
		fprintf(fd, "%4.2f ", phase->quality);
		
		fprintf(fd, "%8s ", phase->phase_name);
		fprintf(fd, "%14.3f ", epoch_time);
		if (RAYLOC_TRUE == rayloc_if_in_the_phase_list(unused_list, phase->phase_name))
			fprintf(fd, "F\n");
		else
			fprintf(fd, "T\n");

		return RAYLOC_SUCCESS;
	}


