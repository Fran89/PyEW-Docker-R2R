/*
	geojson2ew - geoJSON to earthworm 

	Copyright (c) 2014 California Institute of Technology.
	All rights reserved, November 6, 2014.
        This program is distributed WITHOUT ANY WARRANTY whatsoever.
        Do not redistribute this program without written permission.

	Authors: Kevin Frechette & Paul Friberg, ISTI.
*/
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>
#include <unistd.h>  // close
#include <arpa/inet.h> //inet_addr
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <regex.h>

#include <amqp_tcp_socket.h>
#include <amqp.h>
#include <amqp_framing.h>
#include <jansson.h>

#include "earthworm.h"  /* need this for the logit() call */
#include "socket_ew.h"
#include "externs.h"
#include "json_conn.h"

enum SERVER_TYPE {
  SERVER_SOCKET,
#ifndef SOCKET_ONLY
  SERVER_RABBITMQ,
#endif
  SERVER_UNKNOWN
};

// generic
static JSON_CONN_PARAMS *params = NULL;
static int server_type = SERVER_UNKNOWN;
static char *msg_buffer = NULL;
static size_t msg_buffer_size = 0;

// socket sever
#define SOCKET_LINE_BUFFER_SIZE 2048
#define SOCKET_MSG_START  "[ \t\n\r]*[{][ \t\n\r]*"
#define SOCKET_MSG_END    "[ \t\n\r]*[}][ \t\n\r]*\n"
static char socket_line_buffer[SOCKET_LINE_BUFFER_SIZE];
static int socket_line_buffer_len = 0;
static int socket_msg_put_index = 0;
static FILE *socket_f_recv = NULL;
static SOCKET socket_fd = INVALID_SOCKET;
static struct sockaddr_in socket_server;
static int socket_read_line_error;
static int socket_timeout = -1; // default to no timeout
static regex_t socket_re_start,  socket_re_end;

#ifndef SOCKET_ONLY
// RabbitMQ server
static struct timeval *rabbitMQ_timeout = NULL;
static amqp_connection_state_t rabbitMQ_conn = NULL;
static amqp_envelope_t rabbitMQ_envelope;
static amqp_bytes_t rabbitMQ_queuename;
static amqp_table_t rabbitMQ_bindargs;
#endif

static int get_server_type(JSON_CONN_PARAMS *p) {
	if(strcmp(p->servertype, "socket") == 0) {
	  server_type = SERVER_SOCKET;
	} else {
#ifndef SOCKET_ONLY
	  if(strcmp(p->servertype, "rabbitMQ") == 0) {
	  server_type = SERVER_RABBITMQ;
	  } else {
	    // make an intelligent guess
	    if(strcmp(p->queuename, "") != 0 || strcmp(p->exchangename, "") != 0) {
	      server_type = SERVER_RABBITMQ;
	    } else {
#endif
	      if(strcmp(p->username, "") == 0 && strcmp(p->password, "") == 0) {
		server_type = SERVER_SOCKET;
	      } else {
		server_type = SERVER_UNKNOWN;
		return 1;
	      }
#ifndef SOCKET_ONLY
	    }
	  }
#endif
	}

	return 0;
}


static void socket_add_line(char *line) {
	int old_msg_len = (int)msg_buffer_size;
	int len = strlen(line);
	if (len == 0) {
		logit("e", "empty line\n");
		return;
	}
	if (socket_msg_put_index + len >= (int)msg_buffer_size) {
		msg_buffer_size = socket_msg_put_index + len;
		if (msg_buffer != NULL) { // if message buffer exists
			msg_buffer = (char *)realloc(msg_buffer, msg_buffer_size);
		} else {
			msg_buffer = (char *)malloc(msg_buffer_size);
		}
	}
	memcpy(msg_buffer + socket_msg_put_index, line, len + 1);
	socket_msg_put_index += len;
	int new_msg_len = strlen(msg_buffer);
	if (old_msg_len + len != new_msg_len) {
		logit("e", "invalid message length (%d, %d, %d)\n", len, old_msg_len, new_msg_len);
	}

	return;
}


