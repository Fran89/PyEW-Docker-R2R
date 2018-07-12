SET @EWINSTANCE = 'XXXXXXXXXX', @STARTTIME = 'YYYYYYYYYY';
SET @ENDTIME = @STARTTIME + INTERVAL 1 DAY;
SELECT
	ew_instance.ewname,
	ew_module.modname,
	@STARTTIME,
	@ENDTIME,
	ew_scnl.sta,
	ew_scnl.cha, ew_scnl.net, COUNT(*) as tot
FROM ew_pick_scnl, ew_scnl, ew_module, ew_instance
WHERE
    ew_instance.ewname = @EWINSTANCE
    AND
    ew_pick_scnl.tpick_dt >= @STARTTIME
    AND
    ew_pick_scnl.tpick_dt < @ENDTIME
    AND
    ew_scnl.id = ew_pick_scnl.scnlid
    AND
    ew_pick_scnl.ewmodid = ew_module.id
    AND
    ew_module.ewid = ew_instance.id
GROUP BY scnlid
ORDER BY net,sta,cha
