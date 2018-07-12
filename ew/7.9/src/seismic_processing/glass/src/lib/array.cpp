// array.cpp: Implements general array of objects
#include "array.h"

//---------------------------------------------------------------------------------------CArray
CArray::CArray() {
	mObj = 20;
	nObj = 0;
	pObj = (void **)(new int[20]);
}

//---------------------------------------------------------------------------------------~CArray
// Destructor
//		CStr *pstr = new CStr();
//		delete pstr;
CArray::~CArray() {
	delete [] pObj;
}

//---------------------------------------------------------------------------------------Add
// returns index of new element in Array or -1 on error // DK 20030617
int CArray::Add(void *obj) {
	void **ptmp;
	int inc;
	int i;
	if(nObj >= mObj) {
		ptmp = pObj;
		inc = mObj/10;
		if(inc < 10)
			inc = 10;
		mObj += inc;
		if(mObj > MAX_ARRAY)
			mObj = MAX_ARRAY;
		pObj = (void **)(new int[mObj]);
		for(i=0; i<nObj; i++)
			pObj[i] = ptmp[i];
		delete [] ptmp;
	}
	if(nObj >= MAX_ARRAY)
		return -1;
	pObj[nObj++] = obj;
	return nObj-1;
}

//---------------------------------------------------------------------------------------GetSize
int CArray::GetSize() {
	return nObj;
}

//---------------------------------------------------------------------------------------GetAt
void *CArray::GetAt(int i) {
	return pObj[i];
}

//---------------------------------------------------------------------------------------Delete
void CArray::Delete(int i) {
	int j;
	if(i < 0)
		return;
	if(i >= nObj)
		return;
	for(j=i+1; j<nObj; j++)
		pObj[j-1] = pObj[j];
	nObj--;
}

//---------------------------------------------------------------------------------------RemoveAll
void CArray::RemoveAll() {
	delete [] pObj;
	mObj = 20;
	nObj = 0;
	pObj = (void **)(new int[20]);
}
