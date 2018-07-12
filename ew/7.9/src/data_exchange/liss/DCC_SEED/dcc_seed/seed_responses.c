/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_responses.c 44 2000-03-13 23:49:34Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/13 23:47:51  lombard
 *     Initial revision
 *
 *
 *
 */

#include <dcc_std.h>
#include <string.h>
#include <dcc_seed.h>
#include <dcc_misc.h>

/********************Deal with Response Structures**********************/

/*
 *	
 *	Init and Zero a zeros and poles response
 *	
 */

_SUB ZEROS_POLES	*InitZerosPoles(int numzeros,int numpoles)
{

  ZEROS_POLES	*pz;
  int i;

  pz = (ZEROS_POLES *) SafeAlloc(sizeof (ZEROS_POLES));

  pz->Citation_Name = NULL;
  pz->Title = "";
  pz->Function_Type = ' ';
  pz->Response_In = NULL;
  pz->Response_Out = NULL;
  pz->AO_Normalization = 0;
  pz->Desc_Type = NULL;
  pz->Description = NULL;

  pz->Complex_Zeros = numzeros;
  if (numzeros>0) {
    pz->Zero_Real_List = (VOLFLT *) 
      SafeAlloc(sizeof(VOLFLT) * numzeros);
    pz->Zero_Imag_List = (VOLFLT *) 
      SafeAlloc(sizeof(VOLFLT) * numzeros);
    pz->Zero_Real_Errors = (VOLFLT *) 
      SafeAlloc(sizeof(VOLFLT) * numzeros);
    pz->Zero_Imag_Errors = (VOLFLT *) 
      SafeAlloc(sizeof(VOLFLT) * numzeros);
    pz->Zero_Comment = (char **)
      SafeAlloc(sizeof(char *) * numzeros);
  }

  for (i=0; i<numzeros; i++) {
    pz->Zero_Real_List[i] = 0.0;
    pz->Zero_Imag_List[i] = 0.0;
    pz->Zero_Real_Errors[i] = 0.0;
    pz->Zero_Imag_Errors[i] = 0.0;
    pz->Zero_Comment[i] = NULL;
  }

  pz->Complex_Poles = numpoles;
  if (numpoles>0) {
    pz->Pole_Real_List = (VOLFLT *) 
      SafeAlloc(sizeof(VOLFLT) * numpoles);
    pz->Pole_Imag_List = (VOLFLT *) 
      SafeAlloc(sizeof(VOLFLT) * numpoles);
    pz->Pole_Real_Errors = (VOLFLT *) 
      SafeAlloc(sizeof(VOLFLT) * numpoles);
    pz->Pole_Imag_Errors = (VOLFLT *) 
      SafeAlloc(sizeof(VOLFLT) * numpoles);
    pz->Pole_Comment = (char **) 
      SafeAlloc(sizeof(char *) * numpoles);
  }

  for (i=0; i<numpoles; i++) {
    pz->Pole_Real_List[i] = 0.0;
    pz->Pole_Imag_List[i] = 0.0;
    pz->Pole_Real_Errors[i] = 0.0;
    pz->Pole_Imag_Errors[i] = 0.0;
    pz->Pole_Comment[i] = NULL;
  }

  return(pz);
}

/*
 *	
 *	Init and coefficients response
 *	
 */

_SUB COEFFICIENTS	*InitCoefficients(int numnum,int numden)
{

	COEFFICIENTS	*co;
	int i;

	co = (COEFFICIENTS *) SafeAlloc(sizeof (COEFFICIENTS));

	co->Citation_Name = NULL;
	co->Title = "";
	co->Function_Type = ' ';
	/*co->Stage = 0;*/
	co->Response_In = NULL;
	co->Response_Out = NULL;
	co->Desc_Type = NULL;
	co->Description = NULL;

	co->Numerators = numnum;
	if (numnum>0) {
		co->Numer_Coeff = (VOLFLT *) 
			SafeAlloc(sizeof(VOLFLT) * numnum);
		co->Numer_Error = (VOLFLT *) 
			SafeAlloc(sizeof(VOLFLT) * numnum);
		co->Numer_Comment = (char **) 
			SafeAlloc(sizeof(char *) * numnum);
	}

	for (i=0; i<numnum; i++) {
		co->Numer_Coeff[i] = 0.0;
		co->Numer_Error[i] = 0.0;
		co->Numer_Comment[i] = NULL;
	}

	co->Denominators = numden;
	if (numden>0) {
		co->Denom_Coeff = (VOLFLT *) 
			SafeAlloc(sizeof(VOLFLT) * numden);
		co->Denom_Error = (VOLFLT *) 
			SafeAlloc(sizeof(VOLFLT) * numden);
		co->Denom_Comment = (char **) 
			SafeAlloc(sizeof(char *) * numden);
	}

	for (i=0; i<numden; i++) {
		co->Denom_Coeff[i] = 0.0;
		co->Denom_Error[i] = 0.0;
		co->Denom_Comment[i] = NULL;
	}

	return(co);
}

