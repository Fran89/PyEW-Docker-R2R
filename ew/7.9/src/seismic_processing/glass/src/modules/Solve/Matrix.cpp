// matrix.cpp
#define __PDQ__
#define __MATRIX__
#include "matrix.h"
#include "math.h"
#define RAD 0.0174532

// 000622 CEJ Zero matrix on construction

static long nMatrix = 100;
static double dErr = 1.0e-30;

CMatrix::CMatrix()
{
	nId = nMatrix++;
	nRow = 3;
	nCol = 3;
	dA = new double[nRow * nCol];
	for(int i=0; i<nRow*nCol; i++)
		dA[i] = 0.0;
}

CMatrix::CMatrix(int nrow, int ncol)
{
	nId = nMatrix++;
	nRow = nrow;
	nCol = ncol;
	dA = new double[nRow * nCol];
	for(int i=0; i<nRow*nCol; i++)
		dA[i] = 0.0;
}

// Calculate transformation matrix to frame corresponding to given euler angles.
// RG-84-17, p17, (37)
CMatrix::CMatrix(const CEuler &ce)
{
	nId = nMatrix++;
	nRow = 3;
	nCol = 3;
	dA = new double[9];
	double cpsi = cos(ce.dYaw);
	double spsi = sin(ce.dYaw);
	double ctheta = cos(ce.dPitch);
	double stheta = sin(ce.dPitch);
	double cphi = cos(ce.dRoll);
	double sphi = sin(ce.dRoll);
	dA[0] = ctheta * cpsi;
	dA[1] = ctheta * spsi;
	dA[2] = -stheta;
	dA[3] = sphi * stheta * cpsi - cphi * spsi;
	dA[4] = sphi * stheta * spsi + cphi * cpsi;
	dA[5] = sphi * ctheta;
	dA[6] = cphi * stheta * cpsi + sphi * spsi;
	dA[7] = cphi * stheta * spsi - sphi * cpsi;
	dA[8] = cphi * ctheta;
}

// Calculate transformation matrix to frame corresponding to given euler vector.
CMatrix::CMatrix(const CVector &cv)
{
	nId = nMatrix++;
	double w = sqrt(cv[0]*cv[0] + cv[1]*cv[1] + cv[2]*cv[2]);
	double theta = acos(cv[2]/w);
	double phi = atan2(cv[1], cv[0]);
	dA = new double[1];
	*this = RotZ(phi) * RotY(theta) * RotZ(w) * RotY(-theta) * RotZ(-phi);
}

CMatrix::~CMatrix()
{
	delete [] dA;
}

// Copy constructor
CMatrix::CMatrix(const CMatrix &cm)
{
	int i, n;
	
	nId = nMatrix++;
	nRow = cm.nRow;
	nCol = cm.nCol;
	n = nRow * nCol;
	dA = new double[n];
	for(i=0; i<n; i++)
		dA[i] = cm.dA[i];
}

// Matrix = Matrix: Ordinary assignment operator
CMatrix CMatrix::operator=(const CMatrix &cm)
{
	int i, n;
	
	n = cm.nRow * cm.nCol;
	if(nRow != cm.nRow || nCol != cm.nCol) {
		delete [] dA;
		dA = new double[n];
		nRow = cm.nRow;
		nCol = cm.nCol;	
	}
//	delete dA;
//	dA = new double[n];
//	nRow = cm.nRow;
//	nCol = cm.nCol;	
	for(i=0; i<n; i++)
		dA[i] = cm.dA[i];
	return *this;
}

// Matrix = double: Set elements of matrix to constant value
CMatrix CMatrix::operator=(double val)
{
	int n = nRow * nCol;
	for(int i=0; i<n; i++)
		dA[i] = val;
	return *this;
}

// Mat[i][j] = val, or val = Mat[i][j]: Indexed access to matrix elements
double *CMatrix::operator[](int n)
{
	if(n < 0 || n >= nRow)
		return &dErr;
	return &dA[n * nCol];
}
	
