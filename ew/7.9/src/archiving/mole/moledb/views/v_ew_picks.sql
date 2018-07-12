
DROP VIEW IF EXISTS `v_ew_picks`;
CREATE ALGORITHM = MERGE VIEW `v_ew_picks` AS
SELECT
    ew_scnl.sta, ew_scnl.cha, ew_scnl.net, ew_scnl.loc,
    fn_get_str_from_dt_usec(ew_pick_scnl.tpick_dt, ew_pick_scnl.tpick_usec) AS pick_time,
--    ew_pick_scnl.fk_spkseq,
    ew_pick_scnl.dir,
    ew_pick_scnl.wt,
    ew_pick_scnl.pamp_0,
    ew_pick_scnl.pamp_1,
    ew_pick_scnl.pamp_2,
    ew_instance.ewname,
    ew_module.modname,
    ew_spkseq.pkseq,
    ew_pick_scnl.modified
FROM ew_pick_scnl
JOIN ew_module ON (ew_pick_scnl.fk_module=ew_module.id )
JOIN ew_instance ON (ew_module.fk_instance=ew_instance.id)
JOIN ew_scnl ON (ew_pick_scnl.fk_scnl=ew_scnl.id)
JOIN ew_spkseq ON (ew_pick_scnl.fk_spkseq=ew_spkseq.id);

