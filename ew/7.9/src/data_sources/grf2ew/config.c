/* $Id: config.c 6803 2016-09-09 06:06:39Z et $ */
/*-----------------------------------------------------------------------
    Copyright (c) 2000-2007 - DAQ Systems, LLC. - All rights reserved.
-------------------------------------------------------------------------

    Configuration for grf2ew.

-----------------------------------------------------------------------*/

#include "config.h"

/* Module constants ---------------------------------------------------*/
#define MAX_LINE_LENGTH					128
#define MAX_TOKEN_LENGTH				128

/* Module types -------------------------------------------------------*/

/* Module globals -----------------------------------------------------*/
static BOOL fatal_error;
static UINT32 line_number, token_number;
static CHAR *config_filespec;
static FILE *config_fptr;
static BOOL opened = FALSE;

/* Module macros ------------------------------------------------------*/

/* Module prototypes --------------------------------------------------*/
static BOOL ParseCommandLine(int argc, char *argv[], CMDLINE_ARGS * args);
static VOID CmdLineSetDefaults(CMDLINE_ARGS * args);
static VOID CmdLineHelp(void);

static BOOL ParseConfigFile(MAIN_ARGS * args, CHAR *conf_file);
static BOOL IsValidConfiguration(MAIN_ARGS * args);
static BOOL DefineLogging(MAIN_ARGS * args);
static BOOL DefineServer(MAIN_ARGS * args);
static BOOL DefineEarthworm(MAIN_ARGS * args);

static CHAR *GetNextLine(void);
static CHAR *GetNextToken(void);

/*---------------------------------------------------------------------*/
#include "getopt.c"

/*---------------------------------------------------------------------*/
BOOL Configure(MAIN_ARGS * args)
{
	CMDLINE_ARGS cmdline;

	ASSERT(args != NULL);

	/* Parse the command line first to get the config filename */
	CmdLineSetDefaults(&cmdline);
	if (!ParseCommandLine(args->argc, args->argv, &cmdline))
		exit(1);

	/* Parse the configuration file */
	MainSetDefaults(args);
	if (!ParseConfigFile(args, cmdline.confile))
		exit(1);

	/* Fold in command line options overriding config file settings */
	if (cmdline.message_format != ' ') 
		args->trace_buf2 = (cmdline.message_format == 2 ? TRUE : FALSE);
	if (cmdline.debug) 
		args->debug = cmdline.debug;
	if (cmdline.correct_rate) 
		args->correct_rate = cmdline.correct_rate;
	if (cmdline.ring[0] != '\0')
		strcpy(args->ring.name, cmdline.ring);
	if (cmdline.read_timeout != 0)
		args->read_timeout = cmdline.read_timeout;
	if (cmdline.heartbeat != 0)
		args->heartbeat = cmdline.heartbeat;
	if (cmdline.input_spec[0] != '\0') {
		strcpy(args->input_spec, cmdline.input_spec);
		memcpy(&args->endpoint, &cmdline.endpoint, sizeof(ENDPOINT));
	}

	/* Validate the configuration */
	return IsValidConfiguration(args);
}

/*---------------------------------------------------------------------*/
static BOOL IsValidConfiguration(MAIN_ARGS * args)
{
	ASSERT(args != NULL);

	/* Decide whether or not things are kosher... */

	/* An input specification must be defined... */
	if (args->input_spec[0] == '\0') {
		fprintf(stderr,"ERROR: No input GRF source defined!\n");
		return FALSE;
	}

	return TRUE;
}

