#include <stdlib.h>
#include <stdio.h>
#include <direct.h>

// nt_folder.bat

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

  sprintf(szNMake, "nmake %s",argv[2]);

	
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


	fprintf(stderr, "Successfully completed(nt_folder) make in directory: %s\n",_getcwd(szCWD,250));
  return(iRetCode);

  abort:
	
	fprintf(stderr, "Error(nt_folder) during make in directory: %s\n",_getcwd(szCWD,250));
  return(iRetCode);
	
}