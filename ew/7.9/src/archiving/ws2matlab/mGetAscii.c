/********************************************************************
file: mGetAscii.c - Matlab Mex function application

Matlab Mex function application

Matlab command line arguments:
	return_array = mGetAscii([structIn1 structIn2... etc], fill (optional), 
	               timeout  (optional), silent (optional))

where a structIn is in the form:
ss.ipport		IP address and port number in the form "XXX.XXX.XXX.XXX:YYYYYY"
ss.sta			Station ID
ss.chn			Channel ID
ss.net			Instrument type
ss.st			Start time as a date vector ([year month day hour minute second])
ss.et			Start time as a date vector ([year month day hour minute second])
ss.data			Optional when called, since the routine will fill this value in

Fill is an optional . If it is not specified, the value NaN will be used
Timeout is the timeout value in milliseconds. If this is not specified, the timeout is 5000 (5 seconds)
Silent is also optional. If it has any value at all (besides []), no error messages will be sent to MATLAB

Returns an empty structure if the call was bad. If the "silent" argument was not specified,
the function also returns a visible error message and an internal MATLAB string message identifier
in the form:

"earthworm:waveserver_error"
"earthworm:SCN_not_found"
"earthworm:network_problem"
"earthworm:timeout"
"earthworm:out_of_memory"
"earthworm:other_problem"

A good call returns an array of output structures:

structOut struct array with fields:
ipport			IP address and port number in the form "XXX.XXX.XXX.XXX:YYYYYY"
sta				Station ID (untouched)
chn				Channel ID (untouched)
net				Instrument Type (untouched)
st				Start time as a date vector ([year month day hour minute second])
				Modified to actual start time for the data
et				Start time as a date vector ([year month day hour minute second])
				Modified to actual end time for the data
data			Returned data as an array of doubles

********************************************************************/

#include <ws_clientII.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mex.h>

#define DEFAULT_TIMEOUT 5000
#define DEFAULT_SAMPLERATE 100.0
#define DEFAULT_FILL 16777217

/*--------------------------------------------------------------
 function: sCallGetAscii - Main C subroutine
           this function calls wsGetTraceBin in ws_clientii.c
 INPUTS:
   nlhs - number of left hand side arguments (outputs)
   plhs[] - pointer to table where created matrix pointers are
            to be placed
   nrhs - number of right hand side arguments (inputs)
   prhs[] - pointer to table of input matrices
--------------------------------------------------------------*/

