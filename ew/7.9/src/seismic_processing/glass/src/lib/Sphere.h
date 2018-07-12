// sphere.h
#ifndef SPHERE_H
#define SPHERE_H

class CSphere {
public:
// Attributes
	double dX;	// Center of sphere
	double dY;
	double dZ;
	double dR;	// Radius of sphere

// Methods
	CSphere();
	virtual ~CSphere();
	void Set(double x, double y, double z, double r);
	int Intersect(CSphere *sph1, CSphere *sph2, double *x);
};

#endif
