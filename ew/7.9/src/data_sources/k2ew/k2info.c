#include <string.h>
#include <time.h>
#include "k2info.h"
#include "glbvars.h"
#include "terminat.h"

/*****************************************************************/
/* Some static values */

static MSG_LOGO	info_logo;

static K2infoPacket kip;

/*****************************************************************/

/* k2info_init() - initialize output of k2info packets into ring */
/* returns:    	0  upon success 
		-1 upon failure */

int k2info_init() 
{
        char netbuf[K2_NETBUFF_SIZE+2];

	/* initialize the logo */
	if (GetType(K2INFO_TYPE_STRING, &info_logo.type) != 0 )
        {
		return  -1;
	}
	info_logo.instid = g_instid_val;
	info_logo.mod = gcfg_module_idnum;

	/* the net and station will not change over a run */
	/* K2s can have a different network code for each channel,
           but k2info packets only know about one network code.
           So, we ship the network code for the first data stream.
           WMK 3/25/05 */
        memcpy( netbuf, &g_netcode_arr[0][0], K2_NETBUFF_SIZE );
        netbuf[K2_NETBUFF_SIZE - 1] = '\0';
	strcpy(kip.k2info.net, netbuf);
/*	strcpy(kip.k2info.net, gcfg_network_buff); */

	strcpy(kip.k2info.sta, g_stnid);

	return 0;
}

void k2info_send(int data_type, char *item) 
{
	
int output_size, cd;

	kip.k2info.epoch_sent = (uint32_t) time(0);
	kip.k2info.data_type = (short)data_type;
	output_size = sizeof(K2INFO_HEADER);

	/* bundle up the item */
	switch (data_type) {
	case K2INFO_TYPE_HEADER:
		memcpy(&kip.msg[sizeof(K2INFO_HEADER)], item,
			sizeof(K2_HEADER));
		output_size += sizeof(K2_HEADER);
		logit("et", "Injecting K2 header (%d) at %ld\n", 
				output_size, kip.k2info.epoch_sent);
		break;
	case K2INFO_TYPE_STATUS:
		memcpy(&kip.msg[sizeof(K2INFO_HEADER)], item,
			sizeof(struct STATUS_INFO));
		output_size += sizeof(struct STATUS_INFO);
		logit("et", "Injecting K2 STATUS PKT (%d) at %ld\n", 
				output_size, kip.k2info.epoch_sent);
		break;
	case K2INFO_TYPE_ESTATUS:
		memcpy(&kip.msg[sizeof(K2INFO_HEADER)], item,
			sizeof(struct EXT_STATUS_INFO));
		output_size += sizeof(struct EXT_STATUS_INFO);
		logit("et", "Injecting K2 EXT STATUS PKT (%d) at %ld\n", 
				output_size, kip.k2info.epoch_sent);
		break;
	case K2INFO_TYPE_E2STATUS:
		memcpy(&kip.msg[sizeof(K2INFO_HEADER)], item,
			sizeof(struct EXT2_STATUS_INFO));
		output_size += sizeof(struct EXT2_STATUS_INFO);
		logit("et", "Injecting K2 EXT2 STATUS PKT (%d) at %ld\n", 
				output_size, kip.k2info.epoch_sent);
		break;
	case K2INFO_TYPE_COMM:
		memcpy(&kip.msg[sizeof(K2INFO_HEADER)], item,
			sizeof(struct COMM_INFO));
		output_size += sizeof(struct COMM_INFO);
		logit("et", "Injecting K2EW COMM_INFO PKT (%d) at %ld\n", 
				output_size, kip.k2info.epoch_sent);
		break;
	}

	/* try and ship the sucker out */
        if ( (cd = tport_putmsg(&g_tport_region, &info_logo,
                                output_size, (char *)&kip)) != PUT_OK)
        {            /* 'put' function returned error code */
          k2ew_enter_exitmsg(K2TERM_EW_PUTMSG,   /* log & enter exit message */
                             "Earthworm 'tport_putmsg()' function failed (%d)",
                             cd);
          g_terminate_flg = 1;        /* set terminate flag for main thread */           
	}
}
void k2snw_send(char *param_value_msg) 
{
int output_size, cd;
char msg_str[2000];

	sprintf(msg_str, "%s-%s:1:%s\n", kip.k2info.net, kip.k2info.sta, param_value_msg);
	output_size = (int)strlen(msg_str);

        if ( (cd = tport_putmsg(&g_tport_region, &info_logo,
                                output_size, (char *)msg_str)) != PUT_OK)
        {            /* 'put' function returned error code */
          k2ew_enter_exitmsg(K2TERM_EW_PUTMSG,   /* log & enter exit message */
                             "Earthworm SNW 'tport_putmsg()' function failed (%d)",
                             cd);
          g_terminate_flg = 1;        /* set terminate flag for main thread */           
	}
}
