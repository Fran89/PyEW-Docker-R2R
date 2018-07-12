/**********************************************************
 tb_packet_db.c - Module to handle archive TRACE2_HEADER data
 utilizing a lightweight db engine. Currently sqlite.

 dependences: 
 - sqlite v3.3.16 www.sqlite.org : version used for original
 build.
***********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include "tb_packet_db.h"


/* sqlite3 compatible sql script to create db schema if it's
not there. Currently only one table representing TRACE2_HEADER header 
fields and the corresponding raw data packet it describes. */
static const char* sqlPacketDbSchema = 
"CREATE TABLE IF NOT EXISTS PACKETS ( \
	Pinno		INT, \
	NumSamp		INT, \
	StartTime	REAL, \
	EndTime		REAL, \
	SampleRate	REAL, \
	Sta			TEXT(7), \
	Net			TEXT(9), \
	Chan		TEXT(9), \
	Loc			TEXT(3), \
	Version		TEXT(2), \
	DataType	TEXT(3), \
	Quality		TEXT(2), \
	Pad			TEXT(2), \
	DataChunk	BLOB \
); \
CREATE VIEW IF NOT EXISTS SNCLSTATS AS \
SELECT Sta,Net,Chan,Loc,Min(StartTime) AS Oldest, Max(EndTime) AS Newest, \
Count(*) AS NumPackets FROM PACKETS GROUP BY Sta,Net,Chan,Loc;";

/*CREATE INDEX IF NOT EXISTS TIME_NDX ON PACKETS(StartTime, EndTime); \
CREATE INDEX IF NOT EXISTS SCNL_NDX ON PACKETS(Sta,Net,Chan,Loc); \
CREATE INDEX IF NOT EXISTS SCNLTIME_NDX ON PACKETS(Sta,Net,Chan,Loc,StartTime,EndTime);";
*/

/**********************************************************
 tbdb_open - opens db used for packet 
 synchronization. If the database does not exist it is 
 created. More importantly this function should
***********************************************************/
int tbdb_open(TBDB_STORE** ppTBDB, char* sDbFileName) {
	
	int rc =0;
	char* pErr;

	/* allocate a TB_PACKET_DB struct*/
	if((*ppTBDB = (TBDB_STORE*) malloc(sizeof(TBDB_STORE))) != NULL) {
		
		memset((void*)*ppTBDB, sizeof(TBDB_STORE), 0); /* zero struct*/ 

		/* get filename param or use default */
		if((sDbFileName == NULL) || (sDbFileName[0] == '\0')) {
			strncpy((*ppTBDB)->sDbFile, TB_DEFAULT_DBNAME, TB_MAX_DB_NAME);
		} else {
			strncpy((*ppTBDB)->sDbFile, sDbFileName, TB_MAX_DB_NAME);
		}

		/* open sqlite3 database */
		if((rc = sqlite3_open((*ppTBDB)->sDbFile, &(*ppTBDB)->pDb)) == SQLITE_OK) {
			
			/* make sure our schema exists in case we just created the database*/
			if((rc = sqlite3_exec((*ppTBDB)->pDb, sqlPacketDbSchema, NULL, NULL, &pErr)) == SQLITE_OK) {
				
				/* get global range of packet times */
				rc = tbdb_get_packet_range(*ppTBDB, &(*ppTBDB)->Oldest, &(*ppTBDB)->Newest) ? SQLITE_OK : SQLITE_ERROR;

			} else {
				strncpy((*ppTBDB)->LastErr, pErr, TB_MAX_ERRMSG);
				sqlite3_free(pErr);			
			}
		
		} else {
			/* db open failed, copy errmsg and free original */
			strncpy((*ppTBDB)->LastErr, sqlite3_errmsg((*ppTBDB)->pDb), TB_MAX_ERRMSG);
		}
	} /* TBDB_STORE allocated */

	return (rc == SQLITE_OK);
}

