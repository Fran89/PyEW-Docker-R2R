#include <math.h>
#include "comfile.h"
#include "geo.h"

#define RAD2DEG  57.29577951308
#define DEG2RAD	0.01745329251994
#define TWOPI 6.283185307179586

//	CGeo: Base class for GIS objects.
CGeo::CGeo() {
}

CGeo::CGeo(CGeo *geo) {
	dLat = geo->dLat;
	dLon = geo->dLon;
	dRad = geo->dRad;
	dX = geo->dX;
	dY = geo->dY;
	dZ = geo->dZ;
	dT = geo->dT;
	iTag = geo->iTag;
}

CGeo::~CGeo() {
}

void CGeo::Clone(CGeo *geo) {
	dLat = geo->dLat;
	dLon = geo->dLon;
	dRad = geo->dRad;
	dX = geo->dX;
	dY = geo->dY;
	dZ = geo->dZ;
	dT = geo->dT;
	iTag = geo->iTag;
}

// Converts geographic latitude into geocentric latitude.
void CGeo::setGeographic(double lat, double lon, double r) {
	dLat = RAD2DEG*atan(0.993277*tan(DEG2RAD*lat));
	dLon = lon;
	dRad = r;
	dZ = r*sin(DEG2RAD*dLat);
	double rxy = r*cos(DEG2RAD*dLat);
	dX = rxy*cos(DEG2RAD*dLon);
	dY = rxy*sin(DEG2RAD*dLon);
}

void CGeo::getGeographic(double *lat, double *lon, double *r) {
	*lat = RAD2DEG*atan(tan(DEG2RAD*dLat)/0.993277);
	*lon = dLon;
	*r = dRad;
}

// Initialize geocentric coordinates
void CGeo::setGeocentric(double lat, double lon, double r) {
	dLat = lat;
	dLon = lon;
	dRad = r;
	dZ = r*sin(DEG2RAD*dLat);
	double rxy = r*cos(DEG2RAD*dLat);
	dX = rxy*cos(DEG2RAD*dLon);
	dY = rxy*sin(DEG2RAD*dLon);
}

void CGeo::getGeocentric(double *lat, double *lon, double *r) {
	*lat = dLat;
	*lon = dLon;
	*r = dRad;
}

bool CGeo::Com(CComFile *cf) {
	return false;
}

void CGeo::setCart(double x, double y, double z) {
	dX = x;
	dY = y;
	dZ = z;
	dRad = sqrt(x*x + y*y + z*z);
	double rxy = sqrt(x*x + y*y);
	dLat = RAD2DEG*atan2(z, rxy);
	dLon = RAD2DEG*atan2(y, x);
}

// Calculate the distance in radians to a given geogrphic object
double CGeo::Delta(CGeo *geo) {
	double a = cos(DEG2RAD*dLat)*cos(DEG2RAD*dLon);
	double b = cos(DEG2RAD*dLat)*sin(DEG2RAD*dLon);
	double c = sin(DEG2RAD*dLat);
	double A = cos(DEG2RAD*geo->dLat)*cos(DEG2RAD*geo->dLon);
	double B = cos(DEG2RAD*geo->dLat)*sin(DEG2RAD*geo->dLon);
	double C = sin(DEG2RAD*geo->dLat);
	double dlt = acos(a*A + b*B + c*C);
	return dlt;
}

// Calculate the azimuth in radians to a given geographic object
// DK 021104  This code was impossible for me to follow and understand;
//            however, to my dismay, it works!
double CGeo::Azimuth(CGeo *geo) {
	// Station radial normal vector
	double sx = cos(DEG2RAD*geo->dLat)*cos(DEG2RAD*geo->dLon);
	double sy = cos(DEG2RAD*geo->dLat)*sin(DEG2RAD*geo->dLon);
	double sz = sin(DEG2RAD*geo->dLat);
	// Quake radial normal vector
	double qx = cos(DEG2RAD*dLat)*cos(DEG2RAD*dLon);
	double qy = cos(DEG2RAD*dLat)*sin(DEG2RAD*dLon);
	double qz = sin(DEG2RAD*dLat);
	// Normal to great circle
	double qsx = qy*sz - sy*qz;		
	double qsy = qz*sx - sz*qx;
	double qsz = qx*sy - sx*qy;
	// Vector points along great circle
	double ax = qsy*qz - qy*qsz;
	double ay = qsz*qx - qz*qsx;
	double az = qsx*qy - qx*qsy;
	double r = sqrt(ax*ax + ay*ay + az*az);
	ax /= r;
	ay /= r;
	az /= r;
	// North tangent vector
	double nx = -sin(DEG2RAD*dLat)*cos(DEG2RAD*dLon);
	double ny = -sin(DEG2RAD*dLat)*sin(DEG2RAD*dLon);
	double nz = cos(DEG2RAD*dLat);
	// East tangent vector
	double ex = -sin(DEG2RAD*dLon);
	double ey = cos(DEG2RAD*dLon);
	double ez = 0.0;
	double n = ax*nx + ay*ny + az*nz;
	double e = ax*ex + ay*ey + az*ez;
	double azm = atan2(e, n);
	if(azm < 0.0) azm += TWOPI;
	return azm;
}

