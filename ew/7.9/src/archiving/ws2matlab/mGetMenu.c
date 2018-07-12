/********************************************************************
file: mGetMenu.c - Matlab Mex function application

Matlab Mex function application

Matlab command line arguments:
	return_array = mGetMenu({"IPAddress1:PortNumber1" "IPAddress2:PortNumber2" "IPAddress3:PortNumber3"}, timeout, silent)

Where the IP:PORT values are valid strings in a MATLAB cell array, and the timeout is an integer value in seconds
Timeout is the timeout value in milliseconds. If this is not specified, the timeout is 5000 (5 seconds)
Silent is also optional. If it has any value at all (besides []), no error messages will be sent to MATLAB

For example, a good call from MATLAB would be:
structRet = mGetMenu({'136.177.31.187:16023' '136.177.31.187:16025'}) or
structRet = mGetMenu({'136.177.31.187:16023' '136.177.31.187:16025'}, 12000) or
structRet = mGetMenu({'136.177.31.187:16023' '136.177.31.187:16025'}, 12000, 1)

Returns an empty structure if the call was bad. If the "silent" argument was not specified,
the function also returns a visible error message and an internal MATLAB string message identifier
in the form:

"earthworm:network_problem"
"earthworm:timeout"
"earthworm:out_of_memory"
"earthworm:other_problem"

A good call returns:

1xn struct array with fields:
	ipport = IP address and port number in the form "xxx.xxx.xxx.xxx:yyyyyy"
    sta = Station code for the channel
    chn = Component code for the channel
    net = Network code for the channel
    st = Menu start time for the channel in MATLAB datenum format
    et = Menu end time for the channel in MATLAB datenum format

********************************************************************/

#include <ws_clientII.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <mex.h>

#define DEFAULT_TIMEOUT 5000

/*--------------------------------------------------------------
 function: sCallAppendMenu - Main C subroutine
           this function calls wsAppendMenu in ws_clientii.c
 INPUTS:
   nlhs - number of left hand side arguments (outputs)
   plhs[] - pointer to table where created matrix pointers are
            to be placed
   nrhs - number of right hand side arguments (inputs)
   prhs[] - pointer to table of input matrices
--------------------------------------------------------------*/

int nCallAppendMenu(char* sIP, char* sPort, WS_MENU_QUEUE_REC* menulistRet, int nTimeout)
{
	int nRet;

	/*Call the routine in ws_clientii.c*/
	nRet = wsAppendMenu(sIP, sPort, menulistRet, nTimeout);

	/*Return with or without an error code*/
	return nRet;
}

