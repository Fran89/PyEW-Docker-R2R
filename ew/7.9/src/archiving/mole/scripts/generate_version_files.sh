#!/bin/bash

SYNTAX="`basename $0`  MOLEROOTDIR"

MOLEROOTDIR="$1"
MAINVERFILE="${MOLEROOTDIR}/VERSION"

MOLEDBVERFILE="${MOLEROOTDIR}/moledb/data/mole/version.sql"
EW2MOLEDBVERFILE="${MOLEROOTDIR}/ew2moledb/ew2moledb_version.h"
MOLEFACEVERFILE="${MOLEROOTDIR}/moleface/moleface_version.php"

COMMENT="DO NOT EDIT. Automatically generated. Change file VERSION in mole main directory."

if [ ! -d "${MOLEROOTDIR}" ]; then
  echo "${SYNTAX}"
  exit
else
  MOLEDBVER=`grep -w moledb ${MAINVERFILE} | awk '{print $2;}'`
  MOLEDBDATE=`grep -w moledb ${MAINVERFILE} | awk '{print $3;}'`
  EW2MOLEDBVER=`grep -w ew2moledb ${MAINVERFILE} | awk '{print $2;}'`
  EW2MOLEDBDATE=`grep -w ew2moledb ${MAINVERFILE} | awk '{print $3;}'`
  MOLEFACEVER=`grep -w moleface ${MAINVERFILE} | awk '{print $2;}'`
  MOLEFACEDATE=`grep -w moleface ${MAINVERFILE} | awk '{print $3;}'`

  # CALL sp_update_mole_version('1.2.1', '2013-06-03');
  echo "-- ${COMMENT}" > ${MOLEDBVERFILE}
  echo "CALL sp_update_mole_version('${MOLEDBVER}', '${MOLEDBDATE}');" >> ${MOLEDBVERFILE}

  # #define EW2MOLEDB_VERSION "1.2.2 - 2013-06-03"
  echo "/* ${COMMENT} */" > ${EW2MOLEDBVERFILE}
  echo "#define EW2MOLEDB_VERSION \"${EW2MOLEDBVER} - ${EW2MOLEDBDATE}\"" >> ${EW2MOLEDBVERFILE}

  # CALL sp_update_mole_version('1.2.1', '2013-06-03');
  echo "-- ${COMMENT}" > ${MOLEDBVERFILE}
  echo "CALL sp_update_mole_version('${MOLEDBVER}', '${MOLEDBDATE}');" >> ${MOLEDBVERFILE}

  echo "<?php" > ${MOLEFACEVERFILE}
  echo "/* ${COMMENT} */" >> ${MOLEFACEVERFILE}
  echo "\$moleface_version = \"${MOLEFACEVER}\";" >> ${MOLEFACEVERFILE}
  echo "\$moleface_date = \"${MOLEFACEDATE}\";" >> ${MOLEFACEVERFILE}
  echo "?>" >> ${MOLEFACEVERFILE}

fi

