SET @EWINSTANCE = 'XXXXXXXXXX', @STARTTIME = 'YYYYYYYYYY';
SET @ENDTIME = @STARTTIME + INTERVAL 1 DAY;
SELECT
	ew_instance.ewname,
	ew_module.modname,
	@STARTTIME,
	@ENDTIME,
	ew_scnl.sta,
	ew_scnl.cha, ew_scnl.net,
	-- COUNT of all attributes except for version
	COUNT(DISTINCT ew_arc_phase.ewsqkid) as tot
FROM ew_arc_phase, ew_scnl, ew_module, ew_instance
WHERE
    ew_instance.ewname = @EWINSTANCE
    AND
    IF(ew_arc_phase.Ponset='P',
	(
	    ew_arc_phase.Pat_dt >= @STARTTIME
	    AND
	    ew_arc_phase.Pat_dt < @ENDTIME
	),
	(
	    ew_arc_phase.Sat_dt >= @STARTTIME
	    AND
	    ew_arc_phase.Sat_dt < @ENDTIME
	)
    )
    AND
    ew_scnl.id = ew_arc_phase.scnlid
    AND
    ew_arc_phase.ewmodid = ew_module.id
    AND
    ew_module.ewid = ew_instance.id
GROUP BY scnlid
ORDER BY net,sta,cha