/**********************************************************
 tbdb_close - frees open struct and closes database
***********************************************************/
int tbdb_close(TBDB_STORE** ppTBDB) {
	
	int rc = TBDB_ERR;

	/* close up database*/
	if((rc = sqlite3_close((*ppTBDB)->pDb)) == SQLITE_OK) {

		/* free up TB_PACKET_DB struct*/
		free((void*) *ppTBDB);
		*ppTBDB = NULL;

		/* set return code to ok*/
		rc = TBDB_OK;
	
	} else {
		/* error closing db*/
		strncpy((*ppTBDB)->LastErr, sqlite3_errmsg((*ppTBDB)->pDb), TB_MAX_ERRMSG);
	}

	return rc;
}

/**********************************************************
 tbdb_get_all_packets - retrieves all packets in the given
 range and loads them serially into buffer and 
***********************************************************/
int tbdb_get_all_packets(
	TBDB_STORE*		pTBDB,
	char*			sStation,
	char*			sNetwork,
	char*			sChannel,
	char*			sLocation,
	double			from,
	double			to,
	void**			ppBuf,
	long*			pBufSize,
	long*			pBufPageSize,
	long*			pCount
) {
	char sSQL[TB_MAX_SQL_LEN];
	sqlite3_stmt* pStmt;
	long chunkSize = 0;
	long bufPos = 0;
	int rc;

	/* do quick check */ 


	/* init output vars*/
	*ppBuf = NULL;
	*pCount = *pBufSize = *pBufPageSize = 0L;

	snprintf(sSQL, TB_MAX_SQL_LEN,
		"SELECT DataChunk, Pinno, NumSamp, StartTime, EndTime, SampleRate,	\
		Sta, Net, Chan, Loc, Version, DataType, Quality, Pad FROM PACKETS \
		WHERE Sta='%s' AND Net='%s' AND Chan='%s' AND Loc='%s' AND \
		(EndTime > %lf AND EndTime <= %lf) ORDER BY StartTime ASC;",
		sStation, sNetwork, sChannel, sLocation, from, to);

	if((rc = sqlite3_prepare_v2(pTBDB->pDb, sSQL, -1, &pStmt, 0)) == SQLITE_OK) {

		/* step through results and load up buffer. realloc
		as necessary.*/ 
		while((rc = sqlite3_step(pStmt)) == SQLITE_ROW) {
			/* get size of trace_buf packet */ 
			chunkSize = sqlite3_column_bytes(pStmt, 0);
			/* check buffer size and realloc as needed. */
			if((*pBufPageSize - *pBufSize) < chunkSize + sizeof(chunkSize)) {
				if((*ppBuf = realloc(*ppBuf, *pBufPageSize + TB_BLOCKSIZE)) == NULL) {					
					/* error w/ realloc*/
					strncpy(pTBDB->LastErr, "Error allocating packet buffer", TB_MAX_ERRMSG);		
				} else {
					*pBufPageSize += TB_BLOCKSIZE;
				}
			}
			memcpy((unsigned char*)*ppBuf + *pBufSize, &chunkSize, sizeof(chunkSize));
//			*((long*)((char*)*ppBuf + *pBufSize))= chunkSize;
			memcpy((unsigned char*)*ppBuf + *pBufSize + sizeof(chunkSize),
				sqlite3_column_blob(pStmt, 0), chunkSize);
			*pBufSize += chunkSize + sizeof(chunkSize);
			(*pCount)++;
		}
		/* check for error and logit*/
		if(rc != SQLITE_DONE) {
			strncpy(pTBDB->LastErr, sqlite3_errmsg(pTBDB->pDb), TB_MAX_ERRMSG);		
		}
		rc = sqlite3_finalize(pStmt);

	} else {
		/* error creating query. log error*/		
		strncpy(pTBDB->LastErr, sqlite3_errmsg(pTBDB->pDb), TB_MAX_ERRMSG);		
	}

   return (rc == SQLITE_OK);
}
/**********************************************************
 tbdb_open_range_query - 
***********************************************************/
int tbdb_open_range_query(
	TBDB_STORE*		pTBDB,
	TBDB_QUERY**	ppQry,
	char*			sStation,
	char*			sNetwork,
	char*			sChannel,
	char*			sLocation,
	double			from,
	double			to
) {
	char sSQL[TB_MAX_SQL_LEN];

	snprintf(sSQL, TB_MAX_SQL_LEN,
		"SELECT DataChunk, Pinno, NumSamp, StartTime, EndTime, SampleRate,	\
		Sta, Net, Chan, Loc, Version, DataType, Quality, Pad FROM PACKETS \
		WHERE Sta='%s' AND Net='%s' AND Chan='%s' AND Loc='%s' AND \
		(StartTime >= %lf AND EndTime < %lf);",
		sStation, sNetwork, sChannel, sLocation, from, to);

	/* not that 0 is the 0 based column index to DataChunk colume
	in the above SQL statement. */
	return tbdb_open_query(pTBDB, sSQL, ppQry);
}