static int socket_close_json_connection() {
	int status = 0;
	if (socket_f_recv != NULL) {
		(void)fclose(socket_f_recv);
		socket_f_recv = NULL;
	}
	if (socket_fd != INVALID_SOCKET) {
		(void)close(socket_fd);
		socket_fd = INVALID_SOCKET;
	}

	regfree(&socket_re_start);
	regfree(&socket_re_end);

	params = NULL;

	return status;
}


static void socket_free_json_message() {
	socket_line_buffer_len = 0;
	socket_line_buffer[0] = 0;
	socket_msg_put_index = 0;
	if (msg_buffer != NULL)
		free(msg_buffer);
	msg_buffer = NULL;
	msg_buffer_size = 0;

	return;
}


static int socket_connect_to_server() {
	socket_fd = socket_ew(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == INVALID_SOCKET) {
		logit("e", "Could not create socket: %d\n", socketGetError_ew());
		return 1;
	}
	if (connect_ew(socket_fd, (struct sockaddr *) &socket_server, sizeof(socket_server), socket_timeout) < 0) {
		logit("e", "Could not connect: %d\n", socketGetError_ew());
		return 3;
	}
	socket_f_recv = fdopen(socket_fd, "r");
	if (socket_f_recv == NULL) {
		logit("e", "Could not open socket: %d\n", socketGetError_ew());
		return 4;
	}
	return 0;
}

static int socket_open_json_connection(JSON_CONN_PARAMS *p) {
        int status = 0;

        if (p->read_timeout > 0) {
        	socket_timeout = p->read_timeout * 1000; // convert seconds to ms
	}
	socket_server.sin_family = AF_INET;
	socket_server.sin_addr.s_addr = inet_addr(p->hostname);
	if (socket_server.sin_addr.s_addr == INADDR_NONE) {
		struct hostent* hp;
		if ( (hp = gethostbyname(p->hostname)) == NULL)
     		{
       			logit("et", "invalid ip address <%s>\n", p->hostname);
			return 2;
		}
		memcpy((void *) &socket_server.sin_addr, (void*)hp->h_addr, hp->h_length);
	}
	socket_server.sin_port = htons(p->port);
	status = socket_connect_to_server();
	if(status != 0) {
		return status;
	}

	// compile regular expressions used to frame JSON message
	status = regcomp(&socket_re_start, SOCKET_MSG_START, REG_EXTENDED);
	if(status != 0) {
		logit("e", "Failed to compile regex '%s'\n", SOCKET_MSG_START);
		return status;
	}

	status = regcomp(&socket_re_end, SOCKET_MSG_END, REG_EXTENDED);
	if(status != 0) {
		logit("e", "Failed to compile regex '%s'\n", SOCKET_MSG_END);
		return status;
	}

	return status;
}

static char * socket_read_line() {
	socket_read_line_error = 0;
	char *line = fgets(socket_line_buffer + socket_line_buffer_len,
		SOCKET_LINE_BUFFER_SIZE - socket_line_buffer_len, socket_f_recv) ;
	if (line != NULL) {
	  socket_line_buffer_len = strlen(socket_line_buffer);
		if (socket_line_buffer_len > 0 &&
		    socket_line_buffer[socket_line_buffer_len - 1] == '\n') {
			socket_line_buffer_len = 0;
			return socket_line_buffer;
		}
		return NULL; // incomplete line
	}
	socket_read_line_error = socketGetError_ew();
	// if not try again error
	if (socket_read_line_error != EAGAIN) {
		// error, close the connection
		logit("e", "read error: %d\n", socket_read_line_error);
		socket_free_json_message();
		socket_close_json_connection();
	}
	return NULL; // EOF or error
}

