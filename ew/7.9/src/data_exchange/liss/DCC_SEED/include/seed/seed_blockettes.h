/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_blockettes.h 23 2000-03-05 21:49:40Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/05 21:49:40  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/03/05 21:48:09  lombard
 *     Initial revision
 *
 *
 *
 */

typedef struct _blockette {
	char		*BlkBuff;	/* Pointer to the current blockette */
	int		BufLen;		/* Max Length of the buffer */
	int		BlkType;	/* The blockette id number */
	int		BlkVer;		/* The version of the blockette */
	char		BlkRec;		/* Record type of blockette */
	int		PackPos;	/* Number of bytes this blockette */
	int		ParsePos;	/* Where is the parser now? */
} BLOCKETTE;

extern	int	DefBlkLen;		/* Default blockette length */
extern	int	DefBlkExt;		/* Blockette Extend Increment */

typedef struct _logical_record {
	char		*RecBuff;	/* Pointer to current record */
	int		RecNum;		/* Record number of current record */
	int		NextPos;	/* Next position to write to */
	int		RecLen;		/* logical rec len power of 2 exp */
	int		RecSize;	/* The size of the record */
	BOOL		Continue;	/* Is this a continuation record */
	char		RecType;	/* What is the control header type */
	BOOL		(*WriteService)(); /* Subroutine to dump buffers */
	BOOL		(*ReadService)(); /* Routine to read next */
	int		BlockExp;	/* Exponent for block buffer */
	int		BlockMax;	/* The number of bytes of buffer */
	int		BlockSize;	/* Size of last read */
	int		BlockPos;	/* Current position */
	char		*BlockBuffer;	/* Pointer to buffer */
} LOGICAL_RECORD;
