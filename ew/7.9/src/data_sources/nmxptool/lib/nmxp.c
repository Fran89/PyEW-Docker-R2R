/*! \file
 *
 * \brief Nanometrics Protocol Library
 *
 * Author:
 * 	Matteo Quintiliani
 * 	Istituto Nazionale di Geofisica e Vulcanologia - Italy
 *	quintiliani@ingv.it
 *
 * $Id: nmxp.c 5938 2013-09-17 07:14:38Z quintiliani $
 *
 */

#include "nmxp.h"
#include "nmxp_memory.h"

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef HAVE_WINDOWS_H
#include "winsock2.h"
#endif

int nmxp_sendConnect(int isock) {
    return nmxp_sendMessage(isock, NMXP_MSG_CONNECT, NULL, 0);
}

int nmxp_sendTerminateSubscription(int isock, NMXP_SHUTDOWN_REASON reason, char *message) {
    return nmxp_sendMessage(isock, NMXP_MSG_TERMINATESUBSCRIPTION, message, ((message)? strlen(message)-1 : 0));
}

int nmxp_receiveChannelList(int isock, NMXP_CHAN_LIST **pchannelList) {
    int ret;
    int i;
    int recv_errno;

    NMXP_MSG_SERVER type;
    char buffer[NMXP_MAX_LENGTH_DATA_BUFFER]={0};
    int32_t length;

    *pchannelList = NULL;

    ret = nmxp_receiveMessage(isock, &type, buffer, &length, 0, &recv_errno, NMXP_MAX_LENGTH_DATA_BUFFER);
    
    /*TODO controllare ret*/
    if (ret == NMXP_SOCKET_OK) {
        if(type != NMXP_MSG_CHANNELLIST) {
            nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "Type %d is not NMXP_MSG_CHANNELLIST!\n", type);
        } else {
            (*pchannelList) = (NMXP_CHAN_LIST *) NMXP_MEM_MALLOC(length);

            if( (*pchannelList) != NULL) {

                memmove((*pchannelList), buffer, length);

                (*pchannelList)->number = ntohl((*pchannelList)->number);

                nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANNEL, "number of channels %d\n", (*pchannelList)->number);

                /* TODO check*/

                for(i=0; i < (*pchannelList)->number; i++) {
                    (*pchannelList)->channel[i].key = ntohl((*pchannelList)->channel[i].key);
                    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANNEL, "%12d %s\n",
                            (*pchannelList)->channel[i].key,
                            NMXP_LOG_STR((*pchannelList)->channel[i].name));
                }
            } else {
                nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "nmxp_receiveChannelList() Error allocating pchannelList!\n");
                ret = NMXP_SOCKET_ERROR;
            }
        }
    }

    return ret;
}


int nmxp_sendAddTimeSeriesChannel_raw(int isock, NMXP_CHAN_LIST_NET *channelList, int32_t shortTermCompletion, int32_t out_format, NMXP_BUFFER_FLAG buffer_flag) {
    int ret = NMXP_SOCKET_OK;
    int32_t buffer_length = 16 + (4 * channelList->number); 
    char *buffer = NULL;
    int32_t app, i, disp;

    if(buffer_length > 0) {

	buffer = NMXP_MEM_MALLOC(buffer_length);

	disp=0;

	app = htonl(channelList->number);
	memcpy(&buffer[disp], &app, 4);
	disp+=4;

	for(i=0; i < channelList->number; i++) {
	    app = htonl(channelList->channel[i].key);
	    memcpy(&buffer[disp], &app, 4);
	    disp+=4;
	}

	app = htonl(shortTermCompletion);
	memcpy(&buffer[disp], &app, 4);
	disp+=4;

	app = htonl(out_format);
	memcpy(&buffer[disp], &app, 4);
	disp+=4;

	app = htonl(buffer_flag);
	memcpy(&buffer[disp], &app, 4);
	disp+=4;

	ret = nmxp_sendMessage(isock, NMXP_MSG_ADDTIMESERIESCHANNELS, buffer, buffer_length);

	if(buffer) {
	    NMXP_MEM_FREE(buffer);
	    buffer = NULL;
	}
    } else {
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "nmxp_sendAddTimeSeriesChannel_raw() buffer length = %d.\n", buffer_length);
    }

    return ret;
}

#define MAX_LEN_S_CHANNELS 4096
int nmxp_sendAddTimeSeriesChannel(int isock, NMXP_CHAN_LIST_NET *channelList, int32_t shortTermCompletion, int32_t out_format, NMXP_BUFFER_FLAG buffer_flag, int n_channel, int n_usec, int flag_restart) {
    static int i = 0;
    static int first_time = 1;
    /*TODO avoid static Stefano*/
    static struct timeval last_tp_now;

    char s_channels[MAX_LEN_S_CHANNELS];
    int j;
    int ret = 0;
    NMXP_CHAN_LIST_NET split_channelList;
    long diff_usec;
    struct timeval tp_now;
    double estimated_time = 0.0;

    if(n_usec == 0  &&  n_channel == 0) {
	n_channel = channelList->number;
    }

    estimated_time = (double) channelList->number * ( ((double) n_usec / 1000000.0) / (double) n_channel);

    if(flag_restart) {
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW,
		"Estimated time for channel requests: %d * (%d/%d) = %.3f sec.\n",
		channelList->number, n_usec / 1000, n_channel,
		estimated_time);


	first_time = 1;
	i = 0;
    }

    /* Check if requests could be satisfied within NMXP_MAX_MSCHAN_MSEC */
    if(estimated_time > ((double) NMXP_MAX_MSCHAN_MSEC / 1000.0)) {
	n_usec = ( (double) NMXP_MAX_MSCHAN_MSEC * 1000.0 ) * ( (double) n_channel / (double) channelList->number);
	estimated_time = (double) channelList->number * ( ((double) n_usec / 1000000.0) / (double) n_channel);
	if(flag_restart) {
	    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_ANY, "Estimated time exceeds. New values %d/%d and estimated time %.3f sec.\n",
		    n_usec/1000, n_channel, estimated_time);
	}
    }

