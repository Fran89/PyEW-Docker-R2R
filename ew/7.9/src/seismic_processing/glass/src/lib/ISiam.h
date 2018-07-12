// ISiam.h

#ifndef _ISIAM_H_
#define _ISIAM_H_

// This is the siam module interface base class. In many respects it is analogous to
// the IUnknown interface in the classical COM architecture.
// All SIAM module interfaces should extend this class.
struct ISiam {
public:
	virtual void Release() = 0;
};

#endif
