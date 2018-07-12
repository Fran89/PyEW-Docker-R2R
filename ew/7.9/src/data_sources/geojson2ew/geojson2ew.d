#
# geojson2ew configuration file
#
# This code receives geoJSON records from an AMQP server, converts them into
# Earthworm trace buf messages, and stuffs them into a wave ring.
#
#
 ModuleId	MOD_GEOJSON2EW	# module id for this import,
 RingName	GPS_RING	# transport ring to use for input/output,

 HeartbeatInt	10		# Heartbeat interval in seconds
				# this should match the geojson2ew.desc heartbeat!

 LogFile	2		# If 0, don't write logfile;; if 1, do
                                # if 2, log to module log but not stderr/stdout

 HOST		72.233.251.236		# for socket mode specify host and port only
 PORT		5672
#SERVERTYPE     rabbitMQ		# possible values are "rabbitMQ", "socket", or ""; default is "" (automatically determine)
 USERNAME	bsl
 PASSWORD	xxxxxx
 VHOST		/CWU-ppp
#QUEUENAME	MyQueueName		# if commented out bind to an exchange; default is ""
 EXCHANGENAME	eew			# if commented out and there's a queue name then bind to named queue
 EXCHANGETYPE	fanout			# can be "fanout", "direct", "topic" or "headers"; default is "fanout"
#BINDKEY	MyBindingKey		# use for exchange of type 'direct' or 'topic'; default is ""
#BINDARGS	{"xyz":"test","a":1,"b":1.5,"c":true,"d":false,"x-match":"all"} # use for exchange of type "headers"; simple json objects; no spaces; default is ""
 MAP_SNCL       properties.SNCL         # http://previous.rabbitmq.com/v3_4_x/tutorials/tutorial-five-python.html
 MAP_TIME       properties.time
 MAP_SAMPLERATE properties.sampleRate

# Map channels
#               channelCode jsonPath multiplier condition
MAP_CHAN N features[*].geometry.coordinates[0] 1e6 properties.coordinateType=NEU
MAP_CHAN E features[*].geometry.coordinates[1] 1e6 properties.coordinateType=NEU
MAP_CHAN Z features[*].geometry.coordinates[2] 1e6 properties.coordinateType=NEU
MAP_CHAN Q features[*].properties.quality 1
MAP_CHAN 1 features[*].properties.EError 1e6
MAP_CHAN 2 features[*].properties.NError 1e6
MAP_CHAN 3 features[*].properties.UError 1e6
MAP_CHAN 4 features[*].properties.ENCovar 1e6
MAP_CHAN 5 features[*].properties.EUCovar 1e6
MAP_CHAN 6 features[*].properties.NUCovar 1e6