#ifdef HAVE_GETTIMEOFDAY
    gettimeofday(&tp_now, NULL);
#else
#error function gettimeofday() is required.
#endif

    if(i <  channelList->number) {
	    if(first_time) {
		    diff_usec = n_usec + 1;
		    first_time = 0;
		    last_tp_now.tv_sec = 0;
		    last_tp_now.tv_usec = 0;
	    } else {
		    diff_usec = (tp_now.tv_sec - last_tp_now.tv_sec) * 1000000;
		    diff_usec += (tp_now.tv_usec - last_tp_now.tv_usec);
	    }
	    if(diff_usec >= n_usec) {
		    /* while(ret == 0  &&  i <  channelList->number) { */
		    split_channelList.number = 0;
		    while(split_channelList.number < n_channel  &&  i < channelList->number) {
			    split_channelList.channel[split_channelList.number].key = channelList->channel[i].key;
			    /* Not necessary, but it could help for debugging */
			    strncpy(split_channelList.channel[split_channelList.number].name, channelList->channel[i].name, NMXP_CHAN_MAX_SIZE_NAME);
			    split_channelList.number++;
			    i++;
		    }
		    if(split_channelList.number > 0) {
			snprintf(s_channels, MAX_LEN_S_CHANNELS, "%.0f/%d chan %d of %d:",
				(double)diff_usec/1000.0, split_channelList.number, i, channelList->number);
			    for(j=0; j < split_channelList.number; j++) {
				strncat(s_channels, " ", MAX_LEN_S_CHANNELS - strlen(s_channels));
				strncat(s_channels, NMXP_LOG_STR(split_channelList.channel[j].name), MAX_LEN_S_CHANNELS - strlen(s_channels));
			    }
			    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "%s\n", s_channels);
			    ret = nmxp_sendAddTimeSeriesChannel_raw(isock, &split_channelList, shortTermCompletion, out_format, buffer_flag);
		    }
		    /* } */
		    last_tp_now.tv_sec = tp_now.tv_sec;
		    last_tp_now.tv_usec = tp_now.tv_usec;
	    }
    }

    return ret;
}


NMXP_DATA_PROCESS *nmxp_receiveData(int isock, NMXP_CHAN_LIST_NET *channelList, const char *network_code, const char *location_code, int timeoutsec, int *recv_errno ) {
    NMXP_MSG_SERVER type;
    char buffer[NMXP_MAX_LENGTH_DATA_BUFFER]={0};
    int32_t length;
    NMXP_DATA_PROCESS *pd = NULL;

    if(nmxp_receiveMessage(isock, &type, buffer, &length, timeoutsec, recv_errno, NMXP_MAX_LENGTH_DATA_BUFFER) == NMXP_SOCKET_OK) {
	if(type == NMXP_MSG_COMPRESSED) {
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "Type %d is NMXP_MSG_COMPRESSED!\n", type);
	    pd = nmxp_processCompressedData(buffer, length, channelList, network_code, location_code);
	} else if(type == NMXP_MSG_DECOMPRESSED) {
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_PACKETMAN, "Type %d is NMXP_MSG_DECOMPRESSED!\n", type);
	    pd = nmxp_processDecompressedData(buffer, length, channelList, network_code, location_code);
	} else {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_PACKETMAN, "Type %d is not NMXP_MSG_COMPRESSED or NMXP_MSG_DECOMPRESSED!\n", type);
	}
    }

    return pd;
}


int nmxp_sendConnectRequest(int isock, char *naqs_username, char *naqs_password, int32_t connection_time) {
    int ret;
    int i;
    char crc32buf[100];
    char *pcrc32buf = crc32buf;
    int crc32buf_length = 0;
    NMXP_CONNECT_REQUEST connectRequest;
    int naqs_username_length, naqs_password_length;
    int32_t protocol_version = 0;
    char *pp = NULL;

    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CRC, "%s - %s\n",
	    NMXP_LOG_STR(naqs_username), NMXP_LOG_STR(naqs_password));

    naqs_username_length = (naqs_username)? strlen(naqs_username) : 0;
    naqs_password_length = (naqs_password)? strlen(naqs_password) : 0;

    for(i=0; i < 100; i++) {
	crc32buf[i] = 0;
    }

    for(i=0; i<12; i++) {
	connectRequest.username[i] = 0;
    }
    if(naqs_username_length != 0) {
	strncpy(connectRequest.username, naqs_username, NMXP_MAX_SIZE_USERNAME);
    }

    connectRequest.version = protocol_version;
    connectRequest.connection_time = connection_time;

    if(!nmxp_data_bigendianhost()) {
	nmxp_data_swap_4b ((int32_t *)&connection_time);
    }

    if(naqs_username_length == 0  &&  naqs_password_length == 0 ) {
	/* sprintf(crc32buf, "%d%d", protocol_version, connection_time); */
	/* TODO */
    } else if(naqs_username_length != 0  &&  naqs_password_length != 0 ) {
	/* sprintf(crc32buf, "%s%d%d%s", naqs_username, protocol_version,
		connection_time, naqs_password); */

	memcpy(pcrc32buf, naqs_username, naqs_username_length);
	crc32buf_length += naqs_username_length;
	pcrc32buf = crc32buf + crc32buf_length;

	memcpy(pcrc32buf, &(protocol_version), sizeof(protocol_version));
	crc32buf_length += sizeof(protocol_version);
	pcrc32buf = crc32buf + crc32buf_length;

	memcpy(pcrc32buf, &(connection_time), sizeof(connection_time));
	crc32buf_length += sizeof(connection_time);
	pcrc32buf = crc32buf + crc32buf_length;

	memcpy(pcrc32buf, naqs_password, naqs_password_length);
	crc32buf_length += naqs_password_length;
	pcrc32buf = crc32buf + crc32buf_length;

    } else if(naqs_username_length != 0 ) {
	/* sprintf(crc32buf, "%s%d%d", naqs_username, protocol_version, connection_time); */
	/* TODO */
    } else if(naqs_password_length != 0 ) {
	/* sprintf(crc32buf, "%d%d%s", protocol_version, connection_time, naqs_password); */
	/* TODO */
    }
    connectRequest.version = htonl(connectRequest.version);
    connectRequest.connection_time = htonl(connectRequest.connection_time);
    connectRequest.crc32 = htonl(crc32(0L, crc32buf, crc32buf_length));

    ret = nmxp_sendMessage(isock, NMXP_MSG_CONNECTREQUEST, &connectRequest, sizeof(NMXP_CONNECT_REQUEST));

    if(ret == NMXP_SOCKET_OK) {
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CRC, "Send a ConnectRequest crc32buf length %d, crc32 = %d\n",
		crc32buf_length, connectRequest.crc32);
	for(i=0; i < crc32buf_length; i++) {
	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_CRC, "%d ", crc32buf[i]);
	}
	pp = (char *) &connectRequest.crc32;
	for(i=0; i < sizeof(connectRequest.crc32); i++) {
	    nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_CRC, "%d ", pp[i]);
	}
	nmxp_log(NMXP_LOG_NORM_NO, NMXP_LOG_D_CRC, "\n");
    } else {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CRC, "Send a ConnectRequest.\n");
    }

    return ret;
}