/*
 *
 *	Init and zero a decimation record
 *
 */

_SUB DECIMATION	*InitDecimation()
{

	DECIMATION *deci;

	deci = (DECIMATION *) SafeAlloc(sizeof (DECIMATION));

	deci->Citation_Name = NULL;
	deci->Title = "";
	/*deci->Stage = 0;*/
	deci->Input_Rate = 0.0;
	deci->Factor = 0;
	deci->Offset = 0;
	deci->Delay = 0.0;
	deci->Correction = 0.0;

	deci->Desc_Type = NULL;
	deci->Description = NULL;

	return(deci);

}

/*
 *	
 *	Init and zero a response sensitivity 
 *	
 */

_SUB SENSITIVITY	*InitSensitivity(int numcals)
{

	SENSITIVITY *sens;
	int i;

	sens = (SENSITIVITY *) SafeAlloc(sizeof (SENSITIVITY));

	/*sens->Stage = 0;*/
	sens->Sensitivity = 0.0;
	sens->Frequency = 0.0;
	sens->Num_Cals = numcals;
	sens->Desc_Type = NULL;
	sens->Description = NULL;

	if (numcals>0) {
		sens->Cal_Value = (VOLFLT *) 
			SafeAlloc(sizeof(VOLFLT) * numcals);
		sens->Cal_Freq = (VOLFLT *) 
			SafeAlloc(sizeof(VOLFLT) * numcals);
		sens->Cal_Time = (STDTIME *) 
			SafeAlloc(sizeof(STDTIME) * numcals);
	}

	for (i=0; i<numcals; i++) {
		sens->Cal_Value[i] = 0.0;
		sens->Cal_Freq[i] = 0.0;
		sens->Cal_Time[i] = ST_Zero();
	}

	return(sens);
}

/* 
 *
 *      Delete any matching responses
 *
 */

_SUB void DeleteResponses(CHANNEL_TIMES *inchan, RESPONSE *inresp)
{

  int invalue;
  RESPONSE *resploop,*respprev,*respnext;

  invalue = ResponseValue(inresp);

  for (respprev=NULL,
       resploop=inchan->Root_Response;
       resploop!=NULL;
       respprev=resploop,
       resploop=respnext) {

    respnext = resploop->Next;

    if (invalue!=ResponseValue(resploop)) continue;

    if (resploop->Next==NULL) {
      inchan->Tail_Response = respprev;
      if (respprev==NULL)
	inchan->Root_Response = NULL;
    } else
      if (respprev==NULL) 
	inchan->Root_Response = resploop->Next;
      else 
	respprev->Next = resploop->Next;

    SafeFree((void *) resploop);
  }
}

/* 
 *
 *  Insert response into list at proper point
 *
 */

_SUB void InsertResponse(CHANNEL_TIMES *inchan, RESPONSE *inresp)
{

  int invalue;
  RESPONSE *resploop,*respprev;

  invalue = ResponseValue(inresp);

  for (respprev=NULL,
       resploop=inchan->Root_Response;
       resploop!=NULL;
       respprev=resploop,
       resploop=resploop->Next) {

    if (invalue<ResponseValue(resploop)) break;
  }

  if (resploop==NULL) {
    if (inchan->Tail_Response)
      inchan->Tail_Response->Next = inresp;
    else
      inchan->Root_Response = inresp;
    inchan->Tail_Response = inresp;
  } else
    if (respprev==NULL) {
      inresp->Next = inchan->Root_Response;
      inchan->Root_Response = inresp;
    } else {
      respprev->Next = inresp;
      inresp->Next = resploop;
    }
}

  


 
/*
 *	
 *	Add the response pointer mechanism to the channel chain
 *	
 */

