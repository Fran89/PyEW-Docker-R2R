/* @(#)params.c    1.2 07/21/98 */
/*======================================================================
 *
 *  Read import_rtp.d configuration file
 *
 * Revised:
 *   12Jul06  (rs) - use default rates that are typical of reftek 
 *                   equipment
 *    2Aug06  (rs) - provide means to configure if out of order packets
 *                   should be ignored
 *====================================================================*/
#include "import_rtp.h"

#define DELIMITERS " \t"
#define MAX_TOKEN    32

static BOOL fail(CHAR *prog, FILE *fp, CHAR *path, CHAR *token, INT32 lineno)
{
    fprintf(stderr, "%s: FATAL ERROR: syntax error at line ", prog);
    fprintf(stderr, "%ld, file `%s', token `%s'\n", lineno, path, token);
    fclose(fp);
    return FALSE;
}

BOOL read_params(
    CHAR *prog, CHAR *path, CHAR *buffer, INT32 len, struct param *par
){
FILE *fp;
UINT16 status, ntok;
INT32 lineno;
CHAR *token[MAX_TOKEN];
static CHAR *default_host = DEFAULT_HOST;

double DefaultAcceptableSampleRates[] = {40.0,50.0,100.0,200.0};
/* Initialize parameter structure */

    par->MyModName        = (CHAR *) NULL;
    par->WavRing.defined  = FALSE;
    par->RawRing.defined  = FALSE;
    par->SCNLFile         = (CHAR *) NULL;
    par->hbeat            = DEFAULT_HEARTBEAT;
    par->nodata           = DEFAULT_NODATA_ALARM;
    par->host             = default_host;
    par->port             = DEFAULT_PORT;
    par->attr             = RTP_DEFAULT_ATTR;
    par->attr.at_block    = TRUE;
    par->debug            = 0;
    par->retry            = RTP_ERR_NONFATAL; /* not a user option */

    par->SendUnknownChan  = TRUE;
    par->TimeJumpTolerance= 86400;  /* 1 day in seconds */
    par->SendTimeTearPackets= TRUE;
    par->iNumAcceptableSampleRates = sizeof(DefaultAcceptableSampleRates) / sizeof(double);
    memcpy(&par->AcceptableSampleRates, 
           DefaultAcceptableSampleRates, 
           sizeof(DefaultAcceptableSampleRates)); 
    par->FilterOnSampleRate                 = TRUE;
    par->DropPacketsOutOfOrder              = TRUE;
    par->DropPacketsWithDecompressionErrors = TRUE;
    par->GuessNominalSampleRate             = TRUE;

/* Open configuration file */

    if ((fp = fopen(path, "r")) == (FILE *) NULL) {
        fprintf(stderr, "%s: FATAL ERROR: fopen: ", prog);
        perror(path);
        return FALSE;
    }

/* Read configuration file */

    lineno = 0;
    while ((status = util_getline(fp, buffer, len, '#', &lineno)) == 0) {

        ntok = util_parse(buffer, token, DELIMITERS, MAX_TOKEN, 0);

        if (strcasecmp(token[0], "MyModuleId") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            if ((par->MyModName = strdup(token[1])) == (char *) NULL) {
                fprintf(stderr, "%s: FATAL ERROR: ", prog);
                perror("strdup");
                fclose(fp);
                return FALSE;
            }
            if (GetModId(par->MyModName, &par->Mod) != 0) {
                fprintf(stderr, "%s: FATAL ERROR: unknown ",prog);
                fprintf(stderr, "ModuleId `%s'\n", par->MyModName);
                fclose(fp);
                return FALSE;
            }

        } else if (
            strcasecmp(token[0], "WavRing")  == 0 ||
            strcasecmp(token[0], "WaveRing") == 0
        ) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->WavRing.name = (CHAR *) strdup(token[1]);
            if (par->WavRing.name == (CHAR *) NULL) {
                fprintf(stderr, "%s: FATAL ERROR: ", prog);
                perror("strdup");
                fclose(fp);
                return FALSE;
            }
            par->WavRing.defined = TRUE;

        } else if (strcasecmp(token[0], "RawRing") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->RawRing.name = (CHAR *) strdup(token[1]);
            if (par->RawRing.name == (CHAR *) NULL) {
                fprintf(stderr, "%s: FATAL ERROR: ", prog);
                perror("strdup");
                fclose(fp);
                return FALSE;
            }
            par->RawRing.defined = TRUE;

        } else if (strcasecmp(token[0], "HeartBeatInterval") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->hbeat = (INT32) atoi(token[1]);
            if (par->hbeat < 1) {
                fprintf(stderr, "%s: FATAL ERROR: illegal ", prog);
                fprintf(stderr, "HeartBeatInterval `%d'\n", token[1]);
                fclose(fp);
                return FALSE;
            }

        } else if (strcasecmp(token[0], "DebugLevel") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->debug = (UINT16) atoi(token[1]);

        } else if (strcasecmp(token[0], "SCNLFile") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->SCNLFile = (CHAR *) strdup(token[1]);
            if (par->SCNLFile == (CHAR *) NULL) {
                fprintf(stderr, "%s: FATAL ERROR: ", prog);
                perror("strdup");
                fclose(fp);
                return FALSE;
            }

        } else if (strcasecmp(token[0], "Server") == 0) {
            if (ntok != 3) return fail(prog,fp,path,token[0],lineno);
            par->host = (CHAR *) strdup(token[1]);
            if (par->host == (CHAR *) NULL) {
                fprintf(stderr, "%s: FATAL ERROR: ", prog);
                perror("strdup");
                fclose(fp);
                return FALSE;
            }
            par->port = (UINT16) atoi(token[2]);
            if (par->port < 1024) {
                fprintf(stderr, "%s: FATAL ERROR: illegal ", prog);
                fprintf(stderr, "port number `%d'\n", token[2]);
                fclose(fp);
                return FALSE;
            }

        } else if (strcasecmp(token[0], "DASid") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->attr.at_dasid = (UINT32)strtoul(token[1],(char **)NULL,16);

        } else if (strcasecmp(token[0], "PktMask") == 0) {
            if (!rtp_encode_pmask(&token[1], (UINT16) (ntok - 1), &par->attr.at_pmask)) {
                fprintf(stderr, "%s: FATAL ERROR: illegal ", prog);
                fprintf(stderr, "PktMask specification\n");
                fclose(fp);
                return FALSE;
            }

        } else if (strcasecmp(token[0], "StrMask") == 0) {
            if (!rtp_encode_smask(&token[1], (UINT16) (ntok - 1), &par->attr.at_smask)) {
                fprintf(stderr, "%s: FATAL ERROR: illegal ", prog);
                fprintf(stderr, "StrMask specification\n");
                fclose(fp);
                return FALSE;
            }

        } else if (strcasecmp(token[0], "SendUnknownChan") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->SendUnknownChan = (INT32) atoi(token[1]);

        } else if (strcasecmp(token[0], "TimeJumpTolerance") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->TimeJumpTolerance = (INT32) atoi(token[1]);

        /* THESE PARAMS ARE NOW HARDCODED ******** DavidK 06/08/2004
        * } else if (strcasecmp(token[0], "FilterOnSampleRate") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->FilterOnSampleRate = (INT32) atoi(token[1]);

        * } else if (strcasecmp(token[0], "GuessNominalSampleRate") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->GuessNominalSampleRate = (INT32) atoi(token[1]);
        ************************/

        } else if (strcasecmp(token[0], "DropPacketsOutOfOrder") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->DropPacketsOutOfOrder = (INT32) atoi(token[1]);

        } else if (strcasecmp(token[0], "DropPacketsWithDecompressionErrors") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->DropPacketsWithDecompressionErrors = (INT32) atoi(token[1]);

        } else if (strcasecmp(token[0], "SendTimeTearPackets") == 0) {
            if (ntok != 2) return fail(prog,fp,path,token[0],lineno);
            par->SendTimeTearPackets = (INT32) atoi(token[1]);

        } else if (strcasecmp(token[0], "AcceptableSampleRates") == 0) {
            int i;
            if (ntok < 2) return fail(prog,fp,path,token[0],lineno);

            par->iNumAcceptableSampleRates = 0;
            for(i=1; i < ntok; i++)
            {
              if(i > MAX_SAMPLE_RATES)
              {
                fprintf(stderr, "Param File: AcceptableSampleRates: too many specified.\n" 
                        "\t Max %d supported, but %d specified.\n",
                        MAX_SAMPLE_RATES, ntok);
                return fail(prog,fp,path,token[i],lineno);
              }
              if(atof(token[i]) <= 0.0)
              {
                return fail(prog,fp,path,token[i],lineno);
              }
              par->AcceptableSampleRates[i-1] = atof(token[i]);
              par->iNumAcceptableSampleRates++;
            }

        } else {
            fprintf(stderr, "%s: FATAL ERROR: unrecognized ", prog);
            fprintf(stderr, "token `%s'\n", token[0]);
            fclose(fp);
            return FALSE;
        }
    }  /* end while  - read each line in config file */

    fclose(fp);

    if (status != 1) {
        fprintf(stderr, "%s: FATAL ERROR: %s: ", prog, path);
        perror("read");
        return FALSE;
    }

    if (par->MyModName == (CHAR *) NULL) {
        fprintf(stderr, "%s: FATAL ERROR: %s: ", prog, path);
        fprintf(stderr, "missing MyModName\n");
        return FALSE;
    }

    if (!par->WavRing.defined && !par->RawRing.defined) {
        fprintf(stderr, "%s: FATAL ERROR: %s: ", prog, path);
        fprintf(stderr, "neither WavRing nor RawRing given\n");
        return FALSE;
    }

    if (par->WavRing.defined) {
        if ((par->WavRing.key = GetKey(par->WavRing.name)) < 0) {
            fprintf(stderr, "%s: FATAL ERROR: unknown ",prog);
            fprintf(stderr, "WavRing `%s'\n", par->WavRing.name);
            return FALSE;
        }
        if (par->SCNLFile == (CHAR *) NULL) {
            fprintf(stderr, "%s: WARNING: %s: ", prog, path);
            fprintf(stderr, "WavRing w/o SCNLFile, noted\n");
        } else {
            if (!read_scnlfile(prog, par->SCNLFile, buffer, len) != 0) {
                fprintf(stderr, "%s: FATAL ERROR: ", prog);
                fprintf(stderr, "unable to load SCNL file");
                fprintf(stderr, "%s\n", par->SCNLFile);
                return FALSE;
            }
        }
    }

    if (par->RawRing.defined) {
        if ((par->RawRing.key = GetKey(par->RawRing.name)) < 0) {
            fprintf(stderr, "%s: FATAL ERROR: unknown ",prog);
            fprintf(stderr, "RawRing `%s'\n", par->RawRing.name);
            return FALSE;
        }
    }

    if (par->FilterOnSampleRate && (!par->GuessNominalSampleRate))
    {
        fprintf(stderr, "Setting SendUnkownChan to FALSE, because "
                        "FilterOnSampleRate is TRUE and "
                        "GuessNominalSampleRate is FALSE!\n");
        par->SendUnknownChan = FALSE;
    }

    return TRUE;
}