int nmxp_readConnectionTime(int isock, int32_t *connection_time) {
    int ret;
    int recv_errno;
    ret = nmxp_recv_ctrl(isock, connection_time, sizeof(int32_t), 0, &recv_errno);
    *connection_time = ntohl(*connection_time);
    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "Read connection time from socket %d.\n", *connection_time);
    if(ret != NMXP_SOCKET_OK) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Read connection time from socket.\n");
    }
    return ret;
}


int nmxp_waitReady(int isock) {
    int times = 0;
    int rc = NMXP_SOCKET_OK;
    int32_t signature;
    int32_t type = 0;
    int32_t length;
    int recv_errno;

    while(rc == NMXP_SOCKET_OK  &&  type != NMXP_MSG_READY) {
	rc = nmxp_recv_ctrl(isock, &signature, sizeof(signature), 0, &recv_errno);
	if(rc != NMXP_SOCKET_OK) return rc;
	signature = ntohl(signature);
	if(signature == 0) {
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "signature is equal to zero. receive again.\n");
	    rc = nmxp_recv_ctrl(isock, &signature, sizeof(signature), 0, &recv_errno);
	    signature = ntohl(signature);
	}
	if(signature != NMX_SIGNATURE) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "signature is not valid. signature = %d\n", signature);
	    if(signature == 200) {
		    int32_t err_length;
		    int32_t err_reason;
		    char err_buff[200];
		    rc = nmxp_recv_ctrl(isock, &err_length, sizeof(err_length), 0, &recv_errno);
		    err_length = ntohl(err_length);
		    rc = nmxp_recv_ctrl(isock, &err_reason, sizeof(err_reason), 0, &recv_errno);
		    err_reason = ntohl(err_reason);
		    if(err_length > 4) {
			    rc = nmxp_recv_ctrl(isock, err_buff, err_length-4, 0, &recv_errno);
			    err_buff[err_length] = 0;
		    }
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "TerminateMessage from Server: %s (%d).\n",
			    NMXP_LOG_STR(err_buff), err_reason);
	    }
	    return NMXP_SOCKET_ERROR;
	}

	rc = nmxp_recv_ctrl(isock, &type, sizeof(type), 0, &recv_errno);
	if(rc != NMXP_SOCKET_OK) return rc;
	type = ntohl(type);
	if(type != NMXP_MSG_READY) {
	    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "type is not READY. type = %d\n", type);
	    rc = nmxp_recv_ctrl(isock, &length, sizeof(length), 0, &recv_errno);
	    if(rc != NMXP_SOCKET_OK) return rc;
	    length = ntohl(length);
	    if(length > 0) {
		if(type == NMXP_MSG_TERMINATESUBSCRIPTION) {
		    char *str_msg = NULL;
		    char *buf_app = (char *) NMXP_MEM_MALLOC(sizeof(char) * length);
		    int32_t reason;
		    memcpy(&reason, buf_app, sizeof(reason));
		    reason = ntohl(reason);
		    str_msg = buf_app + sizeof(reason);
		    rc = nmxp_recv_ctrl(isock, buf_app, length, 0, &recv_errno);
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "%d %s shutdown: %s\n",
			    reason,
			    (reason == 0)? "Normal" : (reason == 1)? "Error" : (reason == 2)? "Timeout" : "Unknown",
			    str_msg);
		    if(buf_app) {
			NMXP_MEM_FREE(buf_app);
			buf_app = NULL;
		    }
		    /* Close the socket*/
		    nmxp_closeSocket(isock);
		    exit(-1);
		} else if(length == 4) {
		    int32_t app;
		    rc = nmxp_recv_ctrl(isock, &app, length, 0, &recv_errno);
		    if(rc != NMXP_SOCKET_OK) return rc;
		    app = ntohl(app);
		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CONNFLOW, "value = %d\n", app);
		} else {
		    char *buf_app = (char *) NMXP_MEM_MALLOC(sizeof(char) * length);
		    rc = nmxp_recv_ctrl(isock, buf_app, length, 0, &recv_errno);
		    if(buf_app) {
			NMXP_MEM_FREE(buf_app);
			buf_app = NULL;
		    }
		}
	    }
	} else {
	    rc = nmxp_recv_ctrl(isock, &length, sizeof(length), 0, &recv_errno);
	    if(rc != NMXP_SOCKET_OK) return rc;
	    length = ntohl(length);
	    if(length != 0) {
		nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "length is not equal to zero. length = %d\n", length);
		return NMXP_SOCKET_ERROR;
	    }
	}

	times++;
	if(times > 10) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "waiting_ready_message. times > 10\n");
	    rc = NMXP_SOCKET_ERROR;
	}

    }

    return rc;
}


