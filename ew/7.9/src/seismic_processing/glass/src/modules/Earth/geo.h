#ifndef _GEO_H_
#define _GEO_H_

class CComFile;
class CGeo {
public:

// Attributes
	double dLat;	// Geocentric Latitude (degrees)
	double dLon;	// Longitude (degrees)
	double dRad;  	// Radius (kilometers)

	double dX;
	double dY;
	double dZ;

	double dT;		// Associated time (if space-time point)

	int iTag;		// Optional user information

// Methods
	CGeo();
	CGeo(CGeo *geo);
	virtual ~CGeo();
	virtual bool Com(CComFile *cf);
	virtual void Clone(CGeo *geo);
	virtual void setGeographic(double lat, double lon, double r);
	virtual void setGeocentric(double lat, double lon, double r);
	virtual void getGeographic(double *lat, double *lon, double *r);
	virtual void getGeocentric(double *lat, double *lon, double *r);
	virtual void setCart(double x, double  y, double z);
	virtual double Delta(CGeo *geo);
	virtual double Azimuth(CGeo *geo);
	static int Intersect(CGeo *in1, double r1, CGeo *in2, double r2, CGeo *in3, double r3,
		CGeo *out1, CGeo *out2);
	static void Check(char *str, CGeo* geo, double r, double x, double y, double z);

};

#endif
