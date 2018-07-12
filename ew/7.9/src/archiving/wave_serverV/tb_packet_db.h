/***********************************************************
 tb_packet_db.h - definitions and declarations for tb_packet_db.c
 module implementing db storage functionality for synchronizing
 trace_buf data packets. 

dependences: 
 - sqlite v3.3.16 www.sqlite.org : version used for original
 build.

************************************************************/
#ifndef _TB_PACKET_DB_H_
#define _TB_PACKET_DB_H_

#include <sqlite3.h>
#include <trace_buf.h>

/* Windows provides a non-ANSI version of snprintf,
   name _snprintf.  The Windows version has a
   different return value than ANSI snprintf.
   ***********************************************/
#ifdef _WIN32
#define snprintf _snprintf
#endif

/***********************************************************
 definitions and macros
************************************************************/
#define TBDB_ERR				0
#define TBDB_OK					1
#define TB_MAX_DB_NAME			255					/* used to allocate buffer for db file name. */
#define TB_DEFAULT_DBNAME		"TB2PACKETS.SL3DB"	/* default db filename if none given.*/
#define TB_MAX_ERRMSG			512					/* used to allocate buffer for error messages. */
#define TB_MAX_SQL_LEN			1024				/* used to allocate buffer for sql commands. */
#define TB_BLOCKSIZE			MAX_TRACEBUF_SIZ	/* used to allocate buffer when reading multiple packets.*/

/***********************************************************
 TBDB_STORE struct to abstract and encapsulate sqlite3 db
 for trace2_buf packet storage.
*********R**************************************************/
typedef struct tbdb_store {
	char			sDbFile[TB_MAX_DB_NAME];	/* copy of filename of db*/
	sqlite3			*pDb;						/* ptr to sqlite3_db from sqlite3_open*/
	double			Oldest;						/* time of oldest starttime in db*/
	double			Newest;						/* time of most current endtime in db*/
	char			LastErr[TB_MAX_ERRMSG];		/* store last sqlite3 errmsgs here*/
} TBDB_STORE;

/**********************************************************
 TBDB_QUERY = struct to abstract query for multiple
 record reads.
***********************************************************/
typedef struct tbdb_query {
	sqlite3_stmt*	pStmt;					/* sqlite3 statement handle*/
	int				nTBCol;					/* column containing @@TraceBuf Param */
	char			LastErr[TB_MAX_ERRMSG];	/* keep error */
} TBDB_QUERY;

/***********************************************************
 external function prototype definitions. 
************************************************************/
extern int tbdb_open(TBDB_STORE** ppTBDB, char* filename);
extern int tbdb_close(TBDB_STORE** pTBDB);
extern int tbdb_put_packet(TBDB_STORE* pTBDB, TRACE2_HEADER* pTB, const void* pBuf, int BufSiz);
extern int tbdb_purge_range(TBDB_STORE* pTBDB, double from, double to);
extern int tbdb_purge_prior_packets(TBDB_STORE* pTBDB, double retainAfter);
extern int tbdb_purge_all(TBDB_STORE* pTBDB);

extern long tbdb_get_SCNL_packet_count(
	TBDB_STORE* pPDB,
	char* Sta, 
	char* Chan, 
	char* Net,
	char* Loc,
	double from,
	double to
);

extern int tbdb_get_packet_range(TBDB_STORE* pTBDB, double* pMin, double* pMax);

extern int tbdb_open_query(TBDB_STORE* pPDB, char* sSQL, TBDB_QUERY** ppQry);
extern int tbdb_get_packet(TBDB_QUERY* pQry, int nTBCol, TRACE2_HEADER** ppT2Buf, int* pT2BSize);
extern int tbdb_close_query(TBDB_QUERY** ppQry);

extern int tbdb_open_range_query(
	TBDB_STORE*		pTBDB, 
	TBDB_QUERY**	ppQry,
	char*			sStation,
	char*			sNetwork,
	char*			sChannel,
	char*			sLocation,
	double			starttime,
	double			endtime
);
extern int tbdb_has_packets(TBDB_STORE* pTBDB, char* sta, char* net, char* chan, char* loc, double from, double to);
extern int tbdb_get_all_packets(
	TBDB_STORE* pTBDB, 
	char* sta, 
	char* net, 
	char* chan, 
	char* loc, 
	double from, 
	double to, 
	void** ppBuf, 
	long* pBufSize, 
	long* pBufPage, 
	long* pCount
);

#endif	/*_TB_PACKET_DB_H_*/
