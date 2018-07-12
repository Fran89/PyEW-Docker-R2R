// date.h

#ifndef _DATE_H_
#define _DATE_H_

extern "C" {
#include <time_ew.h>
}
#include "str.h"
class  CDate {
public:
  CDate(){ time_t tNow;  time(&tNow);  gmtime_ew(&tNow,&tmsTime); dFracSecs=0.0; tTime=tNow;}
  CDate(double time) 
  { time_t tNow;  tNow=(int)time;  gmtime_ew(&tNow,&tmsTime);  dFracSecs=time-tNow; tTime=tNow;}

	CDate(UINT year, UINT month, UINT day, UINT hour, UINT minutes, double seconds)
  {memset(&tmsTime,0,sizeof(struct tm)); 
   tmsTime.tm_year=year-1900;
   tmsTime.tm_mon=month-1;
   tmsTime.tm_mday=day;
   tmsTime.tm_hour=hour;
   tmsTime.tm_min=minutes;
   tmsTime.tm_sec=(int)seconds;
   dFracSecs = seconds - tmsTime.tm_sec;
   tTime = timegm_ew(&tmsTime);
  }
	~CDate()
  {
  }


	UINT Year()		{ return tmsTime.tm_year+1900;       }
	UINT Month()		{ return tmsTime.tm_mon+1;         }
	UINT Day()			{ return tmsTime.tm_mday;          }
	UINT Hour()		{ return tmsTime.tm_hour;            }
	UINT Minute()		{ return tmsTime.tm_min;           }
	double Secnds()	{ return tmsTime.tm_sec+dFracSecs; }
  double Time()   { return(tTime + dFracSecs);       }

  CStr CDate::Date20() 
  {

    /*  From
        Thu Jan 02 02:03:55 1980\n\0. 
        To
        1988Jan23 1234 12.21
     **********************************/
    char szAscTime[80];
    char c20[80];

    strcpy(szAscTime,asctime(&tmsTime));
    memcpy(&(c20[0]),&(szAscTime[20]),4);
    memcpy(&(c20[4]),&(szAscTime[4]),3);
    memcpy(&(c20[7]),&(szAscTime[8]),2);
    c20[9]  = ' ';
    memcpy(&(c20[10]),&(szAscTime[11]),2);
    memcpy(&(c20[12]),&(szAscTime[14]),2);
    c20[14] = ' ';
    memcpy(&(c20[15]),&(szAscTime[17]),2);
    c20[17] = '.';
    sprintf(&(c20[18]),"%2d",((int)(dFracSecs*100))%100);
    return(CStr(c20));
  }

	
protected:
	struct tm tmsTime;
  time_t  tTime;
  double  dFracSecs;
};

#endif // _DATE_H_