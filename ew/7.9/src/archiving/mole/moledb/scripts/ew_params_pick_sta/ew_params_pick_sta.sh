#!/bin/sh

# cat pick_ew.sta | grep -v -E "^[ ]*#" | sed -e "s/ [ ]*/ /g" | cut -f 24 -d ' ' 

# Check variable EW_PARAMS
if [ -z "${EW_PARAMS}" ]; then
    echo "Error: EW_PARAMS is not defined!"
    exit
fi

EWINSTANCENAME=conf_hew_mole

MODNAME=MOD_PICK_EW

# FILEPICKEWSTA=${EW_PARAMS}/pick_ew.sta
FILEPICKEWSTA=${EW_PARAMS}/db/pick_ew.sta.auto_start.B
FILEPICKEWSTA=$1

if [ -z "$1" ]; then
	echo "Syntax: `basename $0` pick_ew.sta.file"
	exit
fi

grep -v -E "^#.*|^[ ]*$" ${FILEPICKEWSTA} | sed \
						-e "s/#.*$//g" \
						-e "s/^ [ ]*//g" \
						-e "s/ [ ]*$//g" \
						-e "s/ [ ]*/ /g" \
						-e "s/^\([^ ][^ ]*\) \([^ ][^ ]*\) \([^ ][^ ]*\) \([^ ][^ ]*\) \([^ ][^ ]*\) \([^ ][^ ]*\) /\1 \2 \3 \4 \5 '\6' /" \
						-e "s/ [ ]*/, /g" \
						-e "s/\([A-Za-z][A-Za-z0-9]*\)/'\1'/g" \
						-e "s/^/CALL \`mole\`.\`sp_ins_ew_params_pick_sta\`('${EWINSTANCENAME}', '${MODNAME}', /" \
						-e "s/$/);/"