int nmxp_sendDataRequest(int isock, int32_t key, int32_t start_time, int32_t end_time) {
    int ret;
    NMXP_DATA_REQUEST dataRequest;

    dataRequest.chan_key = htonl(key);
    dataRequest.start_time = htonl(start_time);
    dataRequest.end_time = htonl(end_time);

    ret = nmxp_sendMessage(isock, NMXP_MSG_DATAREQUEST, &dataRequest, sizeof(dataRequest));

    if(ret != NMXP_SOCKET_OK) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Send a Request message\n");
    }

    return ret;
}


int nmxp_sendRequestPending(int isock) {
    int ret;

    ret = nmxp_sendMessage(isock, NMXP_MSG_REQUESTPENDING, NULL, 0);

    if(ret != NMXP_SOCKET_OK) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Send a RequestPending message\n");
    }

    return ret;
}


NMXP_CHAN_LIST *nmxp_getAvailableChannelList(char * hostname, int portnum, NMXP_DATATYPE datatype, int (*func_cond)(void)) {
    int naqssock;
    NMXP_CHAN_LIST *channelList = NULL, *channelList_subset = NULL;
    /* int i; */

    /* 1. Open a socket*/
    naqssock = nmxp_openSocket(hostname, portnum, func_cond);

    if(naqssock != NMXP_SOCKET_ERROR) {

	/* 2. Send a Connect*/
	if(nmxp_sendConnect(naqssock) == NMXP_SOCKET_OK) {

	    /* 3. Receive ChannelList*/
	     if(nmxp_receiveChannelList(naqssock, &channelList) == NMXP_SOCKET_OK) {

		 channelList_subset = nmxp_chan_getType(channelList, datatype);
		 nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANNEL, "%d / %d are DataType channel.\n", channelList_subset->number, channelList->number);

		 /* nmxp_chan_sortByKey(channelList_subset);*/
		 nmxp_chan_sortByName(channelList_subset);

		 /*
		 for(i=0; i < channelList_subset->number; i++) {
		     nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_CHANNEL, "%12d %s\n",
			     channelList_subset->channel[i].key,
			     NMXP_LOG_STR(channelList_subset->channel[i].name));
		 }
		 */

		 /* 4. Send a Request Pending (optional)*/

		 /* 5. Send AddChannels*/

		 /* 6. Repeat until finished: receive and handle packets*/

		 /* 7. Send Terminate Subscription*/
		 nmxp_sendTerminateSubscription(naqssock, NMXP_SHUTDOWN_NORMAL, "Good Bye!");

	     } else {
		 nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANNEL, "Error on receiveChannelList()\n");
	     }
	} else {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANNEL, "Error on sendConnect()\n");
	}

	/* 8. Close the socket*/
	nmxp_closeSocket(naqssock);
    }

    if(channelList) {
	NMXP_MEM_FREE(channelList);
	channelList = NULL;
    }

    return channelList_subset;
}