/**********************************************************
 tbdb_open_query - gets packets for specified time
 range from db if any exist. Mempory is allocated with malloc.

 INPUT:
	pDB - pointer to TBDB_STORE from tbdb_open
	pQH - pointer to TBDB_QUERY 
 
 OUTPUT:
	*ppQry - pointer to allocated TBDB_QUERY struct.
***********************************************************/
int tbdb_open_query(
	TBDB_STORE* pTBDB,
	char* sSQL,
	TBDB_QUERY** ppQry
) {
	int rc =TBDB_ERR;

	/* make sure we have a pointer*/
	if(pTBDB && ppQry) {
		/* allocate TBDB_QUERY */
		if((*ppQry = (TBDB_QUERY*) malloc(sizeof(TBDB_QUERY))) != NULL) {
			/* prepare statement*/
			if((rc = sqlite3_prepare_v2(pTBDB->pDb, sSQL, -1, &(*ppQry)->pStmt, 0)) == SQLITE_OK) {
				(*ppQry)->LastErr[0] = '\0';


				rc = TBDB_OK;
			} else {
				/* prepare statement failed. save error message*/
				strncpy((*ppQry)->LastErr, sqlite3_errmsg(pTBDB->pDb), TB_MAX_ERRMSG);
			}
		}
	}
	return rc;
}

/**********************************************************
 tbdb_get_packet - Given a pointer to a TBDB_QUERY obtained
 from tbdb_open_query. 
***********************************************************/
int tbdb_get_packet(TBDB_QUERY* pQry, int nTBCol, TRACE2_HEADER** ppT2Buf, int* pT2BufSize) {
	
	int rc = 0;
	char err[TB_MAX_ERRMSG];

	/* get next record if any */
	if(pQry) {
		if((rc = sqlite3_step(pQry->pStmt)) == SQLITE_ROW) {
			/* get TRACE2_HEADER packet from db */
			*pT2BufSize = sqlite3_column_bytes(pQry->pStmt, nTBCol);
			if((*ppT2Buf = (TRACE2_HEADER*) malloc(*pT2BufSize)) != NULL) {
				memcpy( *ppT2Buf, sqlite3_column_blob(pQry->pStmt, nTBCol), *pT2BufSize);
			} else {
				/*error allocating t*/
				sprintf(err,"malloc failed allocating [%d] bytes.", *pT2BufSize);
				strncpy(pQry->LastErr, err, TB_MAX_ERRMSG);
				rc = SQLITE_NOMEM;
			}
		} else {
			/* no records or error*/
			strncpy(pQry->LastErr, sqlite3_errmsg(sqlite3_db_handle(pQry->pStmt)), TB_MAX_ERRMSG);
		}
	}
	return (rc == SQLITE_ROW);
}

