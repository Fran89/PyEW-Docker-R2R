// Spock.cpp: Prepare a report providing a synopsis of the association process
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include "spock.h"
#include "glass.h"
#include "str.h"
//#include "rank.h"
#include "date.h"
#include "IReport.h"
#include <Debug.h>
extern "C" {
#include "utility.h"
}

#define RAD2DEG  57.29577951308
#define DEG2RAD	0.01745329251994

int ndx;
double latx[1000];
double lonx[1000];
double dxx[1000];

//---------------------------------------------------------------------------------------CEntity
CSpock::CSpock() {
	pRpt = 0;
}

//---------------------------------------------------------------------------------------~CEntity
CSpock::~CSpock() {
}

//---------------------------------------------------------------------------------------Set
void CSpock::Init(CGlass *glass, IReport *report) {
	pGlass = glass;
	pRpt = report;
}

//---------------------------------------------------------------------------------------Report
void CSpock::Report(double t0, double lat0, double lon0, double depth0,
					int nxyz, double *xyz) {
	char txt[128];
	double lat;
	double lon;
	double t;
	double z, r;
	double x0, y0, z0;
	double rlat, rlon;
	double xy;
	int i;
	double dcut = pGlass->dCut;
	double d;
	double dmin;
	double tmin = t0 + 1000.0;
	double tmax = t0 - 1000.0;
	double zmin = 800.0;
	double zmax = 0.0;
	double t5min = tmin;
	double t5max = tmax;
	double latmin = 90.0;
	double latmax = -90.0;
	double lonmin = 180.0;
	double lonmax = -180.0;
	double a, b;
	int red, grn, blu;

	CDebug::Log(DEBUG_MINOR_INFO,"***** REPORT nxyz=%d\n", nxyz);
	if(!pRpt)
		return;
	pRpt->Text("Nucleation Synopsis");
	pRpt->Text("");
	CDate dt(t0);
	sprintf(txt, "Event: %s %.4f %.4f %.2f",
		dt.Date20().GetBuffer(), lat0, lon0, depth0);
	pRpt->Text(txt);
	sprintf(txt, "Threshold nCut=%d dCut=%.1f km",
		pGlass->nCut, pGlass->dCut);
	pRpt->Text(txt);
	pRpt->Text("");

// Associator background analysis
	pRpt->Title("Glass background characterization");
	pRpt->XAxis(0, 100, MAXASS, "%4.1f");
	pRpt->YAxis(0, 50, 20, "%4.1f");
	pRpt->Graph(6.0, 1.0);
	pRpt->Frame();
	int i2 = pGlass->nAss;
	int i1 = i2 - MAXASS;
	if(i1 < 0)
		i1 = 0;
	int j = MAXASS-2;
	for(i=i2-1; i>=i1; i--) {
		d = pGlass->dAss[i%MAXASS]/dcut;
		if(d > 19.9)
			d = 19.9;
		pRpt->Symbol(j--, d);
	}

// Potential hypocenter cluster analysis
	pRpt->Title("Potential hypocenter cluster analysis");
	pRpt->XAxis(-180.0, 45.0, 180.0, "%6.1f");
	pRpt->YAxis(-90.0, 45, 90.0, "%5.1f");
	pRpt->Graph(6.0, 3.0);
	pRpt->Frame();
	for(t=t0-1000.0; t<t0+1000.0; t+=1.0) {
		for(z=0.0; z<800.01; z+=10.0) {
			d = pGlass->Associate(t, z, &lat, &lon);
			if(d < dcut) {
				if(lat > latmax)
					latmax = lat;
				if(lat < latmin)
					latmin = lat;
				if(lon > lonmax)
					lonmax = lon;
				if(lon < lonmin)
					lonmin = lon;
				if(z > zmax)
					zmax = z;
				if(z < zmin)
					zmin = z;
				if(t > tmax)
					tmax = t;
				if(t < tmin)
					tmin = t;
			}
			if(d < 5.0*dcut) {
				if(ndx < 1000) {
					latx[ndx] = lat;
					lonx[ndx] = lon;
					dxx[ndx] = d;
					ndx++;
				}
				b = 0.2*d;
				a = 1.0 - b;
				red = 255;
				grn = (int)(a*255.0);
				blu = 0;
				pRpt->Pen(red, grn, blu, 0);
				pRpt->Symbol(lon, lat);
				if(t > t5max)
					t5max = t;
				if(t < t5min)
					t5min = t;
			}
		}
	}
	pRpt->Pen(0, 0, 0, 0);
	rlat = DEG2RAD*lat0;
	rlon = DEG2RAD*lon0;
	r = 6371.0 - depth0;
	z0 = r*sin(rlat);
	xy = r*cos(rlat);
	x0 = xy*cos(rlon);
	y0 = xy*sin(rlon);

	pRpt->Text("");

	//========== Calculate gross ranges
	double t1 = tmin - 2.0;
	double t2 = tmax + 2.0;
	double z1 = zmin - 50.0;
	if(z1 < 0.0)
		z1 = 0.0;
	double z2 = zmax + 50.0;
	if(z2 > 800.0)
		z2 = 800.0;

	//========== Origin Time Sensitivity plot
	pRpt->Title("Origin Time Sensitivity");
	pRpt->XAxis(t5min-t0, 5.0, t5max-t0, "%3.1f");
	pRpt->YAxis(0.0, 1.0, 5.0, "%3.1f");
	pRpt->Graph(3.0, 2.0);
	pRpt->Frame();
	pRpt->PenUp();
	pRpt->Plot(t5min-t0, 1.0);
	pRpt->Plot(t5max-t0, 1.0);
	pRpt->PenUp();
	for(t=t5min; t<t5max; t+=0.2) {
		dmin = 1000.0;
		for(z=z1; z<z2; z+=10.0) {
			d = pGlass->Associate(t, z, &lat, &lon);
			if(d < dmin)
				dmin = d;
		}
		CDebug::Log(DEBUG_MINOR_INFO,"t=%.2f dmin=%.2f\n", t-t0, dmin);
		d = dmin;
		if(d > 5.0*dcut)
			d = 5.0*dcut;
		pRpt->Plot(t-t0, d/dcut);
	}

	//========== Depth Sensitivity plot
	pRpt->Title("Depth Sensitivity");
	pRpt->XAxis(0.0, 100.0, 800.0, "%3.1f");
	pRpt->YAxis(0.0, 1.0, 5.0, "%3.1f");
	pRpt->Frame();
	pRpt->PenUp();
	pRpt->Plot(0.0, 1.0);
	pRpt->Plot(800.0, 1.0);
	pRpt->PenUp();
	for(z=0.0; z<800.0; z+=1.0) {
		dmin = 1000.0;
		for(t=t1; t<t2; t+=1.0) {
			d = pGlass->Associate(t, z, &lat, &lon);
			if(d < dmin)
				dmin = d;
		}
		d = dmin;
		if(d > 5.0*dcut)
			d = 5.0*dcut;
		pRpt->Plot(z, d/dcut);
	}

	pRpt->Text("");

	//========== Latitude - Longitude profile
	int nx = 500;
	int ny = 500;
	int ix;
	int iy;
	double dlatdy = 360.0/40000.0;
	double dlondx = dlatdy/cos(rlat);
	double drange = 2.0;
	int gb;
	double lat1, lat2, lon1, lon2;
	double range;
	pGlass->Associate(t0, depth0, &lat, &lon);
	pRpt->Title("Lat-Lon Profile");
	range = lonmax - lonmin;
	if(latmax - latmin > range)
		range = latmax - latmin;
	range /= 2.0;
	lat = 0.5*(latmin + latmax);
	lon = 0.5*(lonmin + lonmax);
	lat1 = floor(lat - range);
	lat2 = ceil(lat + range);
	lon1 = floor(lon - range);
	lon2 = ceil(lon + range);
	pRpt->XAxis(lon1, 1.0, lon2, "%4.0f");
	pRpt->YAxis(lat1, 1.0, lat2, "%3.0f");
	pRpt->Graph(2.0, 2.0);
	pRpt->Frame();
	for(i=0; i<ndx; i++) {
		d = dxx[i];
		b = 0.2*d;
		a = 1.0 - b;
		red = 255;
		grn = (int)(a*255.0);
		blu = 0;
		red = (int)(a*255.0);
		grn = 0;
		blu = (int)(b*255.0);
		pRpt->Pen(red, grn, blu, 0);
		pRpt->Symbol(lonx[i], latx[i]);
	}
	pRpt->Pen(0, 0, 0, 0);

	//========== Origin Depth trade off
	nx = 500;
	ny = 500;
	pRpt->Title("Origin-Depth Trade Off");
	pRpt->XAxis(t1-t0, 5.0, t2-t0, "%4.1f");
	pRpt->YAxis(-z2, 50.0, -z1, "%4.1f");
	pRpt->Image(nx, ny);
	pRpt->Frame();
	for(ix=0; ix<nx; ix++) {
		t = t1 + ix*(t2 - t1)/nx;
		for(iy=0; iy<ny; iy++) {
			z = z1 + iy*(z2 - z1)/ny;
			d = pGlass->Associate(t, z, &lat0, &lon0);
			if(d < dcut) {
				gb = (int)(255.0*d/dcut);
				pRpt->Pixel(ix, iy, 255, gb, gb);
			}
		}
	}

	pRpt->Page();
}