NMXP_META_CHAN_LIST *nmxp_getMetaChannelList(char * hostname, int portnum, NMXP_DATATYPE datatype, int flag_request_channelinfo, char *datas_username, char *datas_password, NMXP_CHAN_LIST **pchannelList, int (*func_cond)(void)) {
    int naqssock;
    NMXP_CHAN_PRECISLIST *precisChannelList = NULL;
    NMXP_CHAN_LIST *channelList = NULL;
    NMXP_META_CHAN_LIST *chan_list = NULL;
    NMXP_META_CHAN_LIST *iter = NULL;
    int i = 0;
    int32_t connection_time;
    int ret_sock;
    int recv_errno;
    
    NMXP_MSG_SERVER type;
    char buffer[NMXP_MAX_LENGTH_DATA_BUFFER];
    int32_t length;
    NMXP_PRECISLISTREQUEST precisListRequestBody;
    NMXP_CHANNELINFOREQUEST channelInfoRequestBody;
    NMXP_CHANNELINFORESPONSE *channelInfo = NULL;

    char str_start[NMXP_DATA_MAX_SIZE_DATE], str_end[NMXP_DATA_MAX_SIZE_DATE];
    str_start[0] = 0;
    str_end[0] = 0;
    
    /* DAP Step 1: Open a socket */
    if( (naqssock = nmxp_openSocket(hostname, portnum, func_cond)) == NMXP_SOCKET_ERROR) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Error opening socket!\n");
	return NULL;
    }

    /* DAP Step 2: Read connection time */
    if(nmxp_readConnectionTime(naqssock, &connection_time) != NMXP_SOCKET_OK) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Error reading connection time from server!\n");
	return NULL;
    }

    /* DAP Step 3: Send a ConnectRequest */
    if(nmxp_sendConnectRequest(naqssock, datas_username, datas_password, connection_time) != NMXP_SOCKET_OK) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Error sending connect request!\n");
	return NULL;
    }

    /* DAP Step 4: Wait for a Ready message */
    if(nmxp_waitReady(naqssock) != NMXP_SOCKET_OK) {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CONNFLOW, "Error waiting Ready message!\n");
	return NULL;
    }

    /* DAP Step 5: Send Data Request */
    nmxp_sendHeader(naqssock, NMXP_MSG_CHANNELLISTREQUEST, 0);

    /* DAP Step 6: Receive Data until receiving a Ready message */
    ret_sock = nmxp_receiveMessage(naqssock, &type, buffer, &length, 0, &recv_errno, NMXP_MAX_LENGTH_DATA_BUFFER);
    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "ret_sock = %d, type = %d, length = %d\n", ret_sock, type, length);

    while(ret_sock == NMXP_SOCKET_OK   &&    type != NMXP_MSG_READY) {
	if(channelList) {
	    NMXP_MEM_FREE(channelList);
	    channelList = NULL;
	}
	channelList = (NMXP_CHAN_LIST *) NMXP_MEM_MALLOC(length);
	if(channelList) {
	    memmove(channelList, buffer, length);

	    channelList->number = ntohl(channelList->number);

	    for(i = 0; i < channelList->number; i++) {
		channelList->channel[i].key = ntohl(channelList->channel[i].key);
		if(getDataTypeFromKey(channelList->channel[i].key) == datatype) {
		    nmxp_meta_chan_add(&chan_list, channelList->channel[i].key, channelList->channel[i].name, 0, 0, NULL, NMXP_META_SORT_NAME);
		}
	    }
	} else {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "nmxp_getMetaChannelList() Error allocating channelList.\n");
	}

	/* Receive Message */
	ret_sock = nmxp_receiveMessage(naqssock, &type, buffer, &length, 0, &recv_errno, NMXP_MAX_LENGTH_DATA_BUFFER);
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "ret_sock = %d, type = %d, length = %d\n", ret_sock, type, length);
    }

    *pchannelList = channelList;

    /* DAP Step 5: Send Data Request */
    precisListRequestBody.instr_id = htonl(-1);
    precisListRequestBody.datatype = htonl(NMXP_DATA_TIMESERIES);
    precisListRequestBody.type_of_channel = htonl(-1);
    nmxp_sendMessage(naqssock, NMXP_MSG_PRECISLISTREQUEST, &precisListRequestBody, sizeof(NMXP_PRECISLISTREQUEST));


    /* DAP Step 6: Receive Data until receiving a Ready message */
    ret_sock = nmxp_receiveMessage(naqssock, &type, buffer, &length, 0, &recv_errno, NMXP_MAX_LENGTH_DATA_BUFFER);
    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "ret_sock = %d, type = %d, length = %d\n", ret_sock, type, length);

    while(ret_sock == NMXP_SOCKET_OK   &&    type != NMXP_MSG_READY) {
	if(precisChannelList) {
	    NMXP_MEM_FREE(precisChannelList);
	    precisChannelList = NULL;
	}
	precisChannelList = (NMXP_CHAN_PRECISLIST *) NMXP_MEM_MALLOC(length);
	if(precisChannelList) {
	    memmove(precisChannelList, buffer, length);

	    precisChannelList->number = ntohl(precisChannelList->number);
	    for(i = 0; i < precisChannelList->number; i++) {
		precisChannelList->channel[i].key = ntohl(precisChannelList->channel[i].key);
		precisChannelList->channel[i].start_time = ntohl(precisChannelList->channel[i].start_time);
		precisChannelList->channel[i].end_time = ntohl(precisChannelList->channel[i].end_time);

		nmxp_data_to_str(str_start, precisChannelList->channel[i].start_time);
		nmxp_data_to_str(str_end, precisChannelList->channel[i].end_time);

		if(!nmxp_meta_chan_set_times(chan_list, precisChannelList->channel[i].key, precisChannelList->channel[i].start_time, precisChannelList->channel[i].end_time)) {
		    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANNEL, "Key %d not found for %s!\n",
			    precisChannelList->channel[i].key,
			    NMXP_LOG_STR(precisChannelList->channel[i].name));
		}

		/*
		   nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_ANY, "%12d %12s %10d %10d %20s %20s\n",
		   precisChannelList->channel[i].key, NMXP_LOG_STR(precisChannelList->channel[i].name),
		   precisChannelList->channel[i].start_time, precisChannelList->channel[i].end_time,
		   NMXP_LOG_STR(str_start), NMXP_LOG_STR(str_end));
		   */
	    }
	} else {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "nmxp_getMetaChannelList() Error allocating precisChannelList.\n");
	}

	/* Receive Message */
	ret_sock = nmxp_receiveMessage(naqssock, &type, buffer, &length, 0, &recv_errno, NMXP_MAX_LENGTH_DATA_BUFFER);
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "ret_sock = %d, type = %d, length = %d\n", ret_sock, type, length);
    }


    if(flag_request_channelinfo) {
	for(iter = chan_list; iter != NULL; iter = iter->next) {

	    if(getChannelNumberFromKey(iter->key) == 0) {
		/* DAP Step 5: Send Data Request */
		channelInfoRequestBody.key = htonl(iter->key);
		channelInfoRequestBody.ignored = htonl(0);
		nmxp_sendMessage(naqssock, NMXP_MSG_CHANNELINFOREQUEST, &channelInfoRequestBody, sizeof(NMXP_CHANNELINFOREQUEST));

		/* DAP Step 6: Receive Data until receiving a Ready message */
		ret_sock = nmxp_receiveMessage(naqssock, &type, buffer, &length, 0, &recv_errno, NMXP_MAX_LENGTH_DATA_BUFFER);
		nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "ret_sock = %d, type = %d, length = %d\n", ret_sock, type, length);

		while(ret_sock == NMXP_SOCKET_OK   &&    type != NMXP_MSG_READY) {
		    if(channelInfo) {
			NMXP_MEM_FREE(channelInfo);
			channelInfo = NULL;
		    }
		    channelInfo = (NMXP_CHANNELINFORESPONSE *) NMXP_MEM_MALLOC(length);
		    if(channelInfo) {
			memmove(channelInfo, buffer, length);

			channelInfo->key = ntohl(channelInfo->key);

			if(!nmxp_meta_chan_set_network(chan_list, channelInfo->key, channelInfo->network)) {
			    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_CHANNEL, "Key %d (%d) not found for %s!\n",
				    iter->key, channelInfo->key, NMXP_LOG_STR(iter->name));
			}
		    } else {
			nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY, "nmxp_getMetaChannelList() Error allocating precisChannelList.\n");
		    }

		    /* Receive Message */
		    ret_sock = nmxp_receiveMessage(naqssock, &type, buffer, &length, 0, &recv_errno, NMXP_MAX_LENGTH_DATA_BUFFER);
		    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_EXTRA, "ret_sock = %d, type = %d, length = %d\n", ret_sock, type, length);
		}
	    }
	}
    }



    /* DAP Step 7: Repeat steps 5 and 6 for each data request */

    /* DAP Step 8: Send a Terminate message (optional) */
    nmxp_sendTerminateSubscription(naqssock, NMXP_SHUTDOWN_NORMAL, "Bye!");

    /* DAP Step 9: Close the socket */
    nmxp_closeSocket(naqssock);

    return chan_list;
}


