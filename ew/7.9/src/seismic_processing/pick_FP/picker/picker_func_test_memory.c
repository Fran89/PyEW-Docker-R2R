#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "sachdr.h"

#include "ew_bridge.h"
#include "PickData.h"
#include "FilterPicker5_Memory.h"
#include "FilterPicker5.h"

#define DEBUG 1

#define PACKET_SIZE 10 //simulated packet size (seconds)


void MonthDay(int year, int yearday, int* pmonth, int* pday);

int main(int argc, char *argv[]) {

    int n;

    if (argc < 3) {
        printf("Usage: %s <SAC_file> <pick_file>\n", argv[0]);
        printf("  Picks are appended to end of <pick_file> in NLLOC_OBS format. \n");
        return 0;
    }



    BOOLEAN_INT useMemory = TRUE_INT; // set to TRUE_INT (=1) if function is called for packets of data in sequence, FALSE_INT (=0) otherwise



    // open and read SAC file
    FILE *fp;
    if ((fp = fopen(argv[1], "r")) == 0) {
        perror(argv[1]);
        return -1;
    }
    // read header
    struct HDR sachdr;
    fread(&sachdr, sizeof (sachdr), 1, fp);
    // allocate array for data
    //if (DEBUG) printf("sachdr.NPTS: %d\n", sachdr.NPTS);
    //float* sample = calloc(sachdr.NPTS, sizeof(float));
    int length = (int) rint(1.0 / sachdr.DELTA);
    length *= PACKET_SIZE;
    if (DEBUG) printf("length: %d\n", length);
    float* sample = calloc(length, sizeof (float));
    // read data
    //fread(sample, sizeof(float), sachdr.NPTS, fp);
    //fclose (fp);



    // set picker paramters (TODO: make program arguments?)
    // SEE: _DOC_ in FilterPicker5.c for more details on the picker parameters
    // defaults
    // filp_N filtw 4.0 ltw 10.0 thres1 8.0 thres2 8.0 tupevt 0.2 res PICKS...
    double longTermWindow = 10.0; // NOTE: auto set below
    double threshold1 = 10.0;
    double threshold2 = 10.0;
    double tUpEvent = 0.5; // NOTE: auto set below
    double filterWindow = 4.0; // NOTE: auto set below
    //
    // auto set values
    // get dt
    double dt = sachdr.DELTA;
    if (DEBUG) printf("sachdr.DELTA: %f\n", sachdr.DELTA);
    //dt = dt < 0.02 ? 0.02 : dt;     // aviod too-small values for high sample rate data
    //
    filterWindow = 300.0 * dt;
    long iFilterWindow = (long) (0.5 + filterWindow * 1000.0);
    if (iFilterWindow > 1)
        filterWindow = (double) iFilterWindow / 1000.0;
    //
    longTermWindow = 500.0 * dt; // seconds
    long ilongTermWindow = (long) (0.5 + longTermWindow * 1000.0);
    if (ilongTermWindow > 1)
        longTermWindow = (double) ilongTermWindow / 1000.0;
    //
    //tUpEvent = 10.0 * dt;   // time window to take integral of charFunct version
    tUpEvent = 20.0 * dt; // AJL20090522
    long itUpEvent = (long) (0.5 + tUpEvent * 1000.0);
    if (itUpEvent > 1)
        tUpEvent = (double) itUpEvent / 1000.0;
    //


    printf("picker_func_test_memory: dt %f -> filp_N filtw %f ltw %f thres1 %f thres2 %f tupevt %f res PICKS\n",
            dt, filterWindow, longTermWindow, threshold1, threshold2, tUpEvent);



    // do picker function test
    // definitive pick data
    PickData** pick_list_definative = NULL;
    int num_picks_definative = 0;
    // persistent memory
    FilterPicker5_Memory* mem = NULL;

    // Init TP4 Memory - claudio
    // not needed, initialized autmatically in FilterPicker5.Pick()
    /*
    mem=init_filterPicker5_Memory(
            sachdr.DELTA,
            sample, length,
            filterWindow,
            longTermWindow,
            threshold1,
            threshold2,
            tUpEvent
    );
     */

    int proc_samples = 0;
    int read_samples = 0;
    while ((read_samples = fread(sample, sizeof (float), length, fp)) != 0) { //loop over trace


        // temporary data
        PickData** pick_list = NULL; // array of num_picks ptrs to PickData structures/objects containing returned picks
        int num_picks = 0;

        Pick(
                sachdr.DELTA,
                sample,
                read_samples,
                filterWindow,
                longTermWindow,
                threshold1,
                threshold2,
                tUpEvent,
                &mem,
                useMemory,
                &pick_list,
                &num_picks,
                "TEST"
                );
        if (0) printf("picker_func_test_memory: num_picks: %d\n", num_picks);

        // save pick data
        for (n = 0; n < num_picks; n++) {
            PickData* pick = *(pick_list + n);
            pick->indices[0] += proc_samples; // pick indices returned are relative to start of packet
            pick->indices[1] += proc_samples;
            addPickToPickList(pick, &pick_list_definative, &num_picks_definative);
        }
        // clean up temporary data
        free(pick_list); // do not use free_PickList() since we want to keep PickData objects

        proc_samples += read_samples;
        if (0) printf("sachdr.NPTS: %d processed_samples: %d\n", sachdr.NPTS, proc_samples);

    } //end loop over trace
    fclose(fp);



    // create NLLOC_OBS picks
    // open pick file
    if ((fp = fopen(argv[2], "a")) == 0) {
        perror(argv[2]);
        return -1;
    }
    // date
    int month, day;
    MonthDay(sachdr.NZYEAR, sachdr.NZJDAY, &month, &day);
    double sec = (double) sachdr.B + (double) sachdr.NZSEC + (double) sachdr.NZMSEC / 1000.0;
    // id fields
    char onset[] = "?";
    char* kstnm;
    kstnm = calloc(1, 16 * sizeof (char));
    strncpy(kstnm, sachdr.KSTNM, 6);
    char* kinst;
    kinst = calloc(1, 16 * sizeof (char));
    strncpy(kinst, sachdr.KINST, 6);
    if (strstr(kinst, "(count") != NULL)
        strcpy(kinst, "(counts)");
    char* kcmpnm;
    kcmpnm = calloc(1, 16 * sizeof (char));
    strncpy(kcmpnm, sachdr.KCMPNM, 6);
    char phase[16];
    // create NLL picks
    char* pick_str;
    pick_str = calloc(1, 1024 * sizeof (char));
    for (n = 0; n < num_picks_definative; n++) {
        sprintf(phase, "P%d_", n);
        pick_str = printNlloc(pick_str,
                *(pick_list_definative + n), sachdr.DELTA, kstnm, kinst, kcmpnm, onset, phase,
                sachdr.NZYEAR, month, day, sachdr.NZHOUR, sachdr.NZMIN, sec);
        // write pick to <pick_file> in NLLOC_OBS format
        fprintf(fp, "%s\n", pick_str);
    }


    // clean up
    fclose(fp);
    free(pick_str);
    free(kcmpnm);
    free(kinst);
    free(kstnm);
    free_PickList(pick_list_definative, num_picks_definative); // PickData objects freed here
    free_FilterPicker5_Memory(&mem);
    free(sample);

    return (0);

}



/** date functions */

static char daytab[2][13] = {
    {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

/** function to set month / day from day of year */

void MonthDay(int year, int yearday, int* pmonth, int* pday) {
    int i, leap;

    leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    for (i = 1; yearday > daytab[leap][i]; i++)
        yearday -= daytab[leap][i];
    *pmonth = i;
    *pday = yearday;

}

