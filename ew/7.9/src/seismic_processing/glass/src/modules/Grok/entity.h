// entity.h
#ifndef ENTITY_H
#define ENTITY_H

class CMapWin;
class CEntity {
public:
// Attributes

// Methods
	CEntity();
	virtual ~CEntity();
	virtual void Render(HDC hdc, CMapWin *map);
};

#endif