/*--------------------------------------------------------------
 function: mexFunction - Entry point from Matlab environment
 INPUTS:
   nlhs - number of left hand side arguments (outputs)
   plhs[] - pointer to table where created matrix pointers are
            to be placed
   nrhs - number of right hand side arguments (inputs)
   prhs[] - pointer to table of input matrices
--------------------------------------------------------------*/
void mexFunction( int nlhs, mxArray *plhs[], int nrhs, const mxArray  *prhs[] )
{
	/*Local variables*/
	int nRet;
	int nNumWaveServers;
	int nFields;
	double *dTimeIn;
	int nTimeout;
	bool bSilent;
	mxArray *mxaTempInput;
	mxArray *mxaTempString;
	int nIPPortLen;
	char *sIP;
	char *sPort;
	char *sTempChar;
	static WS_MENU_QUEUE_REC menulistRet;
	WS_MENU menuServer = NULL;
	WS_MENU menuStart = NULL;
	WS_PSCN pscnTank;
	char sRet[10];
	char sBuffer[100];
	int nPscnCount;
    int nDims[2];
	int nFieldCount = 6;
    const char *sFieldNames[] = {"ipport","sta","chn","net","st","et"};
	int i;
	double dStarttime;
	double dEndtime;
	mxArray *mxaFieldValue;
	double *dStructItem;

	/*Set the output struct to null if an error occurs*/
	plhs[0] = mxCreateStructMatrix(0,0, nFieldCount, sFieldNames);

	/*Kill the growing menulistRet since we want a fresh one every time*/
	wsKillMenu(&menulistRet);
	/*I think I need to do this to fix a bug discovered in ws_clientII.c*/
	menulistRet.head = menulistRet.tail = NULL;

	/*Check for proper number of arguments*/
	if ((nrhs < 1) || (nrhs > 3 ))  {
		mexErrMsgIdAndTxt("earthworm:other_problems","Usage structMenu = mGetMenu({'server1:port1' 'server2:port2' 'server3:port3'}, timeout, silent");
		return;
	}
    if (nlhs > 1) {
		mexErrMsgIdAndTxt("earthworm:other_problems","Too many output arguments; 1 expected");
		return;
	}

	/*The first input argument must be a cell array of ip:port*/
	if (mxIsCell(prhs[0]) != true) {
		mexErrMsgIdAndTxt("earthworm:other_problems","First argument {'ip1:port1' 'ip2:port2'} must be a cell array");
		return;
	}

	if (mxGetM(prhs[0]) != true) {
		mexErrMsgIdAndTxt("earthworm:other_problems","First argument {'ip1:port1' 'ip2:port2'} must have at least one string in a cell array");
		return;
	}
	/*Find out how many servers are specified*/
	nFields = mxGetNumberOfFields(prhs[0]);
	if (nFields != 0) {
		mexErrMsgIdAndTxt("earthworm:other_problems","First argument {'ip1:port1' 'ip2:port2'} must be a 1 by n cell array");
		return;
	}
	nNumWaveServers = mxGetNumberOfElements(prhs[0]);

	/*The second argument (timeout) is optional*/
	if ((nrhs < 2) || (mxIsEmpty(prhs[1]))) {
		/*Set the time out to five seconds*/
		nTimeout = DEFAULT_TIMEOUT;
	} else if (mxIsNumeric(prhs[1]) != true) {
		mexErrMsgIdAndTxt("earthworm:other_problems","Second optional argument (timeout milliseconds) must be a valid number");
		return;
	} else {
		dTimeIn = mxGetPr(prhs[1]);
		nTimeout = (int) *dTimeIn;
	}

	/*The third argument (silent) is also optional*/
	if ((nrhs < 3) || (mxIsEmpty(prhs[2]))) {
		/*Set silent to false*/
		bSilent = false;
	} else {
		bSilent = !(mxIsEmpty(prhs[2]));
	}

	/*PLK TODO 9/13/02  Check empty field, proper data type, and data type consistency; get classID for each field*/

	/*Copy over the input array, since we want to change it, and prhs[0] is read-only*/
	mxaTempInput = mxDuplicateArray(prhs[0]);

	/*Initialize the socket system; this is a no-op for Unix*/
	SocketSysInit();

	/*Loop through all the waveservers and create the menu*/
	for (i = 0; i < nNumWaveServers; i++) {

		/*Get the IP address*/
		mxaTempString = mxGetFieldByNumber(mxaTempInput,0,i);
		nIPPortLen = mxGetN(mxaTempString) + 1;
		sIP = mxCalloc(nIPPortLen, sizeof(char));
		nRet = mxGetString(mxaTempString, sIP, nIPPortLen);
		if (nRet != 0) {
			if (!bSilent) {
				strcpy(sBuffer,"Not enough space for ipaddress:port as >");
				strcat(sBuffer,sIP);
				strcat(sBuffer,"< for cell array value ");
				sprintf(sRet, "%d", i);
				strcat(sBuffer,sRet);
				strcat(sBuffer,"; String is truncated\n");
				mexWarnMsgIdAndTxt("earthworm:other_problems",sBuffer);
			}
		}

		/*Tricky code: for a string 'myipaddress:myport', I'm letting
		sIP point to the first character in 'myipaddress' (i.e. the 'm')
		and walking until I find a null character. Then, I turn that into '\0'

		So, by that point I have 'myipaddress\0myport'
		sPort then gets the next character

		Then code also works for bad strings
		an input of '\0' will set both sIP and sPort to the first '\0' character
		while an input of 'ipbutnoport' will set the sIP correctly, but leave the sPort as '\0'
		Both bad inputs above will be caught by the nCallAppendMenu routine as bad input*/

		/*Walk through the string until we find a port number*/
		sTempChar = sIP;
		while ((*sTempChar != ':') && (*sTempChar != '\0')) {
			sTempChar++;
		}
		/*See if we found a colon*/
		if (*sTempChar == ':') {
			/*Set it to '\0', and let the port start with the next character*/
			*sTempChar = '\0';
			sTempChar++;
		}
		/*Set the port*/
		sPort = sTempChar;

		/*PLK: 09/03/2002 TODO Verify that the strings are valid*/
		/*This would avoid a time-wating call to nCallAppendMenu which waits for a timeout*/

		/*Call the C subroutine*/
		nRet = nCallAppendMenu(sIP, sPort, &menulistRet, nTimeout);

		/*Check the returned value*/
		if (nRet == WS_ERR_NO_CONNECTION) {
			if (!bSilent) {
				strcpy(sBuffer,"ERROR: No connection could be established from server ");
				strcat(sBuffer,sIP);
				strcat(sBuffer,":");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:network_problems",sBuffer);
			}
		} else if (nRet == WS_ERR_SOCKET) {
			if (!bSilent) {
				strcpy(sBuffer,"ERROR: Could not create socket to server ");
				strcat(sBuffer,sIP);
				strcat(sBuffer,":");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"\n");
			}
				mexWarnMsgIdAndTxt("earthworm:network_problems",sBuffer);
		} else if (nRet == WS_ERR_BROKEN_CONNECTION) {
			if (!bSilent) {
				strcpy(sBuffer,"ERROR: Connection broken to server ");
				strcat(sBuffer,sIP);
				strcat(sBuffer,":");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:network_problems",sBuffer);
			}
		} else if (nRet == WS_ERR_TIMEOUT) {
			if (!bSilent) {
				strcpy(sBuffer,"ERROR: Timeout to server ");
				strcat(sBuffer,sIP);
				strcat(sBuffer,":");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:timeout",sBuffer);
			}
		} else if (nRet == WS_ERR_MEMORY) {
			if (!bSilent) {
				strcpy(sBuffer,"ERROR: Out of memory for function mGetMenu");
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:out_of_memory",sBuffer);
			}
		} else if (nRet == WS_ERR_INPUT) {
			if (!bSilent) {
				strcpy(sBuffer,"ERROR: bad input parameters of server ->");
				strcat(sBuffer,sIP);
				strcat(sBuffer,"<- and port ->");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"<-");
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:other_problems",sBuffer);
			}
		} else if (nRet != WS_ERR_NONE) {
			if (!bSilent) {
				/*Generic error mesage*/
				sprintf(sRet, "%d", nRet);
				strcpy(sBuffer,"ERROR: Failed to get menu from ");
				strcat(sBuffer,sIP);
				strcat(sBuffer,":");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"; wsAppendMenu returned ");
				strcat(sBuffer,sRet);
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:other_problems",sBuffer);
			}
		}
		/*Free the string memory just to be safe*/
		mxFree(sIP);
	}

	/*Build a return structure*/
	menuServer = menulistRet.head;
	menuStart = menuServer;

	/*Make sure that we have at least one menu*/
	if (menuServer == NULL) {
		if (!bSilent) {
			mexWarnMsgIdAndTxt("earthworm:other_problems","WARNING: wsAppendMenu built no menus; exiting\n");
			return;
		}
	}

	/*Find how many were found*/
	nPscnCount = 0;
	while (menuServer != NULL) {
		pscnTank = menuServer->pscn;
		while (pscnTank != NULL) {
			/*Get the next element in the linked list*/
			pscnTank = pscnTank->next;
			nPscnCount++;
		}
		/*Go to the next waveserver*/
		menuServer = menuServer->next;
	}

	/*Set some data*/
	nDims[0] = 1;
	nDims[1] = nPscnCount;

	/*Create a structure from a menu list*/
    plhs[0] = mxCreateStructArray(2, nDims, nFieldCount, sFieldNames);

	/*Set the head back*/
	menuServer = menuStart;
	/*Set a loop variable*/
	i = 0;
	/*Loop through all the servers we found*/
	while (menuServer != NULL) {
		/*Loop through the linked list again and populate the fields*/
		pscnTank = menuServer->pscn;
		while (pscnTank != NULL) {
			strcpy(sBuffer,menuServer->addr);
			strcat(sBuffer,":");
			strcat(sBuffer,menuServer->port);
			mxSetFieldByNumber(plhs[0],i,0,mxCreateString(sBuffer));

			/*Save the integer*/
			mxSetFieldByNumber(plhs[0],i,1,mxCreateString(pscnTank->sta));
			mxSetFieldByNumber(plhs[0],i,2,mxCreateString(pscnTank->chan));
			mxSetFieldByNumber(plhs[0],i,3,mxCreateString(pscnTank->net));

			/*Convert the double from number of seconds since 1/1/1970
			to the number of days since 0/0/0000*/
			dStarttime = pscnTank->tankStarttime;
			dStarttime = dStarttime / 86400;
			dStarttime = dStarttime + 719529;
			mxaFieldValue = mxCreateDoubleMatrix(1,1,mxREAL);
			dStructItem = mxGetPr(mxaFieldValue);
			*dStructItem = dStarttime;
			mxSetFieldByNumber(plhs[0],i,4,mxaFieldValue);

			/*Convert the double from number of seconds since 1/1/1970
			to the number of days since 0/0/0000*/
			dEndtime = pscnTank->tankEndtime;
			dEndtime = dEndtime / 86400;
			dEndtime = dEndtime + 719529;
			mxaFieldValue = mxCreateDoubleMatrix(1,1,mxREAL);
			dStructItem = mxGetPr(mxaFieldValue);
			*dStructItem = dEndtime;
			mxSetFieldByNumber(plhs[0],i,5,mxaFieldValue);

			/*Increment the counter*/
			i++;
			/*Get the next element in the linked list*/
			pscnTank = pscnTank->next;
		}
		/*Go to the next waveserver*/
		menuServer = menuServer->next;
	}
	
	/*Everything went fine*/
	return;

} /*end mGetMenu()*/
