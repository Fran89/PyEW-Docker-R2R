//
// File: geninv.cpp
//
// Author: Carl Johnson
//
// Purpose: Invert rectangular matrix, even if singular, using SVD
//
// Reference: "Numerical Recipes in C" by Press et al.
//
#include <windows.h>
#include "matrix.h"
#include "math.h"
#include <Debug.h>
extern "C" {
#include "utility.h"
}

//
// Function: GenInv
//
// Input: A matrix to invert and eigenvalue tolerance (epsilon)
// Warning: Matrix may not have fewer rows than columns.
//
// Returns: The inverse of the matrix.
// Note: Return value is a reference into temporary pool.
//
// Purpose: Calculate generalized inverse of matrix of arbitrary dimensions.
//
// Modifications
// 20000510 CEJ Updated Rick's modifications to Version 2 of C Recipies
//
CMatrix GenInv(CMatrix &a, double eigtol)
{
	int irow;
	char txt[80];
	
	CMatrix mTmp(a.nCol, a.nRow);
	
	CMatrix u = a;
	CVector w(a.nCol);
	CMatrix v(a.nCol, a.nCol);
	int res = svdcmp(u, w, v);
	if(res) {
		sprintf(txt, "svdcmp err = %d", res);
	//	AfxMessageBox(txt);
		CDebug::Log(DEBUG_MINOR_INFO,"GenInv():##### %s #####\n", txt);
	}

	//
	// Compute diag(1 / wj), the matrix whose diagonal is the reciprocal of w vector.
	// Don't divide by 0.  Use eigtol to decide when to replace 1/wj with 0.
	// Reference: Numerical Recipes in C section 14.3, function svdfit().
	//
	double wmax = 0.0;
	CMatrix wmat(a.nCol, a.nCol);
	wmat = 0.0;
	for(irow=0; irow<a.nCol; irow++)
		if(w[irow] > wmax)
			wmax = w[irow];
	wmax *= eigtol;
	for(irow=0; irow<a.nCol; irow++)
		if(w[irow] > wmax)
			wmat[irow][irow] = 1.0 / w[irow];

	//
	// Inverse is V * diag(1/wj) * transpose(U)
		//
	mTmp = v * wmat * (u.Transpose());
	return mTmp;
}

//
// Function: svdcmp
//
// Input: Matrix a, references to outputs: vector w and matrix v
// Side effects: Input matrix a is replaced by output matrix u.
//
// Returns: 0	Success
//			101	Dimensionality conflict
//			102	m < n, augment rows with 0.0s
//			103 Failed to converge.
//
// Purpose: Given an MxN matrix a, this routine computes its singular value
//     decomposition (SVD), a = u * w * transpose(v).  The matrix u replaces a
//     on output.  The NxN diagonal matrix of singular values w is output as an
//     N vector.  The NxN matrix v (not its transpose) is also output.  M must
//     be greater or equal to N; if it is smaller, then a should be filled up
//     to square with zero rows.
//
// Reference: Numerical Recipes in C section 2.9 function svdcmp().
//
const int MAXITS = 30;					// Tunable: max convergence iterations

// PYTHAG computes sqrt(a*a + b*b) without destructive overflow or underflow
static double at,bt,ct;
#define PYTHAG(a,b) ((at=fabs(a)) > (bt=fabs(b)) ? \
	(ct=bt/at,at*sqrt(1.0+ct*ct)) : (bt ? (ct=at/bt,bt*sqrt(1.0+ct*ct)): 0.0))
    
static double dmaxarg1,dmaxarg2;
#define DMAX(a,b) (dmaxarg1=(a),dmaxarg2=(b),(dmaxarg1) > (dmaxarg2) ?\
	(dmaxarg1) : (dmaxarg2))

static int iminarg1, iminarg2;
#define IMIN(a,b) (iminarg1=(a), iminarg2=(b), (iminarg1) < (iminarg2) ? \
	(iminarg1) : (iminarg2))

#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))

