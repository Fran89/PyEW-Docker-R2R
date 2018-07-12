#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <earthworm.h>

#include "ew2moledb_mysql.h"


void timestr_to_mysql_datetime_str(char out_str[EW2MOLEDB_DATETIME_MAXLEN_STR], char *in_str) {
    /*
INPUT
20090812001522.56
01234567890123456

OUTPUT
2009-08-12 12:35:37.160000
01234567890123456789012345
       */
    out_str[0] = in_str[0];
    out_str[1] = in_str[1];
    out_str[2] = in_str[2];
    out_str[3] = in_str[3];

    out_str[4] = '-';

    out_str[5] = in_str[4];
    out_str[6] = in_str[5];

    out_str[7] = '-';

    out_str[8] = in_str[6];
    out_str[9] = in_str[7];

    out_str[10] = ' ';

    out_str[11] = in_str[8];
    out_str[12] = in_str[9];

    out_str[13] = ':';

    out_str[14] = in_str[10];
    out_str[15] = in_str[11];

    out_str[16] = ':';

    out_str[17] = in_str[12];
    out_str[18] = in_str[13];

    out_str[19] = in_str[14];

    out_str[20] = in_str[15];
    out_str[21] = in_str[16];

    out_str[22] = '0';
    out_str[23] = '0';
    out_str[24] = '0';
    out_str[25] = '0';

    out_str[26] = 0;

}

int epoch_to_mysql_datetime_str(char *out_str, int len_out_str, double time_d) {
    time_t time_t_start_time;
    struct tm *tm_start_time;
    double integral;

    out_str[len_out_str - 1] = 0;

    if(time_d > 0.0) {
	time_t_start_time = (time_t) time_d;
    } else {
	time_t_start_time = 0;
    }
    tm_start_time = gmtime(&time_t_start_time);

    snprintf(out_str, len_out_str - 1, "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
	    tm_start_time->tm_year + 1900,
	    tm_start_time->tm_mon + 1,
	    tm_start_time->tm_mday,
	    /*
	    tm_start_time->tm_yday + 1,
	    */
	    tm_start_time->tm_hour,
	    tm_start_time->tm_min,
	    tm_start_time->tm_sec,
	    (long) ( (modf(time_d, &integral) * 1000000.0) + 0.5)
	   );
    
    return 0;
}


const char sep_char = '#';
const char *sepline_major = "################################################################################################\n";
const char *sepline_minor = "=========================================================\n";

int flag_debug_mysql = 0;


MYSQL *ew2moledb_mysql_connect(char *hostname, char *username, char *password, char *dbname, long dbport) {
    MYSQL *mysql = NULL;

    /* Init struct mysql */
    mysql = mysql_init(NULL);

    /* Connect to MySQL server */
    if(!mysql_real_connect(mysql,hostname,username,password,dbname,dbport,NULL, CLIENT_MULTI_STATEMENTS)) {
		logit("e", "Error mysql_real_connect() connecting to %s:%ld. (%s)\n", hostname, dbport, mysql_error(mysql));
		mysql = NULL;
    }

    if(flag_debug_mysql) {
		logit("t", "%c hostname: %s; dbport: %ld; dbname: %s; username: %s.\n", sep_char, hostname, dbport, dbname, username);
		logit("t", "%c mysql_get_client_info()   : %s\n", sep_char, mysql_get_client_info());
		logit("t", "%c mysql_get_client_version(): %ld\n", sep_char, mysql_get_client_version());

		/* Print server info after mysql_real_connect() */
		logit("t", "%c mysql_get_host_info()     : %s\n", sep_char, mysql_get_host_info(mysql));
		logit("t", "%c mysql_get_proto_info()    : %d\n", sep_char, mysql_get_proto_info(mysql));
		logit("t", "%c mysql_get_server_info()   : %s\n", sep_char, mysql_get_server_info(mysql));
		logit("t", "%c mysql_get_server_version(): %ld\n", sep_char, mysql_get_server_version(mysql));
		logit("t", "%c mysql_stat()              : %s\n", sep_char, mysql_stat(mysql));
    }
    return mysql;
}

