// solve.cpp
#include <windows.h>
#include "solve.h"
#include "vector.h"
#include "matrix.h"
#include <Debug.h>

extern "C" {
#include "utility.h"
}

// Default constructor
CSolve::CSolve() {
//	TRACE("CSolve constructor\n");
	nCol = 0;
}

// Destructor
CSolve::~CSolve() 
{
	CVector *pVector;
	int i;

	for(i=0; i<arrRow.GetSize(); i++) 
  {
		pVector = (CVector *)arrRow.GetAt(i);
    delete pVector;
	}
	arrRow.RemoveAll();
}

// Init: Establish dimensionality and reset for next solution
void CSolve::Init(int nvar) 
{

  CVector *pVector;
	int i;

	for(i=0; i<arrRow.GetSize(); i++) 
  {
		pVector = (CVector *)arrRow.GetAt(i);
    delete pVector;
	}
	arrRow.RemoveAll();
	nCol = nvar;
}

// Equation: Append equation to set of linear equations
int CSolve::Equation(double *coef, double dB) {
	if(nCol < 1)
		return 0;
	CVector *v = new CVector(nCol + 1);
  // DK CLEANUP.  The below code looks buggy.  The second statement
  //              should only run once, not everytime through the loop.
	for(int i=0; i<nCol; i++) {
		(*v)[i] = coef[i];
		(*v)[nCol] = dB;
	}
	int rc=arrRow.Add(v);
  /*  DK 20030617  check the return code from arrRow.Add().  
      If -1 then the array is full and our new vector did not
      get added.  We need to delete the Vector on our own,
      and we should issue some sort of status message.
   *********************************************************/
  if(rc == -1)
  {
    delete v;
    CDebug::Log(DEBUG_MINOR_ERROR, "CSolve::Equation():  ERROR!  Could not add new equation vector(%d cols) to Array.  Array Full!\n",
                nCol+1);
  }

	return arrRow.GetSize();
}

// Solve: Invert matrix and project solution
double *CSolve::Solve() {
	return Solve(0.0001);
}
double *CSolve::Solve(double tol) {
	int n = arrRow.GetSize();
	CVector *v;
	CMatrix a(n, nCol);
	CVector b(n);
	for(int row=0; row<n; row++) {
		v = (CVector *)arrRow.GetAt(row);
		for(int col=0; col<nCol; col++) {
			a[row][col] = (*v)[col];
		}
		b[row] = (*v)[nCol];
	}
	CMatrix g = GenInv(a, tol);
	vSol = g * b;
//	TRACE("Solve: n:%d x(%d), b(%d), g(%d,%d)\n", n, x.nRow, b.nRow, g.nRow, g.nCol);

	return vSol.data();
}

//Test: Text and excersize routine
bool CSolve::Test() {
	double coef[3];
	double *sol;
	Init(3);
	coef[0] = 1.0;
	coef[1] = 1.0;
	coef[2] = 1.0;
	Equation(coef, 6.0);
	coef[0] = -1.0;
	Equation(coef, 4.0);
	coef[0] = 1.0;
	coef[1] = -1.0;
	Equation(coef, 2.0);
	coef[1] = 1.0;
	coef[2] = -1.0;
	Equation(coef, 0.0);
	sol = Solve();
	CDebug::Log(DEBUG_MINOR_INFO,"Test(): Solution %.2f %.2f %.2f\n", sol[0], sol[1], sol[2]);
	return true;
}

//Release: Interface Release implementation (see ISiam.h)
void CSolve::Release() {
	delete this;
}