int CGeo::Intersect(CGeo *in1, double r1, CGeo *in2, double r2, CGeo *in3, double r3,
					CGeo *out1, CGeo *out2) {
	double s;
	double xd, yd, zd;
	double xs, ys, zs;
	double a1, b1, c1, d1;
	double a2, b2, c2, d2;
	double a3, b3, c3, d3;
	
	// Calculate the plane containing the intersection points between spheres 1 and 2
	// in the form a1*x + b1*y + c1*z - d1;
	xd = in2->dX - in1->dX;
	yd = in2->dY - in1->dY;
	zd = in2->dZ - in1->dZ;
	double r12 = sqrt(xd*xd+yd*yd+zd*zd);
	s = -0.5*(r2*r2 - r1*r1 - r12*r12)/r12;
	a1 = xd/r12;
	b1 = yd/r12;
	c1 = zd/r12;
	xs = in1->dX + a1*s;
	ys = in1->dY + b1*s;
	zs = in1->dZ + c1*s;
	d1 = a1*xs + b1*ys + c1*zs;

	// Calculate the plane containing the intersection points between spheres 1 and 3
	// in the form a2*x + b2*y + c2*z - d2;
	xd = in3->dX - in1->dX;
	yd = in3->dY - in1->dY;
	zd = in3->dZ - in1->dZ;
	double r13 = sqrt(xd*xd+yd*yd+zd*zd);
	s = -0.5*(r3*r3 - r1*r1 - r13*r13)/r13;
	a2 = xd/r13;
	b2 = yd/r13;
	c2 = zd/r13;
	xs = in1->dX + a2*s;
	ys = in1->dY + b2*s;
	zs = in1->dZ + c2*s;
	d2 = a2*xs + b2*ys + c2*zs;

	// Calculate the plane containing all three sphere centers
	// in the form a3*x + b3*y + c3*z - d3. This is done by finding the equation
	// of a plane with normal obtained from the cross product of the vector from
	// center of sphere 1 to 2 and three respectively. Center of sphere one is
	// arbitrarily picked as the origin.
	a3 = b1*c2 - c1*b2;
	b3 = c1*a2 - a1*c2;
	c3 = a1*b2 - b1*a2;
	d3 = a3*in1->dX + b3*in1->dY + c3*in1->dZ;

	// The intersection of these three points is a point on a line containing the
	// two solution points. Calculate the parametric form of this line using the
	// first two planes, then use the quadratic distance to the third sphere center
	// to calculate the possilby two points of intersection of three spheres.
	double bb1 = a2*b1 - a1*b2;
	double cc1 = a2*c1 - a1*c2;
	double dd1 = a2*d1 - a1*d2;
	double bb2 = a3*b1 - a1*b3;
	double cc2 = a3*c1 - a1*c3;
	double dd2 = a3*d1 - a1*d3;

	// Solve system of three equations to calculate a point on the line passing
	// through both solutions (if any) of the problem.
	double zp = (bb2*dd1 - bb1*dd2)/(bb2*cc1 - bb1*cc2);
	double yp = (dd1 - cc1*zp)/bb1;
	double xp = (d1 - c1*zp - b1*yp)/a1;

	// Find two points on the line that satisfy the distance from one of the three
	// centers - in this case sphere 1 is chosen.
	xd = xp - in1->dX;
	yd = yp - in1->dY;
	zd = zp - in1->dZ;
	double a = a3*a3 + b3*b3 + c3*c3;
	double b = 2.0*(a3*xd + b3*yd + c3*zd);
	double c = xd*xd + yd*yd + zd*zd - r1*r1;
	double rad = b*b - 4*a*c;
	if(rad <= 0.0) {
		return 0;
	}
	double root = sqrt(rad);
	double s1 = 0.5*( root-b)/a;
	double s2 = 0.5*(-root-b)/a;
	double x1 = xp + s1*a3;
	double y1 = yp + s1*b3;
	double z1 = zp + s1*c3;
	out1->setCart(x1, y1, z1);
	xd = x1 - in1->dX;
	yd = y1 - in1->dY;
	zd = z1 - in1->dZ;
	double x2 = xp + s2*a3;
	double y2 = yp + s2*b3;
	double z2 = zp + s2*c3;
	out2->setCart(x2, y2, z2);
	xd = x2 - in1->dX;
	yd = y2 - in1->dY;
	zd = z2 - in1->dZ;

	return 2;
}

void CGeo::Check(char *str, CGeo* geo, double r, double x, double y, double z) {
	double xd = x - geo->dX;
	double yd = y - geo->dY;
	double zd = z - geo->dZ;
}
