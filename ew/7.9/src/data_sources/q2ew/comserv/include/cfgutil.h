/*   Configuration file utility module include file
     Copyright 1994, 1997 Quanterra, Inc.
     Written by Woodrow H. Owens

Edit History:
   Ed Date      By  Changes
   -- --------- --- ---------------------------------------------------
    0 15 Mar 94 WHO Translated from cfgutil.pas
    1 20 Mar 94 WHO Split procedure added.
    2 30 May 94 WHO Missing "void" on str_right added (DSN).
    3  3 Nov 97 WHO Larger line widths. Add c++ conditionals.
*/
/* type definitions needed to use this module */
#ifdef _OSK
#define CFGWIDTH 200
#define SECWIDTH 120
#else
#define CFGWIDTH 512
#define SECWIDTH 256
#endif

  typedef struct
    begin
      char lastread[CFGWIDTH] ;
      FILE *cfgfile ;
    end config_struc ;

#ifdef __cplusplus
extern "C" {
#endif

/* remove trailing spaces, the C version must also remove the end of line */
  void untrail (pchar s) ;
    
/* upshift a string */
  void upshift (pchar s) ;
    
/* skip to the specified section */
  short skipto (config_struc *cs, pchar section) ;

/* add a directory separator slash to the end of a string if there isn't one */
  void cs_addslash (pchar s) ;

/* open the configuration file, return TRUE if the file cannot be opened
   or the section is not found */
  short open_cfg (config_struc *cs, pchar fname, pchar section) ;

/* Start at ptr+1 and copy characters into dest up to and including the
   terminator */
  void str_right (pchar dest, pchar ptr) ;

/* Returns the part to the left of the "=" in s1, upshifted. Returns the
   part to the right of the "=" in s2, not upshifted. Returns with s1
   a null string if no more strings in the section */
  void read_cfg (config_struc *cs, pchar s1, pchar s2) ;

  void close_cfg (config_struc *cs) ;
 
  void split (pchar src, pchar right, char sep) ;

#ifdef __cplusplus
}
#endif
