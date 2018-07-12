#ifndef JSON_CONN_H_
#define JSON_CONN_H_

#define JSON_CONN_DEFAULT_SERVER_TYPE		""
#define JSON_CONN_DEFAULT_CHANNEL_NUMBER	1
#define JSON_CONN_DEFAULT_FRAME_MAX_SIZE	AMQP_DEFAULT_FRAME_SIZE
#define JSON_CONN_DEFAULT_HEARTBEAT_SECONDS	20 // AMQP_DEFAULT_HEARTBEAT
#define JSON_CONN_DEFAULT_MAX_CHANNELS		AMQP_DEFAULT_MAX_CHANNELS
#define JSON_CONN_DEFAULT_USERNAME		""
#define JSON_CONN_DEFAULT_PASSWORD		""
#define JSON_CONN_DEFAULT_VHOST			"/"
#define JSON_CONN_DEFAULT_QUEUE_NAME		""
#define JSON_CONN_DEFAULT_EXCHANGE_NAME		""
#define JSON_CONN_DEFAULT_EXCHANGE_TYPE		"fanout"
#define JSON_CONN_DEFAULT_BIND_KEY		""
#define JSON_CONN_DEFAULT_BIND_ARGS		""
#define JSON_CONN_DEFAULT_READ_TIMEOUT		60
#define JSON_CONN_DEFAULT_DATA_TIMEOUT		120

typedef struct {
	const char *servertype;
	const char *hostname;
	int port;
	const char *username;
	const char *password;
	const char *vhost;
	const char *queuename;
 	const char *exchangename;
	const char *exchangetype;
	const char *bindkey;
	const char *bindargs;
	int channel_number;
	int frame_max_size;
	int heartbeat_seconds;
	int max_channels;
	int read_timeout;
	int data_timeout;
} JSON_CONN_PARAMS;

int close_json_connection();
void free_json_message();
int open_json_connection(JSON_CONN_PARAMS *);
char * read_json_message();
void set_json_connection_params_to_defaults(JSON_CONN_PARAMS *);

#endif /* JSON_CONN_H_ */
