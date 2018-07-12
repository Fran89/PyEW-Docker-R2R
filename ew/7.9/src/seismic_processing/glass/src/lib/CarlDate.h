// date.h

#ifndef _DATE_H_
#define _DATE_H_

class CStr;
class  CDate {
public:
  CDate(){}
	~CDate();
	
protected:
	struct tm tmsTime;
  double  dFracSecs;
};

#endif // _DATE_H_