
DROP VIEW IF EXISTS `v_ew_params_pick_sta_last_revision`;
CREATE ALGORITHM = MERGE VIEW `v_ew_params_pick_sta_last_revision` AS
SELECT ew_scnl.sta,ew_scnl.cha,ew_scnl.net,ew_scnl.loc, a.*
FROM ew_params_pick_sta as a
JOIN v_ew_params_pick_id_last_revision b ON (a.fk_module = b.fk_module AND a.fk_scnl = b.fk_scnl AND a.revision = b.last_revision)
JOIN ew_scnl ON (ew_scnl.id = a.fk_scnl);

