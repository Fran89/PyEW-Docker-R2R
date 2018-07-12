// flinn.cpp: Flinn-Engdahl codes
// Implements Flinn-Engdahl regionalization codes as revised in 1995
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "flinn.h"
#include "str.h"
#include <Debug.h>
extern "C" {
#include "utility.h"
}

//---------------------------------------------------------------------------------------FlinnEngdahl
CFlinn::CFlinn() {
}

//---------------------------------------------------------------------------------------~FlinnEngdahl
CFlinn::~CFlinn() {
}

//---------------------------------------------------------------------------------------Ix
// The Flinn-Engdahl region is contained in a large matrix with every degree of latitude
// and longitude represented as one cell. This routine calculates the appropriate index
// based on latitude and longitude
// This routine isn't extremely accurate for areas around the
// poles (+/-90 Lat), and the international date line (+/-180 lon),
// but all other segments (-89:89, -179:179) should work fine.
// DK 020704
int CFlinn::Ix(double lat, double lon) {
  int row, col;

  if(lat < 0)
    row = ((int)(lat - 0.5)) + 90;
  else
    row = ((int)(lat + 0.5)) + 90;

  if(lon < 0)
    col = ((int)(lon - 0.5)) + 180;
  else
    col = ((int)(lon + 0.5)) + 180;

	if(col < 0)		col = 0;
	if(col > 359)	col = 359;
	if(row < 0)		row = 0;
	if(row > 179)	row = 179;
	int ix = 360*row + col;
	return ix;
}

//---------------------------------------------------------------------------------------Load
// Load Flinn-Engdahl region information from a set of files in the directory specified
// The following files must exist in that directory (names.fer, ne.fer, nw.fer, se.fer,
// and sw.fer.
bool CFlinn::Load(char *root) {
	FILE *f;
	CStr crd;
	char base[128];
	char file[128];
	char card[128];
	char name[128];
	int lev1, lev2;
	strcpy(base, root);
	int i = strlen(base) - 1;
	int n;
	if(base[i] != '/' && base[i] != '\\')
		strcat(base, "/");
	int ix1 = 0;
	int ix2 = 0;

	// Load region names (.../names.fer)
	strcpy(file, base);
	strcat(file, "names.fer");
	if(!(f = fopen(file, "rt")))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CFlinn::Load():  ERROR! Could not open file <%s>\n",
                file);
		return false;
  }
	while(fgets(card, sizeof(card), f)) {
		n = strlen(card);
		if(n < 22)
			continue;
		for(i=0; i<n; i++)	// Strip CR or LF
			if(card[i] == 10 || card[i] == 13)
				card[i] = 0;
		crd = card;
		lev1 = crd.Long(2, 13);
		lev2 = crd.Long(3, 15);
		if(lev1 < 1 && lev2 < 1)
			continue;
		strcpy(name, &card[20]);
		if(lev1) {
			ix1 = lev1-1;
			if(ix1 < 0)
				ix1 = 0;
			if(ix1 >= MAX_LEV1)
				ix1 = MAX_LEV1-1;
			cLev1[ix1] = name;
			continue;
		}
		ix2 = lev2-1;
		if(ix2 < 0)
			ix2 = 0;
		if(ix2 >= MAX_LEV2)
			ix2 = MAX_LEV2-1;
		iLev2[ix2] = ix1;
		cLev2[ix2] = name;
	//	TRACE("%3d %3d <%s>\n", lev1, lev2, name);
  }  // end while(fgets)
	fclose(f);
  f=NULL;

	int m = 360*180;
	for(i=0; i<m; i++)
		iReg[i] = -1;

	// NE Quadrant file
	strcpy(file, base);
	strcat(file, "ne.fer");
	if(!Quad(file, 0))
		return false;

	// SE Quadrant file
	strcpy(file, base);
	strcat(file, "se.fer");
	if(!Quad(file, 1))
		return false;

	// NW Quadrant file
	strcpy(file, base);
	strcat(file, "nw.fer");
	if(!Quad(file, 2))
		return false;

	// SW Quadrant file
	strcpy(file, base);
	strcat(file, "sw.fer");
	if(!Quad(file, 3))
		return false;

	double lat;
	double lon;
	int ix;
	for(lat=-89.5; lat<90.5; lat += 1.0) {
		for(lon=-179.5; lon<180.5; lon += 1.0) {
			ix = Ix(lat, lon);
			if(iReg[ix] == -1)
				CDebug::Log(DEBUG_MINOR_ERROR,"Bad FE location: %7.1f %7.1f %d\n", lat, lon, ix);
		}
	}

	return true;
}