MYSQL_RES *ew2moledb_mysql_query_res(MYSQL *mysql, char *query) {

    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    MYSQL_FIELD *field;

    int num_fields;
    unsigned long *lengths;
    int query_length;
    int i;

    if(flag_debug_mysql) {
	logit("t", "ew2moledb_mysql_query_res()\n");
    }

    if(mysql_real_query(mysql,query,(unsigned int)strlen(query)) != 0) {
		logit("e", "Error mysql_real_query(): %d '%s' '%s' in query:\n'%s'\n",
			mysql_errno(mysql), mysql_error(mysql), mysql_sqlstate(mysql), query);
		return res;
    }

    res = mysql_store_result(mysql);

    if(res) {
	if(flag_debug_mysql) {
	    /* Print query with # after each newline */
	    query_length = strlen(query);
	    logit("t", "%c %s", sep_char, sepline_minor);
	    logit("t", "%c ", sep_char);
	    for(i=0; i < query_length; i++) {
		logit("t", "%c", query[i]);
		if(query[i] == '\n') {
		    logit("t", "%c ", sep_char);
		}
	    }
	    logit("t", "%s", sepline_minor);

	    logit("t", "%c ", sep_char);
	    while((field = mysql_fetch_field(res))){
		logit("t", "{%s} ", field->name);
	    }
	    logit("t", "\n");

	    /* End comment in output */
	    logit("t", "%c%s", sep_char, sepline_major);


	    num_fields = mysql_num_fields(res);
	    while( (row = mysql_fetch_row(res)) ) {
		lengths = mysql_fetch_lengths(res);
		for(i = 0; i < num_fields; i++)
		{
		    logit("t", "[%.*s] ", (int) lengths[i], row[i] ? row[i] : "NULL");
		}
		logit("t", "\n");
	    }
	}
    } else {
	logit("e", "Error mysql_store_result(): %d '%s' '%s' in query:\n'%s'\n",
		mysql_errno(mysql), mysql_error(mysql), mysql_sqlstate(mysql), query);
    }
    return res;
}


long ew2moledb_mysql_consume_result(MYSQL_RES *result) {
    MYSQL_ROW row;
    MYSQL_FIELD *field;
    int num_fields;
    int i;
    unsigned long *lengths;
    long do_nothing = 0;

    if(flag_debug_mysql) {
	logit("t", "ew2moledb_mysql_consume_result()\n");
    }

    while((field = mysql_fetch_field(result))){
	/* Do nothing */
	do_nothing++;
    }

    num_fields = mysql_num_fields(result);
    while( (row = mysql_fetch_row(result)) ) {
	lengths = mysql_fetch_lengths(result);
	for(i = 0; i < num_fields; i++) {
	    /* Do nothing */
	    do_nothing++;
	}
    }

    return do_nothing;
}

void ew2moledb_mysql_free_result(MYSQL_RES *result) {
    if(flag_debug_mysql) {
	logit("t", "ew2moledb_mysql_free_result()\n");
    }
    mysql_free_result(result);
}

int ew2moledb_mysql_close(MYSQL *mysql) {
    int ret = 0;
    if(flag_debug_mysql) {
	logit("t", "ew2moledb_mysql_close()\n");
    }
    /* Free struct mysql */
    mysql_close(mysql);
    return ret;
}

int ew2moledb_mysql_query(char *query, char *hostname, char *username, char *password, char *dbname, long dbport) {
    int ret = EW2MOLEDB_OK;

    MYSQL *mysql;
    MYSQL_RES *res;

    if(flag_debug_mysql) {
	logit("t", "ew2moledb_mysql_query()\n");
    }

    /* Connect to the MySQL database */
    mysql = ew2moledb_mysql_connect(hostname, username, password, dbname, dbport);

    if(mysql) {
	/* Execute query */
	res = ew2moledb_mysql_query_res(mysql, query);

	if(res) {
	    /* Free struct res */
	    ew2moledb_mysql_free_result(res);
	} else {
	    ret = EW2MOLEDB_ERR_EXQUERY;
	}

	/* Free struct mysql */
	ew2moledb_mysql_close(mysql);
    } else {
	ret = EW2MOLEDB_ERR_CONNDB;
    }

    return ret;
}

long ew2moledb_mysql_free_results(MYSQL *mysql) {
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;
    MYSQL_FIELD *field;

    int num_fields;
    unsigned long *lengths;
    int i;
    long do_nothing = 0;

    if(flag_debug_mysql) {
	logit("t", "ew2moledb_mysql_free_results()\n");
    }

    if(mysql) {
	while (mysql_more_results(mysql)){
	    mysql_next_result(mysql);
	    res = mysql_store_result(mysql);
	    if(res) {
		while((field = mysql_fetch_field(res))){
		    /* Do nothing */
		    do_nothing++;
		}

		num_fields = mysql_num_fields(res);
		while( (row = mysql_fetch_row(res)) ) {
		    lengths = mysql_fetch_lengths(res);
		    for(i = 0; i < num_fields; i++) {
			/* Do nothing */
			do_nothing++;
		    }
		}
		mysql_free_result(res);
	    }
	}
    }

    return do_nothing;
}