/*---------------------------------------------------------------------*/
static BOOL ParseCommandLine(int argc, char *argv[], CMDLINE_ARGS * args)
{
	CHAR string[MAX_HOST_LEN + 1];
	int i, option;

	ASSERT(args != NULL);
	ASSERT(argv != NULL);

	/* Don't write errors to stderr */
	my_opterr = 0;

	/* Options... */
	while ((option = my_getopt(argc, argv, "hcds:t:H:f:r:")) != -1) {
		switch (option) {
		case 'h':
			CmdLineHelp();
            break;
		case 'd':
			args->debug = TRUE;
			break;
		case 'c':
			args->correct_rate = TRUE;
			break;
		case 's':
			if (my_optarg) {
				if (!ParseEndpoint(&args->endpoint, my_optarg, DFL_PORT))
					return FALSE;
				sprintf(args->input_spec, "%s (%s)", my_optarg, FormatEndpoint(&args->endpoint, string));
			}
			else {
				fprintf(stderr,"ERROR: No endpoint specified: '-s'\n");
				return FALSE;
			}
			break;
		case 't':
			if (my_optarg) {
				args->read_timeout = (USTIME)strtoul(my_optarg, NULL, 0) * UST_SECOND;
			}
			break;
		case 'T':
			if (my_optarg) {
				if (*my_optarg == '1' || *my_optarg == '2') {
					args->message_format = (CHAR)*my_optarg;
				}
			}
			break;
		case 'H':
			if (my_optarg) {
				args->heartbeat = (USTIME)strtoul(my_optarg, NULL, 0) * UST_SECOND;
			}
			break;
		case 'f':
			if (my_optarg)
				strncpy(args->input_spec, my_optarg, MAX_PATH_LEN);
			else {
				fprintf(stderr,"ERROR: No input file specified: '-f'\n");
				return FALSE;
			}
			break;
		case 'r':
			if (my_optarg)
				strncpy(args->ring, my_optarg, MAX_RING_NAME_LEN);
			else {
				fprintf(stderr,"ERROR: No output ring specified: '-o'\n");
				return FALSE;
			}
			break;
		case '?':
			if (isprint(my_optopt))
				fprintf(stderr, "Unknown option `-%c'.\n", my_optopt);
			else
				fprintf(stderr, "Unknown option character `\\x%x'.\n", my_optopt);
			CmdLineHelp();
			return FALSE;
		default:
			abort();
		}
	}

	/* Arguments... */
	for (i = my_optind; i < argc; i++) {
		if (args->confile[0] == '\0') {
			strcpy(args->confile, argv[i]);
		}
		else {
			fprintf(stderr, "unrecognized argument: `%s'\n", argv[i]);
			return FALSE;
		}
	}

	/* If we weren't passed a config filename, use the default */
	if (args->confile[0] == '\0')
		strcpy(args->confile, DFL_CONF_FILE);

	return TRUE;
}

/*---------------------------------------------------------------------*/
static VOID CmdLineSetDefaults(CMDLINE_ARGS * args)
{
	ASSERT(args != NULL);

	/* All args are undefined... */
	args->debug = FALSE;
	args->correct_rate = FALSE;
	args->daemon = FALSE;
	args->message_format = ' ';
	args->read_timeout = 0;
	args->heartbeat = 0;
	args->input_spec[0] = '\0';
	args->ring[0] = '\0';
	args->facility[0] = '\0';
	args->logfile[0] = '\0';
	args->confile[0] = '\0';

	return;
}

/*---------------------------------------------------------------------*/
static VOID CmdLineHelp(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Usage: grf2ew [-hdc] [-s ip_addr[:port] | -f input_file]\n");
	fprintf(stderr, "           [-t seconds] [-r output_ring] [-H seconds]\n");
	fprintf(stderr, "           [-T 1|2] [configuration file]\n");
	fprintf(stderr, "\n");
    fprintf(stderr, "   -h  Help display.\n");
	fprintf(stderr, "   -d  Debug logging.\n");
	fprintf(stderr, "   -c  Apply sampling rate corrections (No).\n");
	fprintf(stderr, "   -s  IP address and port number to connect to for input data.\n");
	fprintf(stderr, "          port number defaults to %u if not otherwise specified.\n", DFL_PORT);
	fprintf(stderr, "   -f  File to read for input data.\n");
	fprintf(stderr, "   -t  Socket read timeout in seconds. (%d)\n", (INT32)(DFL_TIMEOUT / UST_SECOND));
	fprintf(stderr, "   -r  Output ring for messages. (%s)\n", DFL_RING);
	fprintf(stderr, "   -T  Output message format, 1=TRACE_BUF or 2=TRACE_BUF2. (2)\n");
	fprintf(stderr, "   -H  Heartbeat interval in seconds. (%d).\n", (INT32)(DFL_HEARTBEAT / UST_SECOND));
    fprintf(stderr, "\n");
	fprintf(stderr, "The configuration file argument defaults to '%s' if not otherwise specified.\n", DFL_CONF_FILE);
    fprintf(stderr, "\n");
    fprintf(stderr, "[] = optional, () = default, | = mutually exclusive.\n");

	exit(0);
}

