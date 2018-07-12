// Sphere.cpp: Represents a sphere for calculating intersections (3 sphere problem)
#include <windows.h>
#include <math.h>
#include "sphere.h"
extern "C" {
#include "utility.h"
}

//---------------------------------------------------------------------------------------CEntity
CSphere::CSphere() {
}

//---------------------------------------------------------------------------------------~CEntity
CSphere::~CSphere() {
}

//---------------------------------------------------------------------------------------Set
void CSphere::Set(double x, double y, double z, double r) {
	dX = x;
	dY = y;
	dZ = z;
	dR = r;
}

//---------------------------------------------------------------------------------------Intersect
// Calculate the at most two intesection amounst this sphere and two others
// output will be 0, 1, or 2, and points are returned in x which must have a dimension
// of at least 6

	double s;
	double xd, yd, zd;
	double xs, ys, zs;
	double a1, b1, c1, d1;
	double a2, b2, c2, d2;
	double a3, b3, c3, d3;
	double r12 = sqrt(xd*xd+yd*yd+zd*zd);
	double r13 = sqrt(xd*xd+yd*yd+zd*zd);

int CSphere::Intersect(CSphere *sph1, CSphere *sph2, double *x) {
	static double s;
	static double xd, yd, zd;
	static	double xs, ys, zs;
	static	double a1, b1, c1, d1;
	static	double a2, b2, c2, d2;
	static	double a3, b3, c3, d3;
	static	double bb1;
	static	double cc1;
	static	double dd1;
	static	double bb2;
	static	double cc2;
	static	double dd2;
	static	double zp;
	static	double yp;
	static	double xp;
	static	double a;
	static	double b;
	static	double c;
	static	double rad;
	static	double root;
	static	double s1;
	static	double s2;
	static	double x1;
	static	double y1;
	static	double z1;
	static	double x2;
	static	double y2;
	static	double z2;
	
	// Calculate the plane containing the intersection points between spheres 1 and 2
	// in the form a1*x + b1*y + c1*z - d1;

  // Calculate the vector between the center of sphere0 and sphere1
	xd = sph1->dX - dX;
	yd = sph1->dY - dY;
	zd = sph1->dZ - dZ;


  // r12 = vector length between spheres 0 & 1
	/*double*/ r12 = sqrt(xd*xd+yd*yd+zd*zd);
	s = -0.5*(sph1->dR*sph1->dR - dR*dR - r12*r12)/r12;
//	TRACE("s, r12, s/r12 = %.4f, %.4f, %.4f\n", s, r12, s/r12);

  // a1,b1,c1 are the components of the unit vector r12
	a1 = xd/r12;
	b1 = yd/r12;
	c1 = zd/r12;

  // xs,ys,zs are the vector from sph0 to the plane interstion point.
	xs = dX + a1*s;
	ys = dY + b1*s;
	zs = dZ + c1*s;
	d1 = a1*xs + b1*ys + c1*zs;
//	TRACE("Point1: %.4f %.4f %.4f\n", xs, ys, zs);

	// Calculate the plane containing the intersection points between spheres 1 and 3
	// in the form a2*x + b2*y + c2*z - d2;
	xd = sph2->dX - dX;
	yd = sph2->dY - dY;
	zd = sph2->dZ - dZ;
	/*double*/ r13 = sqrt(xd*xd+yd*yd+zd*zd);
	s = -0.5*(sph2->dR*sph2->dR - dR*dR - r13*r13)/r13;
//	TRACE("s, r13, s/r13 = %.4f, %.4f, %.4f\n", s, r13, s/r13);
	a2 = xd/r13;
	b2 = yd/r13;
	c2 = zd/r13;
	xs = dX + a2*s;
	ys = dY + b2*s;
	zs = dZ + c2*s;
	d2 = a2*xs + b2*ys + c2*zs;
//	TRACE("Point1: %.4f %.4f %.4f\n", xs, ys, zs);

	// Calculate the plane containing all three sphere centers
	// in the form a3*x + b3*y + c3*z - d3. This is done by finding the equation
	// of a plane with normal obtained from the cross product of the vector from
	// center of sphere 1 to 2 and three respectively. Center of sphere one is
	// arbitrarily picked as the origin.
	a3 = b1*c2 - c1*b2;
	b3 = c1*a2 - a1*c2;
	c3 = a1*b2 - b1*a2;
	d3 = a3*dX + b3*dY + c3*dZ;
//	TRACE("Matrix...\n");
//	TRACE("%.4f %.4f %.4f %.4f\n", a1, b1, c1, d1);
//	TRACE("%.4f %.4f %.4f %.4f\n", a2, b2, c2, d2);
//	TRACE("%.4f %.f4 %.4f %.4f\n", a3, b3, c3, d3);

	// The intersection of these three points is a point on a line containing the
	// two solution points. Calculate the parametric form of this line using the
	// first two planes, then use the quadratic distance to the third sphere center
	// to calculate the possilby two points of intersection of three spheres.
	/*double*/ bb1 = a2*b1 - a1*b2;
	/*double*/ cc1 = a2*c1 - a1*c2;
	/*double*/ dd1 = a2*d1 - a1*d2;
	/*double*/ bb2 = a3*b1 - a1*b3;
	/*double*/ cc2 = a3*c1 - a1*c3;
	/*double*/ dd2 = a3*d1 - a1*d3;

	// Solve system of three equations to calculate a point on the line passing
	// through both solutions (if any) of the problem.
	/*double*/ zp = (bb2*dd1 - bb1*dd2)/(bb2*cc1 - bb1*cc2);
	/*double*/ yp = (dd1 - cc1*zp)/bb1;
	/*double*/ xp = (d1 - c1*zp - b1*yp)/a1;
//	TRACE("Intersect: %.4f %.4f %.4f\n", xp, yp, zp);

	// Find two points on the line that satisfy the distance from one of the three
	// centers - in this case sphere 1 is chosen.
	xd = xp - dX;
	yd = yp - dY;
	zd = zp - dZ;
	/*double*/ a = a3*a3 + b3*b3 + c3*c3;
	/*double*/ b = 2.0*(a3*xd + b3*yd + c3*zd);
	/*double*/ c = xd*xd + yd*yd + zd*zd - dR*dR;
	/*double*/ rad = b*b - 4*a*c;
	if(rad <= 0.0) {
//		TRACE("No root: %.4f %.4f %.4f %.4f\n", a, b, c, rad);
		return 0;
	}
	/*double*/ root = sqrt(rad);
	/*double*/ s1 = 0.5*( root-b)/a;
	/*double*/ s2 = 0.5*(-root-b)/a;
	// Solution 1
	/*double*/ x1 = xp + s1*a3;
	/*double*/ y1 = yp + s1*b3;
	/*double*/ z1 = zp + s1*c3;
	x[0] = x1 - dX;
	x[1] = y1 - dY;
	x[2] = z1 - dZ;
	// Solution 2
	/*double*/ x2 = xp + s2*a3;
	/*double*/ y2 = yp + s2*b3;
	/*double*/ z2 = zp + s2*c3;
	x[3] = x2 - dX;
	x[4] = y2 - dY;
	x[5] = z2 - dZ;
/*	xd = x[3] = x2 - dX;
	yd = x[4] = y2 - dY;
	zd = x[5] = z2 - dZ;
	TRACE("Solution 1: x:%.2f y:%.2f z:%.2f\n", x[0], x[1], x[2]);
	xd = x[0] - dX;
	yd = x[1] - dY;
	zd = x[2] - dZ;
	TRACE("     r:%.2f\n", sqrt(xd*xd + yd*yd + zd*zd));
	xd = x[0] - sph1->dX;
	yd = x[1] - sph1->dY;
	zd = x[2] - sph1->dZ;
	TRACE("    r1:%.2f\n", sqrt(xd*xd + yd*yd + zd*zd));
	xd = x[0] - sph2->dX;
	yd = x[1] - sph2->dY;
	zd = x[2] - sph1->dZ;
	TRACE("    r2:%.2f\n", sqrt(xd*xd + yd*yd + zd*zd));
	TRACE("Solution 2: x:%.2f y:%.2f z:%.2f\n", x[3], x[4], x[5]);
	xd = x[3] - dX;
	yd = x[4] - dY;
	zd = x[5] - dZ;
	TRACE("     r:%.2f\n", sqrt(xd*xd + yd*yd + zd*zd));
	xd = x[3] - sph1->dX;
	yd = x[4] - sph1->dY;
	zd = x[5] - sph1->dZ;
	TRACE("    r1:%.2f\n", sqrt(xd*xd + yd*yd + zd*zd));
	xd = x[3] - sph2->dX;
	yd = x[4] - sph2->dY;
	zd = x[5] - sph1->dZ;
	TRACE("    r2:%.2f\n", sqrt(xd*xd + yd*yd + zd*zd)); */
	return 2;
}
