SET @EWINSTANCE = 'XXXXXXXXXX', @STARTTIME = 'YYYYYYYYYY';
SET @ENDTIME = @STARTTIME + INTERVAL 1 DAY;

SELECT
    @STARTTIME, @ENDTIME,
    a.sta, a.cha, a.net, a.loc,
    a.modname,
    a.tot,
    b.modname,
    b.tot,
    a.tot - IF(b.tot IS NULL, 0, b.tot)
    -- a.*, b.*
    FROM
(
SELECT
	ew_instance.ewname,
	ew_module.modname,
	@STARTTIME,
	@ENDTIME,
	ew_scnl.sta, ew_scnl.cha, ew_scnl.net, ew_scnl.loc,
	COUNT(*) as tot
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
ORDER BY net,sta,cha,loc
) AS a
LEFT JOIN
(
SELECT
	ew_instance.ewname,
	ew_module.modname,
	@STARTTIME,
	@ENDTIME,
	ew_scnl.sta, ew_scnl.cha, ew_scnl.net, ew_scnl.loc,
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
ORDER BY net,sta,cha,loc
) as b
ON
(
a.sta = b.sta
AND
a.cha = b.cha
AND
a.net = b.net
AND
a.loc = b.loc
)
