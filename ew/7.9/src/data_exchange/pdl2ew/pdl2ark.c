#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pdl2ew.h"

#define ptrAfter(fld) ((char*)(fld)+sizeof(fld))

char *myargs[] = {
    "--property-eventtime=",
    "--property-latitude=",
    "--property-longitude=",
    "--property-depth=",
    "--property-magnitude=",
    "--property-num-phases-used=",
    "--property-azimuthal-gap=",
    "--property-minimum-distance=",
    "--property-horizontal-error=",
    "--property-vertical-error=",
    "--code=",
    "--property-version=",
    "--property-magnitude-type="
    };
enum myarg_idx {
    ORIGIN_TIME_ARG,
    LATITUDE_ARG,
    LONGITUDE_ARG,
    DEPTH_ARG,
    MAGNITUDE_ARG,
    NUM_PHASES_ARG,
    AZIMUTH_GAP_ARG,
    MIN_DIST_ARG,
    HORZ_ERR_ARG,
    VERT_ERR_ARG,
    EVENTID_ARG,
    VERSION_ARG,
    MAGTYPE_ARG
};
    
#define VERSION_STR "0.9.5 - 2015-11-13"

/*
 *
 */
int main( int argc, char** argv ) {
    int i;
    double vals[3] = {0,0,0};
    FILE* f;
    char codaSet = 0, isCoda;        
    ARCDATA msg;
    int reqd = 1;
    
    memset( &msg, ' ', sizeof(msg) );
    msg.term = 0;
    f = fopen( argv[2], "a" );

    if ( argc < 3 ) {
      fprintf(stderr, "Usage: pdl2ark <dest directory> <path to logfile> <data-arg1> <data-arg2> ...\n");
      fprintf(stderr, "Version: %s\n", VERSION_STR );
      exit(1);
    }
    for ( i=4; i<argc; i++ ) {
        int quoted = (argv[i][0] == '\"') ? 1 : 0;
        char* test_arg = argv[i] + quoted;
        if ( strcmp( argv[i], "--delete" ) == 0 ) {
            puts("deleting");
            return 0;
        }
        if ( strncmp( myargs[ORIGIN_TIME_ARG], test_arg, strlen(myargs[ORIGIN_TIME_ARG]) ) == 0 ) {
            char *ot = msg.originTime;
            char *p = argv[i]+strlen(myargs[ORIGIN_TIME_ARG]);
            while ( *p && (ot-msg.originTime<16) ) {
                if ( isdigit(*p) ) 
                    *(ot++) = *p;
                p++;
            }
            //fprintf( f, "%s\n", argv[i] );
        } else if ( strncmp( myargs[LATITUDE_ARG], test_arg, strlen(myargs[LATITUDE_ARG]) ) == 0 ) {
            vals[1] = strtod( test_arg + strlen(myargs[LATITUDE_ARG]), NULL );
            double val = fabs(vals[1]);
            double deg = floor(val);
            double min = 60.0 * (val - deg);
            char *tobesaved = ptrAfter(msg.latitude);
            char save = *tobesaved;
            sprintf( msg.latitude, "%2.0lf%c%04.0lf", deg, vals[1]>=0 ? 'N' : 'S', min*100 );
            *tobesaved = save;
            //fprintf( f, "%s\n", argv[i] );
        } else if ( strncmp( myargs[LONGITUDE_ARG], test_arg, strlen(myargs[LONGITUDE_ARG]) ) == 0 ) {
            vals[2] = strtod( test_arg + strlen(myargs[LONGITUDE_ARG]), NULL );;
            double val = fabs(vals[2]);
            double deg = floor(val);
            double min = 60.0 * (val - deg);
            char *tobesaved = ptrAfter(msg.longitude);
            char save = *tobesaved;
            sprintf( msg.longitude, "%3.0lf%c%04.0lf", deg, vals[2]>=0 ? 'E' : 'W', min*100 );
            *tobesaved = save;
            //fprintf( f, "%s\n", argv[i] );
       } else if ( strncmp( myargs[DEPTH_ARG], test_arg, strlen(myargs[DEPTH_ARG]) ) == 0 ) {
            double val = strtod( test_arg + strlen(myargs[DEPTH_ARG]), NULL );
            char *tobesaved = ptrAfter(msg.depth);
            char save = *tobesaved;
            sprintf( msg.depth, "%5.0lf", val*100 );
            *tobesaved = save;
            //fprintf( f, "%s\n", argv[i] );
        } else if ( strncmp( myargs[MAGNITUDE_ARG], test_arg, strlen(myargs[MAGNITUDE_ARG]) ) == 0 ) {
            double val = strtod( test_arg + strlen(myargs[MAGNITUDE_ARG]), NULL );
            vals[0] = val;
        } else if ( strncmp( myargs[NUM_PHASES_ARG], test_arg, strlen(myargs[NUM_PHASES_ARG]) ) == 0 ) {
            double val = strtod( test_arg + strlen(myargs[NUM_PHASES_ARG]), NULL );
            char *tobesaved = ptrAfter(msg.numPhases);
            char save = *tobesaved;
            sprintf( msg.numPhases, "%3.0lf", val );
            *tobesaved = save;
            //fprintf( f, "%s\n", argv[i] );
        } else if ( strncmp( myargs[AZIMUTH_GAP_ARG], test_arg, strlen(myargs[AZIMUTH_GAP_ARG]) ) == 0 ) {
            double val = strtod( test_arg + strlen(myargs[AZIMUTH_GAP_ARG]), NULL );
            char *tobesaved = ptrAfter(msg.azimuthalGap);
            char save = *tobesaved;
            sprintf( msg.azimuthalGap, "%3.0lf", val );
            *tobesaved = save;
            //fprintf( f, "%s\n", argv[i] );
        } else if ( strncmp( myargs[MIN_DIST_ARG], test_arg, strlen(myargs[MIN_DIST_ARG]) ) == 0 ) {
            double val = strtod( test_arg + strlen(myargs[MIN_DIST_ARG]), NULL );
            char *tobesaved = ptrAfter(msg.minDist);
            char save = *tobesaved;
            sprintf( msg.minDist, "%3.0lf", val );
            *tobesaved = save;
            //fprintf( f, "%s\n", argv[i] );
        } else if ( strncmp( myargs[HORZ_ERR_ARG], test_arg, strlen(myargs[HORZ_ERR_ARG]) ) == 0 ) {
            double val = strtod( test_arg + strlen(myargs[HORZ_ERR_ARG]), NULL );
            char *tobesaved = ptrAfter(msg.horzError);
            char save = *tobesaved;
            sprintf( msg.horzError, "%4.0lf", val*100 );
            *tobesaved = save;
            //fprintf( f, "%s\n", argv[i] );
        } else if ( strncmp( myargs[VERT_ERR_ARG], test_arg, strlen(myargs[VERT_ERR_ARG]) ) == 0 ) {
            double val = strtod( test_arg + strlen(myargs[VERT_ERR_ARG]), NULL );
            char *tobesaved = ptrAfter(msg.vertError);
            char save = *tobesaved;
            sprintf( msg.vertError, "%4.0lf", val*100 );
            *tobesaved = save;
            //fprintf( f, "%s\n", argv[i] );
        } else if ( strncmp( myargs[EVENTID_ARG], test_arg, strlen(myargs[EVENTID_ARG]) ) == 0 ) {
            int j, off;
            for ( j=0, off=strlen(myargs[EVENTID_ARG]); test_arg[j+off] != 0; j++ )
                msg.pad_eventid[j] = test_arg[j+off];
            reqd--;
            //fprintf( f, "%s\n", argv[i] );
        } else if ( strncmp( myargs[VERSION_ARG], test_arg, strlen(myargs[VERSION_ARG]) ) == 0 ) {
            msg.version[0] = test_arg[strlen(myargs[VERSION_ARG])];
            //fprintf( f, "%s\n", argv[i] );
        } else if ( strncmp( myargs[MAGTYPE_ARG], test_arg, strlen(myargs[MAGTYPE_ARG]) ) == 0 ) {
            codaSet = 1;
            isCoda = (strcmp(test_arg,"Mc")==0) || (strcmp(test_arg,"Md"));
            //fprintf( f, "%s\n", argv[i] );
        }
    }
    printf("codaSet=%d, reqd=%d\n", codaSet+0, reqd );
    puts( (char*)&msg );
    if ( codaSet ) {
        char *tobesaved, save;
        tobesaved = ptrAfter(msg.Md);
        save = *tobesaved;
        sprintf( msg.Md, "%3.0lf", vals[0]*100 );
        
        *tobesaved = save;
        tobesaved = ptrAfter(msg.Mpref);
        save = *tobesaved;
        sprintf( msg.Mpref, "%3.0lf", vals[0]*100 );
        *tobesaved = save;
    }
    if ( reqd == 0 ) {
        char path[300];
        sprintf( path, "%s/__________.ark", argv[1] );
        char *p2 = path+strlen(argv[1])+1;
        for ( i=0; i<sizeof(msg.pad_eventid); i++ )
            if ( !isblank(msg.pad_eventid[i]) )
                p2[i] = msg.pad_eventid[i];
        //fprintf( f, "Path: %s\n", path );
        FILE *f2 = fopen( path, "w" );
        puts( path );
        puts( (char*)&msg );
        fwrite( (void*)&msg, sizeof(msg), 1, f2 );
        memset( path, ' ', 100 );
        path[100] = 0;
        fwrite( path, 101, 1, f2 );
        fclose( f2 );
    } else {
        fprintf( f, "Origin but no event id\n" );
    }
    //fprintf( f, "----\n" );
    fclose( f );
    return 0;
}

/*
*/