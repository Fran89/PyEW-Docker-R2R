/*
 *   THIS FILE IS UNDER RCS - DO NOT MODIFY UNLESS YOU HAVE
 *   CHECKED IT OUT USING THE COMMAND CHECKOUT.
 *
 *    $Id: lm_xml_event.h 2903 2007-03-29 20:09:50Z paulf $
 *
 *    Revision history:
 *     $Log$
 *     Revision 1.1  2007/03/29 20:09:50  paulf
 *     added eventXML option from INGV. This option allows writing the Shakemap style event information out as XML in the SAC out dir
 *
 *     Revision 1.1  2005/11/16 09:26:49  mtheo
 *     Added xml_event_write()
 *     	write shakemap event XML file
 *
 *
 *
 */
/*
 * lm_xml_event.h: write shakemap event XML file.
 * Author: Matteo Quintiliani - quintiliani@ingv.it
 */

#ifndef LM_XML_EVENT_H
#define LM_XML_EVENT_H

#include <earthworm.h>
#include "lm.h"
#include "lm_config.h"

int xml_event_write( EVENT *pEvt, LMPARAMS *plmParams );

#endif
