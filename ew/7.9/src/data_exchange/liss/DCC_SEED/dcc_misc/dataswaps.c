/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: dataswaps.c 1248 2003-06-16 22:08:11Z patton $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.2  2003/06/16 22:07:35  patton
 *     Fixed Microsoft WORD typedef issue
 *
 *     Revision 1.1  2000/03/13 23:45:14  lombard
 *     Initial revision
 *
 *
 *
 */

/* POSIX-Phase 1 29-May-92/SH */

#include <dcc_std.h>

_SUB DCC_LONG TwidLONG(DCC_LONG inlong)		/* Twiddle a long word from one to another */
{
  union {
    struct { DCC_BYTE orgbytes[4]; } byte_e;
    struct { DCC_LONG orglong;	   } long_e;
  } cvtlong;

  DCC_BYTE swp;

  cvtlong.long_e.orglong = inlong;

  swp = cvtlong.byte_e.orgbytes[0];
  cvtlong.byte_e.orgbytes[0] = cvtlong.byte_e.orgbytes[3];
  cvtlong.byte_e.orgbytes[3] = swp;
  swp = cvtlong.byte_e.orgbytes[1];
  cvtlong.byte_e.orgbytes[1] = cvtlong.byte_e.orgbytes[2];
  cvtlong.byte_e.orgbytes[2] = swp;

  return(cvtlong.long_e.orglong);
}

_SUB DCC_WORD TwidWORD(DCC_WORD inword)		/* Twiddle a word around */
{
  union {
    struct { DCC_BYTE orgbytes[2]; } byte_e;
    struct { DCC_WORD orgword;	   } word_e;
  } cvtword;

  DCC_BYTE swp;

  cvtword.word_e.orgword = inword;

  swp = cvtword.byte_e.orgbytes[0];
  cvtword.byte_e.orgbytes[0] = cvtword.byte_e.orgbytes[1];
  cvtword.byte_e.orgbytes[1] = swp;

  return(cvtword.word_e.orgword);
}

_SUB DCC_LONG LocGVAX_LONG(DCC_LONG inlong)	/* Local gets vax long */
{

#ifdef LITTLE_ENDIAN
  return(inlong);
#else
  return(TwidLONG(inlong));
#endif
}


_SUB DCC_LONG LocGM68_LONG(DCC_LONG inlong)	/* Local gets M68000 long */
{

#ifdef LITTLE_ENDIAN
  return(TwidLONG(inlong));
#else
  return(inlong);
#endif
}

_SUB DCC_WORD LocGVAX_WORD(DCC_WORD inword)	/* Local gets vax DCC_WORD */
{

#ifdef LITTLE_ENDIAN
  return(inword);
#else
  return(TwidWORD(inword));
#endif
}


_SUB DCC_WORD LocGM68_WORD(DCC_WORD inword)	/* Local gets M68000 DCC_WORD */
{

#ifdef LITTLE_ENDIAN
  return(TwidWORD(inword));
#else
  return(inword);
#endif
}

_SUB DCC_LONG VAXGLoc_LONG(DCC_LONG inlong)	/* Vax gets local long */
{

#ifdef LITTLE_ENDIAN
  return(inlong);
#else
  return(TwidLONG(inlong));
#endif
}


_SUB DCC_LONG M68GLoc_LONG(DCC_LONG inlong)	/* 68000 gets local long */
{

#ifdef LITTLE_ENDIAN
  return(TwidLONG(inlong));
#else
  return(inlong);
#endif
}

_SUB DCC_WORD VAXGLoc_WORD(DCC_WORD inword)	/* vax gets local word */
{

#ifdef LITTLE_ENDIAN
  return(inword);
#else
  return(TwidWORD(inword));
#endif
}

_SUB DCC_WORD M68GLoc_WORD(DCC_WORD inword)	/* 68000 gets local word */
{

#ifdef LITTLE_ENDIAN
  return(TwidWORD(inword));
#else
  return(inword);
#endif
}
