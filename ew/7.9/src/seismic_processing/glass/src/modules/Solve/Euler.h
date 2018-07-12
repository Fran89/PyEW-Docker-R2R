// euler.h
#ifndef _EULER_H_
#define _EULER_H_
#include "comfile.h"

class CVector;
class CEuler
{
// Attributes
public:
	double	dYaw;
	double	dPitch;
	double	dRoll;

// Operations
public:
	CEuler();
	CEuler(const CEuler &ce);
	CEuler(const CVector &cv);
	virtual ~CEuler();
	CEuler &operator=(const CEuler&);
	CEuler &operator=(const CVector &cv);
	CEuler(double yaw, double pitch, double roll);
};

#endif
