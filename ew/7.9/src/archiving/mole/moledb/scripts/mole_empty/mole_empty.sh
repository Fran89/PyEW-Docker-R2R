#!/bin/bash

# FLAG_DELETE=yes
FLAG_DELETE=no

# option to not replicate delete on slave
#      Use the following MySQL command:
#            SET SQL_LOG_BIN=0;
#      http://dev.mysql.com/doc/refman/5.0/en/set-sql-log-bin.html

# FLAG_NOTPROP_TO_SLAVE=no
FLAG_NOTPROP_TO_SLAVE=yes
SQLLOGBINOFF="SET SQL_LOG_BIN=0;"

DATESTART='2011-06-01 00:00:00'
DATEEND='2013-04-01 00:00:00'

DATEFORSECURITY='2013-04-01 00:00:00'

FTMP=/tmp/`basename $0`.tmp
FTMPGREP=/tmp/`basename $0`.tmp.grep

function mysql_test_sql_log_bin_off {
  local myuser="$1"
  local mypass="$2"
  local myhost="$3"
  local myname="$4"
  local mysqlstr="$SQLLOGBINOFF"
  local myftmp=/tmp/`basename $0`.mysql_test_sql_log_bin_off.tmp

  mysql -u${myuser} -p${mypass} -h ${myhost} ${myname} -e "${mysqlstr}" > ${myftmp} 2>&1

  # NLINES=`cat ${myftmp} | wc -l`

  # ERROR 1227 (42000) at line 1: Access denied; you need the SUPER privilege for this operation
  grep "Access denied" ${myftmp} > /dev/null 2>&1
  ACCESSDENIED=$?

  if [ ${ACCESSDENIED} -ne 1 ]; then
    cat ${myftmp} 1>&2
    echo "User ${myuser} has not privilege for disable SQL_LOG_BIN. Exit." 1>&2
    exit
  fi
}

function mysql_exec {
  local myuser="$1"
  local mypass="$2"
  local myhost="$3"
  local myname="$4"
  local mysqlstr="$5"

  if [ "${FLAG_NOTPROP_TO_SLAVE}" == "yes" ]; then
    mysql_test_sql_log_bin_off ${myuser} ${mypass} ${myhost} ${myname}
    mysqlstr="${SQLLOGBINOFF} ${mysqlstr}";
  fi

  mysql -u${myuser} -p${mypass} -h ${myhost} ${myname} -e "${mysqlstr}"
}

DBNAME=mole
DBHOST=hdbrm
DBUSER=moleuser
DBPASS=molepass
# DBUSER=adsreader
# DBPASS=adsreader

LISTTABLEEWSQKID="
ew_strongmotionII
ew_magnitude_phase
ew_magnitude_summary
ew_quake2k
ew_link
ew_arc_phase
ew_arc_summary
"

EWSQKIDTODEL=XXX

while [ ! -z "${EWSQKIDTODEL}" ]; do

  SQLEWSQKID="select a.ot_dt, a.ewsqkid from ew_arc_summary a where a.ot_dt >= '${DATESTART}' AND a.ot_dt < '${DATEEND}' AND a.ot_dt < '${DATEFORSECURITY}' ORDER BY a.ot_dt LIMIT 1 \G"

  mysql_exec ${DBUSER} ${DBPASS} ${DBHOST} ${DBNAME} "${SQLEWSQKID}" > ${FTMP}

  grep -w ewsqkid ${FTMP} > ${FTMPGREP}
  RET=$?
  EWSQKIDTODEL=`cat ${FTMPGREP} | sed -e "s/[ \t]*ewsqkid:[ \t]*//"`

  if [ ${RET} -eq 0 ] && [ ! -z "${EWSQKIDTODEL}" ]; then

    EWOTDT=`grep -w ot_dt ${FTMP} | sed -e "s/[ \t]*ot_dt:[ \t]*//"`

    echo "* * * ewsqkid to delete is ${EWSQKIDTODEL}, ${EWOTDT}"

    if [ "${FLAG_DELETE}" == "yes" ]; then
      SQLDEL="DELETE FROM ew_sqkseq WHERE id=${EWSQKIDTODEL};" 
      mysql_exec ${DBUSER} ${DBPASS} ${DBHOST} ${DBNAME} "${SQLDEL}"
    fi

    SQLDEL=""

    for T in ${LISTTABLEEWSQKID}; do
      # echo "TABLE ${T}"

      # SQLSELECT="SELECT COUNT(id) FROM ${T} WHERE ewsqkid=${EWSQKIDTODEL} \G" 
      # mysql_exec ${DBUSER} ${DBPASS} ${DBHOST} ${DBNAME} "${SQLSELECT}" | grep COUNT

      if [ "${FLAG_DELETE}" == "yes" ]; then
	SQLDEL="${SQLDEL} DELETE FROM ${T} WHERE ewsqkid=${EWSQKIDTODEL};" 
	mysql_exec ${DBUSER} ${DBPASS} ${DBHOST} ${DBNAME} "${SQLDEL}"
      fi

    done

    mysql_exec ${DBUSER} ${DBPASS} ${DBHOST} ${DBNAME} "${SQLDEL}"

  fi

done

