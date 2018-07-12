#ifndef ISOLVE_H
#define ISOLVE_H

#include "ISiam.h"

// ISolve.h : General linear equation solver using SVD method
struct ISolve : public ISiam {
	virtual void Init(int nvar) = 0;
	virtual int Equation(double *coef, double rhs) = 0;
	virtual double *Solve() = 0;
	virtual double *Solve(double tol) = 0;
	virtual bool Test() = 0;
};
#endif
