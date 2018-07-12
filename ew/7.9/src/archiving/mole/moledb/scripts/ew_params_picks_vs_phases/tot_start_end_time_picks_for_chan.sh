#!/bin/sh

FILESQLPICKS=tot_start_end_time_picks_for_chan.sql
FILESQLPHASES=tot_start_end_time_phases_for_chan.sql
FILESQLPICKSVSPHASES=tot_start_end_time_picks_vs_phases_for_chan.sql
FILESQLTMPPICKS=tot_start_end_time_picks_for_chan.sql.temp
FILESQLTMPPHASES=tot_start_end_time_phases_for_chan.sql.temp
FILESQLTMPPICKSVSPHASES=tot_start_end_time_picks_vs_phases_for_chan.sql.temp

EWINSTANCE='hew1_mole'

for START_TIME in 2010-10-14; do 
    echo ${EWINSTANCE} ${START_TIME}
    sed -e "s/XXXXXXXXXX/${EWINSTANCE}/" -e "s/YYYYYYYYYY/${START_TIME}/" ${FILESQLPICKS} > ${FILESQLTMPPICKS}
    mysql_format -s $FILESQLTMPPICKS  -h hdb1 -u moleuser -p molepass -d mole > "picks_${EWINSTANCE}_${START_TIME}".txt

    sed -e "s/XXXXXXXXXX/${EWINSTANCE}/" -e "s/YYYYYYYYYY/${START_TIME}/" ${FILESQLPHASES} > ${FILESQLTMPPHASES}
    mysql_format -s $FILESQLTMPPHASES  -h hdb1 -u moleuser -p molepass -d mole > "phases_${EWINSTANCE}_${START_TIME}".txt

    sed -e "s/XXXXXXXXXX/${EWINSTANCE}/" -e "s/YYYYYYYYYY/${START_TIME}/" ${FILESQLPICKSVSPHASES} > ${FILESQLTMPPICKSVSPHASES}
    mysql_format -s $FILESQLTMPPICKSVSPHASES  -h hdb1 -u moleuser -p molepass -d mole > "picks_vs_phases_${EWINSTANCE}_${START_TIME}".txt
done

