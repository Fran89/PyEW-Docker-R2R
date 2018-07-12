// vector.cpp : General vector opeartions
//
//Note: These routines have been conditionalized for DLL use.

#include <math.h>
#include "vector.h"
#include "euler.h"

static long nVector = 100;
static double dErr = -1.0e30;

// Default constructor
CVector::CVector()
{
	nId = nVector++;
	nRow = 3;
	dV = new double[nRow];
	for(int i=0; i<nRow; i++)
		dV[i] = 0.0;
}

// Construct zero vector of dimension n
CVector::CVector(int n)
{
	nId = nVector++;
	nRow = n;
	dV = new double[nRow];
	for(int i=0; i<nRow; i++)
		dV[i] = 0.0;
}

// Construct vector of dimension 3 from values of elements
CVector::CVector(double x, double y, double z)
{
	nId = nVector++;
	nRow = 3;
	dV = new double[3];
	dV[0] = x;
	dV[1] = y;
	dV[2] = z;
}

// Copy constructor
CVector::CVector(const CVector &cv)
{
	nId = nVector++;
	nRow = cv.nRow;
//	delete dV;
	dV = new double[nRow];
	for(int i=0; i<nRow; i++)
		dV[i] = cv.dV[i];
}

// Construct euler vector from corresponding euler angles
CVector::CVector(const CEuler &ce)
{
}

CVector::~CVector()
{
	delete [] dV;
}

// Calculate vector length
double CVector::Length()
{
	double r;
	int i;
	r = 0.0;
	for(i=0; i<nRow; i++)
		r += dV[i] * dV[i];
	r = sqrt(r);
	return r;
}

double CVector::Normalize()
{
	double r = Length();
	if(r < 0.00001)		// Just a little protection
		return 0.0;
	for(int i=0; i<nRow; i++)
		dV[i] /= r;
	return r;
}

double CVector::Demean() {
	int i;
	double av = 0.0;
	for(i=0; i<nRow; i++)
		av += dV[i];
	av /= nRow;

	for(i=0; i<nRow; i++)
		dV[i] -= av;

	return av;
}

// Condition: Adjust vector to have unit length and 0 mean
double CVector::Condition()
{
	int i;

	// calculate mean
	double av = 0.0;
	for(i=0; i<nRow; i++)
		av += dV[i];
	av /= nRow;

	// calculate first moment
	double r2 = 0.0;
	for(i=0; i<nRow; i++)
		r2 += (dV[i] - av) * (dV[i] - av);
	double r = sqrt(r2);

	// condition vector
	if(r2 > 1.0e-16)
		for(i=0; i<nRow; i++)
			dV[i] = (dV[i] - av) / r;

	return r;
}

CVector CVector::operator=(const CVector& cv)
{
	nRow = cv.nRow;
	delete [] dV;
	dV = new double[nRow];
	for(int i=0; i<nRow; i++)
		dV[i] = cv.dV[i];
	return *this;
}

CVector CVector::operator=(double val) {
	int n = nRow;
	for(int i=0; i<n; i++)
		dV[i] = val;
	return *this;	
}

double &CVector::operator[](int n)
{
	if(n < 0 || n >= nRow)
		return dErr;
	return dV[n];
}


double  CVector::operator[](int n) const
{
	if(n < 0 || n >= nRow)
		return dErr;
	return dV[n];
}

CVector CVector::operator+(const CVector &cv)
{
//	if(nRow != cv.nRow) {
//		AfxMessageBox("Vector: Dimension mismatch");
//		AfxAbort();
//	}
	CVector v(*this);
	for(int i=0; i<nRow; i++)
		v.dV[i] += cv.dV[i];
	return v;
}

CVector CVector::operator-(const CVector &cv)
{
//	if(nRow != cv.nRow) {
//		AfxMessageBox("Vector: Dimension mismatch");
//		AfxAbort();
//	}
	CVector v(nRow);
	for(int i=0; i<nRow; i++)
		v.dV[i] = dV[i] - cv.dV[i];
	return v;
}

CVector CVector::operator+=(const CVector &cv)
{
//	if(nRow != cv.nRow) {
//		AfxMessageBox("Vector: Dimension mismatch");
//		AfxAbort();
//	}
	for(int i=0; i<nRow; i++)
		dV[i] += cv.dV[i];
	return *this;
}

CVector CVector::operator-=(const CVector &cv)
{
//	if(nRow != cv.nRow) {
//		AfxMessageBox("Vector: Dimension mismatch");
//		AfxAbort();
//	}
	for(int i=0; i<nRow; i++)
		dV[i] -= cv.dV[i];
	return *this;
}

// Operator *: Basic dot product, dimension of vectors must match.
double CVector::operator*(const CVector &cv)
{
	double sum = 0.0;
//	if(nRow != cv.nRow) {
//		AfxMessageBox("Vector: Dimension mismatch");
//		AfxAbort();
//	}
	for(int i=0; i<nRow; i++)
		sum += dV[i] * cv.dV[i];
	return sum;
}

// Calculate cross product of two vectors (both must be of dimension 3)
CVector Cross(CVector &v1, CVector &v2) {
//	if(v1.nRow != 3 || v2.nRow != 3) {
//		AfxMessageBox("Cross product only defined for 3-vectors");
//		AfxAbort();
//	}
	CVector vTmp(3);
	vTmp.dV[0] = v1.dV[1] * v2.dV[2] - v1.dV[2] * v2.dV[1];
	vTmp.dV[1] = v1.dV[2] * v2.dV[0] - v1.dV[0] * v2.dV[2];
	vTmp.dV[2] = v1.dV[0] * v2.dV[1] - v1.dV[1] * v2.dV[0];
	return vTmp;
}

// Operator *: Multiplication of a vector by a constant
CVector operator*(double fac, const CVector &cv)
{
	CVector vTmp = cv;
	for(int i=0; i<vTmp.nRow; i++)
		vTmp.dV[i] *= fac;
	return vTmp;
}


int CVector::nRows()
{
  return(nRow);
}

double * CVector::data()
{
  return(dV);
}