int nmxp_raw_stream_seq_no_compare(const void *a, const void *b)
{       
    int ret = 0;
    NMXP_DATA_PROCESS **ppa = (NMXP_DATA_PROCESS **) a;
    NMXP_DATA_PROCESS **ppb = (NMXP_DATA_PROCESS **) b;
    NMXP_DATA_PROCESS *pa = *ppa;
    NMXP_DATA_PROCESS *pb = *ppb;

    if(pa && pb) {
	if(pa->seq_no > pb->seq_no) {
	    ret = 1;
	} else if (pa->seq_no < pb->seq_no) {
	    ret = -1;
	}
    } else {
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
		"nmxp_raw_stream_seq_no_compare() pa %s NULL and pb %s NULL\n", (pa)? "!=" : "=", (pb)? "!=" : "=");
    }

    return ret;
}

void nmxp_raw_stream_init(NMXP_RAW_STREAM_DATA *raw_stream_buffer, int32_t max_tolerable_latency, int timeoutrecv) {
    int j;

    raw_stream_buffer->last_seq_no_sent = -1;
    raw_stream_buffer->last_sample_time = -1.0;
    raw_stream_buffer->last_latency = 0.0;
    /* TODO 
     * Suppose a packet can contain 1/4 secs of data (that is, minimum packet length is 0.25 secs of data) */
    raw_stream_buffer->max_tolerable_latency = max_tolerable_latency;
    raw_stream_buffer->max_pdlist_items = max_tolerable_latency * 4;
    raw_stream_buffer->timeoutrecv = timeoutrecv;
    raw_stream_buffer->n_pdlist = 0;

    raw_stream_buffer->pdlist=NULL;
    raw_stream_buffer->pdlist = (NMXP_DATA_PROCESS **) NMXP_MEM_MALLOC(raw_stream_buffer->max_pdlist_items * sizeof(NMXP_DATA_PROCESS *));
    for(j=0; j<raw_stream_buffer->max_pdlist_items; j++) {
	raw_stream_buffer->pdlist[j] = NULL;
    }

}


void nmxp_raw_stream_free(NMXP_RAW_STREAM_DATA *raw_stream_buffer) {
    int j;
    if(raw_stream_buffer) {
	if(raw_stream_buffer->pdlist) {
	    for(j=0; j<raw_stream_buffer->n_pdlist; j++) {
		if(raw_stream_buffer->pdlist[j]) {
		    if(raw_stream_buffer->pdlist[j]->pDataPtr) {
			NMXP_MEM_FREE(raw_stream_buffer->pdlist[j]->pDataPtr);
			raw_stream_buffer->pdlist[j]->pDataPtr = NULL;
		    }
		    NMXP_MEM_FREE(raw_stream_buffer->pdlist[j]);
		    raw_stream_buffer->pdlist[j] = NULL;
		}
	    }
	    NMXP_MEM_FREE(raw_stream_buffer->pdlist);
	    raw_stream_buffer->pdlist = NULL;
	}
    }
}


