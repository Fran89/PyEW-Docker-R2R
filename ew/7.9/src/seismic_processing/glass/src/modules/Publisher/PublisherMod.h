#include <module.h>
#include <IGlint.h>
#include "publish.h"

typedef struct _PHS {
	PICK	*pPck;
} PHS;
#define MAX_PHS 2000

class CScroll;
class CMod : public CModule {
// attributes
public:
	CScroll	*pScroll;
	char	idEntity[24];
	IGlint	*pGlint;
	int		nPhs;
	PHS		Phs[MAX_PHS];

// Methods
public:
	CMod();
	~CMod();
	bool Action(IMessage *msg);
	bool Poll();
  void  Publish(char * idOrigin, int iVersion);
  void  Retract(char * idOrigin, int iOrigin);
  void  PrunePick(char * idOrigin, char * idPick);
};