VOID log_params(CHAR *prog, CHAR *fname, struct param *par)
{
    CHAR pbuf[RTP_MINPMASKBUFLEN];
    CHAR sbuf[RTP_MINSMASKBUFLEN];
    int i;

    logit("t", "%s version %s\n", prog, VERSION_ID);
    logit("t", "Parameter file %s loaded\n", fname);
    logit("t", "MyModuleId           %s (%d)\n", par->MyModName, par->Mod);
    if (par->WavRing.defined) {
        logit("t", "WaveRing             %s (%d)\n",
            par->WavRing.name, par->WavRing.key
        );
    } else {
        logit("t", "WaveRing             <not given>\n");
    }
    if (par->RawRing.defined) {
        logit("t", "RawRing              %s (%d)\n",
            par->RawRing.name, par->RawRing.key
        );
    } else {
        logit("t", "RawRing              <not given>\n");
    }
    logit("t", "Server               %s %hu\n",
        par->host, par->port
    );
    logit("t", "DASid                %04X\n", par->attr.at_dasid);
    logit("t", "PktMask              %s\n",
        rtp_decode_pmask(par->attr.at_pmask, pbuf)
    );
    logit("t", "StrMask              %s\n",
        rtp_decode_smask(par->attr.at_smask, sbuf)
    );
    logit("t","TimeJumpTolerance    %d sec\n", par->TimeJumpTolerance );
    logit("t","SendUnknownChan      %d\n", par->SendUnknownChan );
    logit("t","SendTimeTearPackets  %d\n", par->SendTimeTearPackets );
    logit("t","DropPacketsWithDecompressionErrors   %d\n", 
          par->DropPacketsWithDecompressionErrors );
    logit("t","DropPacketsOutOfOrder   %d\n",par->DropPacketsOutOfOrder );
    logit("t","FilterOnSampleRate      %d (hardcoded)\n", par->FilterOnSampleRate );
    logit("t","GuessNominalSampleRate  %d (hardcoded)\n", par->GuessNominalSampleRate );
    logit("t","AcceptableSampleRates " );
    for( i=0; i<par->iNumAcceptableSampleRates; i++ ) 
         logit("","  %.2lf", par->AcceptableSampleRates[i] );
    logit("","\n" );

    if (par->SCNLFile != (CHAR *) NULL) {
        logit("t", "SCNLFile                %s\n", par->SCNLFile);
        LogSCNLFile();
    } else {
        logit("t", "SCNLFile                <not given>\n");
    }
}

