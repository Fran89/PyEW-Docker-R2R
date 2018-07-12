/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: steimlib.h 2192 2006-05-25 15:32:13Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:39:43  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:56:01  mark
 *     Initial checkin
 *
 *     Revision 1.1  2000/03/05 21:49:14  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/03/05 21:48:43  lombard
 *     Initial revision
 *
 *
 *
 */


  /*
   *    File:
   *            steimlib.h  -   header file for library of steim compression
   *                            functions "steimlib.c"
   */


#ifdef __STDC__
  LONG endianflip (LONG e) ;
#else
  LONG endianflip () ;
#endif

#ifdef __STDC__
  SHORT swapb (SHORT e) ;
#else
  SHORT swapb () ;
#endif


#ifdef __STDC__
  SHORT compress_frame (ccptype ccp,  SHORT difference,  SHORT level,  SHORT reserve,  SHORT flip) ;
#else
  SHORT compress_frame () ;
#endif


#ifdef __STDC__
  SHORT decompress_frame (cfp framep,  unpacktype unpacked,  sptype final,  SHORT lastframesize,  SHORT level,
                          SHORT recursion,  SHORT flip,  SHORT ignore, sptype statcode) ;
#else
  SHORT decompress_frame () ;
#endif


#ifdef __STDC__
  BOOLEAN dferrorfatal (SHORT statcode, FILE *print) ;
#else
  BOOLEAN dferrorfatal () ;
#endif


#ifdef __STDC__
  void fix (ccptype ccp) ;
#else
  void fix () ;
#endif


#ifdef __STDC__
  void clear_compression (ccptype ccp,  SHORT level) ;
#else
  void clear_compression () ;
#endif


#ifdef __STDC__
  ccptype init_compression (SHORT level) ;
#else
  ccptype init_compression () ;
#endif


#ifdef __STDC__
  LONG final_sample (ccptype ccp) ;
#else
  LONG final_sample () ;
#endif



#ifdef __STDC__
  SHORT peek_threshold_avail (ccptype ccp) ;
#else
  SHORT peek_threshold_avail () ;
#endif


#ifdef __STDC__
  SHORT peek_contents (ccptype ccp) ;
#else
  SHORT peek_contents () ;
#endif


#ifdef __STDC__
  LONG frames (ccptype ccp) ;
#else
  LONG frames () ;
#endif


#ifdef __STDC__
  SHORT blocks_padded (ccptype ccp) ;
#else
  SHORT blocks_padded () ;
#endif


#ifdef __STDC__
  SHORT peek_write (ccptype ccp,  LONG samples[], SHORT numwrite) ;
#else
  SHORT peek_write () ;
#endif

#ifdef __STDC__
  void insert_constant (cfp framep,  LONG sample,  SHORT level,  SHORT flip,  SHORT blockindex, BOOLEAN force) ;
#else
  void insert_constant () ;
#endif


#ifdef __STDC__
  adptype init_adaptivity (SHORT diff, SHORT fpt, SHORT fpp, SHORT level, SHORT flip) ;
#else
  adptype init_adaptivity () ;
#endif



#ifdef __STDC__
  SHORT compress_adaptively (adptype adp) ;
#else
  SHORT compress_adaptively () ;
#endif


#ifdef __STDC__
  void clear_generic_decompression (dcptype dcp) ;
#else
  void clear_generic_decompression () ;
#endif


#ifdef __STDC__
  dcptype init_generic_decompression (void) ;
#else
  dcptype init_generic_decompression () ;
#endif


#ifdef __STDC__
  LONG decompress_generic_record (generic_data_record *gdr, LONG udata[], sptype statcode, dcptype dcp,
                                  SHORT firstframe, LONG headertotal, SHORT level, SHORT flip, SHORT dataframes) ;
#else
  LONG decompress_generic_record () ;
#endif


#ifdef __STDC__
  SHORT compress_generic_record (gdptype gdp,  SHORT firstframe) ;
#else
  SHORT compress_generic_record () ;
#endif


#ifdef __STDC__
  LONG generic_record_samples (gdptype gdp) ;
#else
  LONG generic_record_samples () ;
#endif



#ifdef __STDC__
  void clear_generic_compression (gdptype gdp, SHORT framesreserved) ;
#else
  void clear_generic_compression () ;
#endif


#ifdef __STDC__
  gdptype init_generic_compression (SHORT diff, SHORT fpt, SHORT fpp, SHORT level, SHORT flip, generic_data_record *gdr) ;
#else
  gdptype init_generic_compression () ;
#endif
