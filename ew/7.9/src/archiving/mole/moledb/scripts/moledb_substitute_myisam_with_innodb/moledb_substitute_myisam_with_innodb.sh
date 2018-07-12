#!/bin/bash

DBNAME=mole
DBHOST=localhost
DBUSER=root
DBPASS=ciccione33

# FLAG_EXECSQL=yes
FLAG_EXECSQL=no

GSED=`which gsed`
if [ -z "${GSED}" ]; then
  GSED=sed
fi

# File where are saved all executed SQL commands 
FILEALTERMOLESTATIC="$0.static.sql"

DIRTMP=`dirname $0`/tmp
FTMP=${DIRTMP}/`basename $0`.tmp
FTMPVIEW=${DIRTMP}/`basename $0`.view.tmp
FTMPPROC=${DIRTMP}/`basename $0`.proc.tmp
FTMPFUNC=${DIRTMP}/`basename $0`.func.tmp
FTMPTRIG=${DIRTMP}/`basename $0`.trig.tmp
FTMPGREP=${DIRTMP}/`basename $0`.tmp.grep
FTMPCOUNTDEL=${DIRTMP}/`basename $0`.tmp.countdelete

function init_list_mole_tables() {
# select table_schema, table_name, table_type, engine, table_rows from tables where table_schema='${DBNAME}' and table_type='BASE TABLE';
SQLLISTMOLETABLES="select table_name  from tables where table_schema='${DBNAME}' and table_type='BASE TABLE' \G"
LISTMOLETABLES="`mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} information_schema -e "${SQLLISTMOLETABLES}" | grep "table_name:" | ${GSED} -e "s/table_name: //"`"
}

function init_list_mole_views() {
SQLLISTMOLEVIEWS="select table_name  from tables where table_schema='${DBNAME}' and table_type='VIEW' \G"
LISTMOLEVIEWS="`mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} information_schema -e "${SQLLISTMOLEVIEWS}" | grep "table_name:" | ${GSED} -e "s/table_name: //"`"
}

function init_list_mole_procedures() {
SQLLISTMOLEPROCEDURES="select routine_name from routines where routine_schema='${DBNAME}' and routine_type='PROCEDURE' \G"
LISTMOLEPROCEDURES="`mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} information_schema -e "${SQLLISTMOLEPROCEDURES}" | grep "routine_name:" | ${GSED} -e "s/routine_name: //"`"
}

function init_list_mole_functions() {
SQLLISTMOLEFUNCTIONS="select routine_name from routines where routine_schema='${DBNAME}' and routine_type='FUNCTION' \G"
LISTMOLEFUNCTIONS="`mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} information_schema -e "${SQLLISTMOLEFUNCTIONS}" | grep "routine_name:" | ${GSED} -e "s/routine_name: //"`"
}

function init_list_mole_triggers() {
SQLLISTMOLETRIGGERS="select trigger_name from triggers where trigger_schema='${DBNAME}' \G"
LISTMOLETRIGGERS="`mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} information_schema -e "${SQLLISTMOLETRIGGERS}" | grep "trigger_name:" | ${GSED} -e "s/trigger_name: //"`"
}

function execute_mysql_from_file_and_log() {
  local FILESQLSTR="$1"
  if [ "${FLAG_EXECSQL}" == "yes" ]; then
    time -p cat ${FILESQLSTR} | mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME}
  else
    echo cat ${FILESQLSTR} \| mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME}
  fi
  cat ${FILESQLSTR} >> ${FILEALTERMOLESTATIC}
}

function execute_mysql_and_log() {
  local SQLSTR="$1"
  if [ "${FLAG_EXECSQL}" == "yes" ]; then
    time -p mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}"
  else
    echo mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}"
  fi
  echo "${SQLSTR}" >> ${FILEALTERMOLESTATIC}
}


rm -f ${FILEALTERMOLESTATIC}
echo "-- Created `date`" >> ${FILEALTERMOLESTATIC}
echo "" >> ${FILEALTERMOLESTATIC}
	
mkdir -p ${DIRTMP}
rm -f ${DIRTMP}/*tmp*

#################################
# VIEWS INIT
#################################
init_list_mole_views

for V in ${LISTMOLEVIEWS}; do
  SQLSTR="SHOW CREATE VIEW ${V} \G"
  mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}" > ${FTMP}
  echo "DROP VIEW IF EXISTS ${V};" > ${FTMPVIEW}.${V}
done

#################################
# PROCEDURES INIT
#################################
init_list_mole_procedures

for P in ${LISTMOLEPROCEDURES}; do
  SQLSTR="SHOW CREATE PROCEDURE ${P} \G"
  mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}" > ${FTMP}
  echo "DROP PROCEDURE IF EXISTS ${P};" > ${FTMPPROC}.${P}
done

#################################
# FUNCTIONS INIT
#################################
init_list_mole_functions

for F in ${LISTMOLEFUNCTIONS}; do
  SQLSTR="SHOW CREATE FUNCTION ${F} \G"
  mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}" > ${FTMP}
  echo "DROP FUNCTION IF EXISTS ${F};" > ${FTMPFUNC}.${F}
done

#################################
# TRIGGERS INIT
#################################
init_list_mole_triggers

for T in ${LISTMOLETRIGGERS}; do
  SQLSTR="SHOW CREATE TRIGGER ${T} \G"
  mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}" > ${FTMP}
  echo "DROP TRIGGER IF EXISTS ${T};" > ${FTMPTRIG}.${T}
done


#################################
# TRIGGERS
#################################
echo "" >> ${FILEALTERMOLESTATIC}
echo "-- TRIGGERS" >> ${FILEALTERMOLESTATIC}
for T in ${LISTMOLETRIGGERS}; do
  echo "-- TRIGGERS ${T}"
  execute_mysql_from_file_and_log ${FTMPTRIG}.${T}
  echo ""
done

#################################
# PROCEDURES
#################################
echo "" >> ${FILEALTERMOLESTATIC}
echo "-- PROCEDURES" >> ${FILEALTERMOLESTATIC}
for P in ${LISTMOLEPROCEDURES}; do
  echo "-- PROCEDURE ${P}"
  execute_mysql_from_file_and_log ${FTMPPROC}.${P}
  echo ""
done

#################################
# FUNCTIONS
#################################
echo "" >> ${FILEALTERMOLESTATIC}
echo "-- FUNCTIONS" >> ${FILEALTERMOLESTATIC}
for F in ${LISTMOLEFUNCTIONS}; do
  echo "-- FUNCTION ${F}"
  execute_mysql_from_file_and_log ${FTMPFUNC}.${F}
  echo ""
done

#################################
# VIEWS
#################################
echo "" >> ${FILEALTERMOLESTATIC}
echo "-- VIEWS" >> ${FILEALTERMOLESTATIC}
for V in ${LISTMOLEVIEWS}; do
  echo "-- VIEW ${V}"
  execute_mysql_from_file_and_log ${FTMPVIEW}.${V}
  echo ""
done


#################################
# TABLES
#################################
init_list_mole_tables

echo "" >> ${FILEALTERMOLESTATIC}
echo "-- TABLES" >> ${FILEALTERMOLESTATIC}
for T in ${LISTMOLETABLES}; do
  execute_mysql_and_log "RENAME TABLE ${T} TO old_myisam_${T};"
done