#ifdef DEBUG_TEST

#define BUFFER_LEN 256

int main(argc, argv)
int argc;
char *argv[];
{
CHAR buffer[BUFFER_LEN];
CHAR pbuf[RTP_MINPMASKBUFLEN];
CHAR sbuf[RTP_MINSMASKBUFLEN];
struct param par;

    if (argc != 2) exit(1);

    if (!read_params(argv[0], argv[1], buffer, BUFFER_LEN, &par)) {
        fprintf(stderr, "%s: read_params failed\n", argv[0]);
        exit(1);
    }

    printf("%s version %s\n", argv[0], VERSION_ID);
    printf("Parameter file %s loaded\n", argv[1]);
    printf("MyModuleId        %s (%d)\n", par.MyModName, par.Mod);
    if (par.WavRing.defined) {
        printf("WavRing           %s (%d)\n",
            par.WavRing.name, par.WavRing.key
        );
    } else {
        printf("WavRing           <not given>\n");
    }
    if (par.RawRing.defined) {
        printf("RawRing           %s (%d)\n",
            par.RawRing.name, par.RawRing.key
        );
    } else {
        printf("RawRing           <not given>\n");
    }
    if (par.SCNLFile != (CHAR *) NULL) {
        printf("SCNLFile           %s\n", par.SCNLFile);
    } else {
        printf("SCNLFile           <not given>\n");
    }
    printf("SendUnknownChan   %d\n", par->SendUnknownChan );

    printf("Server            %s %hu\n",
        par.host, par.port
    );
    printf("DASid             %04X\n", par.attr.at_dasid);
    printf("PktMask           %s\n",
        rtp_decode_pmask(par.attr.at_pmask, pbuf)
    );
    printf("StrMask           %s\n",
        rtp_decode_smask(par.attr.at_smask, sbuf)
    );

    if (par.SCNLFile != (CHAR *) NULL) {
        printf("\n");
        printf("Dump of SCNL file\n");
        list_scnlfile();
    }

    exit(0);
}

#endif /* DEBUG_TEST */