static char * socket_read_json_message() {
	char *line;

        // if connection was closed because of an eror
        if (socket_fd == INVALID_SOCKET) {
		if (socket_read_line_error)
			sleep(1);
                // if unable toy to reconnect
                if (socket_connect_to_server() != 0) {
                        logit("e", "error: %d\n", socket_read_line_error);
                        socket_free_json_message();
			socket_close_json_connection();
                        return NULL;
		}
        }

	while ((line = socket_read_line()) != NULL) {
		int status = 0;	
		// frame JSON msg using regular expression pattern matches
		// 'jansson' parser handles the rest 
		if ((status = regexec(&socket_re_start, line, 0, NULL, 0)) == 0) { // msg start
			socket_add_line(line);
			if ((status = regexec(&socket_re_end, line, 0, NULL, 0)) == 0) { // msg end
				return msg_buffer;
			} else {
				// for convenience using line buffer for error message
				regerror(status, &socket_re_end, socket_line_buffer, SOCKET_LINE_BUFFER_SIZE);
				logit("e", "regexec error : %s\n", socket_line_buffer);
				return NULL;
			}
		} else {
			// for convenience using line buffer for error message		  
			regerror(status, &socket_re_start, socket_line_buffer, SOCKET_LINE_BUFFER_SIZE);
			logit("e", "regexec error : %s\n", socket_line_buffer);
			return NULL;
		}
	}
	return NULL;
}

#ifndef SOCKET_ONLY
#if DEBUG
static void rabbitMQ_print_bindargs() {
  int i;

  for(i=0; i <  rabbitMQ_bindargs.num_entries; i++) {
    fprintf(stderr, "key=%s ", (const char*)rabbitMQ_bindargs.entries[i].key.bytes);
  
    switch(rabbitMQ_bindargs.entries[i].value.kind) {
    case AMQP_FIELD_KIND_UTF8:
      fprintf(stderr, "value=%s\n", (char*)rabbitMQ_bindargs.entries[i].value.value.bytes.bytes);
      break;
    case AMQP_FIELD_KIND_I32:
      fprintf(stderr, "value=%d\n", rabbitMQ_bindargs.entries[i].value.value.i32);
      break;
    case AMQP_FIELD_KIND_F64:
      fprintf(stderr, "value=%f\n", rabbitMQ_bindargs.entries[i].value.value.f64);
      break;
    case AMQP_FIELD_KIND_BOOLEAN:
      fprintf(stderr, "value=%d\n", rabbitMQ_bindargs.entries[i].value.value.boolean);
      break;
    default:
      fprintf(stderr, "value=unknown\n");
      break;
    }
  }

  return;
}
#endif

static int rabbitMQ_create_bindargs(char const *prop_str) {

  // get root object
  json_error_t error;
  json_t *root = json_loads(prop_str, 0, &error);
  if (!root) {
     fprintf(stderr, "error: BINDARGS: on line %d: %s\n",error.line, error.text); 
     return 1;
  }

  // find table size
  void *iter = json_object_iter(root);
  int num_entries = 0, i;
  while(iter) {
    num_entries++;
    iter = json_object_iter_next(root, iter);
  }

  amqp_table_entry_t *entries = malloc(sizeof(amqp_table_entry_t));
  const char *key;
  json_t *value;

  // populate table
  iter = json_object_iter(root);
  i = 0;
  while(iter) {

    key = json_object_iter_key(iter);
    value = json_object_iter_value(iter);

    entries[i].key = amqp_cstring_bytes(key);

    switch(json_typeof(value)) {
    case JSON_STRING:
      entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
      entries[i].value.value.bytes = amqp_cstring_bytes(json_string_value(value));
      break;
    case JSON_INTEGER:
      entries[i].value.kind = AMQP_FIELD_KIND_I32;
      entries[i].value.value.i32 = (int)json_integer_value(value);
      break;
    case JSON_REAL:
      entries[i].value.kind = AMQP_FIELD_KIND_F64;
      entries[i].value.value.f64 = json_real_value(value);
      break;
    case JSON_TRUE:
    case JSON_FALSE:
      entries[i].value.kind = AMQP_FIELD_KIND_BOOLEAN;
      entries[i].value.value.boolean = json_boolean_value(value);
      break;
    case JSON_OBJECT:
    case JSON_ARRAY:
    case JSON_NULL:
    default:
      break;
    }

    iter = json_object_iter_next(root, iter);
    i++;
  }
  
  rabbitMQ_bindargs.num_entries = num_entries;
  rabbitMQ_bindargs.entries = entries;
  qsort(rabbitMQ_bindargs.entries, rabbitMQ_bindargs.num_entries, sizeof(amqp_table_entry_t), &amqp_table_entry_cmp);

#ifdef DEBUG 
  rabbitMQ_print_bindargs();
#endif

  return 0;
}

