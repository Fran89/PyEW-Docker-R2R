#ifndef _SOLVE_H_
#define _SOLVE_H_
// solve.h: General solution of linear equations
#include "array.h"
#include "vector.h"
#include "ISolve.h"

class CVector;
class CSolve : public ISolve {
// Attributes
public:
	int			nCol;		// Number of columns in a matrix
	CVector		vSol;		// Solution after Solve called


private:
  	CArray		arrRow;		// Array of row vectors

// Operations
public:
	CSolve();
	~CSolve();
	void Init(int nvar);
	int Equation(double *coef, double rhs);
	double *Solve();
	double *Solve(double tol);
	bool Test();
	void Release();
};

#endif