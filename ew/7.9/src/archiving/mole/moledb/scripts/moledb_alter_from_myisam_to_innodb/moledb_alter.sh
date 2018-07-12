#!/bin/bash

DBNAME=mole
DBHOST=localhost
DBUSER=root
DBPASS=ciccione33

# FLAG_EXECSQLALTER=yes
FLAG_EXECSQLALTER=no

# FLAG_FK_CONSTRAINT_CHECK=yes
FLAG_FK_CONSTRAINT_CHECK=no

GSED=gsed

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

function init_queries() {
echo "-- init_queries"
I=0
for T in ${LISTMOLETABLES}; do
  TABLEALTER[${I}]="${DBNAME}.${T}"
  SQLSTRALTER[${I}]="X"
  echo "-- ${I} ${TABLEALTER[I]} ${SQLSTRALTER[I]}"
  I=$((I + 1))
done
}

function execute_mysql_from_file_and_log() {
  local FILESQLSTR="$1"
  if [ "${FLAG_EXECSQLALTER}" == "yes" ]; then
    time -p cat ${FILESQLSTR} | mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME}
  else
    echo cat ${FILESQLSTR} \| mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME}
  fi
  cat ${FILESQLSTR} >> ${FILEALTERMOLESTATIC}
}

function execute_mysql_and_log() {
  local SQLSTR="$1"
  if [ "${FLAG_EXECSQLALTER}" == "yes" ]; then
    time -p mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}"
  else
    echo mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}"
  fi
  echo "${SQLSTR}" >> ${FILEALTERMOLESTATIC}
}

function execute_cur_alter_table() {
I=0
for T in ${LISTMOLETABLES}; do
  echo "-- ${I} * * * Start `date`"
  if [ "${SQLSTRALTER[I]}" == "X" ]; then
    echo "WARNING: No alter to do for table ${TABLEALTER[I]}. Skip."
  else
    SQLSTRALTER[${I}]="ALTER TABLE ${TABLEALTER[I]} ${SQLSTRALTER[I]};"
    echo "${SQLSTRALTER[I]}"
    execute_mysql_and_log "${SQLSTRALTER[I]}"
  fi
  I=$((I + 1))
  echo "-- ${I} * * * End  `date`"
  echo ""
done
}

rm -f ${FILEALTERMOLESTATIC}
echo "-- Created `date`" >> ${FILEALTERMOLESTATIC}
echo "" >> ${FILEALTERMOLESTATIC}
	