static int rabbitMQ_check_for_error(int x, char const *context) {
	if (x < 0) {
		logit("e", "%s: %s\n", context, amqp_error_string2(x));
		return 1;
	}
	return 0;
}

static int rabbitMQ_check_for_amqp_error(amqp_rpc_reply_t x, char const *context) {
	int status = 1;
	switch (x.reply_type) {
	case AMQP_RESPONSE_NORMAL:
		status = 0;
		break;

	case AMQP_RESPONSE_NONE:
		logit("e", "%s: missing RPC reply type!\n", context);
		break;

	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		logit("e", "%s: library_error=%s\n", context,
				amqp_error_string2(x.library_error));
		break;

	case AMQP_RESPONSE_SERVER_EXCEPTION:
		switch (x.reply.id) {
		case AMQP_CONNECTION_CLOSE_METHOD: {
			amqp_connection_close_t *m =
					(amqp_connection_close_t *) x.reply.decoded;
			logit("e", "%s: server connection error %d, message: %.*s\n",
					context, m->reply_code, (int) m->reply_text.len,
					(char *) m->reply_text.bytes);
			break;
		}
		case AMQP_CHANNEL_CLOSE_METHOD: {
			amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
			logit("e", "%s: server channel error %d, message: %.*s\n",
					context, m->reply_code, (int) m->reply_text.len,
					(char *) m->reply_text.bytes);
			break;
		}
		default:
			logit("e", "%s: unknown server error, method id 0x%08X\n",
					context, x.reply.id);
			break;
		}
		break;
	}

	return status;
}

static int rabbitMQ_close_json_connection() {
	int status = 0;
	if (rabbitMQ_conn != NULL) {
		if (rabbitMQ_check_for_amqp_error(
				amqp_channel_close(rabbitMQ_conn, (amqp_channel_t)params->channel_number,
				AMQP_REPLY_SUCCESS), "Closing channel")
				|| rabbitMQ_check_for_amqp_error(
						amqp_connection_close(rabbitMQ_conn, AMQP_REPLY_SUCCESS),
						"Closing connection")
				|| rabbitMQ_check_for_error(amqp_destroy_connection(rabbitMQ_conn),
						"Ending connection")) {
			status = 1;
		}
		rabbitMQ_conn = NULL;
	}
	if(rabbitMQ_queuename.len > (size_t)0) {
	  amqp_bytes_free(rabbitMQ_queuename);
	}
	if(rabbitMQ_bindargs.num_entries > 0 && rabbitMQ_bindargs.entries != NULL) {
	  free(rabbitMQ_bindargs.entries);
	  rabbitMQ_bindargs.num_entries = 0;
	}
	return status;
}

static void rabbitMQ_free_json_message() {
	amqp_destroy_envelope(&rabbitMQ_envelope);
}