//Create identity matrix
CMatrix Ident(int n)
{
	int i, row, col;

	CMatrix mTmp(n,n);
	i = 0;
	for(row=0; row<n; row++)
		for(col=0; col<n; col++)
			mTmp.dA[i++] = (row == col) ? 1.0 : 0.0;
	return mTmp;			
}

// Transpose: Matrix transpose
CMatrix CMatrix::Transpose()
{
	int i, j;
	
	CMatrix mTmp(nCol, nRow);
	for(i=0; i<nRow; i++)
		for(j=0; j<nCol; j++)
			mTmp[j][i] = (*this)[i][j];
	return mTmp;
}

// SVD: Single value decomposition (wrapper)
int CMatrix::SVD(CMatrix &a, CVector &w, CMatrix &v) {
	return svdcmp(a, w, v);
}

// GenInv: Generalized inverse wrapper
CMatrix CMatrix::GenInv(CMatrix &a, double tol) {
	return ::GenInv(a, tol);
}

CMatrix RotX(double alpha)
{
	CMatrix mTmp(3,3);
	
	double sina = sin(alpha);
	double cosa = cos(alpha);
	mTmp.dA[0] = 1.0;
	mTmp.dA[1] = 0.0;
	mTmp.dA[2] = 0.0;
	mTmp.dA[3] = 0.0;
	mTmp.dA[4] = cosa;
	mTmp.dA[5] = -sina;
	mTmp.dA[6] = 0.0;
	mTmp.dA[7] = sina;
	mTmp.dA[8] = cosa;
	return mTmp;
}

CMatrix RotY(double alpha)
{
	CMatrix mTmp(3,3);
	
	double sina = sin(alpha);
	double cosa = cos(alpha);
	mTmp.dA[0] = cosa;
	mTmp.dA[1] = 0.0;
	mTmp.dA[2] = sina;
	mTmp.dA[3] = 0.0;
	mTmp.dA[4] = 1.0;
	mTmp.dA[5] = 0.0;
	mTmp.dA[6] = -sina;
	mTmp.dA[7] = 0.0;
	mTmp.dA[8] = cosa;
	return mTmp;
}

CMatrix RotZ(double alpha)
{
	CMatrix mTmp(3,3);
	
	double sina = sin(alpha);
	double cosa = cos(alpha);
	mTmp.dA[0] = cosa;
	mTmp.dA[1] = -sina;
	mTmp.dA[2] = 0.0;
	mTmp.dA[3] = sina;
	mTmp.dA[4] = cosa;
	mTmp.dA[5] = 0.0;
	mTmp.dA[6] = 0.0;
	mTmp.dA[7] = 0.0;
	mTmp.dA[8] = 1.0;
	return mTmp;
}

// Vector = Matrix * Vector
CVector CMatrix::operator*(CVector &cv)
{
	int i, j, k;
	double sum;
	
//	if(nCol != cv.nRow) {
//		AfxMessageBox("CMatrix::operator* : Dimensionality mismatch.");
//		AfxAbort();
//	}
	CVector vTmp(nRow);

	k = 0;
	for(i=0; i<nRow; i++) {
		sum = 0.0;
		for(j=0; j<nCol; j++) {
			sum += dA[k++] * cv[j];
		}
		vTmp[i] = sum;
	}
	return vTmp;
}

// Matrix = Matrix * Matrix
CMatrix CMatrix::operator*(CMatrix &cm)
{
	int i, j, k;
	int ix;
	int i1, i2;
	double sum;
	
	CMatrix mTmp(nRow, cm.nCol);

	k = 0;
	ix = 0;
	for(i=0; i<mTmp.nRow; i++) {
		for(j=0; j<mTmp.nCol; j++) {
			sum = 0.0;
			i1 = i * nCol;
			i2 = j;
			for(k=0; k<nCol; k++) {
				sum += dA[i1++] * cm.dA[i2];
				i2 += cm.nCol;
			}
			mTmp.dA[ix++] = sum;
		}
	}
	return mTmp;
}
