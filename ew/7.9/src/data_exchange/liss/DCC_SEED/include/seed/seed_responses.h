/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_responses.h 23 2000-03-05 21:49:40Z lombard $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2000/03/05 21:49:40  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/03/05 21:48:09  lombard
 *     Initial revision
 *
 *
 *
 */

typedef struct _zeros_poles {
	char		*Citation_Name;
	char		*Title;

	char		Function_Type;
	UNIT		*Response_In;
	UNIT		*Response_Out;

	VOLFLT		AO_Normalization;
	VOLFLT		Norm_Frequency;

	int		Complex_Zeros;
	VOLFLT		*Zero_Real_List;
	VOLFLT		*Zero_Imag_List;
	VOLFLT		*Zero_Real_Errors;
	VOLFLT		*Zero_Imag_Errors;
	char**          Zero_Comment;

	int		Complex_Poles;
	VOLFLT		*Pole_Real_List;
	VOLFLT		*Pole_Imag_List;
	VOLFLT		*Pole_Real_Errors;
	VOLFLT		*Pole_Imag_Errors;
	char**          Pole_Comment;

	int           Use_Count;
	char*           Desc_Type;
	char*           Description;
	struct _zeros_poles *Next;
} ZEROS_POLES;

typedef struct _coefficients {
	char		*Citation_Name;
	char		*Title;

	char		Function_Type;
	UNIT		*Response_In;
	UNIT		*Response_Out;

	int		Numerators;
	VOLFLT		*Numer_Coeff;
	VOLFLT		*Numer_Error;
	char**          Numer_Comment;

	int		Denominators;
	VOLFLT		*Denom_Coeff;
	VOLFLT		*Denom_Error;
	char**          Denom_Comment;

	int           Use_Count;
	char*           Desc_Type;
	char*           Description;
	struct _coefficients *Next;
} COEFFICIENTS;

#ifdef BAROUQUE_RESPONSES

typedef struct _resp_generic {
	char		*Citation_Name;
	char		*Title;

	char		Function_Type;
	UNIT		*Response_In;
	UNIT		*Response_Out;

	int		Corners;
	VOLFLT		*Corner_Freq;
	VOLFLT		*Corner_Slope;

	int           Use_Count;
	char*           Desc_Type;
	char*           Description;
	struct _resp_generic *Next;
} RESP_GENERIC;

typedef struct _resp_list {
	char		*Citation_Name;
	char		*Title;

	UNIT		*Response_In;
	UNIT		*Response_Out;

	int		Number_Responses;
	VOLFLT		*Frequency;
	VOLFLT		*Amplitude;
	VOLFLT		*Amplitude_Error;
	VOLFLT		*Phase_Angle;
	VOLFLT		*Phase_Error;

	int           Use_Count;
	char*           Desc_Type;
	char*           Description;
	struct _resp_list *Next;
} RESP_LIST;

#endif

typedef struct _decimation {
	char		*Citation_Name;
	char		*Title;

	VOLFLT		Input_Rate;	/* Input sample rate (sps) */

	int		Factor;		/* Decimation Factor */
	int		Offset;		/* Decimation Offset */

	VOLFLT		Delay;		/* Estimated Delay (secs) */
	VOLFLT		Correction;	/* Correction Applied (secs) */
	int           Use_Count;
	char*           Desc_Type;
	char*           Description;
	struct _decimation *Next;
} DECIMATION;

typedef struct _sensitivity {
	VOLFLT		Sensitivity;	/* Accumulated average sens */
	VOLFLT		Frequency;	/* Frequency of sens */

	char		Num_Cals;	/* Number of calibrations */

	VOLFLT		*Cal_Value;	/* Value of cal */
	VOLFLT		*Cal_Freq;	/* Frequency of cal */
	STDTIME		*Cal_Time;	/* Time of above cal */
	char*           Desc_Type;
	char*           Description;
} SENSITIVITY;

typedef struct _response_ptr {
        int   Stage;
	union	{
		ZEROS_POLES	*PZ;
		COEFFICIENTS	*CO;
#ifdef BAROUQUE_RESPONSES
		RESP_LIST	*RL;
		RESP_GENERIC	*GR;
#endif
		DECIMATION	*DM;
		SENSITIVITY	*SENS;
	} ptr;
	char		type;

	char*           Desc_Type;
	char*           Description;
        struct _response_ptr	*Next;
} RESPONSE;

#define	 RESP_ZEROS_POLES 'P'
#define	 RESP_COEFFICIENTS 'C'
#define  RESP_SENSITIVITY 'S'
#define	 RESP_DECIMATION 'D'
#ifdef BAROUQUE_RESPONSES
#define  RESP_RESP_LIST 'L'
#define  RESP_GENERIC 'G'
#endif