int nCallGetAscii(char* sIP, char* sPort, char* sMSTA, char* sNCHA, char* sNTYP, double dST, double dET, double dFill, int nSilent, int nTimeout, WS_MENU_QUEUE_REC* menulistRet, TRACE_REQ* tracereqRet)
{
	int nRet;
	char *sTraceBuffer;
	double dCalcSize, diff;

	/*PLK Used for testing*/
	char sValue[10];
	char sTestBuf[100];

/*mexWarnMsgTxt("5.1 Inside nCallGetAscii");*/
/*mexWarnMsgTxt("5.2 Calling wsAppendMenu");*/

	/*Call the routine in ws_clientii.c to get a menu object (the call to wsGetTraceAscii needs one)*/
	nRet = wsAppendMenu(sIP, sPort, menulistRet, nTimeout);

/*mexWarnMsgTxt("5.2 Back from wsAppendMenu");*/

	/*Make sure everything went fine*/
	if (nRet != WS_ERR_NONE) {
		return nRet;
	}

/*mexWarnMsgTxt("5.3 Calculating buffer size");*/

	/*Calculate the size of the buffer
	(dET - dST) is the number of seconds requested
	Multiplying this by the DEFAULT_SAMPLERATE give the number of samples
	Then assuming 8 bytes per ASCII number gives the required size
	Even allowing for the largest 2^24 bit seven-character data values*/
	/*dCalcSize = ceil((dET - dST) * DEFAULT_SAMPLERATE * 8);*/

	dCalcSize = 4096.0; 
	diff = dET - dST;
	dCalcSize = diff * DEFAULT_SAMPLERATE * 8.0;

/*mexWarnMsgTxt("5.4 Creating buffer");

	/*Allocate enough space for the return data*/
	sTraceBuffer = mxCalloc((size_t) dCalcSize, sizeof(char));

/*mexWarnMsgTxt("5.5 Set data for call");*/
	
	/*Set the data for the tracereq*/
	strcpy(tracereqRet->sta, sMSTA);
	strcpy(tracereqRet->chan, sNCHA);
	strcpy(tracereqRet->net, sNTYP);
	tracereqRet->reqStarttime = dST;
	tracereqRet->reqEndtime = dET;
	tracereqRet->pBuf = sTraceBuffer;
	tracereqRet->bufLen = (long) dCalcSize;
	tracereqRet->timeout = nTimeout;
	tracereqRet->fill = (long)dFill;

/*mexWarnMsgTxt("5.6 Call to wsGetTraceAscii");*/

	/*Call the function from ws_clientii.c*/
	nRet = wsGetTraceAscii(tracereqRet, menulistRet, nTimeout);

/*mexWarnMsgTxt("5.7 Back from wsGetTraceAscii");*/

	/*Everything went fine*/
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
void mexFunction( int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[] )
{
	/*Local variables*/
	int nRet;
	int i;
	int j;
	char sBuffer[100];
	char sRet[10];
	int nNumStruct;
	int nFields;
	double dFill;
	double *dFillIn;
	int nTimeout;
	double *dTimeIn;
	int nSilent;
	mxArray *mxaTempInput;
	mxArray *mxaIPPort;
	mxArray *mxaMSTA;
	mxArray *mxaNCHA;
	mxArray *mxaNTYP;
	mxArray *mxaST;
	mxArray *mxaET;
	mxArray *mxaData;
	mxArray *mxaSampleRate;
	int nBufLenIPPort;
	int nBufLenMSTA;
	int nBufLenNCHA;
	int nBufLenNTYP;
	char *sIPPort;
	char *sMSTA;
	char *sNCHA;
	char *sNTYP;
	char *sTempChar;
	char *sIP;
	char *sPort;
	double dST;
	double dET;
	double dSTActual;
	double dETActual;
	static TRACE_REQ tracereqRet;
	static WS_MENU_QUEUE_REC menulistRet;
	mxArray *mxaFieldValue;
	int nNumData;
	double dTempDataValue;
	char sSpace[] = " ";
	char *sFullData;
	char *sTempDataString;
	int nFieldCount = 8;
	const char *sFieldNames[] = {"ipport","sta","chn","net","st","et","data","samprate"};
	double *dReturnVal;
	double dTempStartTime;
	double dTempEndTime;
	double dTempSampleRate;
	double dNumSamples;
	double dTempTime;

	/*PLK Used for testing*/
	char sValue[10];
	char sTestBuf[100];

	/*Set the output struct to null if an error occurs*/
	plhs[0] = mxCreateStructMatrix(0, 0, nFieldCount, sFieldNames);

	/*Check for proper number of arguments*/
	if ((nrhs < 1) || (nrhs > 4 ))  {
		mexErrMsgIdAndTxt("earthworm:other_problems","Usage structMenu = mGetMenu([struct1 struct2... etc], fill (optional), timeout (optional), silent (optional)");
		return;
	}
    if (nlhs > 1) {
		mexErrMsgIdAndTxt("earthworm:other_problems","Too many output arguments; 1 expected");
		return;
	}

	/*The first input argument must be a cell array of input structures*/
	if (mxIsStruct(prhs[0]) != true) {
		mexErrMsgIdAndTxt("earthworm:other_problems","First argument must be an array of input structures in the form: ipport / sta / chn / net / st / et");
		return;
	}
	if (mxGetM(prhs[0]) != true) {
		mexErrMsgIdAndTxt("earthworm:other_problems","First argument must have at least one input structure in the form: ipport / sta / chn / net / st / et");
		return;
	}
	/*Make sure a correct structure is passed in*/
	/*09/24/02 Removed for ease of use*/
/*	nFields = mxGetNumberOfFields(prhs[0]);
	if ((nFields < 6) || (nFields > 8)) {
		mexErrMsgIdAndTxt("earthworm:other_problems","First argument must be a 1 by n array of input structures in the form: ipport / sta / chn / net / st / et / data (optional) / samprate (optional)");
		return;
	}
*/
	nNumStruct = mxGetNumberOfElements(prhs[0]);

	/*The second argument (fill value) is optional*/
	if ((nrhs < 2) || (mxIsEmpty(prhs[1]))) {
		/*Set the fill value to a huge value not in the input space (e.g. greater than 2^24)*/
		dFill = DEFAULT_FILL;
	} else if (mxIsNumeric(prhs[1]) != true) {
		mexErrMsgIdAndTxt("earthworm:other_problems","Second optional argument (fill value) must be a valid number");
		return;
	} else {
		dFillIn = mxGetPr(prhs[1]);
		dFill = *dFillIn;
	}

	/*The third argument (timeout milliseconds) is optional*/
	if ((nrhs < 3) || (mxIsEmpty(prhs[2]))) {
		/*Set the time out to five seconds*/
		nTimeout = DEFAULT_TIMEOUT;
	} else if (mxIsNumeric(prhs[2]) != true) {
		mexErrMsgIdAndTxt("earthworm:other_problems","Third optional argument (timeout milliseconds) must be a valid number");
		return;
	} else {
		dTimeIn = mxGetPr(prhs[2]);
		nTimeout = (int) *dTimeIn;
	}

	/*The fourth argument (silent) is also optional*/
	if ((nrhs < 4) || (mxIsEmpty(prhs[3]))) {
		/*Set silent to false*/
		nSilent = false;
	} else {
		nSilent = !(mxIsEmpty(prhs[3]));
	}

	/*Copy over the input array, since we want to change it, and prhs[0] is read-only*/
	mxaTempInput = mxDuplicateArray(prhs[0]);

	/*Create an array of structures to fill*/
	plhs[0] = mxCreateStructMatrix(1, nNumStruct, nFieldCount, sFieldNames);

	/*Loop through all the input structures*/
	for (i = 0; i < nNumStruct; i++) {

		/*Kill the growing menulistRet since we want a fresh one every time*/
		wsKillMenu(&menulistRet);
		/*I think I need to do this to fix a bug discovered in ws_clientII.c*/
		menulistRet.head = menulistRet.tail = NULL;

		/*Copy over each field in turn*/
		mxaIPPort = mxGetField(mxaTempInput, i, "ipport");
		mxaMSTA = mxGetField(mxaTempInput, i, "sta");
		mxaNCHA = mxGetField(mxaTempInput, i, "chn");
		mxaNTYP = mxGetField(mxaTempInput, i, "net");
		mxaST = mxGetField(mxaTempInput, i, "st");
		mxaET = mxGetField(mxaTempInput, i, "et");
		mxaData = mxGetField(mxaTempInput, i, "data");
		mxaSampleRate = mxGetField(mxaTempInput, i, "samprate");

		/*Make sure the ipport, msta, ncha, and ntyp are all non-empty character strings*/
		if (mxIsChar(mxaIPPort) != true) {
			if (!nSilent) {
				mexErrMsgIdAndTxt("earthworm:other_problems","The IP address and port number must be a string in the form IP:PORT");
			}
			return;
		}
		if (mxGetM(mxaIPPort) != true) {
			if (!nSilent) {
				mexErrMsgIdAndTxt("earthworm:other_problems","The IP address and port number must be a a single non-empty string in the form IP:PORT");
			}
			return;
		}
		if (mxIsChar(mxaMSTA) != true) {
			if (!nSilent) {
				mexErrMsgIdAndTxt("earthworm:other_problems","The sta must be a string");
			}
			return;
		}
		if (mxGetM(mxaMSTA) != true) {
			if (!nSilent) {
				mexErrMsgIdAndTxt("earthworm:other_problems","The sta must be a a single non-empty string");
			}
			return;
		}
		if (mxIsChar(mxaNCHA) != true) {
			if (!nSilent) {
				mexErrMsgIdAndTxt("earthworm:other_problems","The chn must be a string");
			}
			return;
		}
		if (mxGetM(mxaNCHA) != true) {
			if (!nSilent) {
				mexErrMsgIdAndTxt("earthworm:other_problems","The chn must be a a single non-empty string");
			}
			return;
		}
		if (mxIsChar(mxaNTYP) != true) {
			if (!nSilent) {
				mexErrMsgIdAndTxt("earthworm:other_problems","The net must be a string");
			}
			return;
		}
		if (mxGetM(mxaNTYP) != true) {
			if (!nSilent) {
				mexErrMsgIdAndTxt("earthworm:other_problems","The net must be a a single non-empty string");
			}
			return;
		}

/*mexWarnMsgTxt("1. Parsing strings");*/

		/*Get the length of the input strings*/
		nBufLenIPPort = mxGetN(mxaIPPort) + 1;
		nBufLenMSTA = mxGetN(mxaMSTA) + 1;
		nBufLenNCHA = mxGetN(mxaNCHA) + 1;
		nBufLenNTYP = mxGetN(mxaNTYP) + 1;
		/*Allocate memory for strings and  copy them over*/
		sIPPort = mxCalloc(nBufLenIPPort, sizeof(char));
		nRet =  mxGetString(mxaIPPort, sIPPort, nBufLenIPPort);
		if (nRet != 0) {
			if (!nSilent) {
				mexWarnMsgTxt("Not enough space for IP address and port number. String is truncated");
			}
		}
		sMSTA = mxCalloc(nBufLenMSTA, sizeof(char));
		nRet =  mxGetString(mxaMSTA, sMSTA, nBufLenMSTA);
		if (nRet != 0) {
			if (!nSilent) {
				mexWarnMsgTxt("Not enough space for sta. String is truncated");
			}
		}
		sNCHA = mxCalloc(nBufLenNCHA, sizeof(char));
		nRet =  mxGetString(mxaNCHA, sNCHA, nBufLenNCHA);
		if (nRet != 0) {
			if (!nSilent) {
				mexWarnMsgTxt("Not enough space for chn. String is truncated");
			}
		}
		sNTYP = mxCalloc(nBufLenNTYP, sizeof(char));
		nRet =  mxGetString(mxaNTYP, sNTYP, nBufLenNTYP);
		if (nRet != 0) {
			if (!nSilent) {
				mexWarnMsgTxt("Not enough space for net. String is truncated");
			}
		}

/*mexWarnMsgTxt("2. Parsing IP:Port");*/

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
		sIP = sIPPort;
		sTempChar = sIPPort;
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

/*mexWarnMsgTxt("3. Finish variable setup");*/

		/*Get the start and end times in the serial date format (number of days since 1/1/0000)*/
		/*Make sure they aren't empty*/
		if (mxIsNumeric(mxaST) != true) {
			if (!nSilent) {
				mexErrMsgIdAndTxt("earthworm:other_problems","The start time must be the date in serial format");
			}
			return;
		}
		if (mxIsNumeric(mxaET) != true) {
			if (!nSilent) {
				mexErrMsgIdAndTxt("earthworm:other_problems","The end time must be a date in serial format");
			}
			return;
		}
		dST = mxGetScalar(mxaST);
		dET = mxGetScalar(mxaET);

		/*PLK: 09/03/2002 TODO Verify that the strings are valid*/
		/*This would avoid a time-wating call to nCallGetAscii which waits for a timeout*/

/*mexWarnMsgTxt("4. SocketSysInit");*/

		/*Initialize the socket system; this is a no-op for Unix*/
		SocketSysInit();

		/*Convert the start and end times from the number of fractional days since 0/0/0000 at midnight
		to the number of SECONDS since 1/1/1970 at midnight
		For example, 1/1/1970 is 719529 days before 0/0/0000*/
		dST = dST - 719529;
		dET = dET - 719529;
		/*Convert days to seconds by multiplying by 60 * 60 * 24 = 86400*/
		dST = dST * 86400;
		dET = dET * 86400;


		/*Make sure the start time is *before* the end time*/
		if (dST > dET) {
			if (!nSilent) {
				mexErrMsgIdAndTxt("earthworm:other_problems","The start time must be before the end time");
			}
			return;
		}


/*mexWarnMsgTxt("5. Call to nCallGetAscii");*/

		/*Call the C subroutine*/
		nRet = nCallGetAscii(sIP, sPort, sMSTA, sNCHA, sNTYP, dST, dET, dFill, nSilent, nTimeout, &menulistRet, &tracereqRet);

/*mexWarnMsgTxt("6. Error handling");*/


		
		/*PLK catch all the most popular errors (-4, -13)*/
		/*Check the returned value*/
		if (nRet == WS_ERR_NO_CONNECTION) {
			if (!nSilent) {
				strcpy(sBuffer,"ERROR: No connection could be established from server ");
				strcat(sBuffer,sIP);
				strcat(sBuffer,":");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:network_problems",sBuffer);
			}
		} else if (nRet == WS_ERR_SOCKET) {
			if (!nSilent) {
				strcpy(sBuffer,"ERROR: Could not create socket to server ");
				strcat(sBuffer,sIP);
				strcat(sBuffer,":");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"\n");
			}
				mexWarnMsgIdAndTxt("earthworm:network_problems",sBuffer);
		} else if (nRet == WS_ERR_BROKEN_CONNECTION) {
			if (!nSilent) {
				strcpy(sBuffer,"ERROR: Connection broken to server ");
				strcat(sBuffer,sIP);
				strcat(sBuffer,":");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:network_problems",sBuffer);
			}
		} else if (nRet == WS_ERR_TIMEOUT) {
			if (!nSilent) {
				strcpy(sBuffer,"ERROR: Timeout to server ");
				strcat(sBuffer,sIP);
				strcat(sBuffer,":");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:timeout",sBuffer);
			}
		} else if (nRet == WS_ERR_MEMORY) {
			if (!nSilent) {
				strcpy(sBuffer,"ERROR: Out of memory for function mGetMenu");
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:out_of_memory",sBuffer);
			}
		} else if (nRet == WS_ERR_BUFFER_OVERFLOW) {
			if (!nSilent) {
				strcpy(sBuffer,"ERROR: Buffer overflow for function mGetMenu");
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:out_of_memory",sBuffer);
			}
		} else if (nRet == WS_ERR_INPUT) {
			if (!nSilent) {
				strcpy(sBuffer,"ERROR: bad input parameters of server ->");
				strcat(sBuffer,sIP);
				strcat(sBuffer,"<- and port ->");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"<-");
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:other_problems",sBuffer);
			}
		} else if (nRet == WS_ERR_EMPTY_MENU) {
			if (!nSilent) {
				strcpy(sBuffer,"ERROR: menu could not be found for server ->");
				strcat(sBuffer,sIP);
				strcat(sBuffer,"<- and port ->");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"<-");
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:other_problems",sBuffer);
			}
		} else if (nRet == WS_ERR_SERVER_NOT_IN_MENU) {
			if (!nSilent) {
				strcpy(sBuffer,"ERROR: server ->");
				strcat(sBuffer,sIP);
				strcat(sBuffer,"<- and port ->");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"<- Could not be found in menu");
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:other_problems",sBuffer);
			}
		} else if (nRet == WS_ERR_SCN_NOT_IN_MENU) {
			if (!nSilent) {
				strcpy(sBuffer,"ERROR: SCN of ->");
				strcat(sBuffer,sMSTA);
				strcat(sBuffer,"/");
				strcat(sBuffer,sNCHA);
				strcat(sBuffer,"/");
				strcat(sBuffer,sNTYP);
				strcat(sBuffer,"<- could not be found for server ->");
				strcat(sBuffer,sIP);
				strcat(sBuffer,"<- and port ->");
				strcat(sBuffer,sPort);
				strcat(sBuffer,"\n");
				mexWarnMsgIdAndTxt("earthworm:other_problems",sBuffer);
			}
		} else if (nRet != WS_ERR_NONE) {
			if (!nSilent) {
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
		} else if (nRet == WS_ERR_NONE) {
		/*The return value was fine*/

/*mexWarnMsgTxt("7. Copy input structure to output");*/

			/*Copy over the input structure to the output*/
			mxSetField(plhs[0],i,"ipport",mxCreateString(sIPPort));
			mxSetField(plhs[0],i,"sta",mxCreateString(sMSTA));
			mxSetField(plhs[0],i,"chn",mxCreateString(sNCHA));
			mxSetField(plhs[0],i,"net",mxCreateString(sNTYP));

/*mexWarnMsgTxt("8. Sample rate");*/

			/*If the sample rate field is empty, create a new one*/
			if (mxaSampleRate == NULL) {
				/*Create a place for the sample rate*/
				if (mxAddField(plhs[0],"samprate") == -1 ) {
					mexErrMsgIdAndTxt("earthworm:other_problems","Could not add sample rate field to return structure");
					return;
				}
				mxaSampleRate = mxGetField(plhs[0], i, "samprate");
			}

			/*Copy the data from tracereqRet.samprate to the new mxaSampleRate*/
			mxaFieldValue = mxCreateDoubleMatrix(1,1,mxREAL);
			dReturnVal = mxGetPr(mxaFieldValue);
			*dReturnVal = tracereqRet.samprate;
			mxSetField(plhs[0],i,"samprate",mxaFieldValue);
			/*Save the sample rate to use later*/
			dTempSampleRate = tracereqRet.samprate;


/*mexWarnMsgTxt("9. Start and end times");*/

			/*Copy the new start and time times*/
			dSTActual = tracereqRet.actStarttime;
			dETActual = tracereqRet.actEndtime;


/*mexWarnMsgTxt("11. Find number of data");*/

			/*Loop through the data string and see how many fields were found*/
			nNumData = 0;
			sTempChar = tracereqRet.pBuf;
			/*Go through the entire string*/
			while (*sTempChar != '\n') {
				/*Loop through characters until we find a space*/
				while ((*sTempChar != ' ') && (*sTempChar != '\n')) {
					sTempChar++;
				}
				nNumData++;
				/*Now, loop through the whitespace until we find another character*/
				while ((*sTempChar == ' ') && (*sTempChar != '\n')) {
					sTempChar++;
				}
			}


			/*Convert the start time from number of seconds since 1/1/1970
			to the number of days since 0/0/0000
			(see mGetMenu.c for more explanation)*/
			if (dSTActual != 0) {
				dTempStartTime = (dSTActual / 86400) + 719529;
			}

			/*Calculate the actual end time by finding the number of samples*/
			dNumSamples = nNumData;
			if ((dETActual == 0) && (dTempSampleRate != 0)) {
				/*Find the number of seconds and add it to the value returned from the waveserver*/
				dTempTime = dNumSamples / dTempSampleRate;
				dTempEndTime = dSTActual + dTempTime;
				/*Convert the end time from number of seconds since 1/1/1970
				to the number of days since 0/0/0000*/
				dTempEndTime = (dTempEndTime / 86400) + 719529;
			}
			/*Copy them to the new fields*/
			mxaFieldValue = mxCreateDoubleMatrix(1,1,mxREAL);
			dReturnVal = mxGetPr(mxaFieldValue);
			*dReturnVal = dTempStartTime;
			mxSetField(plhs[0],i,"st",mxaFieldValue);
			mxaFieldValue = mxCreateDoubleMatrix(1,1,mxREAL);
			dReturnVal = mxGetPr(mxaFieldValue);
			*dReturnVal = dTempEndTime;
			mxSetField(plhs[0],i,"et",mxaFieldValue);

/*mexWarnMsgTxt("10. Data initialization");*/


			/*If the data field is empty, create a new one*/
			if (mxaData == NULL) {
				/*Create a place for the data*/
				if (mxAddField(plhs[0],"data") == -1 ) {
					mexErrMsgIdAndTxt("earthworm:other_problems","Could not add data field to return structure");
					return;
				}
				mxaData = mxGetField(plhs[0], i, "data");
			}


/*mexWarnMsgTxt("12. Allocate data space");*/

			/*Allocate enough room for the data*/
			/*PLK 9/24/02 Not working, I think I have this backwards*/
			/*dTempArray = mxCalloc((size_t) nNumData, sizeof(double));*/

			mxaFieldValue = mxCreateDoubleMatrix(1,nNumData,mxREAL);
			dReturnVal = mxGetPr(mxaFieldValue);
			/*Loop again, this time converting the data into doubles and appending them to the array*/
			sFullData = tracereqRet.pBuf;
			sTempDataString = strtok(sFullData,sSpace);
			j = 0;

/*mexWarnMsgTxt("13. Fill data array");*/

			while ((sTempDataString != NULL) && (j < nNumData)) {
				/*Convert the token to a double*/
				dTempDataValue = atof(sTempDataString);
				/*If this is the default fill value, replace it with "NaN*/
				if (dTempDataValue == DEFAULT_FILL) {
					/*Set it to "Nan"*/
					dReturnVal[j] = (0*(dTempDataValue/0));
				} else {
					dReturnVal[j] = dTempDataValue;
				}
				/*Find the next token*/
				j++;
				sTempDataString = strtok(NULL,sSpace);
			}

/*mexWarnMsgTxt("14. Copy data structure to output");*/

			mxSetField(plhs[0],i,"data",mxaFieldValue);
		}

		/*Free the dynamically allocated memory just to be safe*/
		/*PLK 09/24/02 I think this is causing an error*/
		/*
		mxFree(sIPPort);
		mxFree(sMSTA);
		mxFree(sNCHA);
		mxFree(sNTYP);
		*/
	}

	/*Everything went fine*/
	return;

} /*end mGetAscii()*/