//---------------------------------------------------------------------------------------Quad
// Quad: Load quad file, iquad(0:ne, 1:se, 2:nw, 3:sw)
bool CFlinn::Quad(char *quad, int iquad) {
	char card[128];
	CStr crd;
	int ilat = 0;
	int ilon = 0;
	int ireg = 0;
	int lat;
	int lon;
	int reg;
	int ix;
	int nLatSign = 1;
	if(iquad&1) nLatSign = -1;
	int nLonSign = 1;
	if(iquad&2)	nLonSign = -1;
	CDebug::Log(DEBUG_MINOR_INFO,"quad <%s>\n", quad);
	FILE *f;
	if(!(f = fopen(quad, "rt")))
  {
    CDebug::Log(DEBUG_MINOR_ERROR,"CFlinn::Quad():  ERROR! Could not open file <%s>\n",
                quad);
		return false;
  }
  // DK 020402  Adding code to read first line of file {line len, file name}
  //                       example:                   1914    NORTHEAST QUADRANT  
  // Read first line, ignore contents.
  fgets(card, sizeof(card), f);

	while(fgets(card, sizeof(card), f)) {
		if(strlen(card) < 20)
			continue;
		if(card[5] == ' ')
			continue;
		crd = card;
		lat = crd.Long(2, 3);
		lon = crd.Long(3, 10);
		reg = crd.Long(3, 18);
    if(ilat < lat)
    {
      for(; ilat<lat; ilat++)
      {
        for(; ilon<=180; ilon++) 
        {
          ix = Ix(nLatSign*ilat, nLonSign*ilon);
          iReg[ix] = ireg;
        }
        ilon=0;
      }
    }
    if(lat != ilat) 
      exit(-1);  // DK CLEANUP

    for(; ilon<lon; ilon++) 
    {
      ix = Ix(nLatSign*ilat, nLonSign*ilon);
      iReg[ix] = ireg;
    }
		ireg = reg;
  }  // end while(fgets)

  // Do the last item in the quad, as it
  // will never be done naturally
  ix = Ix(nLatSign*90, nLonSign*180);
  iReg[ix] = ireg;

	fclose(f);
  f=NULL;
	return true;
}

static char *sNada = "**Unknown**";
//---------------------------------------------------------------------------------------Code
// Get FE code from latitude and longitude
int CFlinn::Code(double lat, double lon) {
	int ix = Ix(lat, lon);
	int code = iReg[ix];
	return code;
}

//---------------------------------------------------------------------------------------Region1
// Get level 1 region name (Seismic region) for FE code
char *CFlinn::Region1(int code) {
	if(code < 1 || code > MAX_LEV2)
		return sNada;
	int i2 = code-1;
	int i1 = iLev2[i2];
	if(i1 < 0 || i1 > MAX_LEV1)
  {
    CDebug::Log(DEBUG_MINOR_ERROR, "Error: Region1() could not obtain region string(code=%d  i1=%d)\n",
                code, i1);
		return sNada;
  }
	return cLev1[i1].GetBuffer();
}

//---------------------------------------------------------------------------------------Region1
// Get level 1 region name (Seismic region) for FE code with region code
char *CFlinn::Region1(int code, int *ireg) {
	if(code < 1 || code > MAX_LEV2)
		return sNada;
	int i2 = code-1;
	int i1 = iLev2[i2];
	if(i1 < 0 || i1 > MAX_LEV1) {
		*ireg = 0;
		return sNada;
	}
	*ireg = i1+1;
	return cLev1[i1].GetBuffer();
}

//---------------------------------------------------------------------------------------Region2
// Get level 2 region name (Geographic region) for FE code
char *CFlinn::Region2(int code) {
	if(code < 1 || code > MAX_LEV2)
		return sNada;
	int i2 = code-1;
	return cLev2[i2].GetBuffer();
}