int nmxp_raw_stream_manage(NMXP_RAW_STREAM_DATA *p, NMXP_DATA_PROCESS *a_pd, int (*p_func_pd[NMXP_MAX_FUNC_PD]) (NMXP_DATA_PROCESS *), int n_func_pd) {
    int ret = 0;
    int send_again = 1;
    int seq_no_diff;
    double time_diff;
    double latency = 0.0;
    int j=0, k=0;
    int i_func_pd;
    char str_time[NMXP_DATA_MAX_SIZE_DATE];
    NMXP_DATA_PROCESS *pd = NULL;
    int y, w;
    int count_null_element = 0;
    char netstachan[100];

    /* Allocate pd copy value from a_pd */
    if(a_pd) {
	/*
	if(a_pd->packet_type == 33 || a_pd->packet_type == 97) {
	    nmxp_data_log(a_pd);
	}
	*/

	/* Allocate memory for pd and copy a_pd */
	pd = (NMXP_DATA_PROCESS *) NMXP_MEM_MALLOC(sizeof(NMXP_DATA_PROCESS));
	if (pd == NULL) {
	    nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_RAWSTREAM,"nmxp_raw_stream_manage(): Error allocating memory\n");
	    exit(-1);
	}
	memcpy(pd, a_pd, sizeof(NMXP_DATA_PROCESS));
	if(a_pd->nSamp *  sizeof(int) > 0) {
	    pd->pDataPtr = (int *) NMXP_MEM_MALLOC(a_pd->nSamp * sizeof(int));
	    memcpy(pd->pDataPtr, a_pd->pDataPtr, a_pd->nSamp * sizeof(int));
	} else {
	    pd->pDataPtr = NULL;
	}
    } else {
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_RAWSTREAM,
		"nmxp_raw_stream_manage() passing NMXP_DATA_PROCESS pointer equal to NULL\n");
    }
    /* From here, use only pd */

    /* First time */
    if(p->last_seq_no_sent == -1  &&  pd != NULL) {
	if(p->timeoutrecv == 0) {
	    p->last_seq_no_sent = pd->seq_no - 1;
	    p->last_sample_time = pd->time;
	    p->last_latency = nmxp_data_latency(pd);;
	} else {
	    p->last_seq_no_sent = 0;
	    p->last_sample_time = 0.0;
	    p->last_latency = 0.0;
	}
	nmxp_data_to_str(str_time, pd->time);
	nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_RAWSTREAM,
		"%s.%s.%s [%d, %d] (%s + %.2f sec.) * First time nmxp_raw_stream_manage() * last_seq_no_sent=%d  last_sample_time=%.2f\n",
		NMXP_LOG_STR(pd->network), NMXP_LOG_STR(pd->station), NMXP_LOG_STR(pd->channel),
		pd->packet_type, pd->seq_no,
		NMXP_LOG_STR(str_time), (double) pd->nSamp / (double) pd->sampRate,
		 p->last_seq_no_sent, p->last_sample_time);
    }

    if(p->n_pdlist > 0) {
	latency = nmxp_data_latency(p->pdlist[0]);
    }

    /* Add pd and sort array, in case handle the first item */
    if( ( (p->n_pdlist >= p->max_pdlist_items || latency >= p->max_tolerable_latency) && p->timeoutrecv <= 0 ) 
	    ||
	    ( p->n_pdlist >= p->max_pdlist_items &&  p->timeoutrecv > 0)
	    ) {

	/* Supposing p->pdlist is ordered, handle the first item and over write it.  */
	if(p->n_pdlist > 0) {
	    seq_no_diff = p->pdlist[0]->seq_no - p->last_seq_no_sent;
	    time_diff = p->pdlist[0]->time - p->last_sample_time;
	    latency = nmxp_data_latency(p->pdlist[0]);
	    nmxp_data_to_str(str_time, p->pdlist[0]->time);
	    if( seq_no_diff > 0) {
		nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_RAWSTREAM,
			"%s.%s.%s [%d, %d] (%s + %.2f sec.) * Force handling packet * n_pdlist=%d  seq_no_diff=%d  time_diff=%.2fs  lat. %.1fs!\n",
			NMXP_LOG_STR(p->pdlist[0]->network), NMXP_LOG_STR(p->pdlist[0]->station), NMXP_LOG_STR(p->pdlist[0]->channel),
			p->pdlist[0]->packet_type, p->pdlist[0]->seq_no,
			NMXP_LOG_STR(str_time), (double) p->pdlist[0]->nSamp / (double) p->pdlist[0]->sampRate,
			p->n_pdlist,
			seq_no_diff, time_diff, latency);
		for(i_func_pd=0; i_func_pd<n_func_pd; i_func_pd++) {
		    (*p_func_pd[i_func_pd])(p->pdlist[0]);
		}
		p->last_seq_no_sent = (p->pdlist[0]->seq_no);
		p->last_sample_time = (p->pdlist[0]->time + ((double) p->pdlist[0]->nSamp / (double) p->pdlist[0]->sampRate ));
		p->last_latency = nmxp_data_latency(p->pdlist[0]);
	    } else {
		/* It should not occur */
		nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_RAWSTREAM,
			"%s.%s.%s [%d, %d] (%s + %.2f sec.) * SHOULD NOT OCCUR packet discarded * n_pdlist=%d  seq_no_diff=%d  time_diff=%.2fs  lat. %.1fs!\n",
			NMXP_LOG_STR(p->pdlist[0]->network), NMXP_LOG_STR(p->pdlist[0]->station), NMXP_LOG_STR(p->pdlist[0]->channel),
			p->pdlist[0]->packet_type, p->pdlist[0]->seq_no,
			NMXP_LOG_STR(str_time), (double) p->pdlist[0]->nSamp / (double) p->pdlist[0]->sampRate,
			p->n_pdlist,
			seq_no_diff, time_diff, latency);
	    }

	    /* Free handled packet */
	    if(p->pdlist[0]->pDataPtr) {
		NMXP_MEM_FREE(p->pdlist[0]->pDataPtr);
		p->pdlist[0]->pDataPtr = NULL;
	    }
	    if(p->pdlist[0]) {
		NMXP_MEM_FREE(p->pdlist[0]);
		p->pdlist[0] = NULL;
	    }
	    if(pd) {
		p->pdlist[0] = pd;
	    }
	} else {
	    if(pd) {
		p->n_pdlist = 1;
		p->pdlist[0] = pd;
	    }
	}
    } else {
	if(pd != NULL) {
	    p->pdlist[p->n_pdlist] = pd;
            p->n_pdlist++;
	}
    }

    /* Check if some element in pdlist is NULL and remove it */
    count_null_element = 0;
    y=0;
    while(y < p->n_pdlist) {
	if(p->pdlist[y] == NULL) {
	    count_null_element++;
	    /* Shift array */
	    for(w=y+1; w < p->n_pdlist; w++) {
		p->pdlist[w-1] = p->pdlist[w];
	     }
	    p->n_pdlist--;
	} else {
	    y++;
	}
    }

    if(count_null_element > 0) {
	if(p->n_pdlist > 0) {
	    snprintf(netstachan, 100, "%s.%s.%s",
		    NMXP_LOG_STR(p->pdlist[0]->network),
		    NMXP_LOG_STR(p->pdlist[0]->station),
		    NMXP_LOG_STR(p->pdlist[0]->channel));
	} else {
	    strncpy(netstachan, "Unknown", 100);
	}
	nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
		"nmxp_raw_stream_manage() %d NULL elements in pdlist for %s.\n",
		count_null_element, netstachan);
    }

    /* Sort array */
    qsort(p->pdlist, p->n_pdlist, sizeof(NMXP_DATA_PROCESS *), nmxp_raw_stream_seq_no_compare);

    /* TODO Check for packet duplication in pd->pdlist*/

    /* Print array, only for debugging */
    /*
    if(p->n_pdlist > 1) {
	int y = 0;
	for(y=0; y < p->n_pdlist; y++) {
	    nmxp_data_to_str(str_time, p->pdlist[y]->time);
	    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_RAWSTREAM,
		    "%s.%s.%s [%d, %d] (%s + %.2f sec.) * %02d n_pdlist=%d\n",
		    NMXP_LOG_STR(p->pdlist[y]->network), NMXP_LOG_STR(p->pdlist[y]->station), NMXP_LOG_STR(p->pdlist[y]->channel),
		    p->pdlist[y]->packet_type, p->pdlist[y]->seq_no,
		    NMXP_LOG_STR(str_time), (double) p->pdlist[y]->nSamp / (double) p->pdlist[y]->sampRate,
		    y, p->n_pdlist);
	}
    }
    */

    /* Condition for time-out (pd is NULL) */
    if(pd == NULL && p->n_pdlist > 0) {
	/* Log before changing values */
	nmxp_data_to_str(str_time, p->pdlist[0]->time);
	nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_RAWSTREAM,
		"%s.%s.%s [%d, %d] (%s + %.2f sec.) * pd is NULL and n_pdlist = %d > 0 *  last_seq_no_sent=%d, last_sample_time=%.2f\n",
		NMXP_LOG_STR(p->pdlist[0]->network), NMXP_LOG_STR(p->pdlist[0]->station), NMXP_LOG_STR(p->pdlist[0]->channel),
		p->pdlist[0]->packet_type, p->pdlist[0]->seq_no,
		NMXP_LOG_STR(str_time), (double) p->pdlist[0]->nSamp / (double) p->pdlist[0]->sampRate,
		p->n_pdlist,
		p->last_seq_no_sent, p->last_sample_time);

	/* Changing values */
	p->last_seq_no_sent = p->pdlist[0]->seq_no - 1;
	p->last_sample_time = p->pdlist[0]->time;
	p->last_latency = nmxp_data_latency(p->pdlist[0]);
    }

    /* Manage array and execute func_pd() */
    j=0;
    send_again = 1;
    while(send_again  &&  j < p->n_pdlist) {
	send_again = 0;
	seq_no_diff = p->pdlist[j]->seq_no - p->last_seq_no_sent;
	time_diff = p->pdlist[j]->time - p->last_sample_time;
	latency = nmxp_data_latency(p->pdlist[j]);
	nmxp_data_to_str(str_time, p->pdlist[j]->time);
	if(seq_no_diff <= 0) {
	    /* Duplicated packets: Discarded */
	    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_RAWSTREAM,
		    "%s.%s.%s [%d, %d] (%s + %.2f sec.) * Packet discarded * seq_no_diff=%d  time_diff=%.2fs  lat %.1fs\n",
		    NMXP_LOG_STR(p->pdlist[j]->network), NMXP_LOG_STR(p->pdlist[j]->station), NMXP_LOG_STR(p->pdlist[j]->channel),
		    p->pdlist[j]->packet_type, p->pdlist[j]->seq_no, 
		    NMXP_LOG_STR(str_time), (double) p->pdlist[j]->nSamp / (double) p->pdlist[j]->sampRate,
		    seq_no_diff, time_diff, latency);
	    send_again = 1;
	    j++;
	} else if(seq_no_diff == 1) {
	    /* Handle current packet j */
	    for(i_func_pd=0; i_func_pd<n_func_pd; i_func_pd++) {
		(*p_func_pd[i_func_pd])(p->pdlist[j]);
	    }
	    if(time_diff > TIME_TOLLERANCE || time_diff < -TIME_TOLLERANCE) {
		nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_ANY,
			"%s.%s.%s [%d, %d] (%s + %.2f sec.) * Time is not correct * last_seq_no_sent=%d  seq_no_diff=%d  time_diff=%.2fs  lat. %.1fs\n",
		    NMXP_LOG_STR(p->pdlist[j]->network), NMXP_LOG_STR(p->pdlist[j]->station), NMXP_LOG_STR(p->pdlist[j]->channel), 
		    p->pdlist[j]->packet_type, p->pdlist[j]->seq_no,
		    str_time, (double) p->pdlist[j]->nSamp /  (double) p->pdlist[j]->sampRate,
		    p->last_seq_no_sent,
		    seq_no_diff, time_diff, latency);
	    }
	    p->last_seq_no_sent = p->pdlist[j]->seq_no;
	    p->last_sample_time = (p->pdlist[j]->time + ((double) p->pdlist[j]->nSamp / (double) p->pdlist[j]->sampRate ));
	    p->last_latency = nmxp_data_latency(p->pdlist[j]);
	    send_again = 1;
	    j++;
	} else {
	    nmxp_log(NMXP_LOG_WARN, NMXP_LOG_D_RAWSTREAM,
		    "%s.%s.%s [%d, %d] (%s + %.2f sec.) * seq_no_diff=%d > 1 * last_seq_no_sent=%d  j=%d  n_pdlist=%2d  time_diff=%.2fs  lat. %.1fs\n",
		    NMXP_LOG_STR(p->pdlist[j]->network), NMXP_LOG_STR(p->pdlist[j]->station), NMXP_LOG_STR(p->pdlist[j]->channel), 
		    p->pdlist[j]->packet_type, p->pdlist[j]->seq_no,
		    str_time, (double) p->pdlist[j]->nSamp /  (double) p->pdlist[j]->sampRate,
		    seq_no_diff, p->last_seq_no_sent, j, p->n_pdlist,
		    time_diff, latency);
	}
    }

    /* Shift and free j handled elements */
    if(j > 0) {
	for(k=0; k < p->n_pdlist; k++) {
	    if(k < j) {
		if(p->pdlist[k]->pDataPtr) {
		    NMXP_MEM_FREE(p->pdlist[k]->pDataPtr);
		    p->pdlist[k]->pDataPtr = NULL;
		}
		if(p->pdlist[k]) {
		    NMXP_MEM_FREE(p->pdlist[k]);
		    p->pdlist[k] = NULL;
                    /*nmxp_log(NMXP_LOG_ERR, NMXP_LOG_D_RAWSTREAM, "Freeing p->pdlist[%d]\n", k);*/
		}
	    }
	    if(k + j < p->n_pdlist) {
		p->pdlist[k] = p->pdlist[k+j];
                p->pdlist[k+j]=NULL;
	    } else {
		p->pdlist[k] = NULL;
	    }
	}
	p->n_pdlist = p->n_pdlist - j;
    }

    /* TOREMOVE
    nmxp_log(NMXP_LOG_NORM, NMXP_LOG_D_RAWSTREAM, "j=%d  p->n_pdlist=%d FINAL\n", j, p->n_pdlist);
       */

    return ret;
}


/* TODO */
int nmxp_raw_stream_manage_flush(NMXP_RAW_STREAM_DATA *p, int (*p_func_pd[NMXP_MAX_FUNC_PD]) (NMXP_DATA_PROCESS *), int n_func_pd) {
    int ret = 0;

    return ret;
}
