// euler.cpp : Attidute objects (yaw, pitch, roll)
#include "math.h"
#include "euler.h"
#include "vector.h"
#include "matrix.h"

static CVector vTmp;
static CEuler eTmp;

CEuler::CEuler()
{
	dYaw = 0.0;
	dPitch = 0.0;
	dRoll = 0.0;
}

CEuler::CEuler(const CVector &cv)
{
	CMatrix m(cv);
	dYaw = atan2(m[1][0], m[0][0]);
	dPitch = -asin(m[2][0]);
	dRoll = acos(m[2][2]/cos(dPitch));	
}

// Construct euler angle object from expliciit yaw, pitch, and roll values.
CEuler::CEuler(double yaw, double pitch, double roll)
{
	dYaw = yaw;
	dPitch = pitch;
	dRoll = roll;
}

// Copy constructor
CEuler::CEuler(const CEuler &ce)
{
	dYaw = ce.dYaw;
	dPitch = ce.dPitch;
	dRoll = ce.dRoll;
}

CEuler::~CEuler()
{
}

CEuler &CEuler::operator=(const CEuler& cv)
{
	dYaw = cv.dYaw;
	dPitch = cv.dPitch;
	dRoll = cv.dRoll;
	return *this;
}

CEuler &CEuler::operator=(const CVector &cv)
{
	return eTmp;
}