static int rabbitMQ_open_json_connection(JSON_CONN_PARAMS *p) {
	if (p->read_timeout > 0) {
		rabbitMQ_timeout = malloc(sizeof(struct timeval));
		rabbitMQ_timeout->tv_sec = p->read_timeout;
		rabbitMQ_timeout->tv_usec = 0;
	}
	rabbitMQ_conn = amqp_new_connection();
	amqp_socket_t *socket = amqp_tcp_socket_new(rabbitMQ_conn);
	if (!socket) {
		logit("e", "could not create TCP socket\n");
		return 1;
	}

	if (amqp_socket_open(socket, p->hostname, p->port)) {
		logit("e", "could not open TCP socket\n");
		return 1;
	}

	if (rabbitMQ_check_for_amqp_error(
			amqp_login(rabbitMQ_conn, p->vhost, p->max_channels, p->frame_max_size,
					p->heartbeat_seconds, AMQP_SASL_METHOD_PLAIN, p->username,
					p->password), "Logging in")) {
		return 1;
	}

	amqp_channel_t channel_number =(amqp_channel_t)p->channel_number;
	amqp_channel_open(rabbitMQ_conn, channel_number);
	if (rabbitMQ_check_for_amqp_error(amqp_get_rpc_reply(rabbitMQ_conn), "Opening channel")) {
		return 1;
	}

	// get queuename
	if(strcmp(p->queuename, "") != 0) { /* named queue */
	  rabbitMQ_queuename = amqp_bytes_malloc_dup(amqp_cstring_bytes(p->queuename));
	} else { // exchange
	  amqp_boolean_t passive = 1, durable = 0, auto_delete = 0, internal = 0;
	  amqp_exchange_declare(rabbitMQ_conn, channel_number, amqp_cstring_bytes(p->exchangename),
				amqp_cstring_bytes(p->exchangetype),
				passive, durable, auto_delete, internal, amqp_empty_table);
	  if (rabbitMQ_check_for_amqp_error(amqp_get_rpc_reply(rabbitMQ_conn), "Declaring exchange")) {
	    return 1;
	  }

	  {
	    // system generated temporary queue name
	    amqp_boolean_t passive = 0, durable = 0, exclusive = 1, auto_delete = 1;
	    amqp_queue_declare_ok_t *r = amqp_queue_declare(rabbitMQ_conn, channel_number, amqp_empty_bytes,
							    passive, durable, exclusive, auto_delete,
							    amqp_empty_table);
	    if (rabbitMQ_check_for_amqp_error(amqp_get_rpc_reply(rabbitMQ_conn), "Declaring queue")) {
	      return 1;
	    }
	    rabbitMQ_queuename = amqp_bytes_malloc_dup(r->queue);
	    if (rabbitMQ_queuename.bytes == NULL) {
	      logit("e", "Out of memory while copying queue name");
	      return 1;
	    }
	  }
	  
	  {
	    // for headers exchange: make table of queue bind arguements
	    rabbitMQ_bindargs = amqp_empty_table;
	    if(strcmp(p->exchangetype, "headers") == 0 && strcmp(p->bindargs, "") != 0) {
	      if (rabbitMQ_create_bindargs(p->bindargs)) {
		logit("e", "Failed to create bind args");
		return 1;
	      }
	    }

	    // NOTE: exchangename == "" and bindkey != "" => a namedqueue
	    amqp_queue_bind(rabbitMQ_conn, channel_number, rabbitMQ_queuename, amqp_cstring_bytes(p->exchangename),
			    amqp_cstring_bytes(p->bindkey), rabbitMQ_bindargs);
	    if (rabbitMQ_check_for_amqp_error(amqp_get_rpc_reply(rabbitMQ_conn), "Binding queue")) {
	      return 1;
	    }
	  }
	}

	// start consuming messages
	amqp_boolean_t no_local = 0, no_ack = 1, exclusive = 0;
        amqp_basic_consume(rabbitMQ_conn, channel_number, rabbitMQ_queuename, amqp_empty_bytes,
			     no_local, no_ack, exclusive, amqp_empty_table);
        return rabbitMQ_check_for_amqp_error(amqp_get_rpc_reply(rabbitMQ_conn), "Consuming");
}

static char * rabbitMQ_read_json_message() {
	amqp_maybe_release_buffers(rabbitMQ_conn);
	amqp_rpc_reply_t res = amqp_consume_message(rabbitMQ_conn, &rabbitMQ_envelope, rabbitMQ_timeout, 0);
	if (AMQP_RESPONSE_NORMAL != res.reply_type) {
		rabbitMQ_free_json_message();
		return NULL;
	}
	size_t const len = rabbitMQ_envelope.message.body.len;
	if (len == 0) {
		rabbitMQ_free_json_message();
		return NULL;
	}
	char * firstbyte = (char *) rabbitMQ_envelope.message.body.bytes;
	char * lastbyte = firstbyte + len - 1;
	if (*lastbyte != 0) {
		if (*lastbyte != '\n')
			logit("e", "bytes are not null terminated (%d)\n", *lastbyte);
		*lastbyte = 0;
	}
	// if new length is greater than buffer size
	if (rabbitMQ_envelope.message.body.len > msg_buffer_size) {
		if (msg_buffer != NULL) // if message buffer exists
			free(msg_buffer); // free the message buffer
		msg_buffer_size = (rabbitMQ_envelope.message.body.len / 1024 + 1) * 1024;
		msg_buffer = malloc(msg_buffer_size);
	} else if (memcmp(msg_buffer, rabbitMQ_envelope.message.body.bytes,
			rabbitMQ_envelope.message.body.len) == 0) {
                if (Verbose & VERBOSE_DUP) {
		   logit("e", "duplicate message:\n%s\n", msg_buffer);
		}
		socket_free_json_message();
		return NULL;
	}
	memcpy(msg_buffer, rabbitMQ_envelope.message.body.bytes, rabbitMQ_envelope.message.body.len);
	return msg_buffer;
}
#endif

