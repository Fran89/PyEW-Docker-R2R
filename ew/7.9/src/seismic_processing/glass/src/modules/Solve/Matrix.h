// matrix.h
#ifndef _MATRIX_H_
#define _MATRIX_H_
#include "vector.h"
#include "euler.h"

class CMatrix
{
// Attributes
public:
	long nId;
	int nRow;
	int nCol;
	double *dA;		// Data stored row-wise

// Operations
public:
	CMatrix();
	CMatrix(int row, int col);
	CMatrix(const CEuler &ev);
	CMatrix(const CVector &cv);
	CMatrix(const CMatrix &cm);
	CMatrix operator=(const CMatrix &cm);
	CMatrix operator=(double val);
	virtual ~CMatrix();
	double *operator[](int n);
	friend CMatrix Ident(int n);
	CMatrix Transpose();
	CMatrix Inverse();
	CVector Solve(CVector &cb);
	void LU();
	static CMatrix GenInv(CMatrix &a, double eigtol);
	friend CMatrix GenInv(CMatrix &a, double eigtol);
	static int SVD(CMatrix &a, CVector &w, CMatrix &v);
	friend int svdcmp(CMatrix &a, CVector &w, CMatrix &v);
	friend CMatrix RotX(double alpha);
	friend CMatrix RotY(double theta);
	friend CMatrix RotZ(double phi);
	CVector operator*(CVector &cv);
	CMatrix operator*(CMatrix &cm);
};

#endif
