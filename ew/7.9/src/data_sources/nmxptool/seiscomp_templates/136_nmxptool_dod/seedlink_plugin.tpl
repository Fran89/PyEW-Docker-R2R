* template: #template#
plugin #pluginid# cmd="#pkgroot#/bin/nmxptool -H #srcaddr# -P #srcport# -N IV -F #pkgroot#/nmxptool_dod_channelfile_#srcaddr#.#srcport#.txt -v 16 -T 30 -k"
             timeout = 600
             start_retry = 30
             shutdown_wait = 15

