/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: seed_statchan.h 2192 2006-05-25 15:32:13Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2006/05/25 15:32:13  paulf
 *     first checkin from Hydra
 *
 *     Revision 1.1  2005/06/30 20:39:32  mark
 *     Initial checkin
 *
 *     Revision 1.1  2005/04/21 16:55:45  mark
 *     Initial checkin
 *
 *     Revision 1.1  2000/03/05 21:49:40  lombard
 *     Initial revision
 *
 *     Revision 1.1  2000/03/05 21:48:09  lombard
 *     Initial revision
 *
 *
 *
 */

typedef struct _channel_entry {

	ABBREV		*Instrument;
	char		*Optional;
	UNIT		*Signal_Response;
	UNIT		*Calibration_Input;

	int             Coord_Set;

	VOLFLT		Latitude;
	int             Lat_Prec;
	VOLFLT		Longitude;
        int             Long_Prec;
	ABBREV          *Coord_Type;
        char            *Coord_Map;
	VOLFLT		Elevation;
	int             Elev_Prec;
	ABBREV          *Elev_Type;
	char            *Elev_Map;

	VOLFLT		Local_Depth;
	VOLFLT		Azimuth;
	VOLFLT		Dip;

	FORMAT		*Format_Type;
	char		Data_Exp;
	VOLFLT		Sample_Rate;
	VOLFLT		Max_Drift;
	
	ABBREV          *Clock_Type;

	int		Channel_Flags;
#define CFG_TRIGGERED 	0x00000001	/* Data is triggered */
#define CFG_CONTINUOUS	0x00000002	/* Data recorded continuously */
#define CFG_HEALTHCHAN	0x00000004	/* State of health channel */	
#define CFG_GEODATA	0x00000008	/* Geophysical data */
#define CFG_ENVIRON	0x00000010	/* Weather/Environmental data */
#define CFG_FLAGS	0x00000020	/* Flags/Switch information */
#define CFG_SYNTH	0x00000040	/* Data is synthesized */
#define CFG_CALIN	0x00000080	/* Channel is a cal input signal */
#define CFG_EXPERIMENT	0x00000100	/* Channel is experimental */
#define CFG_MAINTENANCE	0x00000200	/* Maintenance underway on channel */
#define CFG_BEAM	0x00000400	/* Data is a beam synthesis */

	char            *Derived_Location;   
	char            *Derived_Channel;

        char            *Calibration_Location;
	char            *Calibration_Channel;

	ABBREV          *Digitizer_Type;

	char            *Desc_Type;
	char            *Description;

} CHANNEL_ENTRY;

typedef struct	_channel_times {
	STDTIME		Effective_Start;
	STDTIME		Effective_End;
	STDTIME         Last_Modified;
	char		Update_Flag;
	
	CHANNEL_ENTRY	*Channel;
	RESPONSE	*Root_Response,
	                *Tail_Response;

	struct _channel_times	*Next;
} CHANNEL_TIMES;

typedef	struct	_channel_list {

	char		*Location;
	char		*Identifier;
	int		SubChannel;

	CHANNEL_TIMES	*Root_Time,
	                *Tail_Time;

	COMMENT_ENTRY	*Root_Comment,
	                *Tail_Comment;

	VOID		*Root_Data,
	                *Tail_Data;

	int		Use_Count;

	struct _channel_list *Next;

} CHANNEL_LIST;

typedef struct _station_entry {

	char		*Site_Name;
	ABBREV		*Station_Owner;

	VOLFLT		Latitude;
	int             Lat_Prec;
	VOLFLT		Longitude;
        int             Long_Prec;
	ABBREV          *Coord_Type;
        char            *Coord_Map;
	VOLFLT		Elevation;
	int             Elev_Prec;
	ABBREV          *Elev_Type;
	char            *Elev_Map;

	STDTIME		Established;
	int		Long_Order;
	int		Word_Order;

	ABBREV          *Clock_Type;
	ABBREV          *Station_Type;

	char            *Successor_Net;
        char            *Successor_Station;
	
        char            *Software_Revs;
        char            *Vault_Cond;

	char            *Desc_Type;
	char            *Description;

} STATION_ENTRY;

typedef struct _station_times {

	STDTIME		Effective_Start;
	STDTIME		Effective_End;
	STDTIME         Last_Modified;
	char		Update_Flag;

	STATION_ENTRY	*Station;

	VOID		*Root_Data,
			*Tail_Data;		/* Link to user database */

	struct _station_times *Next;

} STATION_TIMES;

typedef struct _station_list {

	char		*Network;		/* New SEED 2 letter code */
	char		*Station;

	STATION_TIMES	*Root_Time,
	                *Tail_Time;

	COMMENT_ENTRY	*Root_Comment,
                	*Tail_Comment;

	CHANNEL_LIST	*Root_Channel,
	                *Tail_Channel;

	int		RecNum;		/* Where was this written */

	int		Use_Count;

	struct _station_list *Next;

} STATION_LIST;


/*
   STATION_LIST(Root_Station,Tail_Station)
       ->COMMENT_ENTRY(Root_Comment,Tail_Comment)
       ->STATION_TIMES(Root_Time,Tail_Time)
           ->STATION_ENTRY(Station)
	   ->User_Data(Root_Data,Tail_Data)
	       ->CHANNEL_LIST(Root_Channel,Tail_Channel)
	           ->COMMENT_ENTRY(Root_Comment,Tail_Comment)
	           ->User_Data(Root_Data,Tail_Data);
	           ->CHANNEL_TIMES(Root_Times,Tail_Times)
	               ->CHANNEL_ENTRY(Channel)
		       ->RESPONSE(Root_Response,Tail_Response)
		           ->ZEROS_POLES(PZ)
		           ->COEFFICIENTS(CO)
		           ->DECIMATION(DM)
		           ->SENSITIVITY(SENS)

*/
