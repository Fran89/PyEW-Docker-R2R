#include <stdlib.h>
#include <stdio.h>
#include <direct.h>

// nt_comp.bat
// compiles a single subdirectory

int main(int argc, char ** argv)
{
  int iRetCode;
	char szNMake[50];
	char szCWD[250];

  if(iRetCode = _chdir(argv[1]))
	{
	  fprintf(stderr, "Error changing directory to %s\n",argv[1]);
		goto abort;
	}

  printf("Making %s\n",argv[1]);

  sprintf(szNMake, "nmake -f makefile.nt");

	
  if(iRetCode = system(szNMake))
	{
	  fprintf(stderr, "Error executing ((%s))\n",szNMake);
		goto abort;
	}

  if(iRetCode = _chdir(".."))
	{
	  fprintf(stderr, "Error changing directory to %s\n","..");
		goto abort;
	}


	fprintf(stderr, "Successfully completed(nt_comp %s) make in directory: %s\n",argv[1],
          _getcwd(szCWD,250));
  return(iRetCode);

  abort:
	
	fprintf(stderr, "Error(nt_comp %s) during make in directory: %s\n",argv[1], 
          _getcwd(szCWD,250));
  return(iRetCode);
	
}