mkdir -p ${DIRTMP}
rm -f ${DIRTMP}/*tmp*

RENATTR="
s/ewmodidpick/fk_module_pick/g
s/ewmodidloc/fk_module_loc/g
s/ewmodid/fk_module/g
s/scnlid/fk_scnl/g
s/ewsqkid/fk_sqkseq/g
s/ewid/fk_instance/g
s/ewspkid/fk_spkseq/g
"

STRSED="${GSED} "
for A in ${RENATTR}; do
  AC=`echo ${A} | ${GSED} -e "s=\(^s/[^/]\+\)=\1\\\\\>=" -e "s=s/=s/\\\\\<="`
  STRSED="${STRSED} -e ${AC}"
done

echo "${STRSED}"

#################################
# VIEWS INIT
#################################
init_list_mole_views

for V in ${LISTMOLEVIEWS}; do
  SQLSTR="SHOW CREATE VIEW ${V} \G"
  mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}" > ${FTMP}
  echo "DROP VIEW IF EXISTS ${V};" > ${FTMPVIEW}.${V}
  cat ${FTMP} | grep -E "[ \t]*Create View: " | ${GSED} -e "s/^[ \t]*Create View: //" | ${STRSED} >> ${FTMPVIEW}.${V}
  echo ";" >> ${FTMPVIEW}.${V}
done

#################################
# PROCEDURES INIT
#################################
init_list_mole_procedures

for P in ${LISTMOLEPROCEDURES}; do
  SQLSTR="SHOW CREATE PROCEDURE ${P} \G"
  mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}" > ${FTMP}
  echo "DROP PROCEDURE IF EXISTS ${P};" > ${FTMPPROC}.${P}
  echo "DELIMITER \$\$" >> ${FTMPPROC}.${P}
  STARTSTR="[ \t]*Create Procedure:"
  ENDSTR="[ \t]*character_set_client:"

  STATUS=0
  cat ${FTMP} | while IFS= read line; do
    case ${STATUS} in
      "0")
	echo "${line}" | grep -E "${STARTSTR}" 
	RET=$?
	if [ ${RET} -eq 0 ]; then
	  STATUS=1
	  echo "${line}" | ${GSED} -e "s/^${STARTSTR}//" | ${STRSED} >> ${FTMPPROC}.${P}
	fi
	;;
      "1")
	echo "${line}" | grep -E "${ENDSTR}" 
	RET=$?
	if [ ${RET} -eq 0 ]; then
	  STATUS=2
	else
	  echo "${line}" | ${STRSED} >> ${FTMPPROC}.${P}
	fi
	;;
      *)
	echo ""
	;;
    esac
  done

  echo "\$\$" >> ${FTMPPROC}.${P}
  echo "DELIMITER ;" >> ${FTMPPROC}.${P}

done

#################################
# FUNCTIONS INIT
#################################
init_list_mole_functions

for F in ${LISTMOLEFUNCTIONS}; do
  SQLSTR="SHOW CREATE FUNCTION ${F} \G"
  mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}" > ${FTMP}
  echo "DROP FUNCTION IF EXISTS ${F};" > ${FTMPFUNC}.${F}
  echo "DELIMITER \$\$" >> ${FTMPFUNC}.${F}
  STARTSTR="[ \t]*Create Function:"
  ENDSTR="[ \t]*character_set_client:"

  STATUS=0
  cat ${FTMP} | while IFS= read line; do
    case ${STATUS} in
      "0")
	echo "${line}" | grep -E "${STARTSTR}" 
	RET=$?
	if [ ${RET} -eq 0 ]; then
	  STATUS=1
	  echo "${line}" | ${GSED} -e "s/^${STARTSTR}//" | ${STRSED} >> ${FTMPFUNC}.${F}
	fi
	;;
      "1")
	echo "${line}" | grep -E "${ENDSTR}" 
	RET=$?
	if [ ${RET} -eq 0 ]; then
	  STATUS=2
	else
	  echo "${line}" | ${STRSED} >> ${FTMPFUNC}.${F}
	fi
	;;
      *)
	echo ""
	;;
    esac
  done

  echo "\$\$" >> ${FTMPFUNC}.${F}
  echo "DELIMITER ;" >> ${FTMPFUNC}.${F}

done

#################################
# TRIGGERS INIT
#################################
init_list_mole_triggers

for T in ${LISTMOLETRIGGERS}; do
  SQLSTR="SHOW CREATE TRIGGER ${T} \G"
  mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}" > ${FTMP}
  echo "DROP TRIGGER IF EXISTS ${T};" > ${FTMPTRIG}.${T}
  echo "DELIMITER \$\$" >> ${FTMPTRIG}.${T}
  STARTSTR="[ \t]*SQL Original Statement:"
  ENDSTR="[ \t]*character_set_client:"

  STATUS=0
  cat ${FTMP} | while IFS= read line; do
    case ${STATUS} in
      "0")
	echo "${line}" | grep -E "${STARTSTR}" 
	RET=$?
	if [ ${RET} -eq 0 ]; then
	  STATUS=1
	  echo "${line}" | ${GSED} -e "s/^${STARTSTR}//" | ${STRSED} >> ${FTMPTRIG}.${T}
	fi
	;;
      "1")
	echo "${line}" | grep -E "${ENDSTR}" 
	RET=$?
	if [ ${RET} -eq 0 ]; then
	  STATUS=2
	else
	  echo "${line}" | ${STRSED} >> ${FTMPTRIG}.${T}
	fi
	;;
      *)
	echo ""
	;;
    esac
  done

  echo "\$\$" >> ${FTMPTRIG}.${T}
  echo "DELIMITER ;" >> ${FTMPTRIG}.${T}

done


#################################
# TABLES
#################################
init_list_mole_tables

for T in ${LISTMOLETABLES}; do
  if [ -z "${LISTLOCKTABLES}" ]; then
    LISTLOCKTABLES="${T} WRITE"
  else
    LISTLOCKTABLES="${LISTLOCKTABLES}, ${T} WRITE"
  fi
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


echo "" >> ${FILEALTERMOLESTATIC}
echo "-- LOCK TABLES must be the FIRST sql command" >> ${FILEALTERMOLESTATIC}
## echo "FLUSH TABLES WITH READ LOCK;" >> ${FILEALTERMOLESTATIC}
echo "LOCK TABLES ${LISTLOCKTABLES};" >> ${FILEALTERMOLESTATIC}
echo "" >> ${FILEALTERMOLESTATIC}

# Delete (if it is necessary) the rows with FOREIGN KEY's values not valid
# If you do not that, following ALTER commands could not work because constraint violation
I=0
for T in ${LISTMOLETABLES}; do
  SQLSTR="SHOW CREATE TABLE ${T} \G"
  # echo "${SQLSTR}"
  mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}" > ${FTMP}
  for A in ${RENATTR}; do
    # echo ${A}
    FROM=`echo ${A} | cut -d'/' -f 2`
    TO=`echo ${A} | cut -d'/' -f 3`
    TOTABLE=`echo ${TO} | ${GSED} -e "s/^fk_/ew_/" | ${GSED} -e "s/ew_module_pick/ew_module/g" -e "s/ew_module_loc/ew_module/g"`

    # echo "T: ${T}, FROM: ${FROM}, TO: ${TO}, TOTABLE: ${TOTABLE}"

    grep -w ${FROM} ${FTMP} | grep -E "^[ \t]*[\`]${FROM}" > ${FTMPGREP}
    RET=$?

    # DELETE missing foreign key
    if [ ${RET} -eq 0 ]; then
      echo ""

      FIELDCOUNTNAME="${T}_${FROM}_countdelete"
      SQLSTR="select COUNT(*) ${FIELDCOUNTNAME} from ${T} left join ${TOTABLE} on (${T}.${FROM}=${TOTABLE}.id) where ${TOTABLE}.id is NULL \G"
      echo "${SQLSTR}"
      mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}" > ${FTMPCOUNTDEL}
      cat ${FTMPCOUNTDEL}
      COUNTDELETE=`cat ${FTMPCOUNTDEL} | grep "${FIELDCOUNTNAME}: " | sed -e "s/${FIELDCOUNTNAME}: //"`
      echo "${T} countdelete=${COUNTDELETE}"

      if [ "${FLAG_FK_CONSTRAINT_CHECK}" == "no" ] || [ ${COUNTDELETE} -gt 0 ]; then
	SQLSTRDEL="delete ${T}.* from ${T} left join ${TOTABLE} on (${T}.${FROM}=${TOTABLE}.id) where ${TOTABLE}.id is NULL;"
	echo "${SQLSTRDEL}"
	execute_mysql_and_log "${SQLSTRDEL}"
      fi

    fi

  done
done

init_queries

# Change Engine in InnoDB
I=0
for T in ${LISTMOLETABLES}; do
  if [ "${SQLSTRALTER[I]}" == "X" ]; then
    SQLSTRALTER[${I}]=""
  fi
  SQLSTRALTER[${I}]="${SQLSTRALTER[I]} ENGINE=InnoDB"
  I=$((I + 1))
done


I=0
for T in ${LISTMOLETABLES}; do
  SQLSTR="SHOW CREATE TABLE ${T} \G"
  # echo "${SQLSTR}"
  mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}" > ${FTMP}

  for A in ${RENATTR}; do
    # echo ${A}
    FROM=`echo ${A} | cut -d'/' -f 2`
    TO=`echo ${A} | cut -d'/' -f 3`
    TOTABLE=`echo ${TO} | ${GSED} -e "s/^fk_/ew_/" | ${GSED} -e "s/ew_module_pick/ew_module/g" -e "s/ew_module_loc/ew_module/g"`

    # grep -w ${FROM} ${FTMP} | grep -E "^[ \t]*[\`]${FROM}" > ${FTMPGREP}
    grep -w ${FROM} ${FTMP} | grep -E "^[ \t]*KEY[ \t][ \t]*\`[^\`]*\`[ \t][ \t]*\(\`${FROM}\`\)" > ${FTMPGREP}
    RET=$?

    # Drop KEY field name
    if [ ${RET} -eq 0 ]; then
      KEYDEL=$(sed -e "s/^[ \t]*KEY[ \t][ \t]*\`\([^\`]*\)\`[ \t][ \t]*(\`${FROM}\`).*$/\1/" ${FTMPGREP})
      if [ "${SQLSTRALTER[I]}" != "X" ]; then
	SQLSTRALTER[${I}]="${SQLSTRALTER[I]}, "
      else
	SQLSTRALTER[${I}]=""
      fi
      SQLSTRALTER[${I}]="${SQLSTRALTER[I]} DROP KEY \`${KEYDEL}\`"
    fi

  done

  I=$((I + 1))
done

execute_cur_alter_table

init_queries

I=0
for T in ${LISTMOLETABLES}; do
  SQLSTR="SHOW CREATE TABLE ${T} \G"
  # echo "${SQLSTR}"
  mysql -u${DBUSER} -p${DBPASS} -h ${DBHOST} ${DBNAME} -e "${SQLSTR}" > ${FTMP}

  for A in ${RENATTR}; do
    # echo ${A}
    FROM=`echo ${A} | cut -d'/' -f 2`
    TO=`echo ${A} | cut -d'/' -f 3`
    TOTABLE=`echo ${TO} | ${GSED} -e "s/^fk_/ew_/" | ${GSED} -e "s/ew_module_pick/ew_module/g" -e "s/ew_module_loc/ew_module/g"`

    grep -w ${FROM} ${FTMP} | grep -E "^[ \t]*[\`]${FROM}" > ${FTMPGREP}
    RET=$?

    # Change field name
    if [ ${RET} -eq 0 ]; then
      NEWDEF=`${GSED} -e "${A}" -e "s/,[ \t]*$//" ${FTMPGREP}`
      if [ "${SQLSTRALTER[I]}" != "X" ]; then
	SQLSTRALTER[${I}]="${SQLSTRALTER[I]}, "
      else
	SQLSTRALTER[${I}]=""
      fi
      SQLSTRALTER[${I}]="${SQLSTRALTER[I]} CHANGE \`${FROM}\` ${NEWDEF}, ADD FOREIGN KEY (\`${TO}\`) REFERENCES ${TOTABLE}(\`id\`) ON DELETE CASCADE ON UPDATE CASCADE"
    fi

  done

  I=$((I + 1))
done

execute_cur_alter_table

echo "" >> ${FILEALTERMOLESTATIC}
echo "-- UNLOCK TABLES must be the LAST sql command " >> ${FILEALTERMOLESTATIC}
echo "UNLOCK TABLES;" >> ${FILEALTERMOLESTATIC}
echo "" >> ${FILEALTERMOLESTATIC}

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
# VIEWS
#################################
echo "" >> ${FILEALTERMOLESTATIC}
echo "-- VIEWS" >> ${FILEALTERMOLESTATIC}
for V in ${LISTMOLEVIEWS}; do
  echo "-- VIEW ${V}"
  execute_mysql_from_file_and_log ${FTMPVIEW}.${V}
  echo ""
done


