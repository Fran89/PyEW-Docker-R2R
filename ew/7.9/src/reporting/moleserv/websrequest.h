
#define MAX_WAVESERVERS			10


typedef enum
{
	WAVES_OK,
	WAVES_MEM_ALLOC_ERR,
	WAVES_INVAL_PARAM,
	WAVES_INVAL_REQ,
	WAVES_WS_UNAVAIL,
	WAVES_WS_ERR
} WAVE_STATUS;

typedef struct _WAVESERVER
{
	char			wsIP[13];
	char			port[7];
} WAVESERVER;

typedef struct _WSPARAMS
{
	struct _WAVESERVER		waveservers[MAX_WAVESERVERS];
	int				nwaveservers;
	long			wstimeout;
	int				MaxSamples;
} WSPARAMS;





/* Functions */
WAVE_STATUS websrequest( char* request, char* buffer, size_t* nbuffer,
		char* fname, int nfname, WSPARAMS* params );
		

