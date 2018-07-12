#include "ITravelTime.h"

class CMod;
struct IMessage;
class CTravelTime : public ITravelTime {
// attributes
public:
	CMod *pMod;

// Methods
public:
	CTravelTime();
	~CTravelTime();
	void Init(CMod *mod);
	bool Action(IMessage *msg);
	bool Ellipticity(char *elf);
	double Delta(double lat1, double lon1, double lat2, double lon2);
	double Azimuth(double lat1, double lon1, double lat2, double lon2);
	TTT *Best(double d, double t, double z);
	TTT *T(int ittt, double d, double z);
	TTT *D(int ittt, double t, double z);
	int Tall(double d, double z, TTT *ttt);
	int Dall(double t, double z, TTT *ttt);
	int NTTT();
	double DMin(int ittt);
	double DMax(int ittt);
	double TMin(int ittt);
	double TMax(int ittt);
	double ZMin(int ittt);
	double ZMax(int ittt);
};