_SUB RESPONSE *AddResponse(CHANNEL_TIMES *inchannel,char intype,
			   void *inresp, int stage)
{

  RESPONSE *rp;

  /* alloc a response and initialize it */
  
  rp = (RESPONSE *) SafeAlloc(sizeof (RESPONSE));

  rp->type = intype;
  rp->Stage = stage;

  /* Set values in union */

  switch (intype) {

  case RESP_ZEROS_POLES:
    rp->ptr.PZ = (ZEROS_POLES *) inresp;
    break;
  case RESP_COEFFICIENTS:
    rp->ptr.CO = (COEFFICIENTS *) inresp;
    break;
  case RESP_DECIMATION:
    rp->ptr.DM = (DECIMATION *) inresp;
    break;
#ifdef BAROUQUE_RESPONSES
  case RESP_RESP_LIST:
    rp->ptr.RL = (RESP_LIST *) inresp;
    break;
  case RESP_GENERIC:
    rp->ptr.GR = (RESP_GENERIC *) inresp;
    break;
#endif
  case RESP_SENSITIVITY:
    rp->ptr.SENS = (SENSITIVITY *) inresp;
    break;
  default:
    bombout(EXIT_ABORT,"Unknown response type %c\n",
	    intype);
  }

  rp->Next = NULL;

  /* Delete all entries of like type and replace */

  DeleteResponses(inchannel, rp);

  InsertResponse(inchannel, rp);

  return(rp);

}

/*
 *	
 *	Link a zeros and poles response to end of the chain
 *	
 */

_SUB void LinkInZerosPoles(CHANNEL_TIMES *inchannel,ZEROS_POLES *inzero, 
			   int stage)
{

	RESPONSE *rp;

	rp = AddResponse(inchannel,RESP_ZEROS_POLES, (void *) inzero, stage);

}

/*
 *	
 *	Link a coefficients response to end of the chain
 *	
 */

_SUB void LinkInCoefficients(CHANNEL_TIMES *inchannel,COEFFICIENTS *incoef,
			     int stage)
{

	RESPONSE *rp;

	rp = AddResponse(inchannel,RESP_COEFFICIENTS,(void *) incoef,stage);

}

/*
 *	
 *	Add a decimation response to the end of the chain
 *	
 */

_SUB void LinkInDecimation(CHANNEL_TIMES *inchannel,DECIMATION *indeci,
			   int stage)
{

	RESPONSE *rp;

	rp = AddResponse(inchannel,RESP_DECIMATION, (void *) indeci, stage);

}

/*
 *	
 *	Add a sensitivity response to the end of the chain
 *	
 */

_SUB void LinkInSensitivity(CHANNEL_TIMES *inchannel,SENSITIVITY *insens,
			    int stage)
{

	RESPONSE *rp;

	rp = AddResponse(inchannel,RESP_SENSITIVITY, (void *) insens, stage);

}

/*
 *	
 *	DupResponse - copy a response and return a pointer to it
 *      *note* that response itself is not duplicated, just the pointer
 *      copied (this will save lots of memory - but may be undesirable)
 *	
 */

_SUB RESPONSE *DupResponse(RESPONSE *inresp)
{
  
  RESPONSE *rp;

  rp = (RESPONSE *) SafeAlloc (sizeof (RESPONSE));

  *rp = *inresp;

  rp->Next = NULL;

  return(rp);

}

/*
 *
 *  CopyResponses - Copy a chain of responses and return the linked list
 *   - only copies the pointer list (saves lots of memory)
 *
 */

_SUB VOID CopyResponses(RESPONSE *instart,
			RESPONSE **root, RESPONSE **tail)
{
  
  RESPONSE *newres, *loopres;

  /* Fix the users link list headers */

  *root = 
    *tail = NULL;

  /* Loop through the original list - copy elements a put on new list */
  
  for (loopres=instart; loopres!=NULL; loopres=loopres->Next) {

    newres = DupResponse(loopres);

    if (*tail!=NULL)
      (*tail)->Next = newres;
    else *root = newres;
    *tail = newres;

  }

}

/*
 *
 * Calculate a relative value to be used to sort responses 
 *
 */

_SUB int ResponseValue(RESPONSE *inresp)
{

  int stg;

  stg = inresp->Stage;

  if (stg<1 || stg> 80) stg = 100;

  stg *= 100;

  switch (inresp->type) {

  case RESP_ZEROS_POLES:
    stg += 10;
    break;
  case RESP_COEFFICIENTS:
    stg += 20;
    break;
#ifdef BAROUQUE_RESPONSES
  case RESP_RESP_LIST:
    stg += 40;
    break;
  case RESP_GENERIC:
    stg += 50;
    break;
#endif
  case RESP_DECIMATION:
    stg += 80;
    break;
  case RESP_SENSITIVITY:
    stg += 90;
    break;
  default:
    bombout(EXIT_ABORT,"Unknown response type %c\n",
	    inresp->type);
  }

  return(stg);

}