/*---------------------------------------------------------------------*/
static BOOL ParseConfigFile(MAIN_ARGS * args, CHAR *conf_file)
{
	CHAR *token;
	CHAR *ew_params;
	CHAR fq_config_filespec[MAX_PATH_LEN + 1];

	ASSERT(args != NULL);
	ASSERT(conf_file != NULL);

	ew_params = getenv("EW_PARAMS");
	if (ew_params != NULL) {
		strcpy(fq_config_filespec, ew_params);
		if (fq_config_filespec[strlen(fq_config_filespec) - 1] != '/' &&
				fq_config_filespec[strlen(fq_config_filespec) - 1] != '\\') {
			strcat(fq_config_filespec, "/");
		}
		strcat(fq_config_filespec, conf_file);
		config_filespec = fq_config_filespec;
	} else {
		config_filespec = conf_file;
	}
	line_number = token_number = 0;
	fatal_error = FALSE;

	while ((token = GetNextToken()) != NULL) {
		if (strncasecmp(token, "Logging", 7) == 0) {
			if (!DefineLogging(args))
				break;
		} else if (strncasecmp(token, "Server", 6) == 0) {
			if (!DefineServer(args))
				break;
		} else if (strncasecmp(token, "Earthworm", 9) == 0) {
			if (!DefineEarthworm(args))
				break;
		} else {
			fprintf(stderr,"Out of scope or unrecognized statement at %s line %lu: '%s'\n",
				config_filespec, (unsigned long)line_number, token);
			fatal_error = TRUE;
			break;
		}
	}

	if (opened) {
		fclose(config_fptr);
		opened = FALSE;
	}

	if (fatal_error)
		return FALSE;

	return TRUE;
}

/*---------------------------------------------------------------------*/
static BOOL DefineLogging(MAIN_ARGS * args)
{
	CHAR *token;

	ASSERT(args != NULL);

	fprintf(stderr,"grf2ew: The configuration file logging group is now deprecated as grf2ew now uses Earthworm logging facility\n");

	/* Find scoping token */
	if ((token = GetNextToken()) == NULL)
		return (FALSE);
	if (*token != '{') {
		fprintf(stderr,"Expected '{' at %s line %lu: '%s'\n", config_filespec, (unsigned long)line_number, token);
		fatal_error = TRUE;
		return (FALSE);
	}

	/* Process statements in scope */
	while ((token = GetNextToken()) != NULL) {
		if (strncasecmp(token, "Syslog", 6) == 0) {
			if ((token = GetNextToken()) == NULL)
				break;
		} else if (strncasecmp(token, "File", 4) == 0) {
			if ((token = GetNextToken()) == NULL)
				break;
		} else if (strncasecmp(token, "Level", 5) == 0) {
			if ((token = GetNextToken()) == NULL)
				break;
			else if (toupper(*token) == 'D')
				args->debug = TRUE;
		} else if (*token == '}') {
			break;
		} else {
			fprintf(stderr,"Out of scope or unrecognized statement at %s line %lu: '%s'\n",
				config_filespec, (unsigned long)line_number, token);
			fatal_error = TRUE;
			return (FALSE);
		}
	}

	return TRUE;
}

/*---------------------------------------------------------------------*/
static BOOL DefineServer(MAIN_ARGS * args)
{
	CHAR string[MAX_HOST_LEN + 1];
	CHAR *token;

	ASSERT(args != NULL);

	/* Find scoping token */
	if ((token = GetNextToken()) == NULL)
		return (FALSE);
	if (*token != '{') {
		fprintf(stderr,"Expected '{' at %s line %lu: '%s'\n", config_filespec, (unsigned long)line_number, token);
		fatal_error = TRUE;
		return (FALSE);
	}

	while ((token = GetNextToken()) != NULL) {
		if (strncasecmp(token, "Endpoint", 8) == 0) {
			if ((token = GetNextToken()) == NULL)
				break;
			ParseEndpoint(&args->endpoint, token, DFL_PORT);
			sprintf(args->input_spec, "%s (%s)", token, FormatEndpoint(&args->endpoint, string));
		} else if (strncasecmp(token, "ReadTimeout", 11) == 0) {
			if ((token = GetNextToken()) == NULL)
				break;
			args->read_timeout = UST_SECOND * strtoul(token, NULL, 0);
		} else if (*token == '}') {
			break;
		} else {
			fprintf(stderr,"Out of scope or unrecognized statement at %s line %lu: '%s'\n",
				config_filespec, (unsigned long)line_number, token);
			fatal_error = TRUE;
			return (FALSE);
		}
	}

	return TRUE;
}

