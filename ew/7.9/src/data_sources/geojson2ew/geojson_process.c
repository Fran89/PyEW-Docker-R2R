/*
	geojson2ew - geoJSON to earthworm 

	Copyright (c) 2014 California Institute of Technology.
	All rights reserved, November 6, 2014.
        This program is distributed WITHOUT ANY WARRANTY whatsoever.
        Do not redistribute this program without written permission.

	Authors: Kevin Frechette & Paul Friberg, ISTI.
*/
#include <stdio.h>
#include <string.h>
#include <jansson.h>
#include "earthworm.h"	/* need this for the logit() call */
#include "externs.h"
#include "trace_buf.h"		/* earthworm TRACE message definition */
#include "die.h"
#include "geojson_map.h"

#define CHANNELCODE_INDEX 2
#define GEOJSON_MAX_PATH_LEN 128
#define JSON_ARRAY_START_CHAR '['
#define JSON_ARRAY_END_CHAR ']'
#define JASON_PATH_SEPARATOR_CHAR '.'
#define SNCL_SEPARATOR_CHAR '.'
#define STR_EQUALS(S1, S2)	((S1 && S2) ? (strcmp(S1, S2) == 0) : ((S1) == (S2)))
#define STR_VALUE(S)			((S)?(json_string_value(S)):NULL)
#define STR_VALUE_FROM_OBJECT(O, K)	STR_VALUE(json_object_get(O, K))

static char jsonPathBuffer[GEOJSON_MAX_PATH_LEN];
static TracePacket trace_buffer;
static uint32_t * trace_data =
   (uint32_t *)&trace_buffer.msg[sizeof(TRACE2_HEADER)];
static TRACE2_HEADER *trace2_header = NULL;
static long trace_len;

TRACE2_HEADER * get_trace_header() {
   if (trace2_header == NULL) {
      memset(&trace_buffer, 0, sizeof(TracePacket));
      trace2_header = &trace_buffer.trh2;
      if (1 == htons(1)) // if big endian
         trace2_header->datatype[0] = 's';
      else
         trace2_header->datatype[0] = 'i';
      trace2_header->datatype[1] = '4';
      trace2_header->version[0] = TRACE2_VERSION0;
      trace2_header->version[1] = TRACE2_VERSION1;
      trace2_header->nsamp = 1;
      trace_len = sizeof(TRACE2_HEADER) +
         trace2_header->nsamp * sizeof(uint32_t);
   }
   return trace2_header;
}

json_t *getValueFromBuffer(json_t *object, char *path,
		const char *conditionPath) {
	char *s;

	for (s = path; *s != 0; s++) {
		if (*s == JSON_ARRAY_START_CHAR) {
			unsigned int arrayIndex = 0;

			*s = 0; // terminate
			++s; // skip array start character
			object = json_object_get(object, path);
			if (!json_is_array(object)) {
				return NULL;
			}

			if (*s == '*') {
				json_t *arrayValue;
				json_t *jsonValue;
				char sbuf[GEOJSON_MAX_PATH_LEN];

				s += 2; // skip wild card and array end character
				if (*s == JASON_PATH_SEPARATOR_CHAR) {
					*s = 0; // terminate
					++s; // skip separator
				}
				json_array_foreach(object, arrayIndex, arrayValue)
				{
					// need to copy so that each attempt gets original path
					strcpy(sbuf, s);
					jsonValue = getValueFromBuffer(arrayValue, sbuf,
							conditionPath);
					if (jsonValue != NULL) {
						// if condition path was specified
						if (conditionPath != NULL && conditionPath[0] != 0) {
							const char *stringValue;

							strcpy(sbuf, conditionPath);
							// find =
							char *valuePath = sbuf;
							while (*valuePath != '=') {
								valuePath++;
							}
							*valuePath = 0;
							valuePath++;
							stringValue = json_string_value(
									getValueFromBuffer(arrayValue, sbuf,
									NULL));
							if (stringValue == NULL
									|| strcmp(stringValue, valuePath) != 0) {
								continue;
							}
						}
						return jsonValue;
					}
				}
				return NULL;
			}

			while (*s != 0 && *s != JASON_PATH_SEPARATOR_CHAR) {
				if (*s != JSON_ARRAY_END_CHAR) {
					if (arrayIndex != 0)
						arrayIndex *= 10;
					arrayIndex += *s - '0';
				}
				s++;
			}
			if (*s == JASON_PATH_SEPARATOR_CHAR) {
				*s = 0; // terminate
				++s; // skip separator
			}
			object = json_array_get(object, arrayIndex);
			if (object == NULL) {
				return NULL;
			}
			if (*s == 0) {
				return object;
			}
			return getValueFromBuffer(object, s, conditionPath);
		}
		if (*s == JASON_PATH_SEPARATOR_CHAR || *s == 0) {
			if (*s == JASON_PATH_SEPARATOR_CHAR) {
				*s = 0; // terminate
				++s; // skip separator
			}
			object = json_object_get(object, path);
			if (object == NULL) {
				return NULL;
			}
			return getValueFromBuffer(object, s, conditionPath);
		}
	}
	return json_object_get(object, path);
}

