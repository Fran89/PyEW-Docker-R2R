// module.cpp
#include <windows.h>
#include "module.h"
#include <stdio.h>
#include <Debug.h>
//#include <string.h>
extern "C" {
#include "utility.h"
}

//---------------------------------------------------------------------------------------Module
CModule::CModule() {
	// strcat(sName, "Unknown");  // DK CHANGE 060303 sName is uninitialized, 
                                //  use strncpy() instead of strcat
	strncpy(sName, "Unknown",sizeof(this->sName));
	sName[sizeof(sName)-1] = 0x00;
	pNexus = NULL;
	pMessage = NULL;
}

//---------------------------------------------------------------------------------------~Module
CModule::~CModule() {
	Debug.Log(DEBUG_FUNCTION_TRACE, "CModule::~CModule entered\n");
	Debug.Log(DEBUG_FUNCTION_TRACE, "CModule::~CModule completed\n");
}

//---------------------------------------------------------------------------------------Initialize
bool CModule::Initialize(INexus *nexus, IMessage *msg) {
	pNexus = nexus;
	pMessage = msg;
	return true;
}


//---------------------------------------------------------------------------------------setName
void CModule::setName(char *name) {
	strncpy(sName, name, sizeof(sName)-1);
	sName[sizeof(sName)-1] = 0;
}

//---------------------------------------------------------------------------------------getName
char *CModule::getName() {
	return sName;
}

//---------------------------------------------------------------------------------------Poll
// Permits polling of module. Polling will occur whenever message queue is empty.
// It is terminated for any module whenever that module returns false, which will
// happen immediately if the application module does not override. It is intended
// that this capability support those applications where a long data set it being
// scanned, but the application designer still wishes to permit user interaction.
bool CModule::Poll() {
	Debug.Log(DEBUG_FUNCTION_TRACE,"CModule::Poll(%s) entered\n",this->sName);
	Debug.Log(DEBUG_FUNCTION_TRACE,"CModule::Poll(%s completed\n",this->sName);
	return false;
}

//---------------------------------------------------------------------------------------Action
bool CModule::Action(IMessage *msg) {
	Debug.Log(DEBUG_FUNCTION_TRACE,"CModule::Action(%s) entered\n",this->sName);
	Debug.Log(DEBUG_FUNCTION_TRACE,"CModule::Action(%s completed\n",this->sName);
	return false;
}


//---------------------------------------------------------------------------------------Release
void CModule::Release() {
	Debug.Log(DEBUG_FUNCTION_TRACE,"CModule::Release entered\n");
	char *str = new char[256];
	sprintf (str, "Module <%s> released.\r\n", sName);
	Debug.Log(DEBUG_FUNCTION_TRACE, str);
	pNexus->Dump(str);		// Log string
	delete str;
	Debug.Log(DEBUG_FUNCTION_TRACE, "CModule::Release completed\n");
	delete this;
}

//---------------------------------------------------------------------------------------Dispatch
// Dispatch message through originiating nexus
bool CModule::Dispatch(IMessage *msg) 
{
  bool bOut=false;

	Debug.Log(DEBUG_FUNCTION_TRACE,"CModule::Dispatch(%s) entered\n",this->sName);

	pNexus->Dump('D', sName, msg);

	bOut = pNexus->Dispatch(msg);

	Debug.Log(DEBUG_FUNCTION_TRACE,"CModule::Dispatch(%s) completed\n",this->sName);

	return(bOut);
}
//---------------------------------------------------------------------------------------Broadcast
// Broadcast message through originating nexus
bool CModule::Broadcast(IMessage *msg) 
{
  bool bOut=false;

	Debug.Log(DEBUG_FUNCTION_TRACE,"CModule::Broadcast(%s) entered\n",this->sName);

	pNexus->Dump('B', sName, msg);

	bOut = pNexus->Broadcast(msg);

	Debug.Log(DEBUG_FUNCTION_TRACE,"CModule::Broadcast(%s) completed\n",this->sName);

	return(bOut);
}

//---------------------------------------------------------------------------------------CreateMessage
IMessage *CModule::CreateMessage(char *code) 
{
  IMessage * pOutMessage=NULL;

	Debug.Log(DEBUG_FUNCTION_TRACE,"CModule::CreateMessage(%s) entered\n",this->sName);
	if(pMessage)
    pOutMessage = pMessage->CreateMessage(code);
	Debug.Log(DEBUG_FUNCTION_TRACE,"CModule::CreateMessage(%s) completed\n",this->sName);
	return(pOutMessage);
}


