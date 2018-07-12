#include "module.h"

class CMod : public CModule {
// attributes
public:
	int iState;
	FILE *fFile;
	int nHyp;
	int iYr;
	int iMo;
	int iDa;
	int	iHr;
	int iMn;

// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
	bool Poll();
	bool NEIC(char *file);
	void NEIC();
	bool PDE(char *file);
	void PDE();
	bool EW(char *file);
	void EW();
};
