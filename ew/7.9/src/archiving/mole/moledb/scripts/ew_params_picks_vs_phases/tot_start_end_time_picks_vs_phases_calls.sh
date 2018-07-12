#!/bin/sh

FILESQLPICKSVSPHASESCALLS=tot_start_end_time_picks_vs_phases_calls.sql
FILEFORMATPICKSVSPHASESCALLS=tot_start_end_time_picks_vs_phases_calls.format
FILESQLTMPPICKSVSPHASESCALLS=tot_start_end_time_picks_vs_phases_calls.sql.temp

EWINSTANCE='hew1_mole'

DAYSTART=1
DAYSTOP=27

for START_TIME_MONTH in 2010-12; do 
    CURDAY=${DAYSTART}
    while [ ${CURDAY} -le ${DAYSTOP} ]; do

	START_TIME=${START_TIME_MONTH}-${CURDAY}
	echo ${EWINSTANCE} ${START_TIME}

	sed -e "s/XXXXXXXXXX/${EWINSTANCE}/" -e "s/YYYYYYYYYY/${START_TIME}/" ${FILESQLPICKSVSPHASESCALLS} > ${FILESQLTMPPICKSVSPHASESCALLS}
	mysql_format -s $FILESQLTMPPICKSVSPHASESCALLS -f ${FILEFORMATPICKSVSPHASESCALLS}  -h hdb1 -u moleuser -p molepass -d mole > "tot_start_end_time_picks_vs_phases_${EWINSTANCE}_${START_TIME}_calls".sql

	CURDAY=$((${CURDAY} + 1))
    done
done

