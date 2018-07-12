// vector.h
#ifndef _VECTOR_H_
#define _VECTOR_H_

class CEuler;
class CVector
{
// Attributes
private:  // DK 20030617
	long	nId;
  int		nRow;
	double	*dV;

// Operations
public:
	CVector();
	CVector(int n);
	virtual ~CVector();
	CVector(const CVector &cv);
	CVector(double x, double y, double z);
	CVector(const CEuler &ce);
	double Length();
	double Normalize();
	double Demean();
	double Condition();
	CVector operator=(const CVector &cv);
	CVector operator=(double val);
	double  &operator[](int n);
	double  operator[](int n) const;
	CVector operator+(const CVector &cv);
	CVector operator-(const CVector& cv);
	CVector operator+=(const CVector &cv);
	CVector operator-=(const CVector &cv);
	double operator*(const CVector &cv);
	friend CVector Cross(CVector &v1, CVector &v2);
	friend CVector operator*(double fac, const CVector &cv);
  int nRows();
  double * data();
};

#endif