json_t *getValue(json_t *object, const char *path, const char *conditionPath) {
	// copy to temporary buffer so that it can be modified
	strcpy(jsonPathBuffer, path);
	return getValueFromBuffer(object, jsonPathBuffer, conditionPath);
}

int setSncl(TRACE2_HEADER* th, const char *string) {
	char *dest;
	int count;
	// sta
	count = 0;
	for (dest = th->sta; *string != SNCL_SEPARATOR_CHAR;
			dest++, string++) {
		if (*string == 0)
			return 1;
		if (++count < TRACE2_STA_LEN) {
			*dest = *string;
		}
	}
	if (count == 0)
		return 1;
	string++; // skip separator
	// net
	count = 0;
	for (dest = th->net; *string != SNCL_SEPARATOR_CHAR;
			dest++, string++) {
		if (*string == 0)
			return 1;
		if (++count < TRACE2_NET_LEN) {
			*dest = *string;
		}
	}
	if (count == 0)
		return 1;
	string++; // skip separator
	// chan
	count = 0;
	for (dest = th->chan; *string != SNCL_SEPARATOR_CHAR;
			dest++, string++) {
		if (*string == 0)
			return 1;
		if (++count < TRACE2_CHAN_LEN) {
			*dest = *string;
		}
	}
	if (count <= CHANNELCODE_INDEX
			|| th->chan[CHANNELCODE_INDEX] != '_')
		return 1;
	string++; // skip separator
	// loc
	count = 0;
	for (dest = th->loc; *string != 0; dest++, string++) {
		if (++count < TRACE2_LOC_LEN) {
			*dest = *string;
		}
	}
	if (count == 0)
		return 1;
	return 0;
}

int process_geojson(char *msg, json_t *root)
{
   const char *path, *conditionPath;
   json_t * jsonValue;
   const char *string;
   TRACE2_HEADER *th = get_trace_header();
   int log_ewsent = Verbose & VERBOSE_EWSENT;

   // check TIME
   path = Map_time;
   conditionPath = NULL;
   jsonValue = getValue(root, path, conditionPath);
   if (!json_is_number(jsonValue))
      return 0; // ignore
   th->starttime = th->endtime =
      json_number_value(jsonValue) / 1000.;

   // check SNCL
   path = Map_sncl;
   conditionPath = NULL;
   jsonValue = getValue(root, path, conditionPath);
   if (!json_is_string(jsonValue))
      return 0; // ignore
   string = json_string_value(jsonValue);
   if (setSncl(th, string)) {
      logit("e", "ignoring invalid SNCL: %s\n", string);
      return 0; // ignore
   }
   if (VerboseSncl && (*VerboseSncl == '*' ||
       strcmp(VerboseSncl, string) == 0)) {
      if (!log_ewsent) log_ewsent = VERBOSE_EWSENT;
      logit("e", "SNCL: %s\n%s\n", string, msg);
   } else if (Verbose & VERBOSE_SNCL) {
      logit("e", "SNCL: %s\n", string);
   }

   // check SAMPLERATE
   path = Map_samplerate;
   conditionPath = NULL;
   jsonValue = getValue(root, path, conditionPath);
   if (!json_is_number(jsonValue))
      return 0; // ignore
   trace2_header->samprate = json_number_value(jsonValue);

   // check channels
   GEOJSON_MAP_ENTRY *eptr = get_channel();
   while (eptr != NULL) {
      path = eptr->path;
      conditionPath = eptr->conditionPath;
      jsonValue = getValue(root, path, conditionPath);
      if (json_is_number(jsonValue))
      {
         *trace_data = (json_number_value(jsonValue) * eptr->multiplier);
         th->chan[CHANNELCODE_INDEX] = eptr->channelCode;
         if (tport_putmsg(&Region, &DataLogo, trace_len, trace_buffer.msg) != PUT_OK) {
            logit("et", "%s: Fatal Error sending trace via tport_putmsg()\n",
                  Progname);
            return GEOJSON2EW_DEATH_EW_PUTMSG;
         } else if (log_ewsent) {
            logit("e",
               "SENT to EARTHWORM %s.%s.%s.%s value=%d samprate=%lf\n",
               th->sta, th->net, th->chan, th->loc, *trace_data,
               th->samprate);
         }
      }
      eptr = eptr->next;
   }
   return 0;
}

int process_json_message(char *msg) {
   const char *str;
   json_error_t error;
   json_t *root = json_loads(msg, 0, &error);
   int status = 0;
   // exit if error
   if (!root) {
      logit("e", "error: on line %d: %s\n", error.line, error.text);
      status = GEOJSON2EW_DEATH_SERVER_ERROR;
   } else {
      // if geoJSON
      str = STR_VALUE_FROM_OBJECT(root, "type");
      if (STR_EQUALS("FeatureCollection", str)) {
         status = process_geojson(msg, root);
      } else if (Verbose & VERBOSE_GENERAL) {
         logit("e", "skipping JSON type %s\n", str);
      }
      json_decref(root);
   }
   return status;
}
