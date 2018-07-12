// array.h
#ifndef ARRAY_H
#define ARRAY_H

#define MAX_ARRAY	100000
class CArray {
public:
// Attributes
	int mObj;
	int nObj;
	void **pObj;

// Methods
	CArray();
	virtual ~CArray();
	int Add(void *obj);
	int GetSize();
	void *GetAt(int i);
	void Delete(int i);
	void RemoveAll();
};

#endif
