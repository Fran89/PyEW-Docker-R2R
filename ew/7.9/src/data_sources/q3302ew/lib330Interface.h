#ifndef __LIB330INTERFACE_H__
#define __LIB330INTERFACE_H__
#include <libclient.h>
#include <libtypes.h>
#include <libmsgs.h>

/*
 * Somewhere along the way we end up with a bunch of pascalisms defined.
 * This one is particularly important elsewhere
 */
#undef mod
#undef addr

void lib330Interface_initialize();
void lib330Interface_handlerError(enum tliberr errcode);
void lib330Interface_initializeCreationInfo();
void lib330Interface_initializeRegistrationInfo();
void lib330Interface_stateCallback(pointer p);
void lib330Interface_msgCallback(pointer p);
void lib330Interface_1SecCallback(pointer p);
void lib330Interface_libStateChanged(enum tlibstate newState);
void lib330Interface_displayStatusUpdate();
void lib330Interface_startDataFlow();
void lib330Interface_startRegistration();
void lib330Interface_changeState(enum tlibstate newState, enum tliberr reason);
void lib330Interface_startDeregistration();
void lib330Interface_ping();
void lib330Interface_cleanup();
int lib330Interface_waitForState(enum tlibstate waitFor, int maxSecondsToWait);
enum tlibstate lib330Interface_getLibState();
#endif
