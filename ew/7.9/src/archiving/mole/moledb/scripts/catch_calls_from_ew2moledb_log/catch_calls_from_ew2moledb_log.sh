#!/bin/bash

DIRNAME=`dirname $0`

DIRLOG=${DIRNAME}/ew_log

grep "DEBUG: sub_sqlstr=" `find ${DIRLOG} -type f -name "*.log"` | sed -e "s/^.*\.log://" | sort > ew2moledb_all.sort.txt

cat ew2moledb_all.sort.txt | sed -e "s/^[0-9][0-9]*.*DEBUG: sub_sqlstr=\"//" -e "s/;\"[ \t]*$/;/" > ew2moledb_all.calls.txt