/*---------------------------------------------------------------------*/
static BOOL DefineEarthworm(MAIN_ARGS * args)
{
	CHAR *token;

	ASSERT(args != NULL);

	/* Find scoping token */
	if ((token = GetNextToken()) == NULL)
		return (FALSE);
	if (*token != '{') {
		fprintf(stderr,"Expected '{' at %s line %lu: '%s'\n", config_filespec, (unsigned long)line_number, token);
		fatal_error = TRUE;
		return (FALSE);
	}

	/* Process statements in scope */
	while ((token = GetNextToken()) != NULL) {
		if (strncasecmp(token, "Ring", 4) == 0) {
			if ((token = GetNextToken()) == NULL)
				break;
			strncpy(args->ring.name, token, MAX_RING_NAME_LEN);
		} else if (strncasecmp(token, "MessageFormat", 13) == 0) {
			if ((token = GetNextToken()) == NULL)
				break;
			if (strncasecmp(token, "TRACE_BUF2", 10) == 0) {
				args->trace_buf2 = TRUE;
			} else if (strncasecmp(token, "TRACE_BUF", 9) == 0) {
				args->trace_buf2 = FALSE;
			}
		} else if (strncasecmp(token, "Module", 6) == 0) {
			if ((token = GetNextToken()) == NULL)
				break;
			strncpy(args->ring.module, token, MAX_RING_NAME_LEN);
		} else if (strncasecmp(token, "Heartbeat", 9) == 0) {
			if ((token = GetNextToken()) == NULL)
				break;
			args->heartbeat = UST_SECOND * strtoul(token, NULL, 0);
		} else if (strncasecmp(token, "InstallationID", 14) == 0) {
			if ((token = GetNextToken()) == NULL)
				break;
			strncpy(args->ring.inst_id, token, MAX_RING_NAME_LEN);
		} else if (strncasecmp(token, "CorrectRate", 11) == 0) {
			if ((token = GetNextToken()) == NULL)
				break;
			if (toupper(*token) == 'Y' || toupper(*token) == 'T' || *token == '1')
				args->correct_rate = TRUE;
			else
				args->correct_rate = FALSE;
		} else if (strncasecmp(token, "MinTimeQuality", 14) == 0) {
			if ((token = GetNextToken()) == NULL)
				break;
			args->min_quality = (UINT8)strtoul(token, NULL, 0);
			if (args->min_quality > GRF_TIME_MAX)
				args->min_quality = GRF_TIME_MAX;
		} else if (*token == '}') {
			break;
		} else {
			fprintf(stderr,"Out of scope or unrecognized statement at %s line %lu: '%s'\n",
				config_filespec, (unsigned long)line_number, token);
			fatal_error = TRUE;
			return (FALSE);
		}
	}

	return TRUE;
}

