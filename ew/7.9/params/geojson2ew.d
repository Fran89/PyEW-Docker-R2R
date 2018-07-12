#
# geojson2ew configuration file
#
# This code receives geoJSON records from an AMQP server, converts them into
# Earthworm trace buf messages, and stuffs them into a wave ring.
#
#
 ModuleId	MOD_GEOJSON2EW	# module id for this import,
 RingName	WAVE_RING	# transport ring to use for input/output,

 HeartbeatInt	10		# Heartbeat interval in seconds
				# this should match the geojson2ew.desc heartbeat!

 LogFile	2		# If 0, don't write logfile;; if 1, do
                                # if 2, log to module log but not stderr/stdout

 HOST		131.215.66.126
 PORT		5672
 QUEUENAME	Results2Earthworm
 USERNAME	seismic
 PASSWORD	secret
 MAP_SNCL       properties.SNCL
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