int svdcmp(CMatrix &a, CVector &w, CMatrix &v)
{
	int m = a.nRow;
	int n = a.nCol;
	int flag,i,its,j,jj,k,l,nm;
	double c,f,h,s,x,y,z;
	double anorm=0.0,g=0.0,scale=0.0;
    
    if(w.nRows() != n)	return 101;
    if(v.nCol != a.nCol) return 101;
    if(v.nRow != a.nCol) return 101;
	if(m < n) return 102;
	CVector rv1(n);

	//
	// Householder reduction to bidiagonal form
	//
	for (i=0;i<n;i++) {
		l=i+1;
		rv1[i]=scale*g;
		g=s=scale=0.0;
		if (i < m) {
			for (k=i;k<m;k++)
				scale += fabs(a[k][i]);
			if (scale) {
				for (k=i;k<m;k++) {
					a[k][i] /= scale;
					s += a[k][i]*a[k][i];
				}
				f=a[i][i];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][i]=f-g;
				for (j=l;j<n;j++) {
					for (s=0.0,k=i;k<m;k++) s += a[k][i]*a[k][j];
					f=s/h;
					for (k=i;k<m;k++) a[k][j] += f*a[k][i];
				}
				for (k=i;k<m;k++) a[k][i] *= scale;
			}
		}
		w[i]=scale*g;
		g=s=scale=0.0;
		if (i < m && i != n-1) {
			for (k=l;k<n;k++) scale += fabs(a[i][k]);
			if (scale) {
				for (k=l;k<n;k++) {
					a[i][k] /= scale;
					s += a[i][k]*a[i][k];
				}
				f=a[i][l];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][l]=f-g;
				for (k=l;k<n;k++) rv1[k]=a[i][k]/h;
				for (j=l;j<m;j++) {
					for (s=0.0,k=l;k<n;k++) s += a[j][k]*a[i][k];
					for (k=l;k<n;k++) a[j][k] += s*rv1[k];
				}
				for (k=l;k<n;k++) a[i][k] *= scale;
			}
		}
		anorm=DMAX(anorm,(fabs(w[i])+fabs(rv1[i])));
	}

	//
	// Accumulation of right-hand transformations.
	//
	for (i=n-1;i>=0;i--) {
		if (i < n-1) {
			if (g) {
				for (j=l;j<n;j++)		// Double division to avoid underflow.
					v[j][i]=(a[i][j]/a[i][l])/g;
				for (j=l;j<n;j++) {
					for (s=0.0,k=l;k<n;k++) s += a[i][k]*v[k][j];
					for (k=l;k<n;k++) v[k][j] += s*v[k][i];
				}
			}
			for (j=l;j<n;j++) v[i][j]=v[j][i]=0.0;
		}
		v[i][i]=1.0;
		g=rv1[i];
		l=i;
	}

	//
	// Accumulation of left-hand transformations.
	//
	for (i=IMIN(m,n)-1;i>=0;i--) {
		l=i+1;
		g=w[i];
		for (j=l;j<n;j++) a[i][j]=0.0;
		if (g) {
			g=1.0/g;
			for (j=l;j<n;j++) {
				for (s=0.0,k=l;k<m;k++) s += a[k][i]*a[k][j];
				f=(s/a[i][i])*g;
				for (k=i;k<m;k++) a[k][j] += f*a[k][i];
			}
			for (j=i;j<m;j++) a[j][i] *= g;
		} else {
			for (j=i;j<m;j++) a[j][i]=0.0;
		}
		++a[i][i];
	}

	//
	// Diagonalization of the bidiagonal form.
	//
	for (k=n-1;k>=0;k--) {					// Loop over singular values.
		for (its=0;its<MAXITS;its++) {		// Loop over allowed iterations.
			flag=1;
			for (l=k;l>=0;l--) {			// Test for splitting:
				nm=l-1;						// Note that rv1[i] is always zero.
				if (fabs(rv1[l])+anorm == anorm) {
					flag=0;
					break;
				}
				if (fabs(w[nm])+anorm == anorm) break;
			}
			if (flag) {
				c=0.0;						// Cancellation of rf1[l], if l>1:
				s=1.0;
				for (i=l;i<=k;i++) {
					f=s*rv1[i];
					rv1[i] = c*rv1[i];
					if (fabs(f)+anorm == anorm)
						break;
					g=w[i];
					h=PYTHAG(f,g);
					w[i]=h;
					h=1.0/h;
					c=g*h;
					s=(-f*h);
					for (j=0;j<m;j++) {
						y=a[j][nm];
						z=a[j][i];
						a[j][nm]=y*c+z*s;
						a[j][i]=z*c-y*s;
					}
				}
			}
			z=w[k];
			if (l == k) {					// Convergence.
				if (z < 0.0) {				// Singular value is made nonnegative.
					w[k] = -z;
					for (j=0;j<n;j++) v[j][k]=(-v[j][k]);
				}
				break;
			}
			if (its == MAXITS-1) return 103;
			x=w[l];							// Shift from bottom 2-by-2 minor:
			nm=k-1;
			y=w[nm];
			g=rv1[nm];
			h=rv1[k];
			f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
			g=PYTHAG(f,1.0);
			f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;

			//
			// Next QR transformation:
			//
			c=s=1.0;
			for (j=l;j<=nm;j++) {	// Rick error corrected, nm C based
				i=j+1;
				g=rv1[i];
				y=w[i];
				h=s*g;
				g=c*g;
				z=PYTHAG(f,h);
				rv1[j]=z;
				c=f/z;
				s=h/z;
				f=x*c+g*s;
				g=g*c-x*s;
				h=y*s;
				y=y*c;
				for (jj=0;jj<n;jj++) {
					x=v[jj][j];
					z=v[jj][i];
					v[jj][j]=x*c+z*s;
					v[jj][i]=z*c-x*s;
				}
				z=PYTHAG(f,h);
				w[j]=z;						// Rotation can be arbitrary if z=0.
				if (z) {
					z=1.0/z;
					c=f*z;
					s=h*z;
				}
				f=(c*g)+(s*y);
				x=(c*y)-(s*g);
				for (jj=0;jj<m;jj++) {
					y=a[jj][j];
					z=a[jj][i];
					a[jj][j]=y*c+z*s;
					a[jj][i]=z*c-y*s;
				}
			}
			rv1[l]=0.0;
			rv1[k]=f;
			w[k]=x;
		}
	}
	return 0;
}