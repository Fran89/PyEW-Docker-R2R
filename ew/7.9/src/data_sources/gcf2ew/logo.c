#include <stdio.h>
#include  "earthworm.h"
#include  "externs.h" 

static unsigned char InstId = 255;

void
setuplogo(MSG_LOGO *logo) {
   /* only get the InstId once */
   if (InstId == 255  && GetLocalInst(&InstId) != 0) {
      fprintf( stderr,
              "%s: Invalid Installation code; exiting!\n", Progname);
      exit(-1);
   }
   logo->mod = GModuleId;
   logo->instid = InstId;
}
