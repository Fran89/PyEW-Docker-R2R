#include <stdarg.h>


/* a quick hack to replicate what hydra does */
int reportError( int value1, int value2, char *format, ... )
{
   auto va_list ap;
   char flag[3] = "et";

   va_start( ap, format );
         logit_core(flag,format,ap);
   va_end( ap );
   return 1;
}

int  reportErrorInit(int bufsize, int flag, char * config) 
{
	short mid  = 1;
	logit_init(config, mid, bufsize, 0);
   return 1;
}

int         reportErrorCleanup() 
{
   return 1;
}


