#ifndef FLINN_H
#define FLINN_H
#include "str.h"

#define MAX_LEV1 64
#define MAX_LEV2 800

class CFlinn {
public:
// Attribues
	CStr cLev1[MAX_LEV1];
	int  iLev2[MAX_LEV2];
	CStr cLev2[MAX_LEV2];
	int iReg[180*360];

// Methods
	CFlinn();
	virtual ~CFlinn();
	int Ix(double lat, double lon);
	bool Load(char *root);
	bool Quad(char *quad, int iquad);
	int Code(double lat, double lon);
	char *Region1(int code);
	char *Region1(int code, int *ireg);
	char *Region2(int code);
};

#endif
