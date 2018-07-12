#!/bin/bash

#################################
# Set the following variables
#################################
MYSQL_CMD=mysql
MYSQLFORMAT_CMD=mysql_format

DBMOLEHOST=localhost
DBMOLEPORT=3306
DBMOLENAME=mole
DBMOLEUSER=moleuser
DBMOLEPASS=molepass

DBSEISNETHOST=localhost
DBSEISNETPORT=3306
DBSEISNETNAME=seisnet
DBSEISNETUSER=adsreader
DBSEISNETPASS=adsreader

FILESQLSCNL=seisnet_scnl.sql
FILEFMTSCNL=seisnet_scnl.format

FILESQLCALLS=seisnet_scnl.out
FILELOGCALLS=seisnet_scnl.log

echo ""
echo "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * "
echo "* * * Script for initializing SCNL in a mole-database * * * "
echo "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * "
echo ""

########################################################
# Create main user and grant
########################################################
echo "Database '${DBSEISNETNAME}' -- Get scnl information from ${DBSEISNETHOST}.${DBSEISNETNAME}..."
${MYSQLFORMAT_CMD} -h ${DBSEISNETHOST} -u ${DBSEISNETUSER} -p ${DBSEISNETPASS} -d ${DBSEISNETNAME} -s ${FILESQLSCNL} -f ${FILEFMTSCNL} > ${FILESQLCALLS}

echo "Database '${DBMOLENAME}' -- Put scnl information to ${DBMOLEHOST}.${DBMOLENAME}..."
${MYSQL_CMD} -h ${DBMOLEHOST} -u ${DBMOLEUSER} -p${DBMOLEPASS} ${DBMOLENAME} < ${FILESQLCALLS} 2>&1 > ${FILELOGCALLS}

less ${FILELOGCALLS}