/*---------------------------------------------------------------------*/
CHAR *GetNextLine(void)
{
	static CHAR line_buffer[MAX_LINE_LENGTH + 1];
	BOOL quote;
	CHAR *cptr = NULL;

	if (!opened) {
		if ((config_fptr = fopen(config_filespec, "r")) == NULL) {
			fprintf(stderr,"ERROR: fopen(%s) failed: %s\n", config_filespec, strerror(errno));
			//fatal_error = TRUE;
			return (NULL);
		}
		line_number = 0;
		opened = TRUE;
	}

	/* Read and cleanup net line... */
	memset(line_buffer, '\0', MAX_LINE_LENGTH);
	while (fgets(line_buffer, MAX_LINE_LENGTH, config_fptr) != NULL) {
		line_number++;
		token_number = 0;

		/* Trim trailing crap (CR, LF, white space, etc...) */
		cptr = line_buffer + MAX_LINE_LENGTH;
		while (!(isalnum((int)(*cptr)) || ispunct((int)(*cptr))) && cptr >= line_buffer) {
			*cptr = '\0';
			cptr--;
		}
		if (cptr < line_buffer) {
			cptr = NULL;
			continue;
		}

		/* Look for and dispose of comments anywhere in the line */
		quote = FALSE;
		cptr = line_buffer;
		while (*cptr) {
			if (*cptr == '"') {
				if (quote)
					quote = FALSE;
				else
					quote = TRUE;
			}
			if (!quote && *cptr == '#') {
				do {
					*cptr = '\0';
					if (cptr == line_buffer)
						break;
					cptr--;
				}
				while (!(isalnum((int)(*cptr)) || ispunct((int)(*cptr))));
				break;
			}
			cptr++;
		}

		if (line_buffer[0] == '\0') {
			cptr = NULL;
			memset(line_buffer, '\0', MAX_LINE_LENGTH);
			continue;
		}

		/* Trim off leading whitespace */
		cptr = line_buffer;
		while (!(isalnum((int)(*cptr)) || ispunct((int)(*cptr))) && *cptr != '\0')
			cptr++;

		/* Success */
		return (cptr);
	}

	/* Check for read error... */
	if (ferror(config_fptr)) {
		fprintf(stderr,"ERROR: Reading config file: %s\n", strerror(errno));
		fatal_error = TRUE;
	}

	//fclose(config_fptr);

	return (NULL);
}

/*---------------------------------------------------------------------*/
CHAR *GetNextToken(void)
{
	static CHAR *cptr = NULL, token[MAX_TOKEN_LENGTH + 1];
	BOOL quote;
	UINT16 i;

	quote = FALSE;

	while (TRUE) {
		if (!cptr) {
			if ((cptr = GetNextLine()) == NULL)
				break;
		}
		else {
			while (*cptr && (isalnum((int)(*cptr)) || ispunct((int)(*cptr))))
				cptr++;
		}

		while (*cptr && !isalnum((int)(*cptr)) && !ispunct((int)(*cptr)))
			cptr++;

		if (*cptr) {
			if (*cptr == '"') {
				cptr++;
				quote = TRUE;
			}
			i = 0;
			while (i < MAX_TOKEN_LENGTH && cptr[i] && cptr[i] != ',' && 
					(isalnum((int)cptr[i]) || ispunct((int)cptr[i]) || quote)) {
				if (quote && cptr[i] == '"') {
					cptr += i;
					break;
				}
				token[i] = cptr[i];
				i++;
			}
			token[i] = '\0';
			token_number++;
			return (token);
		}
		else
			cptr = NULL;
	}

	return (cptr);
}

/*---------------------------------------------------------------------*/
VOID DumpConfiguration(MAIN_ARGS * args)
{

	ASSERT(args != NULL);

	if (!args->debug)
		return;

	logit("ot", "grf2ew: Configuration dump:\n");
	logit("ot", "grf2ew: Logging:\n");
	logit("ot", "grf2ew:  Debug:           %s\n", (args->debug ? "Yes" : "No"));
	logit("ot", "grf2ew: Data:\n");
	logit("ot", "grf2ew:  GRF source:      %s\n", args->input_spec);
	logit("ot", "grf2ew:  Read timeout:    %u seconds\n", (UINT32)(args->read_timeout / UST_SECOND));
	logit("ot", "grf2ew: Earthworm:\n");
	logit("ot", "grf2ew:  Output ring:     %s\n", args->ring.name);
	logit("ot", "grf2ew:  Message format:  %s\n", (args->trace_buf2 ? "TRACE_BUF2" : "TRACE_BUF"));
	logit("ot", "grf2ew:  Installaion ID:  %s\n", args->ring.inst_id);
	logit("ot", "grf2ew:  Module name:     %s\n", args->ring.module);
	logit("ot", "grf2ew:  Heartbeat:       %u seconds\n", (UINT32)(args->heartbeat / UST_SECOND));
	logit("ot", "grf2ew:  Correcting rate: %s\n", (args->correct_rate ? "Yes" : "No"));

	return;
}
