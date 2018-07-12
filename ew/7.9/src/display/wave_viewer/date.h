// date.h

#ifndef _DATE_H_
#define _DATE_H_

class  CDate : public CObject
{
public:
	CDate();
	CDate(double time);
	CDate(UINT year, UINT month, UINT day, UINT hour, UINT minutes, double seconds);
	~CDate();
	UINT Year()			{ return m_nYear; }
	UINT Month()		{ return m_nMonth; }
	UINT Day()			{ return m_nDay; }
	UINT Hour()			{ return m_nHour; }
	UINT Minute()		{ return m_nMinute; }
	double Seconds()	{ return m_dSeconds; }
	double Time()		{ return m_dTime; }
	CString Date18();
	CString Date20();
	
protected:
	UINT	m_nYear;
	UINT	m_nMonth;
	UINT	m_nDay;
	UINT	m_nHour;
	UINT	m_nMinute;
	double	m_dSeconds;
	double	m_dTime;
private:
};

#endif // _DATE_H_