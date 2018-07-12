#include <stdio.h>
#include <string.h>
#include <math.h>

struct Greg {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  float second;
};

int mo[] = {   0,  31,  59,  90, 120, 151, 181, 212, 243, 273, 304, 334,
                 0,  31,  60,  91, 121, 152, 182, 213, 244, 274, 305, 335};
char *cmo[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/*
 * julian : Calculate julian date from gregorian date.
 */
long julian( struct Greg *pg )
{
        long jul;
        int leap;
        int year;
        int n;

        jul = 0;
        year = pg->year;
        if(year < 1) goto x110;
        year--;
        jul = 365;

/* four hundred year rule */
        n = year / 400;
        jul += n * 146097L;
        year -= n * 400;

/* hundred year rule */
        n = year / 100;
        jul += n * 36524L;
        year -= n * 100;

/* four year rule */
        n = year / 4;
        jul += n * 1461L;
        year -= n * 4;

/* one year rule */
        jul += year * 365L;

/* Handle days in current year */
x110:
        leap = 0;
        if(pg->year % 4   == 0) leap = 12;
        if(pg->year % 100 == 0) leap = 0;
        if(pg->year % 400 == 0) leap = 12;
        jul += mo[pg->month + leap - 1] + pg->day + 1721060L;
        return(jul);
}



/*
 * gregor : Calculate gregorian date from julian date.
 */
struct Greg *gregor( long min, struct Greg *pg )
{
        long test;
        long check;
        int leap;
        int left;
        int imo;

        pg->year = (min - 1721061L) / 365L;
        pg->month = 1;
        pg->day = 1;
        test = julian(pg);
        if(test <= min) goto x110;

x20:
        pg->year--;
        test = julian(pg);
        if(test > min) goto x20;
        goto x210;

x105:
        pg->year++;
        test = julian(pg);

x110:
        check = test - min - 366L;
        if(check < 0) goto x210;
        if(check > 0) goto x105;

        if(pg->year % 400 == 0) goto x210;
        if(pg->year % 100 == 0) goto x105;
        if(pg->year %   4 == 0) goto x210;
        goto x105;

x210:
        left = min - test;
        leap = 0;
        if(pg->year %   4 == 0) leap = 12;
        if(pg->year % 100 == 0) leap = 0;
        if(pg->year % 400 == 0) leap = 12;
        for(imo=1; imo<12; imo++) {
                if(mo[imo+leap] <= left)
                        continue;
                pg->month = imo;
                pg->day = left - mo[imo+leap-1] + 1;
                return(pg);
        }
        pg->month = 12;
        pg->day = left - mo[11+leap] + 1;
        return(pg);
}

/*
 * Calculate gregorian date and time from julian minutes.
 */
struct Greg *grg( long min, struct Greg *pg )
{
        long j;
        long m;

        j = min/1440L;
        m = min-1440L*j;
        j += 2305448L;
        gregor(j,pg);
        pg->hour = m/60;
        pg->minute = m - 60 * pg->hour;
        return(pg);
}


/*
 * date17 : Build a 17 char date string in the form 19880123123412.21
 *          from the julian seconds.  Remember to leave space for the
 *          string termination (NULL).
 *          Replaces the non-Y2K-compliant date15() function.
 *          Added to chron3.c on 10/28/98 by LDD
 */
void date17( double secs, char *c17 )
{
        struct Greg g;
        long minute;
        double sex;

        minute = (long) (secs / 60.0);
        sex = secs - 60.0 * (double) minute;
        grg(minute,&g);
        sprintf(c17, "%04d%02d%02d%02d%02d%05.2f\0",
                g.year, g.month, g.day, g.hour, g.minute, sex);
}

double round_decimal(double f, int decimal) {
  int i;
  long pow10 = 1;
  for(i=1; i <= decimal; i++) {
    pow10 *= 10;
  }
  return floorf(f * (double) pow10 + 0.5) / (double) pow10;
}

void date17_new( double secs, char *c17 )
{
        struct Greg g;
        long minute;
        double sex;
	double atof_sex;

        minute = (long) (secs / 60.0);
        sex = round_decimal(secs - 60.0 * (double) minute, 2);
	if(sex >= 60.0) {
	  fprintf(stderr, "Warning: seconds >= 60.0   (%f)\n", sex);
	  minute++;
	  sex -= 60.0;
	}
        grg(minute,&g);

        sprintf(c17, "%04d/%02d/%02d %02d:%02d:%05.2f\0",
                g.year, g.month, g.day, g.hour, g.minute, sex);
}

int main(int argc, char *argv[]) {
  double start_time, end_time, step_time;
  double f;
  char c17[255];
  char c17_new[255];
  double k;

  double a_day_sec = 86400;

  /* works */
  start_time = 1338819298.0 + (a_day_sec * 365.0 * 370.0) + (a_day_sec * 87.0);

  /* works */
  start_time = 1338518759.0 + (a_day_sec * 365.0 * 370.0) + (a_day_sec * 90.0);

  /* does not work, for each k */
  k = 24.0 ;
  start_time = 1338819298.0 + ( 60.0 * 3600.0 * k);

  end_time = start_time + 5.0;
  step_time = 0.1;

  for(f=start_time; f < end_time; f+=step_time) {
    date17(f, c17);
    date17_new(f, c17_new);
    printf("%s %s\n", c17, c17_new);
  }
  return 0;
}