void set_json_connection_params_to_defaults(JSON_CONN_PARAMS *p) {
	memset(p, 0, sizeof(JSON_CONN_PARAMS));
	p->servertype = JSON_CONN_DEFAULT_SERVER_TYPE;
	p->channel_number = JSON_CONN_DEFAULT_CHANNEL_NUMBER;
	p->frame_max_size = JSON_CONN_DEFAULT_FRAME_MAX_SIZE;
	p->heartbeat_seconds = JSON_CONN_DEFAULT_HEARTBEAT_SECONDS;
	p->max_channels = JSON_CONN_DEFAULT_MAX_CHANNELS;
	p->vhost = JSON_CONN_DEFAULT_VHOST;
	p->username = JSON_CONN_DEFAULT_USERNAME;
	p->password = JSON_CONN_DEFAULT_PASSWORD;
	p->queuename = JSON_CONN_DEFAULT_QUEUE_NAME;
	p->exchangename = JSON_CONN_DEFAULT_EXCHANGE_NAME;
	p->exchangetype = JSON_CONN_DEFAULT_EXCHANGE_TYPE;
	p->bindkey = JSON_CONN_DEFAULT_BIND_KEY;
	p->bindargs = JSON_CONN_DEFAULT_BIND_ARGS;
	p->read_timeout = JSON_CONN_DEFAULT_READ_TIMEOUT;
	p->data_timeout = JSON_CONN_DEFAULT_DATA_TIMEOUT;

	return;
}

int close_json_connection() {
	int status = 0;

	switch(server_type) {
	case SERVER_SOCKET:
	  status = socket_close_json_connection();
	  break;
#ifndef SOCKET_ONLY
	case SERVER_RABBITMQ:
	  status = rabbitMQ_close_json_connection();
	  break;
#endif
	case SERVER_UNKNOWN:
	default:
	  status = 1;
	  break;
	}
	return status;
}

void free_json_message() {
	switch(server_type) {
	case SERVER_SOCKET:
	  socket_free_json_message();
	  break;
#ifndef SOCKET_ONLY
	case SERVER_RABBITMQ:
	  rabbitMQ_free_json_message();
	  break;
#endif
	case SERVER_UNKNOWN:
	default:
	  break;
	}
	return;
}

int open_json_connection(JSON_CONN_PARAMS *p) {
	int status = 0;
	params = p;

	status = get_server_type(p);
	if(status != 0) {
	  return 1;
	}

	switch(server_type) {
	case SERVER_SOCKET:
	  status = socket_open_json_connection(p);
	  break;
#ifndef SOCKET_ONLY
	case SERVER_RABBITMQ:
	  status = rabbitMQ_open_json_connection(p);
	  if(status != 0) {
	    rabbitMQ_conn = NULL;
	  }
	  break;
#endif
	case SERVER_UNKNOWN:
	default:
	  status = 1;
	  break;
	}
	
	return status;
}

char * read_json_message() {
	switch(server_type) {
	case SERVER_SOCKET:
	  msg_buffer = socket_read_json_message();
	  break;
#ifndef SOCKET_ONLY
	case SERVER_RABBITMQ:
	  msg_buffer = rabbitMQ_read_json_message();
	  break;
#endif
	case SERVER_UNKNOWN:
	default:
	  break;
	}
	
	return msg_buffer;
}
