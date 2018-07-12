//glint.h: Glint interface service module
#include "IGlint.h"

#define GLINT_MODE_NULL			0
#define GLINT_MODE_ORIGIN_T		1
#define GLINT_MODE_PICK_T		2
#define GLINT_MODE_WAIF			3
#define GLINT_MODE_PICK_ORG		4

#define GLINT_PICKLIST_SIZE 10
typedef struct _PickListStruct
{
  BOOL bValid;
  int iCurrentPick;
} PickListStruct;


class CGlint : public IGlint {
public:
// Attributes
	char		sEntity[8];
	int			iPickEntity;
	int			iOriginEntity;
	int			nMode;			// Current scan mode
	int			iScan;			// Scan index
	int			iOrigin;		// Origin if scanning for origin
  PickListStruct pPLS[10];


// Methods
	CGlint();
	~CGlint();
	int  OriginEntity();
	int  PickEntity();
	bool putPick(PICK *pck);
	bool putOrigin(ORIGIN *org);
	bool OriginPick(char *org, char *pck);
	bool UnOriginPick(char *org, char *pck);
	PICK *getPickFromidPick(char *ent);
	PICK *getPicksForTimeRange(double t1, double t2, size_t* hRef);
	PICK *getWaifsForTimeRange(double t1, double t2, size_t* hRef);
	PICK *getPicksFromOrigin(ORIGIN *org, size_t* hRef);
//	PICK *getPick(char *ent);
//	PICK *getPick(double t1, double t2);
//	PICK *getWaif(double t1, double t2);
//	PICK *getPick(ORIGIN *org);
	ORIGIN *getOrigin(char *ent);
//	ORIGIN *getOriginFromNum(int iOriginNum);
  ORIGIN * getOriginForPick(PICK * pPick);
	ORIGIN *getOrigin(double t1, double t2);
  int ComparePickChannels(PICK * pck, PICK * pPick);
  void endPickList(size_t * piPickRef);
  BOOL ValidatePickList(PickListStruct * pPLI);
  PickListStruct * getPickList();
  bool deleteOrigin(char *ent);

	void Pau();
} ;
