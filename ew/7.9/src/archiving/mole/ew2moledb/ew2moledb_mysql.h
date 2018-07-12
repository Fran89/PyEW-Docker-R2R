#ifndef EW2MOLEDB_MYSQL_H
#define EW2MOLEDB_MYSQL_H 1

#include <mysql.h>

#define EW2MOLEDB_OK 0
#define EW2MOLEDB_ERR_CONNDB -1
#define EW2MOLEDB_ERR_EXQUERY -2

#ifdef _WINNT
#define snprintf _snprintf
#include <winsock.h>
#endif /* _WINNT */

#define EW2MOLEDB_DATETIME_MAXLEN_STR 31

extern int flag_debug_mysql;

int epoch_to_mysql_datetime_str(char *out_str, int len_out_str, double time_d);
void timestr_to_mysql_datetime_str(char out_str[EW2MOLEDB_DATETIME_MAXLEN_STR], char *in_str);

MYSQL *ew2moledb_mysql_connect(char *hostname, char *username, char *password, char *dbname, long dbport);

MYSQL_RES *ew2moledb_mysql_query_res(MYSQL *mysql, char *query);

long ew2moledb_mysql_consume_result(MYSQL_RES *result);

void ew2moledb_mysql_free_result(MYSQL_RES *result);

int ew2moledb_mysql_close(MYSQL *mysql);

int ew2moledb_mysql_query(char *query, char *hostname, char *username, char *password, char *dbname, long dbport);

long ew2moledb_mysql_free_results(MYSQL *mysql);


int ew2moledb_mysql_only_query_p(MYSQL *mysql, char *query);

MYSQL *ew2moledb_mysql_connect_p(MYSQL **pmysql, char *hostname, char *username, char *password, char *dbname, long dbport);

int ew2moledb_mysql_query_p(MYSQL **pmysql, char *query, char *hostname, char *username, char *password, char *dbname, long dbport);

void ew2moledb_mysql_close_p(MYSQL **pmysql);

#endif

