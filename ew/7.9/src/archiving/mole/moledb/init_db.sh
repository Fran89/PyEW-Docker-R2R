#!/bin/bash

SYNTAX="Syntax: `basename $0`  root-password  dbname-dbowner  dbowner-password  dbdirectory  [ flag_root 'yes' or 'no' ]"

ROOTPASS=$1
DATABASENAME=$2
DBOWNERPASS=$3
DBDIR=$4
FLAG_ROOT=$5

if [ -z "${FLAG_ROOT}" ]; then
  FLAG_ROOT=yes
elif [ "${FLAG_ROOT}" != "yes" ] && [ "${FLAG_ROOT}" != "no" ]; then
  echo "Error: flag_root must be 'yes' or 'no'. Exit."
  echo "${SYNTAX}"
  exit
fi

if [ -z "${DBOWNERPASS}" ]; then
  echo "${SYNTAX}"
  exit
fi
if [ -z "${ROOTPASS}" ]; then
  echo "${SYNTAX}"
  exit
fi
if [ -z "${DATABASENAME}" ]; then
  echo "${SYNTAX}"
  exit
fi
if [ -z "${DBDIR}" ]; then
  echo "${SYNTAX}"
  exit
fi

if [ ! -d "${DBDIR}" ]; then
  echo "${SYNTAX}"
  echo "Error: directory ${DBDIR} not found. Exit."
  exit
fi

MYSQLCMD="mysql"

# check root password
if [ "${FLAG_ROOT}" == "yes" ]; then
  ${MYSQLCMD} -uroot -p${ROOTPASS} -e "SELECT 1;"
  RET=$?
  if [ $RET -eq 1 ]; then
    echo "Error: root password is wrong. Exit."
    exit
  fi
fi

INIT_GRANTS=`dirname $0`/init_grants.sh
GRANTSQLPRE=`dirname $0`/grant_pre.sql
GRANTSQLPOST=${DBDIR}/grants/grant_post.tmp.sql

if [ ! -f "${GRANTSQLPRE}" ]; then
  echo "Error: ${GRANTSQLPRE} not found. Exit."
  exit
fi

DBSCRIPTSCONF=${DBDIR}/conf/db_scripts.conf

if [ ! -f "${DBSCRIPTSCONF}" ]; then
  echo "Error: file ${DBSCRIPTSCONF} not found. Exit."
  exit
fi

# ATTENZIONE: verranno eseguiti gli script contenuti in ${DBSCRIPTSCONF}.
# Se specificata una directory, allora saranno eseguiti tutti i file con
# estensione .sql contenuti in quella directory, ma non nelle sue
# sottodirectory.

LISTSQLSCRIPTS=`sed -e "s/#.*$//g" ${DBSCRIPTSCONF}`

if [ "${FLAG_ROOT}" == "yes" ]; then
  echo ${GRANTSQLPRE}
  sed -e "s/__DATABASENAME__/${DATABASENAME}/g" -e "s/__DBOWNERPASS__/${DBOWNERPASS}/g" ${GRANTSQLPRE} | ${MYSQLCMD} -uroot -p${ROOTPASS} ${MYSQLCMD}
fi

FTMP=`mktemp -t adsdbs.XXXX`
echo "Temporary file: ${FTMP}"

for D in ${LISTSQLSCRIPTS}; do
  echo "= = = = = = = = = ="
  echo ${D}
  for F in `find ${DBDIR}/${D} -maxdepth 1 -name "*.sql"`; do
    echo "SET foreign_key_checks=0;" > ${FTMP}
    cat ${F} >> ${FTMP}
    ${MYSQLCMD} -u${DATABASENAME} -p${DBOWNERPASS} ${DATABASENAME} < ${FTMP}
  done
done

if [ "${FLAG_ROOT}" == "yes" ]; then
  # Generate grants
  ${INIT_GRANTS} ${DATABASENAME} ${DBDIR} > ${GRANTSQLPOST} 2>&1

  grep -E "^[ ]*--" ${GRANTSQLPOST}

  echo ${GRANTSQLPOST}
  sed -e "s/__DATABASENAME__/${DATABASENAME}/g" -e "s/__DBOWNERPASS__/${DBOWNERPASS}/g" ${GRANTSQLPOST} | ${MYSQLCMD} -uroot -p${ROOTPASS} ${MYSQLCMD}
fi