/**********************************************************
tbdb_put_packet - Writes packet to database for a
given TRACE2_HEADER.
***********************************************************/
int tbdb_put_packet(
	TBDB_STORE* pTBDB, 
	TRACE2_HEADER* pTB, 
	const void* pT2Buf, 
	int T2BufSize
) {

	int	rc = 0;
        char *quality, *pad;
	sqlite3_stmt *pStmt = NULL;
	char sSql[TB_MAX_SQL_LEN];

	/* make sure we have a pointer*/
	if(pTBDB == NULL) {
		/* error TB_PACKET_DB_PTR is null*/
		return TBDB_ERR;
	}

	/* create sql statement */
        quality = GET_TRACE2_QUALITY(pTB);
        pad = GET_TRACE2_PAD(pTB);
	snprintf(sSql, TB_MAX_SQL_LEN,
		"INSERT INTO PACKETS(Pinno, NumSamp, StartTime, EndTime, SampleRate, \
		Sta, Net, Chan, Loc, Version, DataType, Quality, Pad, DataChunk) \
		VALUES(%d, %d, %lf, %lf, %lf, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', @Trace)",
		pTB->pinno, pTB->nsamp, pTB->starttime,	pTB->endtime,
		pTB->samprate, pTB->sta, pTB->net, pTB->chan, 
		pTB->loc, pTB->version, pTB->datatype, quality, pad);

	/* prepare(compile) sql, and then bind TRACE2_HEADER data to DataChunk field*/
	rc = sqlite3_prepare_v2(pTBDB->pDb, sSql, -1, &pStmt ,0);
	if((rc == SQLITE_OK) && (pStmt != NULL)) {
		/* bind Data buffer to DataChunk */
		
    	/* printf("parameter name 1:%s\n", sqlite3_bind_parameter_name(pStmt,1)); */

		if((rc = sqlite3_bind_blob(pStmt, 1, pT2Buf, T2BufSize, SQLITE_STATIC)) == SQLITE_OK) { 
			if((rc = sqlite3_step(pStmt)) == SQLITE_DONE) {
				/* update Oldest and Newest*/
				if(pTBDB->Oldest > pTB->starttime) 
					pTBDB->Oldest = pTB->starttime;

				if(pTBDB->Newest < pTB->endtime)
					pTBDB->Newest = pTB->endtime;

			} else {
				/* sql statement failed to execute via sqlite3_step */
				strncpy(pTBDB->LastErr, sqlite3_errmsg(pTBDB->pDb), TB_MAX_ERRMSG);
			}
		} else {
			/* bind failed*/
			strncpy(pTBDB->LastErr, sqlite3_errmsg(pTBDB->pDb), TB_MAX_ERRMSG);
		}
		/* cleanup statement*/
		rc = sqlite3_finalize(pStmt);

	} else {
		/* prepare failed*/
		strncpy(pTBDB->LastErr, sqlite3_errmsg(pTBDB->pDb), TB_MAX_ERRMSG);
	}

	return (rc == SQLITE_OK);
}

/**********************************************************
tbdb_purge_range - deletes all packets between starttime
and endtime.
INPUT:
pPDB -	valid pointer to TBDB_STORE returned 
		from tbdb_open().
from - Inclusive starttime or range of packets to delete
to	 - Exclusive endtime of range of packets to delete
OUTPUT:
returns TRUE if successful or FALSE on failure
***********************************************************/
int tbdb_purge_range(TBDB_STORE* pTBDB, double from, double to) {

	int rc = TBDB_OK;
	char sSql[TB_MAX_SQL_LEN];
	char* pErrMsg;

	/* valid (non-null) pointers for database and TBDB pointer. */
	if(pTBDB && pTBDB->pDb && (from != to)) {
		
		snprintf(sSql, TB_MAX_SQL_LEN, "DELETE FROM PACKETS WHERE (starttime >= %lf) AND (endtime< %lf);", from, to);
		
		/* run delete query and record error if any*/
		if((rc = sqlite3_exec(pTBDB->pDb, sSql, NULL, 0, &pErrMsg)) != SQLITE_OK) {
			strncpy(pTBDB->LastErr, pErrMsg, TB_MAX_ERRMSG);
		}
	}
	return (rc == SQLITE_OK);
}

/**********************************************************
 tbdb_get_SCNL_packet_count - returns the number of records
 within the given time range.
 **********************************************************/
long tbdb_get_SCNL_packet_count(
	TBDB_STORE* pTBDB,
	char* Sta,
	char* Net,
	char* Chan,
	char* Loc,
	double from,
	double to
) {
	int rc;
	long count = -1;
	sqlite3_stmt*	pStmt;
	char sSQL[TB_MAX_SQL_LEN];

	/* script query*/
	snprintf(sSQL, TB_MAX_SQL_LEN, 
		"SELECT Count(*) FROM PACKETS \
		WHERE Sta='%s' AND Net='%s' AND Chan='%s' AND Loc='%s' AND \
		(StartTime >= %lf AND EndTime < %lf);",
		Sta, Net, Chan, Loc, from, to);

	/* if */
	if(pTBDB) {
		if((rc = sqlite3_prepare_v2(pTBDB->pDb, sSQL, -1, &pStmt, 0)) == SQLITE_OK) {
			if((rc = sqlite3_step(pStmt)) == SQLITE_ROW) {
				count = sqlite3_column_int(pStmt, 0);
			}
			rc = sqlite3_finalize(pStmt);
		}
	}
	return count;
}

