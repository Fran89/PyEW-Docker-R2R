#!/bin/bash

DIRNAME=`dirname $0`

HEADERFILE=${DIRNAME}/HEADERFILE

SVNAUTHORS=${DIRNAME}/SVNAUTHORS
CHANGELOGFILE=ChangeLog
CHANGELOGSVN2CLFILE=ChangeLogSVN2CL

date "+%Y-%m-%d %H:%M" > ${HEADERFILE}
echo "" >> ${HEADERFILE}
echo "	* Mole:" >> ${HEADERFILE}
echo "" >> ${HEADERFILE}
echo "	   An open near real-time database-centric Earthworm subsystem." >> ${HEADERFILE}
echo "	   Matteo Quintiliani and Stefano Pintore" >> ${HEADERFILE}
echo "	   Istituto Nazionale di Geofisica e Vulcanologia - Italy" >> ${HEADERFILE}
echo "	   Mail bug reports and suggestions to <quintiliani@ingv.it> " >> ${HEADERFILE}
echo "" >> ${HEADERFILE}

svn2cl -a -i --authors=${SVNAUTHORS} --linelen=80 -f ${CHANGELOGSVN2CLFILE}
cat ${HEADERFILE} ${CHANGELOGSVN2CLFILE} > ${CHANGELOGFILE}
rm -f ${HEADERFILE} ${CHANGELOGSVN2CLFILE}

