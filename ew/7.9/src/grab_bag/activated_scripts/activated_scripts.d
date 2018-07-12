#
#         activated_scripts Configuration File 
#
MyModuleId    MOD_ACT_SCR # activated_scripts' module id

InRing     TEST_RING	# Transport ring to find ACTIVATE_MODULE messages on,
OutRing    HYPO_RING	# Transport ring to write messages to

#MaxMsgSize	1000		# Maximum message size; default is 100
#MaxScriptArgs 20		# Maximum # of args to script; default is 10

Exec "echo 'arg 1: $1' > output.txt"
Exec "echo 'arg 2: $2' >> output.txt"
Exec "echo 'arg 3: $3' >> output.txt"
Exec "echo 'arg 4: $4' >> output.txt"