int ew2moledb_mysql_only_query_p(MYSQL *mysql, char *query) {
    int ret = 0;

    if(flag_debug_mysql) {
	logit("t", "ew2moledb_mysql_query_res()\n");
    }

    if(mysql_real_query(mysql,query,(unsigned int)strlen(query)) != 0) {
		logit("e", "Error mysql_real_query(): %d '%s' '%s' in query:\n'%s'\n",
			mysql_errno(mysql), mysql_error(mysql), mysql_sqlstate(mysql), query);
		ret = -1;
    }

    return ret;
}

MYSQL *ew2moledb_mysql_connect_p(MYSQL **pmysql, char *hostname, char *username, char *password, char *dbname, long dbport) {

    if(flag_debug_mysql) {
	logit("t", "ew2moledb_mysql_connect_p()\n");
	logit("t", "%c hostname: %s; dbport: %ld; dbname: %s; username: %s.\n", sep_char, hostname, dbport, dbname, username);
	logit("t", "%c *pmysql %s\n", sep_char, (*pmysql)? "NOT NULL" : "NULL");
	/* Print client info */
	logit("t", "%c mysql_get_client_info()   : %s\n", sep_char, mysql_get_client_info());
	logit("t", "%c mysql_get_client_version(): %ld\n", sep_char, mysql_get_client_version());
	logit("t", "%c mysql_thread_safe(): %u\n", sep_char, mysql_thread_safe());
    }

    if(*pmysql == NULL) {

	logit("t", "connecting to the database %s on %s:%ld, user %s ...\n", dbname, hostname, dbport, username);

	/* Init struct mysql */
	*pmysql = mysql_init(NULL);

	if(flag_debug_mysql) {
	    logit("t", "%c *pmysql %s\n", sep_char, (*pmysql)? "NOT NULL" : "NULL");
	}

	if(*pmysql) {
	    /* Connect to MySQL server */
	    if(!mysql_real_connect(*pmysql,hostname,username,password,dbname,dbport,NULL, CLIENT_MULTI_STATEMENTS)) {
		logit("e", "Error mysql_real_connect() connecting to %s:%ld. (%s)\n", hostname, dbport, mysql_error(*pmysql));
		*pmysql = NULL;
	    } 
	}

	if(*pmysql) {
	    logit("t", "connected to the database.\n");
	    if(flag_debug_mysql) {
		/* Print server info after mysql_real_connect() */
		logit("t", "%c mysql_get_host_info()     : %s\n", sep_char, mysql_get_host_info(*pmysql));
		logit("t", "%c mysql_get_proto_info()    : %d\n", sep_char, mysql_get_proto_info(*pmysql));
		logit("t", "%c mysql_get_server_info()   : %s\n", sep_char, mysql_get_server_info(*pmysql));
		logit("t", "%c mysql_get_server_version(): %ld\n", sep_char, mysql_get_server_version(*pmysql));
		logit("t", "%c mysql_stat()              : %s\n", sep_char, mysql_stat(*pmysql));
	    }
	} else {
	    logit("et", "not connected to the database.\n");
	}

    }

    return *pmysql;
}

int ew2moledb_mysql_query_p(MYSQL **pmysql, char *query, char *hostname, char *username, char *password, char *dbname, long dbport) {
    int ret = EW2MOLEDB_OK;

    int err_query = 0;

    if(flag_debug_mysql) {
	logit("t", "ew2moledb_mysql_query_p()\n");
    }

    /* Connect to the MySQL database */
    *pmysql = ew2moledb_mysql_connect_p(pmysql, hostname, username, password, dbname, dbport);

    if(*pmysql) {
	/* Execute query */
	err_query = ew2moledb_mysql_only_query_p(*pmysql, query);

	if(err_query == 0) {
	    /* Consume and free result */
	    /* ew2moledb_mysql_consume_result(res); */
	    /* ew2moledb_mysql_free_result(res); */
	    ew2moledb_mysql_free_results(*pmysql);
	} else {
	    ret = EW2MOLEDB_ERR_EXQUERY;

	    /* Free struct pmysql */
	    ew2moledb_mysql_close_p(pmysql);
	}

	/* DO NOT CLOSE HERE !!!!!!!!!! */

    } else {
	ret = EW2MOLEDB_ERR_CONNDB;
    }

    return ret;
}

void ew2moledb_mysql_close_p(MYSQL **pmysql) {

    if(flag_debug_mysql) {
	logit("t", "ew2moledb_mysql_close_p()\n");
    }

    if(*pmysql != NULL) {

	logit("t", "disconnecting from server.\n");

	/* Free struct mysql */
	ew2moledb_mysql_close(*pmysql);

	/* Reset struct mysql to NULL */
	*pmysql = NULL;
    } else {
	logit("t", "Warning: trying to disconnect from server but structure is NULL.\n");
    }
}

