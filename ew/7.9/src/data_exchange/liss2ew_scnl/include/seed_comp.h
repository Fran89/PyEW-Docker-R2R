/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_comp.h 2192 2006-05-25 15:32:13Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:39:55  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:55:26  mark
 *     Initial checkin
 *
 *     Revision 1.2  2003/06/16 22:04:58  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/05 21:47:33  lombard
 *     Initial revision
 *
 *
 *
 */

#include <seed_data.h>

struct _steim_frame {		
	UDCC_LONG	ctlflags;		/* Control headers */
	union long_element {
		struct { DCC_BYTE byteval[4]; } type_1;
		struct { DCC_WORD wordval[2]; } type_2;
		struct { DCC_LONG longval;    } type_3;
	} chunkstore[15];
};

#define NUMFRAM 64		/* 64*64, 4096 byte records */
struct steim_frames {
	struct _steim_frame framestore[NUMFRAM];
};

struct _sd_context {
	UDCC_BYTE	Network[3];
	UDCC_BYTE	Station[6];		/* Id stuff */
	UDCC_BYTE	Location[3];
	UDCC_BYTE	Channel[4];

	DCC_LONG	ms_sam;			/* Sample size in ms (0 for logs) */
	STDTIME	begtime;		/* Time of start this record */
	STDTIME	marktime;		/* Last time given (for logs) */
	int	samp_pop;		/* Current number samps */

	int	frmidx;			/* Ring buffer index */
	int	frmpop;                 /* Current ring buffer population */
	int	blocksz;		/* How big is the data rec */
	DCC_LONG	frmbuf[4];		/* Ring buffer of deltas */
	DCC_LONG	tstbuf[4];              /* Ring buffer of abs-deltas */
	int	frmwt[4];		/* Frame weight */
	DCC_LONG	sambuf[4];		/* Buffer for samples */
	int	frame,chkidx;		/* Current frame/chunk in record */
	int	sframe;			/* Starting frame number */
	DCC_LONG	movic;			/* Moving constant (rev ic) */
	DCC_LONG	psam;			/* Previous Sample - forward ic */

	UDCC_BYTE	databuf[4096];
	SEED_DATA *outrec;		/* Body of our seed record */
	struct	steim_frames *out_data;	/* Pointer to frames of prev */

	VOID	(*flushoutput)(VOID *datrec);	/* User dump routine */
};

typedef struct _sd_context SD_Context;

#define SD_FORMAT_STEIM 1 	/* Standard 68000 order format */
#define SD_FORMAT_STEIMV 2	/* VAX order format */
#define SD_FORMAT_WORD 3	/* 68000 2 byte format (DWWSSN) */
#define SD_FORMAT_LONG 4	/* 68000 4 byte format */
#define SD_FORMAT_CDSN 5	/* CDSN/RSTN gain ranged fmt */
#define SD_FORMAT_SRO 6		/* SRO gain ranged fmt */	

#include <seed/seed_comp_proto.h>