/**********************************************************
 tbdb_get_packet_range - returns the range of packets for
 a given SCNL
 **********************************************************/
int tbdb_get_packet_range(
	TBDB_STORE* pTBDB,
	double* pMin,
	double* pMax
) {
	
	int rc = TBDB_ERR;
	sqlite3_stmt*	pStmt;

	*pMin = 0.0;
	*pMax = 0.0;

	/* make sure TBDB points somewhere */
	if(pTBDB) {
		/* compile SQL*/
		if((rc = sqlite3_prepare_v2(pTBDB->pDb,
			"SELECT Min(Oldest), Max(Newest) FROM SNCLSTATS;",
			-1, &pStmt, 0)) == SQLITE_OK)
		{
			/* step query to get result*/
			if((rc = sqlite3_step(pStmt)) == SQLITE_ROW) {
				/* get first and second param of results*/
				*pMin = sqlite3_column_int(pStmt, 0);
				*pMax = sqlite3_column_int(pStmt, 1);
			}
			rc = sqlite3_finalize(pStmt);
		}
	}
	return (rc == SQLITE_OK);
}

/**********************************************************
 tbdb_close_query - close resources free memory, and cleanup
 TBDB_QUERY_HANDLE previously created with tbdb_query_open
 **********************************************************/
int tbdb_close_query(TBDB_QUERY** ppQry) {
	int rc = 0;

	/* free up sqlite3_stmt */
	if((rc = sqlite3_finalize((*ppQry)->pStmt)) == SQLITE_OK) {

		/* free RB_QUERY*/
		free((void*) *ppQry);
		*ppQry = 0;
		rc = SQLITE_OK;
	}
	return (rc == SQLITE_OK);
}

/***********************************************************
tbdb_purge_all - drop all PACKET records
************************************************************/
int tbdb_purge_all(TBDB_STORE* pTBDB) {
	
	int rc = TBDB_OK;
	char* pErrMsg;

	/* valid (non-null) pointers TBDB pointer. */
	if(pTBDB) {
		/* run delete query and record error if any*/
		if((rc = sqlite3_exec(pTBDB->pDb, "DELETE FROM PACKETS;", NULL, 0, &pErrMsg)) != SQLITE_OK) {
			strncpy(pTBDB->LastErr, pErrMsg, TB_MAX_ERRMSG);
		}
	}
	return (rc == SQLITE_OK);
}
/***********************************************************
tbdb_purge_prior_packets - drop all PACKET records prior to specified
time in epoch seconds. Query compares against starttime field
************************************************************/
int tbdb_purge_prior_packets(TBDB_STORE* pTBDB, double retainAfter) {
	
	int rc = TBDB_OK;
	char* pErrMsg;
	char  sSQL[TB_MAX_SQL_LEN];

	/* valid (non-null) pointers TBDB pointer. */
	if(pTBDB) {

		snprintf(sSQL, TB_MAX_SQL_LEN, "DELETE FROM PACKETS WHERE (StartTime < %lf);", retainAfter);
		/* run delete query and record error if any*/
		if((rc = sqlite3_exec(pTBDB->pDb, sSQL, NULL, 0, &pErrMsg)) != SQLITE_OK) {
			strncpy(pTBDB->LastErr, pErrMsg, TB_MAX_ERRMSG);
		}
	}
	return (rc == SQLITE_OK);
}

/************************************************************
 tbdb_has_packets - returns true if caller should bother to
 query db for given range. Currently looks and the global 
 settings. Currently returns true if there is an overlap of
 time periods.
*************************************************************/
int tbdb_has_packets(
	TBDB_STORE* pTBDB,
	char* sta,
	char* net, 
	char* chan, 
	char* loc, 
	double from, 
	double to
) {
	return ((pTBDB->Oldest < to) && (pTBDB->Newest >= from) && (from < to));	
}
