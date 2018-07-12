#!/bin/bash

SYNTAX="Syntax: `basename $0`  dbname  dbdirectory  [ grants_configuration_file ]"

DATABASENAME=$1
DATABASEDIR=$2
GRANTFILECONF=$3

GRANTTPLDIR=${DATABASEDIR}/grants
if [ -z "${GRANTFILECONF}" ]; then
  GRANTFILECONF=${DATABASEDIR}/conf/grants.conf
fi

if [ -z "${DATABASENAME}" ]; then
  echo "${SYNTAX}"
  exit
fi
if [ -z "${DATABASEDIR}" ]; then
  echo "${SYNTAX}"
  exit
fi

if [ ! -d "${GRANTTPLDIR}" ]; then
  echo "Error: directory ${GRANTTPLDIR} not found. Exit."
  exit
fi

if [ ! -f "${GRANTFILECONF}" ]; then
  echo "Error: file ${GRANTFILECONF} not found. Exit."
  exit
fi


function echo_mysql_comment {
  echo "-- $1" 1>&2
}

FTMP=`mktemp -t adsdbs_grants.XXXX`

SEPLINE="============================================================"
SEPLINE2="- - - - - - - - - - - - - - - - - - - - - - - - - - - - - -"

NLINE=1
cat ${GRANTFILECONF} | while read userline; do
  USERNAME=""
  DBHOSTNAME=""
  USERPASS=""
  USERGRANTTPL=""
  # remove comments, spaces and tabs from userline
  userline=`echo "${userline}" | sed -e "s/#.*$//g" | sed -e "s/[ 	]*//g"`
  if [ ! -z ${userline} ]; then
    echo_mysql_comment "${SEPLINE2}"
    USERNAME=`echo ${userline} | cut -s -d';' -f 1`
    DBHOSTNAME=`echo ${userline} | cut -s -d';' -f 2`
    USERPASS=`echo ${userline} | cut -s -d';' -f 3`
    USERGRANTTPL=`echo ${userline} | cut -s -d';' -f 4`
    USERPASSLENGTH=`echo ${USERPASS} | wc -c | sed -e "s/[ ]*//g"`
    USERPASSFIRSTCHAR=`echo ${USERPASS} | cut -b 1`
    USERGRANTTPL_FULLPATH="${GRANTTPLDIR}/${USERGRANTTPL}"
    if [ ! -z "${USERNAME}" ] && [ ! -z "${DBHOSTNAME}" ] && [ ! -z "${USERPASS}" ] && [ ! -z "${USERGRANTTPL}" ] && [ -f ${USERGRANTTPL_FULLPATH} ]; then
      echo_mysql_comment "User: ${USERNAME}, Pass: ${USERPASS}, PassLength: ${USERPASSLENGTH}, Host: ${DBHOSTNAME}, GrantTemplate: ${USERGRANTTPL}"
      if [ ${USERPASSLENGTH} -ne 42 ] || [ "${USERPASSFIRSTCHAR}" != "*" ]; then
	echo_mysql_comment "WARNING: password should be encrypted for user ${USERNAME}."
	# the password is not encrypted
	GRANTPASSOPT=""
      else
	# the password is encrypted
	GRANTPASSOPT="PASSWORD"
      fi

      # TODO: reset all privileges for current user on current database (i.e. REVOKE ALL PRIVILEGES, GRANT OPTION  FROM '__USERNAME__'@'$__HOSTNAME__';)

      # GRANT USAGE for current user added by this script
      GRANTUSAGEUSER="GRANT USAGE ON ${DATABASENAME}.* TO '${USERNAME}'@'${DBHOSTNAME}' IDENTIFIED BY ${GRANTPASSOPT} '${USERPASS}';"
      echo "${GRANTUSAGEUSER}"

      sed -e "s/__DATABASENAME__/${DATABASENAME}/g" -e "s/__USERNAME__/${USERNAME}/g" -e "s/__USERPASS__/${USERPASS}/g" -e "s/__HOSTNAME__/${DBHOSTNAME}/g" ${USERGRANTTPL_FULLPATH}
    else
      echo_mysql_comment "ERROR in ${GRANTFILECONF} at line ${NLINE}: \"${userline}\""
      if [ -z "${USERNAME}" ]; then
	echo_mysql_comment "ERROR: user name is missing."
      fi
      if [ -z "${DBHOSTNAME}" ]; then
	echo_mysql_comment "ERROR: database hostname  is missing."
      fi
      if [ -z "${USERPASS}" ]; then
	echo_mysql_comment "ERROR: user password is missing."
      fi
      if [ -z "${USERGRANTTPL}" ]; then
	echo_mysql_comment "ERROR: grant template file  is missing."
      else
	if [ ! -f ${USERGRANTTPL_FULLPATH} ]; then
	  echo_mysql_comment "ERROR: grant template file ${USERGRANTTPL_FULLPATH} not found. Skip."
	fi
      fi
    fi
  fi
  NLINE=$((${NLINE} + 1))
done

echo ""
echo "-- Last SQL commands"
echo "FLUSH PRIVILEGES;"
echo "COMMIT;"

